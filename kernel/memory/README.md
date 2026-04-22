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

# Slab allocator

The heap is often for general purpose, but say we have a lot of the same objects. For that case, we can use the slab allocator to speed it up: just a collection of objects, all of the same type. This makes it so that the lookup is almost instant, since we can just skip by `sizeof(object)`.

> [!NOTE]
> Each of these functions and variables are suffixed with the number of objects they allocate. Each slab is a page big, thus `Slab64` can have objects of size `PAGE_SIZE / 64`. The valid slabs are: `Slab64`, `Slab32`, `Slab16`, and `Slab8`.

```c
Slab* create_slab(uint16_t object_size);
```

This function creates a new slab: takes a single page, initialises all the values, and calculates (as fast as possible) the exponent of the power of 2 that is just over the provided `object_size`. Of course, we hope that the given object is not more than 64 bytes, and if we need that, we'll make a new slab struct for that. This is for less that 64 byte objects.

```c
void delete_slab(Slab* slab);
```

This simply stitches the slabs together and frees it from the PMM. If you want to skip the checks entirely, simple free the slab from PMM yourself.

```c
void* slab_alloc(Slab* head);
```

This function finds the first free slab, and allocates to it. If no slab is found, we create a new one, and don't bother checking once more in the loop. We compute the index in the mask using `__builtin_ctzll` or `__builtin_ctz`, which would translate to just one assembly instruction (cannot go faster than that).

```c
void slab_free(void* ptr);
```

This function computes which slab it is in by the fact that it knows it's in a 64 byte aligned slab, skipping over all checks. We compute the index by basic maths, and also free this page if its empty, and isn't the head. If it was the head, we shouldn't free it, since: if we freed off all objects of this type from the slab, and it was gone, we would have to create a new slab for this object. Perhaps, if this happens often enough, our slab becomes inefficient. Therefore, we leave the head slab alive, to avoid this.
