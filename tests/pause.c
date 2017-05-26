#include <unistd.h>
int main(void) {
  alarm(120);
  while (1) pause();
  return 1;
}
