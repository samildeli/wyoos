#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

#define MAIN_START asm ("movl %%eax, %0" : "=r" (n)); // Read the argument passed to main.
#define MAIN_END exit(); // Exit main and give control back to kernel.

namespace myos { 
    class SyscallHandler : public hardwarecommunication::InterruptHandler {  
    public:
        SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber, TaskManager *TaskManager);
        ~SyscallHandler();
        
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);
    private:
        TaskManager *taskManager;
    };

    void exit();
    int fork();
    void sysprintf(const char* str);
    void sysprintfHex32(myos::common::uint32_t num);
    int waitpid(int pid);
    void execve(EntryPoint entryPoint, myos::common::uint32_t n);
    int getpid();
}

#endif
