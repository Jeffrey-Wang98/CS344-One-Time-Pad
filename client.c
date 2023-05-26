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

// Allows for easier printing of errors exactly how 
// I like them
void
error(int exitCode, char* msg) {
  fprintf(stderr, "%s", msg);
  exit(exitCode);
}

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
}

void
find_bad_char(const char* line) {
  const char* badChars = "!@#$%^&*()_+-={}[]|;:\'\"\\<,>./?`~0123456789";
  char* badCharPtr = strpbrk(line, badChars);
  if (badCharPtr != NULL) {
    fprintf(stderr, "CLIENT: ERROR invalid characters in file '%c'", *badCharPtr);
    exit(1);
  }
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
      error(2, "client: error wrong server connection");
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

int main(int argc, char* argv[])
{
#ifdef DEC
  printf("I'm dec_client!\n");
  char* password = "dec_";
#else
  printf("I'm enc_client!\n");
  char* password = "enc_";
#endif
  // Setting up needed variables for socket
  int socketFD;
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
  printf("Made it to opening files\n");
  // Open files
  FILE* inputFile = fopen(argv[1], "re");
  if (inputFile == NULL) {
    error(1, "CLIENT: ERROR could not open input file");
  }
  FILE* keyFile = fopen(argv[2], "re");
  if (keyFile == NULL) {
    error(1, "CLIENT: ERROR could not open key file");
  }
  printf("Opened the files\n");
  size_t n;
  // Save the contents of the files as strings
  char* input = NULL;
  char* key = NULL;
  ssize_t inputLength = getline(&input, &n, inputFile);
  if (inputLength == -1) {
    error(1, "CLIENT: ERROR could not read input file");
  }
  printf("Managed to read input file of length %ld\n", inputLength);
  ssize_t keyLength = getline(&key, &n, keyFile);
  if (keyLength == -1) {
    error(1, "CLIENT: ERROR could not read key file");
  }
  printf("Managed to read key file of length %ld\n", keyLength);
  if (inputLength > keyLength) {
    error(1, "CLIENT: ERROR key is shorter than input");
  }
  // Check if there are bad chars in the given files
  find_bad_char(input);
  find_bad_char(key);
  printf("The input is %s\n", input);
  printf("The key is %s\n", key);

  // TODO continue with input and key preparation for server
  //fclose(inputFile);
  //fclose(keyFile);
  //exit(0);
  // Remove the trailing \n
  inputLength--;
  keyLength--;

  // Create socket with TCP 
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) {
    error(2, "CLIENT: ERROR opening socket");
  }
  setupAddressStruct(&serverAddress, atoi(argv[3]));
  
  // Connect to socket
  if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    error(2, "CLIENT: ERROR connecting to socket");
  }
  
  
  // Send password and input length
  char* checkConnect;
  asprintf(&checkConnect, "%s%ld", password, inputLength);
  int checkWritten = send(socketFD, checkConnect, strlen(checkConnect), 0);
  if (checkWritten < 0) {
    error(2, "CLIENT: ERROR writing to socket");
  }

  // Start sending input
  // Loop until it is fully given
  // Old code
  /*
  while (charsWritten < inputLength) {
    charsWritten = send(socketFD, input, inputLength, 0);
    if (charsWritten < 0) {
      fprintf(stderr, "CLIENT: ERROR writing to socket");
      exit(2);
    }
    if (charsWritten == 0) {
      fprintf(stderr, "CLIENT: ERROR wrong server");
      exit(2);
    }

  }
  */
  // Send text file
  // send_all returning = everything was sent
  send_all(socketFD, input, inputLength);
  // Only need to send the key up to the end of input
  send_all(socketFD, key, inputLength);

  char responseBuffer[inputLength + 1];
  // fill buffer with new lines
  memset(responseBuffer, '\n', inputLength + 1);
  // recv_all returning = everything was received
  recv_all(socketFD, &responseBuffer, inputLength);
  printf("%s", responseBuffer);  

  // Close socket
  close(socketFD);

  exit(0);
}
