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

int main(int argc, char* argv[])
{
#ifdef DEC
  printf("I'm dec_client!\n");
#else
  printf("I'm enc_client!\n");
#endif
  exit(0);
}
