/*
Author : @volcaniqueo
CMPE 322 Project 2, Implementation of a Scheduler
Type of Scheduler: Preemptive Priority
If there is a tie in priority, Round Robin is used
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CONTEXT_SWITCH 10  // Context switch time is 10ms
#define SILVER_QUANTUM 80  // Round Robin quantum for silver processes is 80ms
#define GOLD_QUANTUM 120  // Round Robin quantum for gold processes is 120ms
#define PLATINUM_QUANTUM 120  // Round Robin quantum for platinum processes is 120ms but it is NOT used
#define MAX_PROCESSES 10  // Maximum number of processes is 10, P1 to P10; given in the project description

/*
The Process Struct definition
*/
typedef struct {
    int pid;  // Process ID, P1 to P10
    int priority;  // Priority of the process, higher number indicates higher priority
    int type; // 0 = silver, 1 = gold, 2 = platinum
    int last_executed_instruction_index;  // Index of the last executed instruction, initially -1
    int quantums_used;  // Number of quantums used, initially 0; resets to 0 when type upgrades
    int completed;  // 0 = not completed, 1 = completed
    int arrivalTime;  // Arrival time of the process, to the system
    int readyTime;  // Arrival time of the process, to the ready queue
    int finish_time;  // Time when the process finishes execution
}Process;

/*
Time take to complete each insruction, stored in array; index is the instruction number
Hardcoded from instructions.txt
0th index is not used
21th index is exit instruction
*/
int instructionTime[22] = {0, 90, 80, 70, 60, 50, 40, 30, 20, 30, 40, 50, 60, 70, 80, 90, 80, 70, 60, 50, 40, 10};
int burstTime[11];
int turnaroundTime[11];
int waitingTime[11];

