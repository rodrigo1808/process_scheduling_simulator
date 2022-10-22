#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* constante com o tamanho da tabela de processos
    (o maximo que ele suporta em numero de processos) */
#define PROCESS_TABLE_SIZE 200

/* constantes com a duração de cada um dos tipos de IO */
#define DISK_IO_DURATION 5
#define TAPE_IO_DURATION 8
#define PRINTER_IO_DURATION 14

/* constantes para definir o range maximo e minimo
    da geração de pid e tempos de serviço aleatório,
    a prioridade padrão de cada processo e o máximo
    de vezes que um processo pode pedir io */
#define MAX_PID 9999
#define MIN_PID 1000
#define MAX_SERVICE_TIME 20
#define MIN_SERVICE_TIME 2
#define INITIAL_PRIORITY 0
#define MAX_IO_REQ 3

/* constantes com os valores do status possiveis */
#define EXECUTION_STATUS "execution"
#define READY_STATUS "ready"
#define BLOCKED_STATUS "blocked"

/* constantes para o trabalho do escalonador */
#define QUEUE_SIZE 20
#define QUEUES_NUMBER 2     // 2 filas: alta e baixa prioridade
#define QUANTUM 5

/* probabilidade de gerar um processo por clock (em %) */
#define PROBABILTY_FOR_GENERATE_PROCESS 20

/* struct de io que o processo precisa para pedir vários */
typedef struct {
    char type[10];                       // tipo de io
    int duration;                       // duração do io
    int req_instant;                    // instante que o processo pede io a partir de seu inicio
    int current_time;
} Io;

/* struct com as informações do processo, ou PCB */
typedef struct {
    int pid;                            // pid do processo
    int ppid;                           // pid do processo pai que o crio
    int service_time;                   // tempo de serviço do processo
    int priority;                       // prioridade do processo
    char status[9];                     // status do processo
    int current_time;                   // tempo de serviço executado ate o momento
    Io io[MAX_IO_REQ];                  // ios pedidos pelo processo
    int io_req_quantity;                // variavel auxiliar para quantificar os ios
    int io_req_remaining;
} Process;

/* struct para construir a estrutura de dados das filas */
typedef struct {
    Process processes[QUEUE_SIZE];      // fila de processos com o tamanho definido pela constante
    int rear;                           // posição do último da fila
} ProcessQueue;

/* struct para montar a lista dos processos que estão ativos no "sistema".
    é necessário para não ser possível criar processos com pid's repetidos. */
typedef struct {
    Process running_processes[PROCESS_TABLE_SIZE];  // tabela dos processos que estão rodando no sistema
    int length;                                     // quantidade desses processos
} ProcessTable;

int timer = 0;
int unit_time_from_quantum = 0; // quanto do quantum já foi utilizado no atual processo
void clockTimeUnit();

void initProcessTable();

void initQueue(ProcessQueue *queue);
void enqueue(ProcessQueue *queue, Process process);
Process dequeue(ProcessQueue *queue);

Process createProcess();
int containsPid(int pid);
void generateIoRequests(Process *process);
void destroyProcess(Process process);

int generateRandomNumber(int lower_limit, int upper_limit);
void printQueues();
void generateProcessesRandomly();
void runCPU();
Io shouldRunIo();
void runIOs();
void run_disk();
void run_printer();
void run_tape();

ProcessTable table;
ProcessQueue feedback_queue[QUEUES_NUMBER];

Process cpu_running;

ProcessQueue disk_queue;
ProcessQueue printer_queue;
ProcessQueue tape_queue;

Process disk_running;
Process printer_running;
Process tape_running;

Process blank_process;

int main() {
    srand(time(NULL));                      // inicialização para randomizar os números

    for (int i = 0; i < QUEUES_NUMBER; i++)
    {
        initQueue(&feedback_queue[i]);
    }

    // Filas diferentes para cada tipo de dispositivo
    initQueue(&disk_queue);
    initQueue(&printer_queue);
    initQueue(&tape_queue);

    blank_process = createProcess();
    blank_process.pid = -1;
    blank_process.service_time = -1;

    cpu_running = blank_process;
    disk_running = blank_process;
    printer_running = blank_process;
    tape_running = blank_process;

    clockTimeUnit();
    return 0;
}

void clockTimeUnit() {
    while(1) {
        printf("\n------------------ %d ------------------ \n", timer++);

        generateProcessesRandomly();

        printQueues();

        runIOs();
        runCPU();

        sleep(1);
    }
}

