#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos {
    struct CPUState {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;
    } __attribute__((packed));
    
    enum class TaskState {
        Ready,
        Running,
        Blocked,
        Terminated
    };

    typedef void (*EntryPoint)(common::uint32_t);

    class Task {
        friend class TaskManager;
    private:
        common::uint8_t stack[4096]; // 4 KiB
        CPUState* cpuState;
        TaskState state = TaskState::Terminated;
        int pid = -1;
        int ppid = -1;
        int waitPid = -2; // not waiting for any child
    };
    
    class TaskManager {
    private:
        Task tasks[256];
        int currentTaskPid;
    public:
        TaskManager(GlobalDescriptorTable* gdt);

        CPUState* Schedule(CPUState* cpuState);

        void Fork(CPUState* cpuState);
        CPUState* Exec(CPUState* cpuState, EntryPoint entryPoint, common::uint32_t n);
        void GetPid(CPUState* cpuState);
        CPUState* WaitPid(CPUState* cpuState, int pid);
        CPUState* Exit(CPUState* cpuState);

        void PrintProcessTable(Task* currentTask, Task* nextTask, int n);
    };
}

#endif
