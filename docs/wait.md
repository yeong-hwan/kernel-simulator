## wait

- System call triggers mode switching; see the specifications from the second assignment for
more details.
- **(Caution)** Note that the point at which the parent process waiting for the wait command
becomes ready and is inserted into the ready queue is the point at which the system call to
the child process' exit command is processed.