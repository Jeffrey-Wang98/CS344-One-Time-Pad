#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>

// Function Definitions
void error(int exitCode, char* msg);
void setupAddressStruct(struct sockaddr_in* address, int portNumber);
char calculations(const char textChar, const char keyChar); 
char* check_pw(const char* haystack, const char* needle);
int send_all(int fd, const void* buffer, size_t count);
int recv_all(int fd, const void* buffer, size_t count);
int create_response(int fd, const void* text, const void* key, void* response, size_t count); 
void handle_connection(int* socketPtr);
void* thread_function(void* arg);
void enqueue(int* socketPtr);
int* dequeue();

// Set global variables
#ifdef DEC
  const char* password = "dec_";
#else
  const char* password = "enc_";
#endif
// Making a pool of threads for concurrency
#define THREAD_POOL_SIZE 5
pthread_t thread_pool[THREAD_POOL_SIZE];

// Create mutex and condition for critical sections
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueFull = PTHREAD_COND_INITIALIZER;

// Create shared socket pointer queue
int* queue[10];
int writeIndex = 0;
int readIndex = 0;

int main(int argc, char* argv[])
{
  // setting up socket variables
  int connectionSocket, listenSocket;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // checking args and usage
  if (argc < 2) {
    fprintf(stderr, "USAGE: ./%s port", argv[0]);
    exit(1);
  }
  
  // setup listening socket
  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error(1, "SERVER: Error opening socket");
  }

  int portNumber = atoi(argv[1]);

  setupAddressStruct(&serverAddress, portNumber);

  // bind listen socket to the given port number
  if (bind(listenSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    error(1, "SERVER: Error binding socket to port");
  }

  listen(listenSocket, 5);
  
  // Setup thread pool
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    pthread_create(&thread_pool[i], NULL, thread_function, NULL);
  }

  while(1) {
    // accept connection
    connectionSocket = accept(listenSocket, (struct sockaddr*) &clientAddress, &sizeOfClientInfo);
    if (connectionSocket < 0) {
      fprintf(stderr, "SERVER: ERROR on accept\n");
    }
    // Try setting sockopt to TCP_NODELAY
    int one = 1;
    setsockopt(connectionSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    int* socketPtr = &connectionSocket;
    // Critical section of adding connections to the queue
    pthread_mutex_lock(&queueMutex);
    enqueue(socketPtr);
    // signal to the 5 threads that there are new connections
    pthread_cond_signal(&queueFull);
    pthread_mutex_unlock(&queueMutex);
  }
  exit(0);
}

// Allows for easier printing of errors
// exactly how I like them
void
error(int exitCode, char* msg) {
  fprintf(stderr, "%s", msg);
  exit(exitCode);
}

/*
 * Creates the address struct needed for socket
 * and network capabilities
 */
