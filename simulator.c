#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

/* constante com o tamanho da tabela de processos 
    (o maximo que ele suporta em numero de processos) */
#define PROCESS_TABLE_SIZE 20

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
#define MIN_SERVICE_TIME 1
#define INITIAL_PRIORITY 0
#define MAX_IO_REQ 3

/* constantes com os valores do status possiveis */
#define EXECUTION_STATUS "execution"
#define READY_STATUS "ready"
#define BLOCKED_STATUS "blocked"

/* constantes para o trabalho do escalonador */
#define QUEUE_SIZE 10
#define QUEUES_NUMBER 10
#define QUANTUM 5

/* struct de io que o processo precisa para pedir vários */
typedef struct {
    int duration;                       // duração do io
    int req_instant;                    // instante que o processo pede io a partir de seu inicio
} Io;

/* struct com as informações do processo, ou PCB */
typedef struct {
    int pid;                            // pid do processo
    int ppid;                           // pid do processo pai que o crio
    int service_time;                   // tempo de serviço do processo
    int priority;                       // prioridade do processo
    char status[9];                     // status do processo
    Io io[MAX_IO_REQ];                  // ios pedidos pelo processo
    int io_req_quantity;                // variavel auxiliar para quantificar os ios
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

void initProcessTable();

void initQueue(ProcessQueue *queue);
void enqueue(ProcessQueue *queue, Process process);
Process dequeue(ProcessQueue *queue);

Process createProcess(); 
int containsPid(int pid);
void generateIoRequests(Process *process);
void destroyProcess(Process process);

int generateRandomNumber(int lower_limit, int upper_limit);

ProcessTable table;

int main() {
    ProcessQueue ready_queue, blocked_queue, feedback_queue[QUEUES_NUMBER];

    srand(time(NULL));          // inicialização para randomizar os números
    initQueue(&ready_queue);    
    
    Process process1 = createProcess();
    Process process2 = createProcess();
    Process process3 = createProcess();
    
    for(int i = 0; i < table.length; i++) printf("%d ", table.running_processes[i].pid);
    
    printf("\n");
    
    destroyProcess(process2);
    
    for(int i = 0; i < table.length; i++) printf("%d ", table.running_processes[i].pid);
    
    printf("\n\n");
    
    printf("ppid: %d\npid: %d\nservice time: %d\npriority: %d\nio times: %d\n", 
        process1.ppid, process1.pid, process1.service_time, process1.priority, process1.io_req_quantity);
    
    for(int i = 0; i < process1.io_req_quantity; i++) printf("io duration: %d, io request instant: %d\n", 
        process1.io[i].duration, process1.io[i].req_instant);
    
    printf("\n");
    
    printf("ppid: %d\npid: %d\nservice time: %d\npriority: %d\nio times: %d\n", 
        process2.ppid, process2.pid, process2.service_time, process2.priority, process2.io_req_quantity);
    
    for(int i = 0; i < process2.io_req_quantity; i++) printf("io duration: %d, io request instant: %d\n", 
        process2.io[i].duration, process2.io[i].req_instant);
        
    printf("\n");
    
    printf("ppid: %d\npid: %d\nservice time: %d\npriority: %d\nio times: %d\n", 
        process3.ppid, process3.pid, process3.service_time, process3.priority, process3.io_req_quantity);
    
    for(int i = 0; i < process3.io_req_quantity; i++) printf("io duration: %d, io request instant: %d\n", 
        process3.io[i].duration, process3.io[i].req_instant);
    
    return 0;
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
        printf("Fila cheia! Não é possível inserir.");
    
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
        printf("Tabela cheia! Não é possível criar mais processo.");
        
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
    
    if (process->service_time > 1) {
        while (process->io_req_quantity < 3 && (process->io_req_quantity == 0 ||
            process->service_time - possible_next_req_instant > 1)) {
                
            io_type = generateRandomNumber(0,3);
            
            if (io_type == 0) {
                break;
                
            } else if (io_type == 1) {
                process->io[process->io_req_quantity].duration = DISK_IO_DURATION;
                
            } else if (io_type == 2) {
                process->io[process->io_req_quantity].duration = TAPE_IO_DURATION;
                
            } else if (io_type == 3) {
                process->io[process->io_req_quantity].duration = PRINTER_IO_DURATION;
                
            }
            
            process->io[process->io_req_quantity].req_instant = generateRandomNumber(possible_next_req_instant, process->service_time - 1);
            possible_next_req_instant = process->io[process->io_req_quantity].req_instant + 1;
            process->io_req_quantity++;
            
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