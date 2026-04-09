This section documents the **Kernel Heap Manager**, the primary system for dynamic memory allocation within the kernel. It is a general-purpose, architecture-independent implementation designed for reliability and ease of debugging.

# Kernel Heap Manager

The heap is implemented as a **doubly-linked list of metadata headers**. It provides a "First-Fit" allocation strategy with immediate coalescing to minimize fragmentation. It is backed by the PMM and VMM, allowing it to grow dynamically as system demands increase.

## Memory Structures

```c
typedef struct HeapSegment {
    size_t size;
    struct HeapSegment* next;
    struct HeapSegment* prev;
    bool is_free;
    uint32_t magic;
    uint32_t caller;
} __attribute__((aligned(4))) HeapSegment;
```

Every allocation is preceded by this header. The `magic` value is verified on every `kfree` and `check_heap` call to catch buffer overflows early.

## Allocation Logic

```c
void* kmalloc(size_t size);
```

Searches the segment chain for the first free block that fits the requested size.
* **Alignment:** Automatically rounds the requested size up to the nearest 4-byte boundary.
* **Splitting:** If a found block is significantly larger than the request (header size + 4 bytes), it is split. A new `HeapSegment` is created for the remainder, keeping the heap "tight."
* **Expansion:** If no suitable block is found, `kheap_expand` is called to map more physical pages and append them to the heap.

## Deallocation and Coalescing

```c
void kfree(void* ptr);
```

Marks a segment as free and immediately performs **bidirectional merging**.
* **Merge Right:** If the next segment is free, it is absorbed into the current one.
* **Merge Left:** If the previous segment is free, the current segment is absorbed into the previous one.
This prevents the "shredded memory" problem where many small free blocks exist but none are large enough for a single allocation.

## Integrity and Monitoring

```c
bool check_heap();
```

Performs a full traversal of the heap to verify:
1. All segments have the correct `HEAP_MAGIC`.
2. Segment pointers are 4-byte aligned.
3. The linked list is consistent (no circular links, backlinks match forward links).

```c
size_t get_heap_total();
size_t get_heap_used();
```

These provide real-time metrics on kernel memory consumption by walking the segment list and summing the sizes of used vs. free blocks.
