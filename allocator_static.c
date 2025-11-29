#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdalign.h>
#include "allocator.h"

/******************
 * Allocator code *
 ******************/

#define POOL_SIZE 4096              // 4kb memory pool
#define MIN_BLOCK_SIZE 32           // Minimum size for splitting
#define BLOCK_MAGIC 0xDEADBEEF      // Valid allocated block
#define FREED_MAGIC 0xFEEDFACE      // Block has been freed
#define CANARY_VALUE 0xDEADC0DE
#define ALIGNMENT 8                 // Minimum to respect alignment.

/*
 * Block header structure (24 bytes total)
 *
 * Layout is carefully designed for alignment:
 * - magic number 4 bytes
 * - is_free only needs 1 byte (boolean), saving 3 bytes vs int
 * - Explicit padding ensures 'next' pointer is 8-byte aligned
 * - size_t (8 bytes) naturally aligns to 8-byte boundary
 * - Total overhead: 24 bytes per block
 */
typedef struct block_header {
    unsigned int magic;
    uint8_t is_free;
    uint8_t padding[3];         // 3 bytes explicit padding.
    size_t size;
    struct block_header* next;  // Pointer to next block in the list
} block_header_t;

// 16-byte alignment ensures compatibility with SIMD operations
// and provides extra safety margin for all common data types
static char memory_pool[POOL_SIZE] __attribute__((aligned(16)));
static block_header_t* free_list_head = NULL;
static int initialized = 0; // False

size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Allocator initiation
void init_allocator() {
    if (initialized) return; // skip if true

    // Treat start of pool as the first header
    free_list_head = (block_header_t*)memory_pool;
    free_list_head->size = POOL_SIZE - sizeof(block_header_t);
    free_list_head->is_free = 1;
    free_list_head->next = NULL;
    free_list_head->magic = FREED_MAGIC;

    initialized = 1;
    printf("[INIT] Allocator initialized with %zu bytes\n", free_list_head->size);
}

// Custom malloc implementation
void* my_malloc(size_t size) {
    if (!initialized) init_allocator();
    if (size == 0) return NULL;

    size = align_size(size);
    size_t actual_size = size + sizeof(unsigned int);
    actual_size = align_size(actual_size);

    block_header_t* current = free_list_head;

    // First-fit strategy: find first block that's big enough.
    while (current != NULL) {
        if (current ->is_free && current->size >= actual_size) {
            printf("[ALLOC] Found free block: size=%zu at %p\n", current->size, (void*)current);

            // Should block be split?
            // Only split if remaining space is useful (> MIN_BLOCK_SIZE)
            if (current->size >= actual_size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
                block_header_t* new_block = (block_header_t*) ((char*)current + sizeof(block_header_t) + actual_size);

                new_block->size = current->size - actual_size - sizeof(block_header_t);
                new_block->is_free = 1; // True
                new_block->next = current->next;
                new_block->magic = FREED_MAGIC;

                // Update current block
                current->size = actual_size;
                current->next = new_block;

                printf("[SPLIT] Split block: allocated=%zu, remaining=%zu\n", size, new_block->size);
            }
            current->is_free = 0;
            current->magic = BLOCK_MAGIC; // Valid allocated Block.

            // Return pointer to data area (skip header)
            void* ptr = (char*)current + sizeof(block_header_t);

            // Place canary at the END of the user's data
            unsigned int* end_canary = (unsigned int*)((char*)ptr + size);
            *end_canary = CANARY_VALUE;

            printf("[ALLOC] Returning pointer %p (canary placed at offset %zu)\n", ptr, size);
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

    if (header->magic == FREED_MAGIC) {
        printf("[ERROR] Double free detected at %p!\n", ptr);
        return;
    }

    if (header->magic != BLOCK_MAGIC) {
        printf("[ERROR] Invalid pointer passed to my_free: %p\n", ptr);
        return;
    }

    // Check end canary for buffer overflow
    unsigned int* end_canary = (unsigned int*)((char*)ptr + header->size - sizeof(unsigned int));
    if (*end_canary != CANARY_VALUE) {
        printf("[ERROR] Buffer overflow detected at %p! Canary was 0x%X, expected 0x%X\n",ptr, *end_canary, CANARY_VALUE);
        // Continue to free, but user knows there was corruption.
    } else {
        printf("[CANARY] Buffer overflow check passed\n");
    }

    header->magic = FREED_MAGIC;
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