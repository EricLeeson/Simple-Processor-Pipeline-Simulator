#define  _GNU_SOURCE
#include<stdio.h>
#include<time.h>
#include<math.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<float.h>
#include <stdbool.h>
#include<string.h>


// Instruction types (1 - 5)
typedef enum {
    INTEGER_INSTRUCTION = 1,
    FLOATING_POINT,
    BRANCH,
    LOAD,
    STORE
} InstructionType;

// Represents instruction
struct node {
    int address;
    InstructionType type;
    int dependencies[5]; // up to 5 dependencies
    int n; // Number of dependencies
    struct node* next;
}; 
typedef struct node QueueNode;

// Instruction queue
typedef struct {
    QueueNode* head;
    QueueNode* tail;
} Queue;

static bool branching = false;

Queue* create_empty_queue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue -> head = NULL;
    queue -> tail = NULL;
    return queue;
}

void FreeQueue(Queue* Q) {
  QueueNode* ptr = Q->head;
  while (ptr) {
    Q->head = ptr->next;
    free(ptr);
    ptr = NULL;
    ptr = Q->head;
  }
  Q->tail = NULL;
  free(Q);
}

void dumpQueue(Queue* q) {
	QueueNode* current = q->head;
    int i = 1;
	while (current != NULL) {
        printf("------------------------\n");
        printf("Instruction %d:\n", i);
        printf("Address: %d\n", current->address);
        printf("Type: %d\n", current->type);
        printf("# dependencies: %d\n", current->n);
        i++;
		current = current->next;
	}
}

/**
 * Inserts instruction (QueueNode*) from src Queue to dest Queue.
*/
void insert_instruction(Queue* src, Queue* dest) {
    if (src -> head == NULL) return;

    if (dest -> head == NULL) {
        dest -> head = src -> head;
        dest -> tail = src -> head;
    } else {
        dest -> tail -> next = src -> head;
        dest -> tail = src -> head;
    }

    src -> head = src -> head -> next;

    if (src -> head == NULL) {
        src -> tail = NULL;
    }

    dest -> tail -> next = NULL;
}

void move_instructions(Queue* src, Queue* dest) {
    // for (int i = 0; i < W; i++) {
    //     insert_instruction(src, dest);
    // }
    insert_instruction(src, dest);
}

void fetch_instructions(Queue* src, Queue* dest, int W) {
    int i = 0;
    while (i < W && !branching) {
        insert_instruction(src, dest);
        i++;
    }
}

/**
 * Completes IF stage
*/
void fetch(Queue* instruction_queue, Queue* if_queue, Queue* id_queue, int W) {
    move_instructions(if_queue, id_queue);

    if (!branching) {
        fetch_instructions(instruction_queue, if_queue, W);
    }
}

/**
 * Completes ID stage
*/
void decode(Queue* id_queue, Queue* ex_queue, int W) {
    move_instructions(id_queue, ex_queue);
}

/**
 * Completes EX stage
*/
void execute(Queue* ex_queue, Queue* mem_queue, int W) {
    move_instructions(ex_queue, mem_queue);
}

/**
 * Completes MEM stage
*/
void memory_access(Queue* mem_queue, Queue* wb_queue, int W) {
    move_instructions(mem_queue, wb_queue);
}

/**
 * Completes WB stage
*/
void writeback(Queue* wb_queue) {
    // free head?
    if (wb_queue->head != NULL) {
        QueueNode* ptr = wb_queue->head;
        // printf("Freeing node: %d\n", ptr->address);
        wb_queue->head = ptr->next;
        free(ptr);
        ptr = NULL;
    }
}

void print_process(Queue* if_queue, Queue* id_queue, Queue* ex_queue, Queue* mem_queue, Queue* wb_queue) {
    printf("IF\n");
    dumpQueue(if_queue);
    printf("ID\n");
    dumpQueue(id_queue);
    printf("EX\n");
    dumpQueue(ex_queue);
    printf("MEM\n");
    dumpQueue(mem_queue);
    printf("WB\n");
    dumpQueue(wb_queue);
}

