## exit

- For more information, see the specifications for the second assignment.
- **(Caution)** When the exit system call is processed in kernel mode, the parent process waiting due to the wait command is inserted into the ready queue.
- During system call processing, all pages in that process are released from physical memory. To be precise, it is same as the result of all the process's memory_release operations being executed. This means that if you only have read access, you will not release thecorresponding frame in physical memory. 