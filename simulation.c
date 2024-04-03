#include<stdio.h>
#include<time.h>
#include<math.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<float.h>
#include <stdbool.h>

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

    struct node* next;
}; 
typedef struct node QueueNode;

// Instruction queue
typedef struct {
    QueueNode* head;
    QueueNode* tail;
} Queue;

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
        src -> head = src -> head -> next;
    }

    if (src -> head == NULL) {
        src -> tail = NULL;
    }

    dest -> tail -> next = NULL;
}

/**
 * Completes IF stage
*/
void fetch(Queue* instruction_queue, Queue* if_queue, Queue* id_queue, int W) {
    for (int i = 0; i < W; i++) {
        insert_instruction(if_queue, id_queue);
    }
}

/**
 * Completes ID stage
*/
void decode() {

}

/**
 * Completes EX stage
*/
void execute() {

}

/**
 * Completes MEM stage
*/
void memory_access() {

}

/**
 * Completes WB stage
*/
void writeback() {

}

void complete_stages(Queue* instruction_queue, Queue* if_queue, Queue* id_queue, Queue* ex_queue, Queue* mem_queue, Queue* wb_queue, int W) {
    for (int i = 0; i < W; i++) {
        fetch(instruction_queue, if_queue, id_queue, W);
    } 
    for (int i = 0; i < W; i++) {
        decode(id_queue);
    } 
    for (int i = 0; i < W; i++) {
        execute(ex_queue);
    } 
    for (int i = 0; i < W; i++) {
        memory_access(mem_queue);
    } 
    for (int i = 0; i < W; i++) {
        writeback(wb_queue);
    } 
}

Queue* create_empty_queue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue -> head = NULL;
    queue -> tail = NULL;

    return queue;
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
        complete_stages(instruction_queue, if_queue, id_queue, ex_queue, mem_queue, wb_queue, W);
        time++;
    }
}

/**
 * Parses trace and returns Queue of instructions
*/
Queue* parse_trace() {
    // IMPLEMENT THIS
    return (Queue*)malloc(sizeof(Queue));
}

// Program's main function
int main(int argc, char* argv[]){

	if(argc >= 5){
        int start_inst = atoi(argv[2]);
        int inst_count = atoi(argv[3]);
        int W = atoi(argv[4]);

        Queue* instructions = parse_trace();
        simulation(instructions, start_inst, inst_count, W);
    }

	else printf("Insufficient number of arguments provided!\n");
   
	return 0;
}