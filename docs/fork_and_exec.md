## fork_and_exec arg1

- arg1: the mane of the program to run (string)
  - Programs with the same name but with different process IDs can be run simultaneously.
- This command is a system call, which causes a mode switch to kernel mode.
- Creates a new process. ID of the created process will be ‘last created process ID + 1’. Once used, process IDs are not reused.
- A newly created child process copies the virtual memory and page table of its parent process based on Copy-on-Write at the time of creation (i.e., during system call processing). The pages of the parent and child processes point to the same frame of physical memory, and are copied from the parent to the child when the write command for the page is executed.
  - The contents of the parent process’ page ID are copied based on CoW.
  - All the pages of the parent and child processes becomes protected as ‘R’ permission.
  - The parent process’s Allocation ID is also copied.
- For more information, see the specifications for the second assignment.
- In this assignment, only init process calls this system call.