# kernel-simulator

## Virtual Program Commands
- System call: **memory_allocate, memory_release, fork_and_exec, wait, exit**
  - Assume the parent process executes and waits for the execution time of fork_and_exec.
  - Sleep from the second assignment is not implemented in this assignment.
- User code commands: **memory_read, memory_write**

## Documatation

**System Call**
- [memory_allocate](docs/memory_allocate.md)
- [memory_release](docs/memory_release.md)
- [for_and_exec](docs/fork_and_exec.md)
- [wait](docs/wait.md)
- [exit](docs/exit.md)

**User code commands**
- [memeory_write](docs/memory_write.md)
- [memory_read](docs/memory_read.md)