void complete_stages(Queue* instruction_queue, Queue* if_queue, Queue* id_queue, Queue* ex_queue, Queue* mem_queue, Queue* wb_queue, int W) {
    for (int i = 0; i < W; i++) {
        fetch(instruction_queue, if_queue, id_queue, W);
    } 
    for (int i = 0; i < W; i++) {
        decode(id_queue, ex_queue, W);
    } 
    for (int i = 0; i < W; i++) {
        execute(ex_queue, mem_queue, W);
    } 
    for (int i = 0; i < W; i++) {
        memory_access(mem_queue, wb_queue, W);
    } 
    for (int i = 0; i < W; i++) {
        writeback(wb_queue);
    }
    print_process(if_queue, id_queue, ex_queue, mem_queue, wb_queue);
}

void simulation(Queue* instruction_queue, int start_inst, int inst_count, int W){
    // Create queue for each stage
    Queue* if_queue = create_empty_queue();
    Queue* id_queue = create_empty_queue();
    Queue* ex_queue = create_empty_queue();
    Queue* mem_queue = create_empty_queue();
    Queue* wb_queue = create_empty_queue();
    
    int time = 0;
    while (time < inst_count) {
        // Note: May want to change condition to check if all queues are empty, since #cycles will be greater than instruction count
        printf("============ New loop ============\n");
        complete_stages(instruction_queue, if_queue, id_queue, ex_queue, mem_queue, wb_queue, W);
        time++;
    }


    FreeQueue(if_queue);
    FreeQueue(id_queue);
    FreeQueue(ex_queue);
    FreeQueue(mem_queue);
    FreeQueue(wb_queue);

}

/**
 * Parses trace and returns Queue of instructions
*/
Queue* parse_trace(FILE* file, int start_inst, int inst_count) {
    Queue* queue = create_empty_queue();
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int current_instruction = 1;
    int num_instructions = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        // Initialize Queue Node
        if (current_instruction >= start_inst) {
            printf("Starting at instruction %d\n", current_instruction);
            num_instructions++;
            QueueNode* node = malloc(sizeof(QueueNode));
            node->next = NULL;
            node->n = 0;
            int i = -2; // Don't know how many tokens, must count
            //             Start at -2 so index lines up with dependencies array
            // Format: PC, Type, List[5]
            //     i = -2,  -1,    0-4
            char temp[100];
            strcpy(temp, line);
            char* token = strtok(temp, ",");
            while( token != NULL ) {
                if (token[strlen(token) -1] == '\n') {
                    token[strlen(token) -1] = '\0';
                }
                if (i == -2) {
                    node->address = (int)strtol(token, NULL, 16);
                } else if (i == -1) {
                    int type = atoi(token);
                    if (type == 1) {
                        node->type = INTEGER_INSTRUCTION;
                    } else if (type == 2) {
                        node->type = FLOATING_POINT;
                    } else if (type == 3) {
                        node->type = BRANCH;
                    } else if (type == 4) {
                        node->type = LOAD;
                    } else if (type == 5) {
                        node->type = STORE;
                    }
                } else {
                    node->dependencies[i] = (int)strtol(token, NULL, 16);
                    node->n++;
                }
                i++;
                token = strtok(NULL, ",");
            }
            // insert node
            if (queue->head == NULL && queue->tail == NULL) {
                queue->head = node;
                queue->tail = node;
            } else {
                queue->tail->next = node;
                queue->tail = node;
            }
        }
        current_instruction++;
        if (num_instructions >= inst_count) {
            break;
        }
    }
    printf("Processed %d instructions\n", num_instructions);
    free(line);
    return queue;
}

// Program's main function
int main(int argc, char* argv[]){
    // argv[1]: trace_file_name 
    // argv[2]: start_inst 
    // argv[3]: inst_count 
    // argv[4]: W 
	if(argc >= 5) {
        int start_inst = atoi(argv[2]);
        int inst_count = atoi(argv[3]);
        int W = atoi(argv[4]);

        FILE* file;

        file = fopen(argv[1], "r");
        if (file == NULL) {
            printf("Invalid file.\n");
            return 0;
        }

        Queue* instructions = parse_trace(file, start_inst, inst_count);

        // struct node* current = instructions->head;
        // int i = 0;
        // while (current != NULL) {
        //     printf("========== ==========\n");
        //     printf("%d\n", i);
        //     printf("Address: %d\n", current->address);
        //     printf("Type: %d\n", current->type);
        //     for (int j = 0; j < current->n; j++) {
        //         printf("Dependencies: %d\n", current->dependencies[j]);
        //     }
        //     i++;
        //     current = current->next;
        // }
        
        simulation(instructions, start_inst, inst_count, W);

        FreeQueue(instructions);
        fclose(file);
    }

	else printf("Insufficient number of arguments provided!\n");
   
	return 0;
}