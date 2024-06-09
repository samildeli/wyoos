#include <multitasking.h>
#include <syscalls.h>
#include <programs.h>

using namespace myos;
using namespace myos::common;

#define len(arr) (sizeof(arr) / sizeof(arr[0]))

void printf(char* str);
void printfHex32(uint32_t key);

void initMain() {
    if (fork() == 0) {
        execve(collatz, 7);
    }

    if (fork() == 0) {
        execve(collatz, 10);
    }

    if (fork() == 0) {
        execve(collatz, 15);
    }

    if (fork() == 0) {
        execve(longRun, 15000);
    }

    if (fork() == 0) {
        execve(longRun, 45000);
    }

    if (fork() == 0) {
        execve(longRun, 30000);
    }

    int pid;
    while ((pid = waitpid(-1)) != -1) {
        printf("Process ");
        printfHex32(pid);
        printf(" is terminated.\n");
    }

    printf("All processes are terminated.\n");

    while (true) {}
}


TaskManager::TaskManager(GlobalDescriptorTable* gdt) {
    // Initialize the init task.
    Task *init = &tasks[0];
    
    init->cpuState = (CPUState*)(init->stack + sizeof(init->stack) - sizeof(CPUState));
    *init->cpuState = {};

    init->cpuState->eip = (uint32_t)initMain;
    init->cpuState->cs = gdt->CodeSegmentSelector();
    init->cpuState->eflags = 0x202;
    init->cpuState->esp = (uint32_t)init->cpuState;

    init->state = TaskState::Ready;
    init->pid = 0;

    currentTaskPid = 0;
}

CPUState* TaskManager::Schedule(CPUState* cpuState) {
    Task *currentTask = &tasks[currentTaskPid];

    if (currentTask->state == TaskState::Ready) {
        // Special case for running the init task for the first time.
        /* This is the only time the state of currentTask is Ready, meaning its cpuState points to the
         * correct location on its stack and the cpuState we receive as argument points to kernel's stack. */
        currentTask->state = TaskState::Running;
        return currentTask->cpuState;
    }

    // Try to find the next task that is ready.
    int nextTaskPid = currentTaskPid;
    while (tasks[nextTaskPid].state != TaskState::Ready) {
        nextTaskPid = (nextTaskPid + 1) % len(tasks);
        if (nextTaskPid == currentTaskPid) {
            // No other task is ready.
            if (currentTask->state == TaskState::Running) {
                // Continue running the current task.
                return cpuState;
            } else {
                // No task is ready or running.
                while (true) {
                    printf("DEADLOCK\n");
                }
            }
        }
    }

    Task *nextTask = &tasks[nextTaskPid];
    currentTaskPid = nextTaskPid;

    if (currentTask->state == TaskState::Running) {
        currentTask->state = TaskState::Ready;
    }

    nextTask->state = TaskState::Running;

    // PrintProcessTable(currentTask, nextTask, 7);

    // Save the stack pointer of the current task.
    currentTask->cpuState = cpuState;
    // Return the stack pointer of the next task and it will be used to restore its registers from its stack.
    return nextTask->cpuState;
}

void TaskManager::Fork(CPUState* cpuState) {
    // Try to find a task with a terminated state to use for the child task.
    int childPid = 0;
    while (tasks[childPid].state != TaskState::Terminated) {
        childPid++;
        if (childPid >= len(tasks)) {
            // No task with a terminated state, return error.
            cpuState->eax = -1;
            return;
        }
    }

    Task *parent = &tasks[currentTaskPid];
    Task *child = &tasks[childPid];

    parent->cpuState = cpuState;
    
    for (int i = 0; i < len(child->stack); i++) {
        child->stack[i] = parent->stack[i];
    }

    // Make child's pointers point to correct locations on its stack.
    uint32_t diff = (uint32_t)child->stack - (uint32_t)parent->stack;
    child->cpuState = (CPUState*)((uint32_t)parent->cpuState + diff);
    child->cpuState->esp = (uint32_t)child->cpuState;
    child->cpuState->ebp = parent->cpuState->ebp + diff;

    child->pid = childPid;
    child->ppid = currentTaskPid;
    child->state = TaskState::Ready;
    child->cpuState->eax = 0; // Return 0 to child.

    // Return childPid to parent.
    cpuState->eax = childPid;
}

