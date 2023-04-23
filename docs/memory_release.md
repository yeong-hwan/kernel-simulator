## memory_release arg1

- arg1: a positive integer of 100 or less
  - Frees the pages corresponding to the allocation ID given by arg1 from virtual memory. If the pages also exist in physical memory, they are also freed from physical memory.
  - If the contents of the pages being freed were shared with one or more child processes due to CoW, they will be copied to each of the child processes at the time of the parent process’ memory release. This means that after this point, the parent process’ pages will no longer exist, and each page/frame for that Page ID will be independent in the child processes.
    - During this process, the child process's page becomes independent of the parent process's page, and the parent and child processes have ‘W’ permission to the copied page. However, at this point, it doesn’t lead to fault handling and the child process's copied page is not immediately allocated in physical memory. The next time a write or read command is issued by the child process, the page fault occurs and the physical memory allocation is executed. 
  - If the page being freed is shared with the parent process and has not yet been copied, only the page of child process is removed from virtual memory. This means that if you only have Read permission, the page is not freed from physical memory.
- This command is a system call, which causes a mode switch to kernel mode.
  - This means that the memory allocation operations described above are done in kernel mode

