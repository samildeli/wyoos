#include <programs.h>
#include <syscalls.h>

void collatz(uint32_t n) {
    sysprintf("Collatz sequence for ");
    sysprintfHex32(n);
    sysprintf(": ");

    while (n > 1) {
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = n * 3 + 1;
        }

        sysprintfHex32(n);
        sysprintf(", ");
    }

    sysprintf("\n");
}

void longRun(uint32_t n) {
    uint32_t result = 0;

    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            result += i * j;
        }
    }

    sysprintf("Long running program result for ");
    sysprintfHex32(n);
    sysprintf(": ");
    sysprintfHex32(result);
    sysprintf("\n");
}
