Multitasking, in summary, is really fast switching of tasks that, to the human eye, look concurrent.

# The Scheduler

The scheduler is the brain of multitasking. It’s responsible for deciding which task gets to use the CPU and for how long. It manages a queue of tasks and ensures that even if a process is hanging or stuck in a loop, the rest of the system stays responsive. By constantly cycling through the tasks, it creates the illusion that everything is happening at once.

# Context Switching

The kernel saves the state of the current task, registers, stack pointer, and instruction pointer, and swaps them for the next task. It is the core mechanism that allows the CPU to move between different threads of execution without losing progress. It has to be incredibly fast and precise; if the state isn't saved perfectly, the program will resume with corrupted data and crash.

# Task Control Block (TCB)

The TCB, `task` is the data structure that represents a process in memory. It holds everything the kernel needs to know about a specific task to keep it running correctly, such as its unique ID, its private stack pointer, and its virtual memory map. When the scheduler picks a new task, it uses the info in the TCB to figure out exactly where the CPU left off.

# Storage

Each task is stored in a parent-child tree relationship, and, thus, finding a specific task is a hastle. Instead of spending CPU cycles just to find a task in memory, we store its pointer in a seperate linked list of arrays.
