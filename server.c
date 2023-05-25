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
  exit(0);
}
