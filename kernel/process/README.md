Multitasking is obviously useful, as it lets us run multiple tasks at once. We do this by making the CPU switch from one task to another very rapidly, so that the human eye can't see a difference. Now, its obvious how multitasking works just by that description, but, unfortunately, my idea for multitasking exceeds that, and I have to now explain it.

```c
void init_multitasking();
```

This creates an `init` task, that is the main task, and serves no purpose. It doesn't run anything, it doesn't do anything, it just exists, because its essential for the way my kernel scheduler algorithm works.

```c
task* create_task(void (*entry_point)(), const char* name, const bool privilege)
```

This creates a new task (and returns it). You pass in a function, a name, and the privilege (0 for kernel, 1 for user). The schedulers can then deal with this structure:

Each new task records the task that created it as its `parent`, and has no children at creation. Its neighbors are a circular singly linked list of all the children of its parent. Each parent is not informed of all the children, but has one pointer to a child, `next`. When a new task is created, we snuggle in the task into this parent-child tree strcture.

<table align="right" style="margin-left: 20px;">
  <tr>
    <th colspan="2">Scheduler Algorithm</th>
  </tr>
  <tr>
    <td><img src="../../readme-assets/task1.svg" width="100" alt="Step 1"><br><sub>Step 1</sub></td>
    <td><img src="../../readme-assets/task2.svg" width="100" alt="Step 2"><br><sub>Step 2</sub></td>
  </tr>
  <tr>
    <td><img src="../../readme-assets/task3.svg" width="100" alt="Step 3"><br><sub>Step 3</sub></td>
    <td><img src="../../readme-assets/task4.svg" width="100" alt="Step 4"><br><sub>Step 4</sub></td>
  </tr>
  <tr>
    <td><img src="../../readme-assets/task5.svg" width="100" alt="Step 5"><br><sub>Step 5</sub></td>
    <td><img src="../../readme-assets/task6.svg" width="100" alt="Step 6"><br><sub>Step 6</sub></td>
  </tr>
  <tr>
    <td><img src="../../readme-assets/task7.svg" width="100" alt="Step 7"><br><sub>Step 7</sub></td>
    <td><img src="../../readme-assets/task8.svg" width="100" alt="Step 8"><br><sub>Step 8</sub></td>
  </tr>
  <tr>
    <td><img src="../../readme-assets/task9.svg" width="100" alt="Step 9"><br><sub>Step 9</sub></td>
    <td><img src="../../readme-assets/task10.svg" width="100" alt="Step 10"><br><sub>Step 10</sub></td>
  </tr>
</table>

```c
void schedule();
```

This function is called by the timer every tick. Since the `create_task` function already sets up the whole tree structure, this function has no problem except to just run through that tree:

We start at `init`, and it has a `next` task, which we move to, and we let it execute from some time. Before switching to it, we make sure to point the parent's `next` to the current task's `neighbor` task, which is the next child of the parent. Then we keep moving down through the tree from the root node to a leaf node. Wherever we end up, we will end at a node with no children, a leaf node. Once we reach there, we just move back to init, but the path down has shifted the pointers around, and caused the traversal path to be different, and thus we go down a different child of `init`.

What's intresting is that there is no requirement for computing any "priority", because it seems that comes as a natural consequence of the structure itself.

```c
void kill_task(uint32_t id);
```

It is obvious that the tree structure we have is literally the most biggest hastle to move through. It would be a nightmare to just parse through it just to find a specific task, so I decided I don't like to suffer, and made a new way of storing the tasks. We still traverse through the tree, but we just use a different way to store them. I thought of using arrays, but they were finite, so I thought of a linked list, but they were slow, due to cache-misses, so, I got a better idea: linked list of arrays.

```c
typedef struct task_list {
    task* tasks[TASKS_LIST_LEN];
    task_list_mask_t mask; // uint16_t since 16 tasks per list
    struct task_list* next;
} task_list;
```

This stores out the list of tasks in an array, and if its full, we make a new one, and point to it. We store the head of the linked list though, so we don't lose the actual linked list in memory. Of course, that array, if large, would take up memory, even if nothing is there, but if small, would mean a lot of linked lists. This means, depending upon what you are doing, you have to set this number. Fortunately, this is literally a single number change in the header, and the rest of the header follows suite:

```c
#if TASKS_LIST_LEN == 8
    typedef uint8_t task_list_mask_t;
#elif TASKS_LIST_LEN == 16
    typedef uint16_t task_list_mask_t;
#elif TASKS_LIST_LEN == 32
    typedef uint32_t task_list_mask_t;
#else
    typedef uint64_t task_list_mask_t;
#endif

#define TASK_LIST_MASK_FULL ((task_list_mask_t)((1ULL << TASKS_LIST_LEN) - 1))
```

These happen in compile time, so there is no runtime slowness for all this. I also chose to use bit masks to just make everything faster in the array itself.

```c
task* get_task(uint32_t id);
```

This is just a nice helper function to get the task, instead of having to re-write the linked list traversal yourself.
