#define _POSIX_C_SOURCE 200809L
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
#include <error.h>
#include <fcntl.h>

void
setupAddressStruct(struct sockaddr_in* address, int portNumber) {
  memset((char*) address, '\0', sizeof(*address));
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);

  struct hostent* hostInfo = gethostbyname("localhost");
  if (hostInfo == NULL) {
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(1);
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
 * down the socket. Returns 0 on success and -1 on write() error.
 */
int
write_all(int fd, const void* buffer, size_t count) {
  const unsigned char* pos = buffer;
  while (count) {
    int n = write(fd, pos, count);
    if (n == -1) return -1;
    pos += 1;
    count -= n;
  }
  return 0;
}

int main(int argc, char* argv[])
{
#ifdef DEC
  printf("I'm dec_client!\n");
  char* password = "dec_client";
#else
  printf("I'm enc_client!\n");
  char* password = "enc_client";
#endif
  // Setting up needed variables
  int socketFD, portNumber, charsWritten, charsRead;

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
    fprintf(stderr, "CLIENT: ERROR could not open input file");
    exit(1);
  }
  FILE* keyFile = fopen(argv[2], "re");
  if (keyFile == NULL) {
    fprintf(stderr, "CLIENT: ERROR could not open key file");
    exit(1);
  }
  printf("Opened the files\n");
  size_t n;
  // Save the contents of the files as strings
  char* input = NULL;
  char* key = NULL;
  ssize_t inputLength = getline(&input, &n, inputFile);
  if (inputLength == -1) {
    fprintf(stderr, "CLIENT: ERROR could not read input file");
    exit(1);
  }
  printf("Managed to read input file of length %ld\n", inputLength);
  ssize_t keyLength = getline(&key, &n, keyFile);
  if (keyLength == -1) {
    fprintf(stderr, "CLIENT: ERROR could not read key file");
    exit(1);
  }
  printf("Managed to read key file of length %ld\n", keyLength);
  if (inputLength > keyLength) {
    fprintf(stderr, "CLIENT: ERROR key is shorter than input");
    exit(1);
  }
  // Check if there are bad chars in the given files
  find_bad_char(input);
  find_bad_char(key);
  printf("The input is %s\n", input);
  printf("The key is %s\n", key);

  // TODO continue with input and key preparation for server
  fclose(inputFile);
  fclose(keyFile);
  exit(0);
  // Create socket with TCP 
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) {
    fprintf(stderr, "CLIENT: ERROR opening socket");
    exit(2);
  }
  setupAddressStruct(&serverAddress, atoi(argv[2]));
  
  // Connect to socket
  if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    fprintf(stderr, "CLIENT: ERROR connecting");
    exit(2);
  }
  
  

  exit(0);
}
