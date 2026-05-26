#include <stdio.h>

long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    long result = fib(40);
    printf("Result: %ld\n", result);
    return 0; // Success!
}
