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
    int dependencies[4]; // up to 4 dependencies
    int n; // Number of dependencies
    struct node* next;
};
typedef struct node QueueNode;

// Instruction queue
typedef struct {
    QueueNode* head;
    QueueNode* tail;
    int to_move; // # of nodes to move to next stage (<= W)
} Queue;

struct t_node{
    int address;
    struct t_node* left;
    struct t_node* right;
};

typedef struct t_node treeNode;

typedef struct {
    treeNode* root;
} BST;

BST* create_tree() {
    BST* tree = (BST*)malloc(sizeof(BST));
    tree -> root = NULL;

    return tree;
}

void free_tree_nodes(treeNode* node) {
    if (node != NULL) {
        free_tree_nodes(node -> left);
        free_tree_nodes(node -> right);

        free(node);
    }
}

void free_tree(BST* tree) {
    if (tree != NULL) {
        free_tree_nodes(tree -> root);
        free(tree);
    }

}

treeNode* create_node(int address) {
    treeNode* new_node = (treeNode*)malloc(sizeof(treeNode));

    new_node -> address = address;
    new_node -> left = NULL;
    new_node -> right = NULL;

    return new_node;
}

treeNode* insert_node(treeNode* parent, int address, BST* tree) {
    if (parent == NULL) {
        treeNode* new_node = create_node(address);
        if (tree -> root == NULL) {
            tree -> root = new_node;
        }
        return new_node;
    }

    if (address == parent -> address) {
        // duplicates?
    } else if (address < parent -> address) {
        parent -> left = insert_node(parent -> left, address, tree);
    } else if (address > parent -> address) {
        parent -> right = insert_node(parent -> right, address, tree);
    }

    return parent;
}

treeNode* binary_search(treeNode* parent, int address) {
    if (parent == NULL || parent -> address == address) {
        return parent;
    }

    if (address < parent -> address) {
        return binary_search(parent -> left, address);
    } else if (address > parent -> address) {
        return binary_search(parent -> right, address);
    }

    return NULL;
}

void satisfy_dependency(QueueNode* instruction, BST* satisfied_dependencies) {
    insert_node(satisfied_dependencies -> root, instruction -> address, satisfied_dependencies);
}

bool dependencies_handled(QueueNode* instruction, BST* satisfied_dependencies) {
    int dependency;
    for (int i = 0; i < 4; i++) {
        dependency = (instruction -> dependencies)[i];
        if (dependency == 0) {
            break;
        }
        if (!binary_search(satisfied_dependencies -> root, dependency)) {
            return false;
        }
    }

    return true;
}

static int int_count = 0;
static int float_count = 0;
static int branch_count = 0;
static int load_count = 0;
static int store_count = 0;

static bool branching = false;

Queue* create_empty_queue(int w) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue -> head = NULL;
    queue -> tail = NULL;
    queue -> to_move = w;
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
    printf("------------------------\n");
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
    for (int i = 0; i < src->to_move; i++) {
        insert_instruction(src, dest);
    }
    src->to_move = 0;
}

void fetch_instructions(Queue* src, Queue* dest, int W) {
    int i = 0;
    while (i < W && !branching) {
        insert_instruction(src, dest);
        i++;
    }
}

/**
 * Completes Instruction list to IF stage
*/
void initial_fetch(Queue* instruction_queue, int W, int num_moved_from_next) {
    // move_instructions(if_queue, id_queue);

    // fetch_instructions(instruction_queue, if_queue, W);
    QueueNode* current = instruction_queue->head;
    int x = 0;
    while (x < W) {
        if (current == NULL) {
            instruction_queue->to_move = W; // Set to W for num_moved_from_next for previous stage to work
            break;
        }
        // Limited movement by # of moved in next stage
        // (Some instructions are stalled, in that case we can't move current to next)
        if (x >= num_moved_from_next) {
            break;
        }

        // initial_fetch moves from main instruction list into IF list.
        // Therefore, if branching == true, then can't fetch.
        // (the fetch_to_decode function moves instructions from IF to ID)
        if (branching == true)
            break;
        if (current -> type == BRANCH)
            branching = true;

        // No problems. Increment x and move to next
        current = current->next;
        x++;
        instruction_queue->to_move = x;
    }


    // Ignore branching for now, just handle instruction movement through queue
    // if (!branching) {
    //     fetch_instructions(instruction_queue, if_queue, W);
    // }
}

/**
 * Completes IF stage
*/
void fetch_to_decode(Queue* if_queue, int W, int num_moved_from_next) {
    // move_instructions(if_queue, id_queue);

    // fetch_instructions(instruction_queue, if_queue, W);
    QueueNode* current = if_queue->head;
    int x = 0;
    while (x < W) {
        if (current == NULL) {
            if_queue->to_move = W; // Set to W for num_moved_from_next for previous stage to work
            break;
        }

        // Limited movement by # of moved in next stage
        // (Some instructions are stalled, in that case we can't move current to next)
        if (x >= num_moved_from_next) {
            break;
        }

        // No problems. Increment x and move to next
        current = current->next;
        x++;
        if_queue->to_move = x;
    }
}

