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

int main(int argc, char* argv[])
{
#ifdef DEC
  printf("I'm dec_server!\n");
#else
  printf("I'm enc_server!\n");
#endif
  if (argc != 2) {
    fprintf(stderr, "USAGE: ./%s port", argv[0]);
    exit(1);
  }
  int portNumber = atoi(argv[1]);
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
