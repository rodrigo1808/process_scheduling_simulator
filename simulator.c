#include <stdio.h>

#define QUEUE_SIZE 10
#define QUEUES_NUMBER 10
#define QUANTUM 5

typedef struct
{
    int pid;
    int time_remaining;
} Process;

typedef struct 
{
    Process processes[QUEUE_SIZE];
    int rear;
} ProcessQueue;

// TO DO? Structs for Execution, Ready and Blocked queues

typedef struct
{
    /* Each row is a queue and each column is the position of process on queue */
    Process queues[QUEUES_NUMBER];
} FeedbackQueue;

void initQueue(ProcessQueue *queue);
void enqueue(ProcessQueue *queue, Process process);
Process dequeue(ProcessQueue *queue);

void new_process(); // TO DO
void delete_process(); // TO DO

int main() {
    ProcessQueue ready_queue;

    initQueue(&ready_queue);

    return 0;
}

void initQueue(ProcessQueue *queue) {
    queue->rear = -1;
}

void enqueue(ProcessQueue *queue, Process process) {
    if (queue->rear == QUEUE_SIZE - 1) {
        printf("Fila cheia! Não é possível inserir.");
    
    } else {
        queue->rear++;
        queue->processes[queue->rear] = process;

    } 
}

Process dequeue(ProcessQueue *queue) {
    if (queue->rear == -1) {
        printf("Fila vazia! Não é possível retirar.");

        Process empty;
        empty.pid = -1;
        return empty;

    } else {
        Process front = queue->processes[0];

        for (int i = 0; i < QUEUE_SIZE - 1; i++) {
            queue->processes[i] = queue->processes[i + 1];

        }

        queue->rear--;
        return front;
    }
}
