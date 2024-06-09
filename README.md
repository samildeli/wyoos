Forked from wyoos part 20 ([repository](https://github.com/AlgorithMan-de/wyoos), [video series](https://www.youtube.com/playlist?list=PLHh55M_Kq4OApWScZyPl5HhgsTJS9MZ6M)).

Implemented round robin scheduling and fork, execve, getpid, waitpid and exit system calls.

To run, first install g++ and xorriso. Then run make, which should generate wyoos.iso. Use VirtualBox to create a VM named wyoos, choose Other for Type and Other/Unknown for Version. From the VM's settings add wyoos.iso as an optical drive. To start/restart the VM, run make run.

You can uncomment line 107 in multitasking.cpp to watch scheduling happen in real time.
