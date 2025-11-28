# Custom Memory Allocator

## Implementations

### Static Pool Allocator (v1.0)
Educational implementation using a fixed static memory pool.
- Simple and portable
- Demonstrates core concepts
- Perfect for learning

[See allocator_static.c]

### Dynamic Heap Allocator (v2.0)
More realistic implementation using OS memory allocation.
- Uses mmap() on Linux/Unix
- Can allocate larger pools
- Closer to production allocators

[See allocator_dynamic.c]

## Features
- First-fit allocation strategy
- Block coalescing
- Double-free detection
- Buffer overflow protection (canaries)
- 8-byte alignment