void printQueues() {
    printf("Fila de Prontos\n");

    printf("Prioridade Alta [ ");

    // printf("rear %d\n", feedback_queue[0].rear);
    for (int i = 0; i < feedback_queue[0].rear + 1; i++)
    {
        printf("%d ", feedback_queue[0].processes[i]);
    }

    printf("]\n");

    printf("Prioridade Baixa [ ");

    // printf("rear %d\n", feedback_queue[0].rear);
    for (int i = 0; i < feedback_queue[QUEUES_NUMBER - 1].rear + 1; i++)
    {
        printf("%d ", feedback_queue[QUEUES_NUMBER - 1].processes[i]);
    }

    printf("]\n");

    printf("\n");

    printf("Fila de Bloqueados - Aguardando DISCO [ ");

    for (int i = 0; i < disk_queue.rear + 1; i++)
    {
        printf("%d ", disk_queue.processes[i]);
    }

    printf("]\n");

    printf("Fila de Bloqueados - Aguardando FITA [ ");

    for (int i = 0; i < tape_queue.rear + 1; i++)
    {
        printf("%d ", tape_queue.processes[i]);
    }

    printf("]\n");

    printf("Fila de Bloqueados - Aguardando IMPRESSORA [ ");

    for (int i = 0; i < printer_queue.rear + 1; i++)
    {
        printf("%d ", printer_queue.processes[i]);
    }

    printf("]\n");

    printf("\n");
}

/* Gera processos aleatoriamente dada a probabilidade PROBABILTY_FOR_GENERATE_PROCESS */
void generateProcessesRandomly() {
    int random_number = generateRandomNumber(0, 100);
    if(random_number < PROBABILTY_FOR_GENERATE_PROCESS) {
        Process new_process = createProcess();
        printf("\nNovo Processo\n", new_process.pid);
        printf("PID: %d\nPPID: %d\nTempo de Servico: %d\nVezes que pede IO: %d\n",
                    new_process.pid, new_process.ppid, new_process.service_time, new_process.io_req_quantity);
        for (int i = 0; i < new_process.io_req_quantity; i++)
        {
            printf("IO: %s - Duracao: %d - Pedido no instante de tempo: %d\n", new_process.io[i].type, new_process.io[i].duration, new_process.io[i].req_instant);
        }

        printf("\n");

        // Processos novos - entram na fila de alta prioridade;
        enqueue(&feedback_queue[0], new_process);
    }
}

void runCPU() {
    // Caso a CPU esteja sem processos
    if(cpu_running.pid == -1) {

        Process to_execute;

        // Busca processo na fila
        for (int i = 0; i < QUEUES_NUMBER; i++)
        {
            to_execute = dequeue(&feedback_queue[i]);

            if(to_execute.pid != -1) {
                break;
            }
        }

        // Caso encontre um processo na fila
        if(to_execute.pid != -1) {
            cpu_running = to_execute;
            printf("Novo processo executando na CPU - %d\n", cpu_running.pid);
        } else {
            printf("CPU ociosa\n");
        }
    }

    // Caso a CPU esteja executando um processo
    if(cpu_running.pid != -1) {
        cpu_running.current_time++;

        printf("CPU rodando processo %d - Quantum %d - Tempo restante %d - Tempo executado %d\n", cpu_running.pid, unit_time_from_quantum + 1, cpu_running.service_time - cpu_running.current_time, cpu_running.current_time);
        unit_time_from_quantum++;

        shouldRunIo(cpu_running);



        if(cpu_running.current_time == cpu_running.service_time) {
            printf("Processo %d Terminou\n", cpu_running.pid);
            cpu_running = blank_process;
            unit_time_from_quantum = 0;
        } else if (unit_time_from_quantum == QUANTUM) {
            printf("\nPreempcao\n\n");

            // Processos que sofreram preempção – retornam na fila de baixa prioridade.
            enqueue(&feedback_queue[QUEUES_NUMBER-1], cpu_running);

            cpu_running = blank_process;
            unit_time_from_quantum = 0;
        }
    }
}

Io shouldRunIo(Process process) {
    Io to_run;

    if(process.io_req_remaining > 0) {
        if(process.io[0].req_instant == process.current_time) {
            to_run = process.io[0];

            printf("Processo %d pediu o IO %s\n", process.pid, to_run.type);
            if (strcmp(to_run.type, "DISK") == 0) {
                enqueue(&disk_queue, process);
                cpu_running = blank_process;
                unit_time_from_quantum = 0;
            } else if(strcmp(to_run.type, "TAPE") == 0) {
                enqueue(&tape_queue, process);
                cpu_running = blank_process;
                unit_time_from_quantum = 0;
            } else if(strcmp(to_run.type, "PRINTER") == 0) {
                enqueue(&printer_queue, process);
                cpu_running = blank_process;
                unit_time_from_quantum = 0;
            }
        } else {
            to_run.duration = -1;
        }
    } else {
        to_run.duration = -1;
    }

    return to_run;
}

void runIOs() {
    run_disk();
    run_printer();
    run_tape();
}

