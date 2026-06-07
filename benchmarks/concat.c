// concat.c
#include <stdlib.h>
#include <string.h>
int main() {
  char *s1 = "Hello";
  char *s2 = "World";
  for (int i = 0; i < 100000; i++) {
    char *res = malloc(11);
    strcpy(res, s1);
    strcat(res, s2);
    free(res);
  }
}
