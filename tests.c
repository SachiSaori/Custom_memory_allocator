
#include <stdio.h>
#include <string.h>

#include "allocator.h"

/****************
 * Test program *
 ****************/
int main() {
    printf("Custom Memory Allocator Test\n");
    init_allocator();

    printf("--- Test 1: Basic Allocation ---\n");
    int* a = my_malloc(sizeof(int) * 10);  // 40 bytes
    char* b = my_malloc(100);
    double* c = my_malloc(sizeof(double) * 5);  // 40 bytes
    int* e = my_malloc(sizeof(int) * 10);
    print_memory_state();

    printf("--- Test 2: Using Allocated Memory ---\n");
    if (a) {
        a[0] = 42;
        a[9] = 99;
        printf("a[0] = %d, a[9] = %d\n", a[0], a[9]);
    }
    if (b) {
        strcpy(b, "Hello from custom allocator!");
        printf("b = \"%s\"\n", b);
    }
    printf("\n");

    printf("--- Test 3: Freeing Memory ---\n");
    my_free(b);
    print_memory_state();

    printf("--- Test 4: Coalescing ---\n");
    my_free(a);
    print_memory_state();

    my_free(c);
    print_memory_state();

    printf("--- Test 5: Reuse Freed Memory ---\n");
    char* d = my_malloc(200);
    if (d) {
        strcpy(d, "Reusing freed memory!");
        printf("d = \"%s\"\n", d);
    }
    print_memory_state();

    printf("--- Test 6: Allocation Failure ---\n");
    void* huge = my_malloc(10000);  // Too large
    if (!huge) {
        printf("Allocation failed as expected (requested too much)\n");
    }
    print_memory_state();

    printf("--- Test 7: Double Free ---\n");
    my_free(e);
    my_free(e);
    print_memory_state();

    printf("--- Test 8: Buffer Overflow Detection ---\n");
    int* overflow_test = my_malloc(10 * sizeof(int));  // 40 bytes
    if (overflow_test) {
        // Write within bounds - OK
        overflow_test[0] = 1;
        overflow_test[9] = 10;

        // Write OUT of bounds - corrupts canary!
        overflow_test[12] = 999;  // 12 * 4 = 48 bytes (past the 40 allocated)

        printf("Attempting to free buffer with overflow...\n");
        my_free(overflow_test);  // Should detect corruption!
    }
    print_memory_state();

    printf("--- Test 9: Alignment Verification ---\n");
    for (int i = 0; i < 5; i++) {
        void* ptr = my_malloc(40);
        printf("Allocated pointer: %p (address mod 8 = %lu)\n",
               ptr, (unsigned long)ptr % 8);
        if ((unsigned long)ptr % 8 != 0) {
            printf("❌ MISALIGNED!\n");
        } else {
            printf("✓ Aligned\n");
        }
    }

    my_free(d);
    print_memory_state();

    return 0;
}