void run_disk() {
    if(disk_running.pid == -1) {
        Process to_execute = dequeue(&disk_queue);

        if(to_execute.pid != -1) {
            disk_running = to_execute;
            printf("Novo processo usando o DISCO - %d\n", disk_running.pid);
        } else {
            printf("DISCO ocioso\n");
        }
    }

    if(disk_running.pid != -1) {
        disk_running.io[0].current_time++;

        printf("DISCO rodando pedido do processo %d - Tempo restante %d - Tempo executado %d\n", disk_running.pid, disk_running.io[0].duration - disk_running.io[0].current_time, disk_running.io[0].current_time);

        if(disk_running.io[0].current_time == disk_running.io[0].duration) {
            printf("DISCO terminou o pedido do processo %d\n", disk_running.pid);

            for (int i = 0; i < MAX_IO_REQ - 1; i++) {
                disk_running.io[i] = disk_running.io[i + 1];
            }

            disk_running.io_req_remaining--;

            // Disco – retorna para a fila de baixa prioridade
            enqueue(&feedback_queue[QUEUES_NUMBER - 1], disk_running);

            disk_running = blank_process;
        }
    }
}

void run_printer() {
    if(tape_running.pid == -1) {
        Process to_execute = dequeue(&tape_queue);

        if(to_execute.pid != -1) {
            tape_running = to_execute;
            printf("Novo processo usando FITA - %d\n", tape_running.pid);
        } else {
            printf("FITA ociosa\n");
        }
    }

    if(tape_running.pid != -1) {
        tape_running.io[0].current_time++;

        printf("FITA rodando pedido do processo %d - Tempo restante %d - Tempo executado %d\n", tape_running.pid, tape_running.io[0].duration - tape_running.io[0].current_time, tape_running.io[0].current_time);

        if(tape_running.io[0].current_time == tape_running.io[0].duration) {
            printf("FITA terminou o pedido do processo %d\n", tape_running.pid);

            for (int i = 0; i < MAX_IO_REQ - 1; i++) {
                tape_running.io[i] = tape_running.io[i + 1];
            }

            tape_running.io_req_remaining--;

            // Impressora - retorna para a fila de alta prioridade
            enqueue(&feedback_queue[0], tape_running);

            tape_running = blank_process;
        }
    }
}

void run_tape() {
    if(printer_running.pid == -1) {
        Process to_execute = dequeue(&printer_queue);

        if(to_execute.pid != -1) {
            printer_running = to_execute;
            printf("Novo processo usando IMPRESSORA - %d\n", printer_running.pid);
        } else {
            printf("IMPRESSORA ociosa\n");
        }
    }

    if(printer_running.pid != -1) {
        printer_running.io[0].current_time++;

        printf("IMPRESSORA rodando pedido do processo %d - Tempo restante %d - Tempo executado %d\n", printer_running.pid, printer_running.io[0].duration - printer_running.io[0].current_time, printer_running.io[0].current_time);

        if(printer_running.io[0].current_time == printer_running.io[0].duration) {
            printf("IMPRESSORA terminou o pedido do processo %d\n", printer_running.pid);

            for (int i = 0; i < MAX_IO_REQ - 1; i++) {
                printer_running.io[i] = printer_running.io[i + 1];
            }

            printer_running.io_req_remaining--;

            // Fita magnética - retorna para a fila de alta prioridade
            enqueue(&feedback_queue[0], printer_running);

            printer_running = blank_process;
        }
    }
}

/* Inicializa a tabela de processos */
void initProcessTable() {
    table.length = 0;
}

/* Inicializa a estrutura de dados de numa fila */
void initQueue(ProcessQueue *queue) {
    queue->rear = -1;
}

/* Adiciona um processo numa fila de processos. Quando está cheia,
    printa na tela uma mensagem e não adiciona. */
void enqueue(ProcessQueue *queue, Process process) {
    if (queue->rear == QUEUE_SIZE - 1) {
        printf("Fila cheia! Nao e possivel inserir.\n");
    } else {
        queue->rear++;
        queue->processes[queue->rear] = process;
    }
}

/* Remove um processo na primeira posição de uma fila e
    retorna ele. Na remoção, ele anda com todos os
    processos uma posição para frente.

    Quando a fila está vazia, printa na tela uma mensagem
    e retorna um processo com pid inválido,
    indicando que não foi possível fazer a operação */