/*
The contents of the processes stored in a 2D array
Hardcoded from P1.txt, P2.txt, P3.txt etc.
Max number of instructions is in P6.txt with 15 instructions including exit
So, every process is represented by int[15] array
*/
int processes[11][15] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 19, 15, 18, 3, 2, 20, 15, 18, 3, 2, 21, 0, 0, 0},
    {18, 2, 5, 6, 5, 6, 5, 6, 21, 0, 0, 0, 0, 0, 0},
    {8, 7, 12, 11, 13, 16, 19, 8, 7, 21, 0, 0, 0, 0, 0},
    {9, 2, 19, 9, 2, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {9, 2, 19, 9, 2, 2, 19, 9, 2, 19, 21, 0, 0, 0, 0},
    {10, 9, 20, 11, 4, 5, 7, 10, 9, 20, 11, 4, 5, 7, 21},
    {8, 1, 10, 11, 2, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {14, 4, 3, 1, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {19, 12, 9, 1, 7, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {20, 3, 19, 5, 2, 11, 8, 13, 14, 21, 0, 0, 0, 0, 0}
};

Process queue[MAX_PROCESSES];  // Array of Process structs, all processes are stored here
int time = 0;  // System time, increases with discrete steps

/*
Compares two processes based on their type
Platinum > Gold = Silver, as given in the project description
*/
int type_comparator(int a, int b){
    if (a == 2){
        if (b == 2){
            return 0;
        }
        else{
            return 1;
        }
    }else if (b == 2){
        return -1;
    }else{
        return 0;
    }
}

/*
Compares two processes according to the hierarchy given in the project description
Type > Priority > Ready Time > PID
PID comparison is done with alphabetical comparison
*/
int process_comparator(int p, int j){
    Process *a = &queue[p];
    Process *b = &queue[j];
    int a_priority = a->priority;
    int b_priority = b->priority;
    int a_type = a->type;
    int b_type = b->type;
    char a_pid[10];
    char b_pid[10];
    sprintf(a_pid, "%d", a->pid);
    sprintf(b_pid, "%d", b->pid);
    int a_readyTime = a->readyTime;
    int b_readyTime = b->readyTime;

    int type_comparison = type_comparator(a_type, b_type);
    if (type_comparison == 1){
        return 1;
    } else if (type_comparison == -1){
        return -1;
    } else {
        if (a_priority > b_priority){
            return 1;
        } else if (a_priority < b_priority){
            return -1;
        } else {
            if (a_readyTime > b_readyTime){
                return -1;
            } else if (a_readyTime < b_readyTime){
                return 1;
            } else {
                if (strcmp(a_pid, b_pid) < 0){
                    return 1;
                } else {
                    return -1;
                }
            }
        }
    }
}

/*
Checks whether type or priority based preemption is needed
If a platinum or higher priority process is ready, preemption is needed
queue[i].completed == 1 means the process is completed, so it is skipped
queue[i].readyTime > time means the process is not ready yet, so it is skipped
queue[i].pid == 0 means the index does not have any process, so it is skipped
*/
int preemption_detection(int i){
    for (int j = 0; j < MAX_PROCESSES; j++){
        if (j == i || queue[j].completed == 1 || queue[j].pid == 0){
            continue;
        }else{
            if (queue[j].readyTime <= time){
                if (queue[j].type == 2){
                    return 1;
                }
                if (queue[j].priority > queue[i].priority){
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*
The process at index i in the queue is executed
The execution scheme given in the project description is implemented
*/
void execute(int i){
    Process *p = &queue[i];
    int quantum;
    int used_time = 0;
    if (p->type == 0){
        quantum = SILVER_QUANTUM;
    }else if (p->type == 1){
        quantum = GOLD_QUANTUM;
    }

    while (1) {
        p->last_executed_instruction_index++;
        int instruction = processes[p->pid][p->last_executed_instruction_index];
        time += instructionTime[instruction];
        used_time += instructionTime[instruction];
        if (processes[p->pid][p->last_executed_instruction_index] == 21){  // Last instruction is exit, process is completed
            p->quantums_used++;
            p->finish_time = time;
            p->completed = 1;
            break;
        }
        if (p->type != 2 && preemption_detection(i)){  // Platinum processes can not be preempted
            p->quantums_used++;
            p->readyTime = time;
            if ((p->quantums_used == 3 && p->type == 0) || (p->quantums_used == 5 && p->type == 1)){
                p->type++;
                p->quantums_used = 0;
            }   
            break;
        }
        if (p->type != 2 && used_time >= quantum){  // Gold and Silver processes are preempted after their Round Robin quantum is used
            p->quantums_used++;
            p->readyTime = time;
            if ((p->quantums_used == 3 && p->type == 0) || (p->quantums_used == 5 && p->type == 1)){
                p->type++;
                p->quantums_used = 0;
            }
            break;
        }
    }
}

/*
Function to choose the next process to be executed
Returns the index of the process in the queue array
If there is no process to be executed, returns -1; (STILL LAST ARRIVAL TIME SHOULD BE CHECKED)
Comparison is done with process_comparator function
queue[i].completed == 1 means the process is completed, so it is skipped
queue[i].readyTime > time means the process is not ready yet, so it is skipped
queue[i].pid == 0 means the index does not have any process, so it is skipped
*/
int choose_process(){
    int i = 0;
    for (int j = 1; j < MAX_PROCESSES; j++){
        if (queue[i].completed == 1 || queue[i].readyTime > time || queue[i].pid == 0){
            i = j;
            continue;
        }
        if (queue[j].completed == 1 || queue[j].readyTime > time || queue[j].pid == 0){
            continue;
        }else{
            if (process_comparator(i, j) == 1){
                continue;
            }else{
                i = j;
                continue;
            }
        }
    }
    if (queue[i].completed == 1 || queue[i].pid == 0 || queue[i].readyTime > time){
        return -1;
    }else{
        return i;
    } 
}
    
int main(){
    /*
    Read the file definition.txt
    File consists of lines of the form: P<pid> <priority> <arrival_time> <type>
    Creates a Process struct for each line and stores it in an array named queue
    */
    FILE *fp;
    fp = fopen("definition.txt", "r");
    char pid[3];
    int priority;
    int arrivalTime;
    char type[10];
    int i = 0;
    int lastArrivalTime = 0;  // Last arrival time is needed to check if there is a process to be executed in the future
    while(fscanf(fp, "%s %d %d %s", pid, &priority, &arrivalTime, type) != EOF){
        queue[i].pid = atoi(&pid[1]);
        queue[i].priority = priority;
        queue[i].arrivalTime = arrivalTime;
        queue[i].readyTime = arrivalTime;
        queue[i].last_executed_instruction_index = -1;
        queue[i].finish_time = 0;
        queue[i].quantums_used = 0;
        queue[i].completed = 0;
        if(strcmp(type, "SILVER") == 0){
            queue[i].type = 0;
        }
        else if(strcmp(type, "GOLD") == 0){
            queue[i].type = 1;
        }
        else if(strcmp(type, "PLATINUM") == 0){
            queue[i].type = 2;
        }
        if (arrivalTime > lastArrivalTime){
            lastArrivalTime = arrivalTime;
        }
            
        i++;
    }
    fclose(fp);

    int next = choose_process();  // Choose the first process to be executed, at the time 0
    time += CONTEXT_SWITCH;  // The system starts with context switch, given in the project description
    int last_process = next;
    while (next != -1){  // next == -1 means there is no process to be executed
        if (next != -2){  // next == -2 means there is no process to be executed yet, but there is a process to be executed later
            execute(next);
        }
        next = choose_process();
        if (next == -1 && time < lastArrivalTime){  // If last arrival time is not reached yet, there is a process to be executed later
            time += 1;  // Time is increased by 1ms, until reaches next arrival time, or last arrival time at most
            next = -2;
        }
            
        if (next != last_process && next != -2){  // Last process is changed, context switch is needed
            time += CONTEXT_SWITCH;
        }
        last_process = next;
    }

    int num_of_processes = 0;  // Counts the number of processes in the queue such that queue[i].pid != 0

    /*
    For loop to calculate burst time, turnaround time and waiting time for each process
    */
    for (int i = 0; i < MAX_PROCESSES; i++){
        int pid = queue[i].pid;
        if (pid != 0){
            int burst = 0;
            for (int j = 0; j < 15; j++){
                burst += instructionTime[processes[pid][j]];
            }
            burstTime[pid] = burst;
            turnaroundTime[pid] = queue[i].finish_time - queue[i].arrivalTime;
            waitingTime[pid] = turnaroundTime[pid] - burstTime[pid];
            num_of_processes++;
        }
    }

    int total_turnaround_time = 0;
    int total_waiting_time = 0;

    /*
    For loop to calculate total turnaround time and total waiting time
    */
    for (int i = 1; i <= MAX_PROCESSES; i++){
        total_turnaround_time += turnaroundTime[i];
        total_waiting_time += waitingTime[i];
    }

    float average_turnaround_time = (float)total_turnaround_time / (float)num_of_processes;
    float average_waiting_time = (float)total_waiting_time / (float)num_of_processes;

    /*
    Average turnaround time and average waiting time printed to stdout with .1 precision if type is float
    If type is int, printed directly
    */

    if (average_waiting_time - (int) average_waiting_time == 0){
        printf("%d\n", (int) average_waiting_time);
    }else{
        printf("%.1f\n", average_waiting_time);
    }

    if (average_turnaround_time - (int) average_turnaround_time == 0){
        printf("%d\n", (int) average_turnaround_time);
    }else{
        printf("%.1f\n", average_turnaround_time);
    }

    return 0;
}






    


