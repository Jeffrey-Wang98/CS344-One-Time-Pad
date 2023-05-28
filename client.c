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
    error(1, "CLIENT: ERROR could not open input file\n");
  }
  FILE* keyFile = fopen(argv[2], "re");
  if (keyFile == NULL) {
    error(1, "CLIENT: ERROR could not open key file\n");
  }
  size_t n;
  // Save the contents of the files as strings
  int tries = 0;
retry_input:;
  char* input = NULL;
  // label to retry getline for input
  ssize_t inputLength = getline(&input, &n, inputFile);
  if (inputLength == -1) {
    // retry 5 times
    if (tries < 5) {
      fprintf(stderr, "This is input try #%d\n", tries);
      clearerr(inputFile);
      free(input);
      tries++;
      goto retry_input;
    }
    fprintf(stderr, "CLIENT: Read from input file '");
    write(2, input, 20);
    fprintf(stderr, "'\n");
    error(1, "CLIENT: ERROR could not read input file\n");
  }
    tries = 0;
  // label to retry getline for key
retry_key:;
  char* key = NULL;

  ssize_t keyLength = getline(&key, &n, keyFile);
  if (keyLength == -1) {
    // retry 5 times
    if  (tries < 5) {
      fprintf(stderr, "This is key try #%d\n", tries);
      clearerr(keyFile);
      free(key);
      tries++;
      goto retry_key;
    }
    fprintf(stderr, "CLIENT: Read from key file '");
    write(2, key, 20);
    fprintf(stderr, "'\n");
    error(1, "CLIENT: ERROR could not read key file\n");
  }
  if (inputLength > keyLength) {
    error(1, "CLIENT: ERROR key is shorter than input\n");
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
    error(2, "CLIENT: ERROR opening socket\n");
  }
  portNumber = atoi(argv[3]);
  setupAddressStruct(&serverAddress, portNumber);
  
  // Connect to socket
  if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    close(socketFD);
    error(2, "CLIENT: ERROR connecting to socket\n");
  }
  
  // Send password and input length
  send_all(socketFD, password, strlen(password));
  
  int acceptance = 1;
  recv_all(socketFD, &acceptance, sizeof(acceptance));
  // Acceptance of 0 = accepted
  if (acceptance == -1) {
    close(socketFD);
    fprintf(stderr, "CLIENT: ERROR could not contact %sserver on port %d\n", password, portNumber);
    exit(2);
  }
  else if (acceptance == 1) {
    close(socketFD);
    error(2, "CLIENT: ERROR could not receive acceptance from server\n");
  }

  // Send textLength
  send_all(socketFD, &inputLength, sizeof(ssize_t));

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
    fprintf(stderr, "CLIENT: ERROR invalid characters in file '%c'\n", *badCharPtr);
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
      error(2, "CLIENT: ERROR failed to send data\n");
    }
    if (n == 0) {
      error(2, "CLIENT: ERROR wrong server connection\n");
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
      error(2, "CLIENT: ERROR failed to receive data\n");
    }
    pos += n;
    count -= n;
  }
  return;
}