/**
 * Completes ID stage
*/
void decode_to_execute(Queue* id_queue, BST* satisfied_dependencies, int W, int num_moved_from_next) {
    // move_instructions(id_queue, ex_queue);

    QueueNode* current = id_queue->head;
    int x = 0;
    while (x < W) {
        if (current == NULL) {
            id_queue->to_move = W; // Set to W for num_moved_from_next for previous stage to work
            break;
        }

        // Limited movement by # of moved in next stage
        // (Some instructions are stalled, in that case we can't move current to next)
        if (x >= num_moved_from_next) {
            break;
        }

        // Structural Dependencies:
        // Int/Float/Branch instruction can't execute in same cycle as another of its type.
        // This must be checked in the ID stage, since this function determines which
        // instructions will enter EX next cycle.
        // For each new current (i.e., current after id_queue->head), check first x in list.
        // If type is the same, break.
        int i = 0;
        QueueNode* previous = id_queue->head;
        while (i < x) {
            if ((current->type == INTEGER_INSTRUCTION || current->type == FLOATING_POINT ||
                current->type == BRANCH) && previous->type == current->type) {
                // Previous occurence of its type. Break.
                break;
            }
            previous = previous->next;
            i++;
        }

        // Data Hazards:
        // Cannot move from ID to EX until all data dependencies are satisfied.
        if (!dependencies_handled(current, satisfied_dependencies)) {
            // if (current -> type == INTEGER_INSTRUCTION || current -> type == FLOATING_POINT) {
            //     satisfy_dependency(current, satisfied_dependencies);
            // }
            // printf("Instruction %d dependencies not satisfied.\n", current -> address);
            break;
        }

        // No problems. Increment x and move to next
        current = current->next;
        x++;
        id_queue->to_move = x;
    }
}

/**
 * Completes EX stage
*/
void execute_to_memory(Queue* ex_queue, BST* satisfied_dependencies, int W, int num_moved_from_next) {
    // move_instructions(ex_queue, mem_queue);

    QueueNode* current = ex_queue->head;
    int x = 0;
    while (x < W) {
        if (current == NULL) {
            ex_queue->to_move = W; // Set to W for num_moved_from_next for previous stage to work
            break;
        }

        // Limited movement by # of moved in next stage
        // (Some instructions are stalled, in that case we can't move current to next)
        if (x >= num_moved_from_next) {
            break;
        }

        int i = 0;
        QueueNode* previous = ex_queue->head;
        while (i < x) {
            if ((current->type == LOAD || current->type == STORE) &&
                previous->type == current->type) {
                // Previous occurence of its type. Break.
                break;
            }
            previous = previous->next;
            i++;
        }

        if (current -> type == INTEGER_INSTRUCTION || current -> type == FLOATING_POINT) {
            satisfy_dependency(current, satisfied_dependencies);
        }

        if (current -> type == BRANCH)
            branching = false;

        current = current->next;
        x++;
        ex_queue->to_move = x;
    }
}

/**
 * Completes MEM stage
*/
void memory_to_writeback(Queue* mem_queue, BST* satisfied_dependencies, int W, int num_moved_from_next) {
    // move_instructions(mem_queue, wb_queue);

    QueueNode* current = mem_queue->head;
    int x = 0;
    while (x < W) {
        if (current == NULL) {
            mem_queue->to_move = W; // Set to W for num_moved_from_next for previous stage to work
            break;
        }

        // Limited movement by # of moved in next stage
        // (Some instructions are stalled, in that case we can't move current to next)
        if (x >= num_moved_from_next) {
            break;
        }

        if (current -> type == LOAD || current -> type == STORE) {
            satisfy_dependency(current, satisfied_dependencies);
        }
        
        // No problems. Increment x and move to next
        current = current->next;
        x++;
        mem_queue->to_move = x;
    }
}

/**
 * Completes WB stage
*/
void writeback(Queue* wb_queue, int W) {

    QueueNode* current = wb_queue->head;
    int x = 0;
    while (x < W) {
        if (current == NULL) {
            wb_queue->to_move = W; // Set to W for num_moved_from_next for previous stage to work
            break;
        }

        // No problems. Increment x and move to next


        current = current->next;
        x++;
        wb_queue->to_move = x;
    }
}

void print_process(Queue* if_queue, Queue* id_queue, Queue* ex_queue, Queue* mem_queue, Queue* wb_queue) {
    printf("========== IF ==========\n");
    dumpQueue(if_queue);
    printf("========== ID ==========\n");
    dumpQueue(id_queue);
    printf("========== EX ==========\n");
    dumpQueue(ex_queue);
    printf("========== MEM ==========\n");
    dumpQueue(mem_queue);
    printf("========== WB ==========\n");
    dumpQueue(wb_queue);
}