void EntryPointWrapper() {
    EntryPoint entryPoint;
    uint32_t n;
    
    asm ("movl %%eax, %0" : "=r" (entryPoint));
    asm ("movl %%edx, %0" : "=r" (n));

    entryPoint(n);

    exit();
}

CPUState* TaskManager::Exec(CPUState* cpuState, EntryPoint entryPoint, uint32_t n) {
    Task *task = &tasks[currentTaskPid];

    // Reset stack pointer.
    task->cpuState = (CPUState*)(task->stack + sizeof(task->stack) - sizeof(CPUState));

    // Copy old cpuState to carry over constant registers (cs specifically).
    *task->cpuState = *cpuState;

    // Reset other registers (set eax and edx for use in EntryPointWrapper).
    task->cpuState->eax = (uint32_t)entryPoint;
    task->cpuState->ebx = 0;
    task->cpuState->ecx = 0;
    task->cpuState->edx = n;

    task->cpuState->esi = 0;
    task->cpuState->edi = 0;
    task->cpuState->ebp = 0;

    task->cpuState->eip = (uint32_t)EntryPointWrapper;
    task->cpuState->eflags = 0x202;
    task->cpuState->esp = (uint32_t)task->cpuState;
    task->cpuState->ss = 0;

    return task->cpuState;
}

void TaskManager::GetPid(CPUState* cpuState) {
    cpuState->eax = currentTaskPid;
}

CPUState* TaskManager::WaitPid(CPUState* cpuState, int pid) {
    Task *task = &tasks[currentTaskPid];

    if (pid == -1) {
        // Look for any child.
        for (int childPid = 1; childPid < len(tasks); childPid++) {
            if (tasks[childPid].ppid == task->pid) {
                // Don't block if the child is already terminated.
                if (tasks[childPid].state == TaskState::Terminated) {
                    cpuState->eax = childPid; // Return child pid.
                    task->waitPid = -2; // Stop waiting for child.
                    tasks[childPid].ppid = -1; // Won't have this child anymore.
                    return cpuState;
                }

                task->state = TaskState::Blocked;
                task->waitPid = -1;
                return Schedule(cpuState);
            }
        }

        // No child found, return error.
        cpuState->eax = -1;
        return cpuState;
    }

    if (tasks[pid].ppid == task->pid) {
        // Don't block if the child is already terminated.
        if (tasks[pid].state == TaskState::Terminated) {
            cpuState->eax = pid; // Return child pid.
            task->waitPid = -2; // Stop waiting for child.
            tasks[pid].ppid = -1; // Won't have this child anymore.
            return cpuState;
        }

        task->state = TaskState::Blocked;
        task->waitPid = pid;
        return Schedule(cpuState);
    }

    // Given pid is not a child, return error.
    cpuState->eax = -1;
    return cpuState;
}

CPUState* TaskManager::Exit(CPUState* cpuState) {
    Task *task = &tasks[currentTaskPid];
    Task *parent = &tasks[task->ppid];

    // Check if parent was waiting for this or any task to be terminated.
    if (parent->waitPid == task->pid || parent->waitPid == -1) {
        parent->cpuState->eax = task->pid; // Return child pid to parent.
        parent->waitPid = -2; // Parent stops waiting for child.
        parent->state = TaskState::Ready;
        task->ppid = -1; // Parent won't have this child anymore.
    }

    task->state = TaskState::Terminated;

    return Schedule(cpuState);
}

void TaskManager::PrintProcessTable(Task* currentTask, Task* nextTask, int n) {
    printf("c");
    printf("---- Switching from ");
    printfHex32(currentTask->pid);
    printf(" to ");
    printfHex32(nextTask->pid);
    printf(" ----\n");
    printf("| PID      | STATE    | PPID     | WAITPID  |\n");
    for (int i = 0; i < n; i++) {
        printf("| ");
        printfHex32(tasks[i].pid);
        printf(" |");
        switch (tasks[i].state)
        {
        case TaskState::Ready:
            printf(" Ready    ");
            break;
        case TaskState::Running:
            printf(" Running  ");
            break;
        case TaskState::Blocked:
            printf(" Blocked  ");
            break;
        case TaskState::Terminated:
            printf(" Term.    ");
            break;
        }
        printf("| ");
        printfHex32(tasks[i].ppid);
        printf(" | ");
        printfHex32(tasks[i].waitPid);
        printf(" |\n");
    }
    printf("---------------------------------------------\n");
}
