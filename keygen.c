#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  // Handle errors
  if (argc < 2) {
    fprintf(stderr, "keygen: not enough arguments");
    exit(EXIT_FAILURE);
  }
  else if (argc != 2) {
    fprintf(stderr, "keygen: too many arguments");
    exit(EXIT_FAILURE);
  }
  int length = atoi(argv[1]);
  if (length <= 0) {
    fprintf(stderr, "keygen: need a positive integer length");
    exit(EXIT_FAILURE); 
  }

  // Initialize RNG
  FILE* randfp = fopen("/dev/urandom", "r"); 
  if (!randfp) {
    fprintf(stderr, "keygen: file or directory does not exist");
    exit(1);
  }

  char output[length + 1];
  output[length] = '\n';
  for (int i = 0; i < length; ++i) {
    // Get a random integer from 0 - 26 to make it 65 - 91
    unsigned long long rand_num;
    fread(&rand_num, sizeof(rand_num), 1, randfp);
    unsigned long long rand_int = (rand_num % 27) + 65;
    output[i] = (rand_int == 91) ? ' ' : (char)rand_int;
  }
  fflush(stdout);
  write(1, output, length + 1);
  fclose(randfp);
  exit(EXIT_SUCCESS);
}