void retire_instruction(QueueNode* instruction) {
    InstructionType type = instruction -> type;
    switch (type) {
        case INTEGER_INSTRUCTION:
            int_count++;
            break;
        case FLOATING_POINT:
            float_count++;
            break;
        case BRANCH:
            branch_count++;
            break;
        case LOAD:
            load_count++;
            break;
        case STORE:
            store_count++;
            break;
        default:
            break;
    }
}

void complete_stages(Queue* instruction_queue, Queue* if_queue, Queue* id_queue, Queue* ex_queue, Queue* mem_queue, Queue* wb_queue, BST* satisfied_dependencies, int W) {
    // initial_fetch(instruction_queue, W);
    // fetch_to_decode(if_queue, W);
    // decode_to_execute(id_queue, satisfied_dependencies, W);
    // execute_to_memory(ex_queue, satisfied_dependencies, W);
    // memory_to_writeback(mem_queue, satisfied_dependencies, W);
    // writeback(wb_queue, W);

    // Changed the order of processing in order to function with dependency checking.
    writeback(wb_queue, W);
    memory_to_writeback(mem_queue, satisfied_dependencies, W, wb_queue->to_move);
    execute_to_memory(ex_queue, satisfied_dependencies, W, mem_queue->to_move);
    decode_to_execute(id_queue, satisfied_dependencies, W, ex_queue->to_move);
    fetch_to_decode(if_queue, W, id_queue->to_move);
    initial_fetch(instruction_queue, W, if_queue->to_move);

    // Move eligible instructions to next stage
    // First, process writeback nodes
    // NOTE: May have to change when this happens? causes an extra emtpy cycle to occur just to process this.
    for (int i = 0; i < wb_queue->to_move; i++) {
        if (wb_queue->head != NULL) {
            QueueNode* ptr = wb_queue->head;
            // printf("Freeing node: %d\n", ptr->address);
            retire_instruction(ptr);
            wb_queue->head = ptr->next;
            free(ptr);
            ptr = NULL;
        }
    }
    wb_queue->to_move = 0;

    move_instructions(mem_queue, wb_queue);
    move_instructions(ex_queue, mem_queue);
    move_instructions(id_queue, ex_queue);
    move_instructions(if_queue, id_queue);
    move_instructions(instruction_queue, if_queue);

    print_process(if_queue, id_queue, ex_queue, mem_queue, wb_queue);

}

void simulation(Queue* instruction_queue, int start_inst, int inst_count, int W){
    // Create queue for each stage
    Queue* if_queue = create_empty_queue(W);
    Queue* id_queue = create_empty_queue(W);
    Queue* ex_queue = create_empty_queue(W);
    Queue* mem_queue = create_empty_queue(W);
    Queue* wb_queue = create_empty_queue(W);

    BST* satisfied_dependencies = create_tree();

    int cycle = 0;
    while (instruction_queue->head != NULL || if_queue->head != NULL || id_queue->head != NULL ||
           ex_queue->head != NULL || mem_queue->head != NULL || wb_queue->head != NULL) {
        cycle++;
        printf("=================================== CYCLE %d ===================================\n", cycle);
        complete_stages(instruction_queue, if_queue, id_queue, ex_queue, mem_queue, wb_queue, satisfied_dependencies, W);
    }

    printf("Finished after %d cycles.\n", cycle);

    FreeQueue(if_queue);
    FreeQueue(id_queue);
    FreeQueue(ex_queue);
    FreeQueue(mem_queue);
    FreeQueue(wb_queue);

    free_tree(satisfied_dependencies);
}

/**
 * Parses trace and returns Queue of instructions
*/
Queue* parse_trace(FILE* file, int start_inst, int inst_count) {
    Queue* queue = create_empty_queue(0);
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int current_instruction = 1;
    int num_instructions = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        // Initialize Queue Node
        if (current_instruction >= start_inst) {
            // printf("Starting at instruction %d\n", current_instruction);
            num_instructions++;
            QueueNode* node = malloc(sizeof(QueueNode));
            node->next = NULL;
            node->n = 0;
            node->dependencies[0] = 0;
            node->dependencies[1] = 0;
            node->dependencies[2] = 0;
            node->dependencies[3] = 0;

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
    // printf("Processed %d instructions\n", num_instructions);
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
        int start_inst = atoi(argv[2]); // !! Assumes that instruction count starts from 1 !!
        int inst_count = atoi(argv[3]);
        int W = atoi(argv[4]);

        FILE* file;

        file = fopen(argv[1], "r");
        if (file == NULL) {
            printf("Invalid file.\n");
            return 0;
        }

        Queue* instructions = parse_trace(file, start_inst, inst_count);

        //
        // !! Debug print. Remove later.
        //
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