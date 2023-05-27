#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>


// Setting up function definitions
void error(int exitCode, char* msg);
void setupAddressStruct(struct sockaddr_in* address, int portNumber);
void find_bad_char(const char* line);
void send_all(int fd, const void* buffer, size_t count);
void recv_all(int fd, const void* buffer, size_t count);

// Setting up global variables
#ifdef DEC
  const char* password = "dec_";
#else
  const char* password = "enc_";
#endif


int main(int argc, char* argv[])
{
  // Setting up needed variables for socket
  int socketFD, portNumber;
  struct sockaddr_in serverAddress;

  // Check usage and args
  if (argc < 4) {
#ifdef DEC
    fprintf(stderr, "USAGE: ./%s ciphertext key port\n", argv[0]);
#else
    fprintf(stderr, "USAGE: ./%s plaintext key port\n", argv[0]);
#endif
    exit(0);
  }
  // Open files
  FILE* inputFile = fopen(argv[1], "re");
  if (inputFile == NULL) {
    error(1, "CLIENT: ERROR could not open input file");
  }
  FILE* keyFile = fopen(argv[2], "re");
  if (keyFile == NULL) {
    error(1, "CLIENT: ERROR could not open key file");
  }
  size_t n;
  // Save the contents of the files as strings
  char* input = NULL;
  char* key = NULL;
  ssize_t inputLength = getline(&input, &n, inputFile);
  if (inputLength == -1) {
    error(1, "CLIENT: ERROR could not read input file");
  }
  ssize_t keyLength = getline(&key, &n, keyFile);
  if (keyLength == -1) {
    error(1, "CLIENT: ERROR could not read key file");
  }
  if (inputLength > keyLength) {
    error(1, "CLIENT: ERROR key is shorter than input");
  }
  // Check if there are bad chars in the given files
  find_bad_char(input);
  find_bad_char(key);

  // Remove the trailing \n
  inputLength--;
  keyLength--;

  // Create socket with TCP 
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) {
    error(2, "CLIENT: ERROR opening socket");
  }
  portNumber = atoi(argv[3]);
  setupAddressStruct(&serverAddress, portNumber);
  
  // Connect to socket
  if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    close(socketFD);
    error(2, "CLIENT: ERROR connecting to socket");
  }
  
  // Send password and input length
  char* checkConnect;
  asprintf(&checkConnect, "%s%ld", password, inputLength);
  int checkWritten = send(socketFD, checkConnect, strlen(checkConnect), 0);
  if (checkWritten < 0) {
    close(socketFD);
    error(2, "CLIENT: ERROR writing to socket");
  }

  // recv for server response to connection
  // Give socket a 5 sec timeout
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout));
  char* acceptance = malloc(2 * sizeof(char));
  // Clear acceptance buffer 
  memset(acceptance, '\0', 2);
  n = recv(socketFD, acceptance, 1, 0);
  if (n < 0) {
    close(socketFD);
    error(2, "CLIENT: ERROR receiving from socket");
  }
  if (atoi(acceptance) == 1) {
    close(socketFD);
    fprintf(stderr, "CLIENT: ERROR could not contact %sserver on port %d\n", password, portNumber);
    exit(2);
  }

  // Start sending input
  // Send text file
  // send_all returning = everything was sent
  send_all(socketFD, input, inputLength);
  // Only need to send the key up to the end of input
  send_all(socketFD, key, inputLength);

  char responseBuffer[inputLength + 1];
  // fill buffer with \n
  memset(responseBuffer, '\n', inputLength + 1);
  // recv_all returning = everything was received
  recv_all(socketFD, &responseBuffer, inputLength);

  write(1, responseBuffer, inputLength + 1); 
  // Close socket
  close(socketFD);
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
 * Sets up the address struct to find
 * the right server to create a socket
 */
void
setupAddressStruct(struct sockaddr_in* address, int portNumber) {
  memset((char*) address, '\0', sizeof(*address));
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);

  struct hostent* hostInfo = gethostbyname("localhost");
  if (hostInfo == NULL) {
    error(1, "CLIENT: ERROR, no such host\n");
  }
  memcpy((char*)&address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
  return;
}

/*
 * Checks the input and key files for bad characters
 * that cannot be encrypted or decrypted. Exits
 * if there are, returns if there aren't.
 */
void
find_bad_char(const char* line) {
  const char* badChars = "!@#$%^&*()_+-={}[]|;:\'\"\\<,>./?`~0123456789";
  char* badCharPtr = strpbrk(line, badChars);
  if (badCharPtr != NULL) {
    fprintf(stderr, "CLIENT: ERROR invalid characters in file '%c'", *badCharPtr);
    exit(1);
  }
  return;
}

/*
 * Makes sure that the client writes everything it wants to send
 * down the socket. Returns on success and exit(2) on send() error.
 */
void
send_all(int fd, const void* buffer, size_t count) {
  const unsigned char* pos = buffer;
  while (count) {
    int n = send(fd, pos, count, 0);
    if (n < 0) {
      error(2, "CLIENT: ERROR failed to send data");
    }
    if (n == 0) {
      error(2, "CLIENT: ERROR wrong server connection");
    }
    pos += n;
    count -= n;
  }
  return;
}

/*
 * Makes sure that the client reads everything it should receive
 * from the server. Returns on success and exit(2) on recv() error.
 */
void
recv_all(int fd, const void* buffer, size_t count) {
  const unsigned char* pos = buffer;
  while (count) {
    int n = recv(fd, (void*)pos, count, 0);
    if (n < 0) {
      error(2, "CLIENT: ERROR failed to receive data");
    }
    pos += n;
    count -= n;
  }
  return;
}