void
setupAddressStruct(struct sockaddr_in* address, int portNumber){
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

/*
 * Takes a char from text and key and calculates
 * the char to save in server response string.
 * Will formula depending on DEC defined
 * during compilation
 */
char
calculations(const char textChar, const char keyChar) {
  int textInt = (int)textChar - 65;
  int keyInt = (int)keyChar - 65;
  // space will become -33. Replace to 26.
  // A - Z = 0 - 25, ' ' = 26
  if (textInt == -33) textInt = 26;
  if (keyInt == -33) keyInt = 26;
#ifdef DEC
  int step1 = textInt - keyInt;
#else
  int step1 = textInt + keyInt;
#endif
  if (step1 < 0) step1 += 27;
  int result = (char)(step1 % 27 + 65);
  if (result == '[') result = ' ';

  return result;
}

/*
 * Runs strstr() to check if the password
 * is contained within the password given
 * by the client. Password changes depending
 * on DEC defined during compilation
 */
char*
check_pw(const char* haystack, const char* needle) {
  char* needleLoc = strstr(haystack, needle);
  if (needleLoc == NULL) return NULL;
  return needleLoc;
}

/*
 * Makes sure that the server writes everything it wants to send
 * down the socket. Returns 0 on success and -1 on send() error.
 */
int
send_all(int fd, const void* buffer, size_t count) {
  const unsigned char* pos = buffer;
  while (count) {
    int n = send(fd, pos, count, 0);
    if (n < 0) {
      fprintf(stderr, "SERVER: ERROR failed to send data\n");
      return -1;
    }
    if (n == 0) {
      fprintf(stderr, "SERVER: ERROR client broke connection\n");
      return -1;
    }
    pos += n;
    count -= n;
  }
  return 0;
}

/*
 * Makes sure that the server reads everything it should receive
 * from the client. Returns 0 on success and -1 on recv() error.
 */
int
recv_all(int fd, const void* buffer, size_t count) {
  const unsigned char* pos = buffer;
  int totalRead = 0;
  while (count && totalRead < 5) {
    int n = recv(fd, (void*)pos, count, 0);
    if (n < 0) {
      fprintf(stderr, "SERVER: ERROR failed to receive data\n");
      return -1;
    }
    pos += n;
    count -= n;
  }
  return 0;
}

/*
 * Does recv_all(), but calculates the response sent by
 * using the key at the same time. Returns 0 on success
 * and -1 on recv() error
 */
int
create_response(int fd, const void* text, const void* key, void* response, size_t count) {
  const unsigned char* textPos = text;
  const unsigned char* keyPos1 = key;
  unsigned char* resPos = response;
  const unsigned char* keyPos2 = key;
  while (count) {
    int n = recv(fd, (void*)keyPos1, count, 0);
    if (n < 0) {
      fprintf(stderr, "SERVER: ERROR failed to receive key\n");
      return -1;
    }
    keyPos1 += n;
    count -= n;
    // Now calculate the response using what's given from client
    while(keyPos2 < keyPos1) {
      *resPos = calculations(*textPos, *keyPos2);
      resPos++;
      textPos++;
      keyPos2++;
    }

  }
  return 0;
}

/*
 * Run by the thread function in order to handle sockets
 * created by accept in main. 
 */
void
handle_connection(int* socketPtr) {
  int connectionSocket = *socketPtr;
  int charsRead;
  char pwBuffer[5];
  memset(pwBuffer, '\0', 5);
  charsRead = recv_all(connectionSocket, pwBuffer, 4);
  if (charsRead < 0) {
    fprintf(stderr, "SERVER: ERROR on receiving password\n");
    close(connectionSocket);
    return;
  }
  // Checking recv
  char* pwLoc = check_pw(pwBuffer, password);
  // Checking the password
  // Wrong client
  int accept = 0;
  int reject = -1;
  if (pwLoc == NULL) {
    fprintf(stderr, "SERVER: Received pwd '%s'\n", pwBuffer);
    fprintf(stderr, "SERVER: ERROR wrong client connection\n");
    if (send_all(connectionSocket, &reject, sizeof(reject)) < 0) {
      fprintf(stderr, "SERVER: ERROR failed to send rejection\n");
      close(connectionSocket);
      return;
    }
    close(connectionSocket);
    return;
  }
  // Correct client
  if (send_all(connectionSocket, &accept, sizeof(accept)) < 0){
    fprintf(stderr, "SERVER: ERROR failed to send acceptance\n");
    close(connectionSocket);
    return;
  }

  // recv_all length
  ssize_t textLength = 0;
  if (recv_all(connectionSocket, &textLength, sizeof(textLength)) < 0) {
    fprintf(stderr, "SERVER: ERROR failed to receive length\n");
    close(connectionSocket);
    return;
  } 


  // Getting text file and key from client
  char textBuffer[textLength];
  char keyBuffer[textLength];
  char resBuffer[textLength];
  // Get all of textBuffer
  if (recv_all(connectionSocket, textBuffer, textLength) < 0) {
    fprintf(stderr, "SERVER: ERROR failed to get input text from client\n");
    close(connectionSocket);
    return;
  }
  // While getting keyBuffer, encrypt/decrypt while waiting for more
  if (create_response(connectionSocket, textBuffer, keyBuffer, resBuffer, textLength) < 0) {
    fprintf(stderr, "SERVER: ERROR failed to get key text from client\n");
    close(connectionSocket);
    return;
  }
  if (send_all(connectionSocket, resBuffer, textLength) < 0) {
    fprintf(stderr, "SERVER: ERROR failed to send results\n");
  }
  close(connectionSocket);
  return;

}

/*
 * Thread function that forever waits and dequeues
 * connection sockets from the socket queue when available
 */
void*
thread_function(void* args) {
  while(1) {
    // Critical section to dequeue from shared queue
    pthread_mutex_lock(&queueMutex);
    int* socketPtr = dequeue();
    // if socketPtr is NULL, wait for signal
    // that main has added more connections to queue
    while (socketPtr == NULL) {
      pthread_cond_wait(&queueFull, &queueMutex);
      socketPtr = dequeue();
    }
    pthread_mutex_unlock(&queueMutex);
    handle_connection(socketPtr);
  }
  return NULL;
}

/*
 * Adds socket pointers to the shared queue
 * for the threads to take when they finished
 * their encryption/decryption
 */
void
enqueue(int* socketPtr) {
  queue[writeIndex++] = socketPtr;
  // loop back to 0 at 10
  writeIndex %= 10;
  return;
}

/*
 * Gets a socket pointer from the shared queue.
 * Returns NULL if readIndex catches up to
 * the writeIndex.
 */
int*
dequeue() {
  if (readIndex == writeIndex) return NULL;
  int* result = queue[readIndex++];
  // loop back to 0 at 10
  readIndex %= 10;
  return result;
}
