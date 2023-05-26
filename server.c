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

// Allows for easier printing of errors exactly how 
// I like them
void
error(int exitCode, char* msg) {
  fprintf(stderr, "%s", msg);
  exit(exitCode);
}

void
setupAddressStruct(struct sockaddr_in* address, int portNumber){
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char* argv[])
{
#ifdef DEC
  printf("I'm dec_server!\n");
#else
  printf("I'm enc_server!\n");
#endif
  // setting up socket variables
  int connectionSocket, listenSocket, charsRead;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  char pwBuffer[100];

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
  
  while(1) {
    // accept connection
accept_start:;
    connectionSocket = accept(listenSocket, (struct sockaddr*) &clientAddress, &sizeOfClientInfo);
    if (connectionSocket < 0) {
      fprintf(stderr, "SERVER: ERROR on accept\n");
    }

    printf("SERVER: Connected to client running at host %d port %d\n", 
                          ntohs(clientAddress.sin_addr.s_addr),
                          ntohs(clientAddress.sin_port));

    memset(pwBuffer, '\0', 100);
    charsRead = recv(connectionSocket, pwBuffer, 99, 0);
    if (charsRead < 0) {
      fprintf(stderr, "SERVER: ERROR on recv() from socket\n");
      goto accept_start;
    }

  }








  struct hostent* hostInfo;
  struct sockaddr_in address;
  // Clear out address for reassign
  memset((char*) &address, '\0', sizeof(address));
  address.sin_port = htons(portNumber);
  hostInfo = gethostbyname("localhost");
  if (hostInfo == NULL) {
    fprintf(stderr, "SERVER: ERROR, no such host\n");
    exit(1);
  }
  // Get the first IP address in DNS for localhost
  memcpy(&address.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
  
  printf("The IP address of the localhost is : %s\n", inet_ntoa(address.sin_addr));
  printf("The port number in the socket address: %d\n", ntohs(address.sin_port));




  exit(0);
}
