Anything that interacts and changes how the CPU works goes here.

# Interrupts

Making a kernel, a piece of software, manually manage everything by constantly checking stuff like keyboard inputs, mouse inputs, page faults, etc. is really slow, and throttles our performance. Often times, when such situations show up, the hardware comes to the rescue.

Interrupts are architecture specified addresses that the CPU jumps to, whenever a specific 'event' is triggered. If a key on the keyboard is pressed, it jumps to the keyboard interrupt, and if we enounter a page fault, it jumps to its address. This keeps everything lean, and throws all the overhead of these checks to the CPU.

This is actually architecture dependant, so you will find its respective implementation in each of the arch files under the unique name of that architecture (eg: IDT for x86, EVT for ARM).

```c
void init_interrupts();
```

This function sets all the addresses up, and informs whatever architecture the kernel is running on of all the things to watch out for. Since the CPU only jumps to these addresses, we actually have to tell it what to exactly do once we reach these addresses. These are defined in assembly functions that call functions defined in C.

# System Timer

Without a hardware timer, the kernel has no concept of time passing. It wouldn't be able to multitask, and a single process could hold the CPU forever. To solve this, we use a hardware timer chip that acts like the "heartbeat" of the system.

The timer is a hardware device that sends a periodic interrupt signal to the CPU at a fixed frequency. Every time this "tick" occurs, the CPU pauses current execution and jumps to the timer interrupt handler. This allows the kernel to regain control, update system uptime, and decide if it's time to switch to a different task (preemptive multitasking).

This is architecture specfic, and you will find the specific implementations in the arch folder.

```c
void init_timer(uint32_t frequency);
```

This function calculates the necessary divisors for the hardware clock to fire at the requested frequency (in Hz). It then registers the timer interrupt handler so the kernel can begin tracking time and performing context switches.