Process dequeue(ProcessQueue *queue) {
    if (queue->rear == -1) {
        //printf("Fila vazia! Nao e possivel retirar.\n");

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

/* Cria um processo, inicializando todos os dados do PCB e
    adicionando-o na tabela de processos.

    Se a tabela estiver cheia, printa na tela uma mensagem
    e retorna um processo com pid inválido, indicando que
    não foi possível fazer a operação.

    Dados inicializados:
        ppid: pid do processo do programa (o "pai")
        pid: um número aleatório de 4 digitos, único na tabela de processos
        service_time: um número aleatório entre 1 e 20, definindo o tempo
            de execução que o processo precisa
        priority: número definido na constante INITIAL_PRIORITY
        io: ios que o processo irá pedir, com duração e instantes definidos

    */
Process createProcess() {
    if (table.length == PROCESS_TABLE_SIZE) {
        printf("Tabela cheia! Não é possível criar mais processo.\n");

        Process empty;
        empty.pid = -1;
        return empty;
    }

    Process new_process;
    new_process.ppid = getpid();
    new_process.pid = generateRandomNumber(MIN_PID, MAX_PID);
    while (containsPid(new_process.pid)) {
        new_process.pid = generateRandomNumber(MIN_PID, MAX_PID);

    }
    new_process.service_time = generateRandomNumber(MIN_SERVICE_TIME, MAX_SERVICE_TIME);
    new_process.current_time = 0;
    new_process.priority = INITIAL_PRIORITY;
    generateIoRequests(&new_process);

    table.running_processes[table.length] = new_process;
    table.length++;

    return new_process;
}

/* Verifica se já existe um processo com um certo pid */
int containsPid(int pid) {
    for (int i = 0; i < table.length; i++) {
        if (table.running_processes[i].pid == pid) {
            return 1;
        }
    }

    return 0;
}

/* Gera os requisições de IO que o processo irá pedir.
    Para ser possível, ele deve ter pelo menos 2 de
    tempo de serviço, pois ele precisa de 1 para
    inicializar e 1 para finalizar. Além disso, cada
    processo tem um número máximo de pedidos de IO que
    não pode ultrapassar, sendo definido pela constante
    MAX_IO_REQ

    Tendo a possibilidade de pedir IO, é gerado um
    número aleatório para determinar qual tipo o
    processo irá pedir. São eles:
        - 0: o processo não irá pedir IO;
        - 1: o processo irá pedir IO do disco;
        - 2: o processo irá pedir IO da fita;
        - 3: o processo irá pedir IO da impressora;

    Pedindo de fato IO, suas durações variam de acordo
    com suas respectivas constantes e os instantes são
    determinados aleatoriamente dentro dos limites
    possíveis.

    Havendo vários pedidos de IO, cada um é gerado
    consecutivamente, ou seja, o primeiro é gerado entre
    o instante 1 e o tempo de serviço - 1 e o próximo é
    gerado entre o instante do último pedido + 1 e o
    tempo de serviço - 1.
    */
void generateIoRequests(Process *process) {
    int io_type;
    int possible_next_req_instant = 1;

    process->io_req_quantity = 0;
    process->io_req_remaining = 0;

    if (process->service_time > 1) {
        while (process->io_req_quantity < 3 && (process->io_req_quantity == 0 ||
            process->service_time - possible_next_req_instant > 1)) {

            io_type = generateRandomNumber(0,3);

            if (io_type == 0) {
                break;
            } else if (io_type == 1) {
                process->io[process->io_req_quantity].duration = DISK_IO_DURATION;
                strcpy(process->io[process->io_req_quantity].type, "DISK");
                process->io[process->io_req_quantity].current_time = 0;
            } else if (io_type == 2) {
                process->io[process->io_req_quantity].duration = TAPE_IO_DURATION;
                strcpy(process->io[process->io_req_quantity].type, "TAPE");
                process->io[process->io_req_quantity].current_time = 0;
            } else if (io_type == 3) {
                process->io[process->io_req_quantity].duration = PRINTER_IO_DURATION;
                strcpy(process->io[process->io_req_quantity].type, "PRINTER");
                process->io[process->io_req_quantity].current_time = 0;
            }

            process->io[process->io_req_quantity].req_instant = generateRandomNumber(possible_next_req_instant, process->service_time - 1);
            possible_next_req_instant = process->io[process->io_req_quantity].req_instant + 1;
            process->io_req_quantity++;
            process->io_req_remaining++;
        }

    }

}

/* Remove o processo da tabela de processos. Na remoção,
    ele anda com todos os processos que estavam atrás da posição
    dele na tabela uma posição para frente.

    Caso não encontre o processo, exibe uma mensagem de erro. */
void destroyProcess(Process process) {
    int index = -1;

    for (int i = 0; i < table.length; i++) {
        if (table.running_processes[i].pid == process.pid) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("Processo não encontrado!");
        return;
    }

    for (int i = index; i < table.length - 1; i++) {
        table.running_processes[i] = table.running_processes[i+1];
    }
    table.length--;
}

/* Função auxiliar para gerar número aleatórios entre um
    limite mínimo e um limite máximo */
int generateRandomNumber(int lower_limit, int upper_limit) {
    return (rand() % (upper_limit - lower_limit + 1)) + lower_limit;
}