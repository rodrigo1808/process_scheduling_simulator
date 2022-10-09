#include <stdio.h>
#define QUEUE_SIZE 10
#define QUEUES_NUMBER 10
#define QUANTUM 5

void new_process(); // TO DO
void delete_process(); // TO DO
typedef struct
{
    int pid;
} Process;

// TO DO? Structs for Execution, Ready and Blocked queues

typedef struct
{
    /* Each row is a queue and each column is the position of process on queue */
    Process Queues[QUEUES_NUMBER][QUEUE_SIZE];
} Feedback_queue;

int main() {
    printf("[TO DO] Read input table");
    return 0;
}