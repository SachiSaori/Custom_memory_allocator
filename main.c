#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/******************
 * Allocator code *
 ******************/

// pool and block size
#define POOL_SIZE 4096 // 4kb memory pool
#define MIN_BLOCK_SIZE 32// Minimum size for splitting

/*
 * Block header structure (24 bytes total)
 *
 * Layout is carefully designed for alignment:
 * - size_t (8 bytes) naturally aligns to 8-byte boundary
 * - is_free only needs 1 byte (boolean), saving 3 bytes vs int
 * - Explicit padding ensures 'next' pointer is 8-byte aligned
 * - Total overhead: 24 bytes per block
 */
typedef struct block_header {
    size_t size;                // Size without header
    uint8_t is_free;            // Boolean
    uint8_t padding[7];         // 7 bytes explicit padding.
    struct block_header* next;  // Pointer to next block in the list
} block_header_t;


static char memory_pool[POOL_SIZE];
static block_header_t* free_list_head = NULL;
static int initialized = 0; // False

// Allocator initiation
void init_allocator() {
    if (initialized) return; // skip if true

    // Treat start of pool as the first header
    free_list_head = (block_header_t*)memory_pool;
    free_list_head->size = POOL_SIZE - sizeof(block_header_t);
    free_list_head->is_free = 1; // True
    free_list_head->next = NULL;

    initialized = 1; // True
    printf("[INIT] Allocator initialized with %zu bytes\n", free_list_head->size);
}

// Custom malloc implementation
void* my_malloc(size_t size) {
    if (!initialized) init_allocator(); // Init if not done yet.
    if (size == 0) return NULL; // Can't allocate if no room.

    // Align size to 8 bytes for better performance
    size = (size + 7) & ~7;

    block_header_t* current = free_list_head;

    // First-fit strategy: find first block that's big enough.
    while (current != NULL) {
        if (current ->is_free && current->size >= size) {
            printf("[ALLOC] Found free block: size=%zu at %p\n", current->size, (void*)current);

            // Should block be split?
            // Only split if remaining space is useful (> MIN_BLOCK_SIZE)
            if (current->size >= size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
                block_header_t* new_block = (block_header_t*) ((char*)current + sizeof(block_header_t) + size);

                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->is_free = 1; // True
                new_block->next = current->next;

                // Update current block
                current->size = size;
                current->next = new_block;

                printf("[SPLIT] Split block: allocated=%zu, remaining=%zu\n", size, new_block->size);
            }

            // Mark block as not free
            current->is_free = 0;

            // Return pointer to data area (skip header)
            void* ptr = (char*)current + sizeof(block_header_t);
            printf("[ALLOC] Returning pointer %p\n", ptr);
            return ptr;
        }
        current = current->next;
    }
    printf("[ALLOC] FAILED: No suitable block found for size %zu\n", size);
    return NULL;
}

// Custom free implementation
void my_free(void* ptr) {
    if (!ptr) return;

    printf("[FREE] Freeing pointer %p\n", ptr);

    // Get header from user pointer
    block_header_t* header = (block_header_t*) ((char*)ptr - sizeof(block_header_t));

    // Mark as free
    header->is_free = 1;

    // Coalesce with next block if it's free
    if (header->next && header->next->is_free) {
        printf("[COALESCE] Merging with next block: %zu + %zu\n", header->size, header->next->size);
        header->size += sizeof(block_header_t) + header->next->size;
        header->next = header->next->next;
    }

    // Coalesce with previous block if it's free
    // Need to find previous block by walking from head
    block_header_t* current = free_list_head;
    while (current && current->next != header) {
        current = current->next;
    }

    if (current && current->is_free) {
        printf("[COALESCE] Merging with previous block: %zu + %zu\n", current->size, header->size);
        current->size += sizeof(block_header_t) + header->size;
        current->next = header->next;
    }
}

// Debug function to print memory state
void print_memory_state() {
    printf("\n=== Memory State ===\n");
    block_header_t* current = free_list_head;
    int block_num = 0;
    size_t total_free = 0;
    size_t total_allocated = 0;

    while (current != NULL) {
        printf("Block %d: size=%zu, %s, addr=%p\n",
            block_num++,
            current->size,
            current->is_free ? "FREE" : "ALLOCATED",
            (void*)current);

        if (current->is_free) {
            total_free += current->size;
        } else {
            total_allocated += current->size;
        }

        current = current->next;
    }

    printf("Total free: %zu bytes\n", total_free);
    printf("Total used: %zu bytes\n", total_allocated);
    printf("===================\n\n");
}
/*********************
 * Test program code *
 *********************/

int main() {
    printf("Custom Memory Allocator Test\n");
    init_allocator();

    // Test 1: Allocate some memory
    printf("--- Test 1: Basic Allocation ---\n");
    int* a = (int*)my_malloc(sizeof(int) * 10);  // 40 bytes
    char* b = (char*)my_malloc(100);
    double* c = (double*)my_malloc(sizeof(double) * 5);  // 40 bytes
    print_memory_state();

    // Test 2: Use the allocated memory
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

    // Test 3: Free some memory
    printf("--- Test 3: Freeing Memory ---\n");
    my_free(b);
    print_memory_state();
    
    // Test 4: Free more and watch coalescing
    printf("--- Test 4: Coalescing ---\n");
    my_free(a);
    print_memory_state();

    my_free(c);
    print_memory_state();

    // Test 5: Allocate after freeing
    printf("--- Test 5: Reuse Freed Memory ---\n");
    char* d = (char*)my_malloc(200);
    if (d) {
        strcpy(d, "Reusing freed memory!");
        printf("d = \"%s\"\n", d);
    }
    print_memory_state();

    // Test 6: Allocation failure
    printf("--- Test 6: Allocation Failure ---\n");
    void* huge = my_malloc(10000);  // Too large
    if (!huge) {
        printf("Allocation failed as expected (requested too much)\n");
    }
    print_memory_state();

    my_free(d);
    print_memory_state();

    return 0;
}
