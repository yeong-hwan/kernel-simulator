## memory_read arg1

- arg1: Page ID of the page you want to read (a positive integer of 1000 or less)
- This command runs in user mode as a normal user code command.
- If the page corresponding to arg1 is not allocated in physical memory, a page fault occurs and enters kernel mode, where the page fault handler allocates the page to physical memory. If necessary, the page is replaced using the page replacement algorithm.
- No protection fault will occur