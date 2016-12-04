/*
  Group 1
  Created Decmeber 2016 by Ian Thomas.

  Using 32 bit addressing we will be able to access 4GB of data.
  4294967296 decimal and 0x100000000 hex
  With addresses, words, and instructions all being 32 bits each

  The text segment will start at address 0x50000000
  The data segment will start at address 0x40000000
  The kernal segment will start at address 0x60000000
*/

#include <stdint.h> // int32_t
#include <stdio.h> // pritntf, getline, fopen
#include <stdlib.h> // strtol
#include <string.h> // strtok, strcmp, strcspn

/* Typedefs and Defines*/
typedef int32_t mem_addr;
typedef int32_t mem_word;
typedef int32_t instruction;
typedef int bool;

#define true 1
#define false 0
#define MAX_SEG_SIZE 33554428
#define NUM_STRINGS 10
#define STRING_SIZE 100
#define TEXT_START ((mem_word) 0x50000000)
#define DATA_START ((mem_word) 0x40000000)
#define KERNAL_START ((mem_word) 0x60000000)
#define DATA_TOP (TEXT_START -1)
#define TEXT_TOP (KERNAL_TOP -1)
#define KERNAL_TOP (((mem_word) (0x70000000)) -1)
#define NUM_LABELS 50 // 50 Labels allowed
#define LABEL_SIZE 50 // Label string size =50
#define INT_REG_NUM 32
#define FLOAT_REG_NUM 16

/*Global Variables:  */
instruction* text_segment;
float data_segment[MAX_SEG_SIZE/4];
mem_word *kernal_segment;
mem_addr data_limit;
char labels[NUM_LABELS][LABEL_SIZE]; // Keeping up with labels
int label_addresses[NUM_LABELS];
int last_label = 0;
int last_string = 0;
int top_string_mem = 1500;
char string_allocation[NUM_STRINGS][STRING_SIZE]; // Keeping up with Allocation
int string_addresses[NUM_STRINGS];
int string_lengths[NUM_STRINGS];
int instruction_count; // number of instructions

/*
	Each type of instruction gets its own struct for easy organization.
*/

struct instr_type1{
    unsigned int op_code;
    mem_word r_dest;
    mem_word r_src;
    mem_word label;
};

struct instr_type2{
    unsigned int op_code;
    mem_word r_target;
    mem_word label;
};

struct instr_type3{
    unsigned int op_code;
    mem_word label;
};

/*
	We package the instruction types together to allow dynamic use during
	the execution process.
*/
struct instruction_container{
    struct instr_type1 type1;
    struct instr_type2 type2;
    struct instr_type3 type3;
    int type;
};

/*Scoreboard needed DataStructures*/
struct func_status {
    bool busy;
    unsigned int op;
    int dest;
    int src1;
    int src2;
    int Fu_src1;
    int Fu_src2;
    bool src1_ready;
    bool src2_ready;
    int time;
};

struct instr_status{
    bool waiting[4];
    int r_stage;
    int fadd_stage;
    int fmult_stage;
    int fload_stage;
};

struct reg_status{
    int f_reg_status[FLOAT_REG_NUM];
    int r_reg_status[INT_REG_NUM];

};
/*
    Scoreboard holds all data structures.
*/
struct scoreboard{
    struct func_status func_status[4];
    struct instr_status instr_status;
    struct reg_status reg_status;
};
/*
  Stores each functional units instruction.

*/
struct fetch_buf{
    struct instruction_container integer;
    struct instruction_container f_add;
    struct instruction_container f_mult;
    struct instruction_container load;
};
/*
  Operands that each functional unit has.
*/
struct operands{
    float fadd_opA;
    float fadd_opB;
    float fmult_opA;
    float fmult_opB;
    int load_A;
    int load_B;
    float store_A;
    int r_opA;
    int r_opB;
    int branch;
};
/*
  Used to be passed to the write back stage.
*/
struct output{
    int integer;
    float float_add;
    float float_mult;
    float float_load;
};
/*
  Keep up with the performance of the simulator.
*/
struct performance{
    int nop_count;
    int clock_cycles;
    int instruction_count;
};
/* Prototypes */
void make_memory();
void parse_source_code(char *filename);
void load_data(char* token);
void load_text(char* token, int *index);
int get_opCode(char* instr);
float read_mem(mem_addr address);
void write_instr(mem_addr address, instruction instr);
void write_mem(mem_addr address, float value);
instruction type1_create(struct instr_type1 instr);
instruction type2_create(struct instr_type2 instr);
instruction type3_create(struct instr_type3 instr);
int find_label(char* to_search);
int check_allocated(char* match);
int syscall(mem_word function);
int get_index(int match);

/*
  New functions used for the scoreboard
*/
void scoreboard_issue(struct scoreboard *scob, struct instruction_container instruction, unsigned int op);
unsigned int get_src_fu(struct reg_status reg_status, int reg, unsigned int f_or_w);
bool instruction_issue(mem_addr *pc, struct scoreboard scob_old, struct scoreboard *scob_new, struct fetch_buf *fetch_buffer,
                                struct performance *performance, int *to_run);
void set_fetch_buffer(struct fetch_buf *buf, struct instruction_container instr, unsigned int op);
bool check_waw(struct reg_status reg_stat, mem_word reg_dest, int r_or_f);
bool check_struct(struct func_status fu_status, unsigned int op);
// need to change int to int * possible
instruction fetch(mem_addr *address, int *to_run);
struct instruction_container decode(instruction to_decode);
int get_fuctional_unit(unsigned int op);
void init_reg(struct reg_status *reg_status, int int_reg_file[], float float_reg_file[]);
void read_operands(struct scoreboard scob_old, int int_reg_file[], float float_reg_file[], struct operands *operands,
                                        struct fetch_buf fetch_buf, struct scoreboard *scob_new);
void int_execution(struct scoreboard scob_old, struct operands operands, struct output *out_new, mem_addr *pc,struct scoreboard *new);
void fp_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new);
void fmult_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new);
void fload_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new);
bool check_war(struct scoreboard scob_old,struct scoreboard scob_new ,int reg_dest, int unit);
void write_back(struct scoreboard scob_old, struct scoreboard *scob_new, struct fetch_buf fetch_buf,
                int int_regs[], float float_regs[], struct output output);
void scoreboard_refresh(struct scoreboard *scob, int unit, int reg);

/*Main entry into simulator, one argument should be passed, the filename.*/
int main(int argc, char *argv[]){
    //Reg files
    int int_reg_file[INT_REG_NUM];
    float float_reg_file[FLOAT_REG_NUM];

    // simulator structs
    struct scoreboard scob_new;
    struct scoreboard scob_old;
    struct operands operands;
    struct fetch_buf fetch_old;
    struct fetch_buf fetch_new;
    struct output out_old;
    struct output out_new;
    // used to stop sim
    int to_run = 1;

    mem_addr pc = TEXT_START; // PC

    struct performance performance; // performance
    //init
    performance.instruction_count = 0;
    performance.nop_count = 0;
    performance.clock_cycles = 0;

    make_memory();
    parse_source_code(argv[1]);
    // -1 out all reg files
    init_reg(&scob_new.reg_status, int_reg_file, float_reg_file);
    int i;

    while(to_run){
        scob_old = scob_new;
        fetch_old = fetch_new;
        out_old = out_new;
        instruction_issue(&pc, scob_old, &scob_new, &fetch_new, &performance, &to_run);
        read_operands(scob_old, int_reg_file, float_reg_file, &operands, fetch_old, &scob_new);
        int_execution(scob_old, operands, &out_new, &pc, &scob_new);
        fp_execution(scob_old, operands, &out_new, &pc, &scob_new);
        fmult_execution(scob_old, operands, &out_new, &pc, &scob_new);
        fload_execution(scob_old, operands, &out_new, &pc, &scob_new);
        write_back(scob_old, &scob_new, fetch_old, int_reg_file, float_reg_file, out_old);
    }

    printf("\nNumber of clock cycles: %d\n", performance.clock_cycles);
    printf("\nNumber of instructions executed: %d\n", performance.instruction_count);
    printf("\nNumber of nop's: %d\n", performance.nop_count);
    free(kernal_segment);
    free(text_segment);
    return 0;
}

/*
  Refreshed all the values in the scoreboard so that waiting instructions can read
    from their sources.
*/
void scoreboard_refresh(struct scoreboard *scob, int unit, int dest){
        //int update
        if(unit == 0 && scob->func_status[3].op == 15){
            scob->func_status[3].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[3].src1, 0);
            scob->func_status[3].src1_ready = check_waw(scob->reg_status, scob->func_status[3].src1, 0);
        }
        // Fadd update
        if(unit == 1){
            if((scob->func_status[2].src1 == dest) || (scob->func_status[2].src2 == dest)){
                scob->func_status[2].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[2].src1, 2);
                scob->func_status[2].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[2].src2, 2);
                scob->func_status[2].src1_ready = check_waw(scob->reg_status, scob->func_status[2].src1, 2);
                scob->func_status[2].src2_ready = check_waw(scob->reg_status, scob->func_status[2].src2, 2);
            }
            if(scob->func_status[3].src1 == dest){
                scob->func_status[3].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[3].src1, 0);
                scob->func_status[3].src1_ready = check_waw(scob->reg_status, scob->func_status[3].src1, 0);
            }

        }
        // Fmult update
        if(unit == 2){
            if((scob->func_status[1].src1 == dest) || (scob->func_status[1].src2 == dest)){
                scob->func_status[1].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[1].src1, 1);
                scob->func_status[1].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[1].src2, 1);
                scob->func_status[1].src1_ready = check_waw(scob->reg_status, scob->func_status[1].src1, 1);
                scob->func_status[1].src2_ready = check_waw(scob->reg_status, scob->func_status[1].src2, 1);
            }
            // Update for store, needs a float
            if(scob->func_status[3].src1 == dest){
                scob->func_status[3].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[3].src1, 0);
                scob->func_status[3].src1_ready = check_waw(scob->reg_status, scob->func_status[3].src1, 0);
            }

        }
        // Update after a store or load
        if(unit == 3){
            if((scob->func_status[1].src1 == dest) || (scob->func_status[1].src2 == dest)){
                scob->func_status[1].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[1].src1, 1);
                scob->func_status[1].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[1].src2, 1);
                scob->func_status[1].src1_ready = check_waw(scob->reg_status, scob->func_status[1].src1, 1);
                scob->func_status[1].src2_ready = check_waw(scob->reg_status, scob->func_status[1].src2, 1);
            }
            if((scob->func_status[2].src1 == dest) || (scob->func_status[2].src2 == dest)){
                scob->func_status[2].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[2].src1, 2);
                scob->func_status[2].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[2].src2, 2);
                scob->func_status[2].src1_ready = check_waw(scob->reg_status, scob->func_status[2].src1, 2);
                scob->func_status[2].src2_ready = check_waw(scob->reg_status, scob->func_status[2].src2, 2);
            }

        }


}

/*
  Write back stage of the simulator.

*/
void write_back(struct scoreboard scob_old, struct scoreboard *scob_new, struct fetch_buf fetch_buf,
                int int_regs[], float float_regs[], struct output output){
    int i;
    bool update = 0;
    // For each functional unit if you have to write, write_back
    for(i = 0; i < 4; i++){
        // you can write back
        if(i == 0 && scob_old.func_status[i].busy == true && scob_old.instr_status.r_stage == 3){
            //Go ahead and pass all our branches they won't write anything
            if((unsigned int)scob_old.func_status[i].op == 1 ||(unsigned int)scob_old.func_status[i].op == 2 ||
               (unsigned int)scob_old.func_status[i].op == 4 ||(unsigned int)scob_old.func_status[i].op == 3 ||
               (unsigned int)scob_old.func_status[i].op == 9 ||(unsigned int)scob_old.func_status[i].op == 10){
                scob_new->func_status[i].busy = false;
                scob_new->func_status[i].dest = -1;
                scob_new->func_status[i].src1 = -1;
                scob_new->func_status[i].src2 = -1;
                scob_new->func_status[i].Fu_src1 = -1;
                scob_new->func_status[i].Fu_src2 = -1;
                scob_new->func_status[i].src1_ready = false;
                scob_new->func_status[i].src2_ready = false;
                continue;
            }

            // If a war, we are going to stall
            if(check_war(scob_old, *scob_new, scob_old.func_status[0].dest, i) == true){
                continue;
            }

            printf("Integer: Now writing back to this register: %d, this value:%d\n\n", scob_old.func_status[0].dest, output.integer);
            // Integer write backs
            int_regs[scob_old.func_status[i].dest] = output.integer;
            //change reg status since we aren't using it anymore
            scob_new->reg_status.r_reg_status[scob_old.func_status[i].dest] = -1;
            scoreboard_refresh(scob_new, 0, scob_old.func_status[i].dest);
            scob_new->func_status[i].busy = false;
            scob_new->func_status[i].dest = -1;
            scob_new->func_status[i].src1 = -1;
            scob_new->func_status[i].src2 = -1;
            scob_new->func_status[i].Fu_src1 = -1;
            scob_new->func_status[i].Fu_src2 = -1;
            scob_new->func_status[i].src1_ready = false;
            scob_new->func_status[i].src2_ready = false;
        }
        // Float add or sub Writeback
        if(i == 1 && scob_old.func_status[i].busy == true && scob_old.instr_status.fadd_stage == 3){
            if(check_war(scob_old, *scob_new, scob_old.func_status[0].dest, i) == true){
                continue;
            }
            printf("FLoat add or sub: Now writing back to this register: %d, this value: %f\n\n"
                                    , scob_old.func_status[1].dest, output.float_add);
            // Fload add write backs
            float_regs[scob_old.func_status[i].dest] = output.float_add;
            scob_new->reg_status.f_reg_status[scob_old.func_status[i].dest] = -1;
            scoreboard_refresh(scob_new, 1, scob_old.func_status[i].dest);
            scob_new->func_status[i].busy = false;
            scob_new->func_status[i].dest = -1;
            scob_new->func_status[i].src1 = -1;
            scob_new->func_status[i].src2 = -1;
            scob_new->func_status[i].Fu_src1 = -1;
            scob_new->func_status[i].Fu_src2 = -1;
            scob_new->func_status[i].src1_ready = false;
            scob_new->func_status[i].src2_ready = false;
        }

        // Float mult write back
        if(i == 2 && scob_old.func_status[i].busy == true && scob_old.instr_status.fmult_stage == 3){
            if(check_war(scob_old, *scob_new, scob_old.func_status[0].dest, i) == true){
                continue;
            }
            printf("Mult: Now writing back to this register: %d this value: %f\n\n",
                                 scob_old.func_status[2].dest, output.float_mult);
            // Float mult write backs
            float_regs[scob_old.func_status[i].dest] = output.float_mult;
            scob_new->reg_status.f_reg_status[scob_old.func_status[i].dest] = -1;
            scoreboard_refresh(scob_new, 2, scob_old.func_status[i].dest);
            scob_new->func_status[i].busy = false;
            scob_new->func_status[i].dest = -1;
            scob_new->func_status[i].src1 = -1;
            scob_new->func_status[i].src2 = -1;
            scob_new->func_status[i].Fu_src1 = -1;
            scob_new->func_status[i].Fu_src2 = -1;
            scob_new->func_status[i].src1_ready = false;
            scob_new->func_status[i].src2_ready = false;
        }

        //Load or store write back
        if(i == 3 && scob_old.func_status[i].busy == true && scob_old.instr_status.fload_stage == 3){
            if(check_war(scob_old, *scob_new, scob_old.func_status[0].dest, i) == true){
                continue;
            }
            if(scob_old.func_status[3].time != 0){
                scob_new->func_status[3].time -= 1;
                continue;
            }
            printf("Load: Now writing back to this register: %d, this value: %f\n\n",
                        scob_old.func_status[3].dest, output.float_load);
            // Float load write back sd does nothing here
            if((unsigned int)scob_old.func_status[i].op == 16){

            }
            else{
                float_regs[scob_old.func_status[i].dest] = output.float_load;
                scob_new->reg_status.f_reg_status[scob_old.func_status[i].dest] = -1;
                scoreboard_refresh(scob_new, 3, scob_old.func_status[i].dest);
            }
            scob_new->func_status[i].busy = false;
            scob_new->func_status[i].dest = -1;
            scob_new->func_status[i].src1 = -1;
            scob_new->func_status[i].src2 = -1;
            scob_new->func_status[i].Fu_src1 = -1;
            scob_new->func_status[i].Fu_src2 = -1;
            scob_new->func_status[i].src1_ready = false;
            scob_new->func_status[i].src2_ready = false;
        }
    }
}
/*
  Checks for a write after a read.
*/
bool check_war(struct scoreboard scob_old,struct scoreboard scob_new ,int reg_dest, int unit){
    if(unit == 0){
        //if(scob_old.instr_status.r_stage == scob_new.instr_status.r_stage && (scob_old.func_status[0].src1 == reg_dest || scob_old.func_status[0].src2 == reg_dest)){
        //    return true;

        //}
    }
    if(unit == 1){
        if(scob_old.instr_status.fadd_stage == scob_new.instr_status.fadd_stage &&
                        scob_old.func_status[1].dest == reg_dest){
            return true;

        }

    }
    if(unit == 2){
        if(scob_old.instr_status.fmult_stage == scob_new.instr_status.fmult_stage &&
                        scob_old.func_status[2].dest == reg_dest){
            return true;

        }

    }
    if(unit == 3){
        if(scob_old.instr_status.fload_stage == scob_new.instr_status.fload_stage &&
                        scob_old.func_status[3].dest == reg_dest){
            return true;

        }

    }
    return false;
}

/*
  Execution function for the interger functional unit.
*/
void int_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new){
    // Able to execute
    if(scob_old.func_status[0].src1_ready == true && scob_old.func_status[0].src2_ready == true
                        && scob_old.func_status[0].busy == true){
            printf("Integer execution going on: op code is: %lu\n\n", scob_old.func_status[0].op);
            switch((unsigned int)scob_old.func_status[0].op){
                // Addi
                case 0:
                    out->integer = operands.r_opA + operands.r_opB;
                    break;
                // B
                case 1:
                    *pc = TEXT_START + operands.r_opA;
                    break;
                // BEQZ
                case 2:
                    if(operands.r_opA == 0){
                        *pc = TEXT_START + operands.branch;
                    }
                    break;
                // BGE
                case 3:
                    if(operands.r_opA > operands.r_opB){
                        *pc = TEXT_START + operands.branch;
                    }
                    break;
                // BNE
                case 4:
                    if(operands.r_opA != operands.r_opB){
                        *pc = TEXT_START + operands.branch;
                    }
                    break;
                // LA
                case 5:
                    out->integer= operands.r_opA;
                    break;
                // LB
                case 6:
                    out->integer = operands.r_opA + operands.r_opB;
                    break;
                // LI
                case 7:
                    out->integer = operands.r_opA;
                    break;
                // SUBI
                case 8:
                    out->integer = operands.r_opA - operands.r_opB;
                    break;
                // SYSCALL
                case 9:
                    syscall(operands.r_opA);
                    break;
                // NOP
                case 10:
                    break;
                // Add
                case 11:
                    out->integer = operands.r_opA + operands.r_opB;
                    break;
                }
        }
}
/*
  Execution function for the load functional unit.
*/
void fload_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new){
    mem_addr address;
    if(scob_old.func_status[3].src1_ready == true && scob_old.func_status[3].src2_ready == true
                    && scob_old.func_status[3].busy == true){
         switch((unsigned int)scob_old.func_status[3].op){
            case 15:
                address = operands.load_A + operands.load_B;
                out->float_load = read_mem(address + DATA_START);
                break;
            case 16:
                write_mem((mem_addr) operands.load_B + DATA_START, operands.store_A);
                break;
        }

    }
}

/*
  Execution function for the floating point add and sub functional unit.
*/
void fp_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new){
    if(scob_old.func_status[1].src1_ready == true && scob_old.func_status[1].src2_ready == true
                        && scob_old.func_status[1].busy == true){
        printf("FAdd or Fsubb just took place\n\n");
        switch((unsigned int)scob_old.func_status[1].op){
            case 12:
                out->float_add = operands.fadd_opA + operands.fadd_opB;
                break;
            case 14:
                out->float_add = operands.fadd_opA - operands.fadd_opB;
                break;
        }
    }
}
/*
  Execution function for the floating point mult functional unit.
*/
void fmult_execution(struct scoreboard scob_old, struct operands operands, struct output *out, mem_addr *pc, struct scoreboard *scob_new){
    if(scob_old.func_status[2].src1_ready == true && scob_old.func_status[2].src2_ready == true
                    && scob_old.func_status[2].busy == true){
            printf("Fmult just took place\n\n");
            out->float_mult = operands.fmult_opA * operands.fmult_opB;
    }
}
/*
  Reads all the operands avaible for every instruction issued.
*/
void read_operands(struct scoreboard scob_old, int int_reg_file[], float float_reg_file[], struct operands *operands,
                                        struct fetch_buf fetch_buf, struct scoreboard *scob_new){
    int i;
    for(i = 0; i < 4; i++){
            //Both source registers are ready
        if(scob_old.func_status[i].src1_ready == true && scob_old.func_status[i].src2_ready == true
                        && scob_old.func_status[i].busy == true){
            printf("Operands read for this unit %d\n\n", i);
            //Set them to false so it hasn't get pulled during execution next clock
            scob_new->func_status[i].src1_ready = false;
            scob_new->func_status[i].src2_ready = false;
            if(i == 0){scob_new->instr_status.r_stage = 3;}
            if(i == 1){scob_new->instr_status.fadd_stage = 3;}
            if(i == 2){scob_new->instr_status.fmult_stage = 3;}
            if(i == 3){scob_new->instr_status.fload_stage = 3;}
            //Need to grab operands
            switch((unsigned int)scob_old.func_status[i].op){
                // Addi
                case 0:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->r_opB = fetch_buf.integer.type1.label;
                    break;
                // B
                case 1:
                    operands->r_opA = fetch_buf.integer.type3.label;
                    break;
                // BEQZ
                case 2:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->branch = fetch_buf.integer.type2.label;
                    break;
                // BGE
                case 3:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->r_opB = int_reg_file[scob_old.func_status[i].src2];
                    operands->branch = fetch_buf.integer.type1.label;
                    break;
                // BNE
                case 4:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->r_opB = int_reg_file[scob_old.func_status[i].src2];
                    operands->branch = fetch_buf.integer.type1.label;
                    break;
                // LA
                case 5:
                    operands->r_opA = fetch_buf.integer.type2.label;
                    break;
                // LB
                case 6:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->r_opB = fetch_buf.integer.type1.label;
                    break;
                // LI
                case 7:
                    operands->r_opA = fetch_buf.integer.type2.label;
                    break;
                // SUBI
                case 8:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->r_opB = fetch_buf.integer.type1.label;
                    break;
                // SYSCALL
                case 9:
                    operands->r_opA = fetch_buf.integer.type3.label;
                    break;
                // NOP
                case 10:
                    operands->r_opA = fetch_buf.integer.type3.label;
                    break;
                // Add
                case 11:
                    operands->r_opA = int_reg_file[scob_old.func_status[i].src1];
                    operands->r_opB = int_reg_file[scob_old.func_status[i].src2];
                    break;
                // fAdd
                case 12:
                    operands->fadd_opA = float_reg_file[scob_old.func_status[i].src1];
                    operands->fadd_opB = float_reg_file[scob_old.func_status[i].src2];
                    break;
                // fmult
                case 13:
                    operands->fmult_opA = float_reg_file[scob_old.func_status[i].src1];
                    operands->fmult_opB = float_reg_file[scob_old.func_status[i].src2];
                    break;
                // fsub
                case 14:
                    operands->fadd_opA = float_reg_file[scob_old.func_status[i].src1];
                    operands->fadd_opB = float_reg_file[scob_old.func_status[i].src2];
                    break;
                // ld
                case 15:
                    operands->load_A = int_reg_file[scob_old.func_status[i].src1];
                    operands->load_B = fetch_buf.load.type1.label;
                    break;
                // sd
                case 16:
                    // src value to store
                    operands->store_A = float_reg_file[scob_old.func_status[i].src1];
                    // offset = offset + value in integer register
                    operands->load_B = fetch_buf.load.type1.label + int_reg_file[scob_old.func_status[i].dest];
                    break;
            }
        }
    }

}
/*
  Sets all registers to -1;
*/
void init_reg(struct reg_status *reg_status, int int_reg_file[], float float_reg_file[]){
    int i;
    for(i = 0; i < INT_REG_NUM; i++){
        int_reg_file[i] = 0;
        reg_status->r_reg_status[i] = -1;
    }
    for(i = 0; i < FLOAT_REG_NUM; i++){
        float_reg_file[i] = 0;
        reg_status->f_reg_status[i] = -1;
    }
}
/*
  Actually does the issue of setting the scoreboard for a new instrction that is issued.
*/
void scoreboard_issue(struct scoreboard *scob, struct instruction_container instruction, unsigned int op){
    int type = instruction.type;

    //Issue the Instructions that use Integer FU
    if(op < 12 && op >= 0){
            scob->func_status[0].busy = true;
            scob->func_status[0].op = op;
        if(type == 1){
            // ADDI LB SUBI
            if((unsigned int) op == 0 ||  (unsigned int) op == 6 || (unsigned int) op == 8){
                scob->func_status[0].dest = instruction.type1.r_dest;
                scob->func_status[0].src1 = instruction.type1.r_src;
                scob->func_status[0].src2 = -1;
                scob->func_status[0].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[0].src1, 0);
                scob->func_status[0].Fu_src2 = -1;
                scob->func_status[0].src1_ready = check_waw(scob->reg_status, scob->func_status[0].src1, 0);
                scob->func_status[0].src2_ready = true;
            }
            // BGE BNE
            if((unsigned int) op == 3 || (unsigned int)op == 4){
                scob->func_status[0].dest = -1;
                scob->func_status[0].src1 = instruction.type1.r_dest;
                scob->func_status[0].src2 = instruction.type1.r_src;
                scob->func_status[0].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[0].src1, 0);
                scob->func_status[0].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[0].src2, 0);
                scob->func_status[0].src1_ready = check_waw(scob->reg_status, scob->func_status[0].src1, 0);
                scob->func_status[0].src2_ready = check_waw(scob->reg_status, scob->func_status[0].src2, 0);
            }
            // ADD
            else{
            scob->func_status[0].dest = instruction.type1.r_dest;
            scob->func_status[0].src1 = instruction.type1.r_src;
            scob->func_status[0].src2 = instruction.type1.label;
            scob->func_status[0].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[0].src1, 0);
            scob->func_status[0].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[0].src2, 0);
            scob->func_status[0].src1_ready = check_waw(scob->reg_status, scob->func_status[0].src1, 0);
            scob->func_status[0].src2_ready = check_waw(scob->reg_status, scob->func_status[0].src2, 0);
            }
        }
        if(type == 2){
            scob->reg_status.r_reg_status[instruction.type2.r_target] = 0;
            scob->func_status[0].dest = instruction.type2.r_target;
            scob->func_status[0].src1 = -1;
            scob->func_status[0].src2 = -1;
            scob->func_status[0].Fu_src1 = -1;
            scob->func_status[0].Fu_src2 = -1;
            scob->func_status[0].src1_ready = true;
            scob->func_status[0].src2_ready = true;
            if((unsigned int) op == 2){
                scob->func_status[0].src1 = instruction.type2.r_target;
                scob->func_status[0].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[0].src1, 0);
                scob->func_status[0].src1_ready = check_waw(scob->reg_status, scob->func_status[0].src1, 0);
            }
        }
        scob->reg_status.r_reg_status[scob->func_status[0].dest] = 0;
        scob->instr_status.r_stage = 2;
    }
    //Issue the instructions that use the FLOAT ADDER and SUB
    if((unsigned int)op == 12  || (unsigned int)op == 14){
                scob->reg_status.f_reg_status[instruction.type1.r_dest] = 1;
                // Only type 1 instructions for FLOAT
                scob->func_status[1].busy = true;
                scob->func_status[1].op = op;
                scob->func_status[1].dest = instruction.type1.r_dest;
                scob->func_status[1].src1 = instruction.type1.r_src;
                scob->func_status[1].src2 = instruction.type1.label;
                scob->func_status[1].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[1].src1, 1);
                scob->func_status[1].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[1].src2, 1);
                scob->func_status[1].src1_ready = check_waw(scob->reg_status, scob->func_status[1].src1, 1);
                scob->func_status[1].src2_ready = check_waw(scob->reg_status, scob->func_status[1].src2, 1);
                scob->instr_status.fadd_stage = 2;
    }
        // Mult
        if((unsigned int)op == 13){
                scob->reg_status.f_reg_status[instruction.type1.r_dest] = 2;
                scob->func_status[2].busy = true;
                scob->func_status[2].op = op;
                scob->func_status[2].dest = instruction.type1.r_dest;
                scob->func_status[2].src1 = instruction.type1.r_src;
                scob->func_status[2].src2 = instruction.type1.label;
                scob->func_status[2].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[2].src1, 1);
                scob->func_status[2].Fu_src2 = get_src_fu(scob->reg_status, scob->func_status[2].src2, 1);
                scob->func_status[2].src1_ready = check_waw(scob->reg_status, scob->func_status[2].src1, 1);
                scob->func_status[2].src2_ready = check_waw(scob->reg_status, scob->func_status[2].src2, 1);
                scob->instr_status.fmult_stage = 2;
        }
        //Load
        if((unsigned int)op == 15){
                scob->reg_status.f_reg_status[instruction.type1.r_dest] = 3;
                scob->func_status[3].busy = true;
                scob->func_status[3].op = op;
                scob->func_status[3].dest = instruction.type1.r_dest;
                scob->func_status[3].src1 = instruction.type1.r_src;
                scob->func_status[3].src2 = -1;
                scob->func_status[3].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[3].src1, 0);
                scob->func_status[3].Fu_src2 = -1;
                scob->func_status[3].src1_ready = check_waw(scob->reg_status, scob->func_status[3].src1, 0);
                scob->func_status[3].src2_ready = true;
                scob->instr_status.fload_stage = 2;
                scob->func_status[3].time = 2;
        }
        // Store
        if((unsigned int)op == 16){
                scob->func_status[3].busy = true;
                scob->func_status[3].op = op;
                scob->func_status[3].dest = instruction.type1.r_src;
                scob->func_status[3].src1 = instruction.type1.r_dest;
                scob->func_status[3].src2 = -1;
                scob->func_status[3].Fu_src1 = get_src_fu(scob->reg_status, scob->func_status[3].src1, 0);
                scob->func_status[3].Fu_src2 = -1;
                scob->func_status[3].src1_ready = check_waw(scob->reg_status, scob->func_status[3].src1, 0);
                scob->func_status[3].src2_ready = true;
                scob->instr_status.fload_stage = 2;
                scob->func_status[3].time = 2;
        }

}

/*
  Gets the functional unit that is related to the src register of some instruction.
*/
unsigned int get_src_fu(struct reg_status reg_status, int reg, unsigned int r_or_f){
    // Integer register
    if(r_or_f == 0){
        if(reg > 32 || reg < 0){return -1;}
        return reg_status.r_reg_status[reg];
    }
    // Floating Registers
    if(r_or_f > 0){
        if(reg > 16 || reg < 0){return -1;}
        return reg_status.f_reg_status[reg];
    }
    return -1;
}

/*
    Issues instruction, ie checks for hazards and then gives to true issue method.
    Returns true if instruction can move to next stage, false if it must stall.
    Params: Pc - instruction pointer, scob_old - scoreboard for this clock cycle, fetch_buf - store
    moving to another stage.
*/
bool instruction_issue(mem_addr *pc, struct scoreboard scob_old, struct scoreboard *scob_new, struct fetch_buf *fetch_buffer,
                                                struct performance *performance, int *to_run){

    performance->clock_cycles += 1;

    struct instruction_container instr_decoded;
    instruction instr_fetched = fetch(pc, to_run);
    unsigned int op;
    mem_word reg_dest;
    instr_decoded = decode(instr_fetched);
    int r_or_f = 1;
    //Get op code of instruction
    if(instr_decoded.type == 1){op = instr_decoded.type1.op_code;}
    if(instr_decoded.type == 2){op = instr_decoded.type2.op_code;}
    if(instr_decoded.type == 3){op = instr_decoded.type3.op_code;}
    if((unsigned int) op < 12){
        r_or_f = 0;
    }
    // Get reg destination of instruction
    if(instr_decoded.type == 1){reg_dest = instr_decoded.type1.r_dest;}
    if(instr_decoded.type == 2){reg_dest = instr_decoded.type2.r_target;}


    // Check to see if the FUNC UNIT is already in use
    if(check_struct(scob_old.func_status[get_functional_unit(op)], op) == true){
        printf("Stalling! Structure hazard. \n");
        *pc -= 1;
        return false;
    }
    // Check to see if any instructions in FUNC UNITS are writing to same dest
    if(check_waw(scob_old.reg_status, reg_dest, r_or_f) == false){
        printf("Stalling! Write after Write dependency\n");
        *pc -= 1;
        return false;
    }
    // Set buffer to know which instructions are in in execution
    set_fetch_buffer(fetch_buffer, instr_decoded, op);
    //Actually update the scoreboard to show instruction is moving to execution stage
    scoreboard_issue(scob_new, instr_decoded, op);
    if(scob_new->func_status[0].op == 10){performance->nop_count += 1;}
    performance->instruction_count += 1;
    printf("Just issued this instruction with opcode: %lu\n", op);
    return true;
}

/*
  Sets the fetch buffer with the new instruction that comes in.
*/
void set_fetch_buffer(struct fetch_buf *buf, struct instruction_container instr, unsigned int op){
    // Integer
    if(op < 12 && op >= 0){
        // 0 is integer unit
        buf->integer = instr;
    }
    //Fpoint add and sub
    if(op == 12 || op == 14){
        buf->f_add = instr;
    }
    // Fmult
    if(op == 13){
        buf->f_mult = instr;
    }
    // LOAD/STORE
    if(op == 15 || op == 16){
        buf->load = instr;
    }
}

/*
  Checks for writes after writes.
*/
bool check_waw(struct reg_status reg_stat, mem_word reg_dest, int r_or_f){
    // r_or_f tells whether the register is a float or integer, a float will never write to integer register
    // 0 if integer, 1 if float
    //Store -1 when not in use
    switch(r_or_f){
        case 0:
            if(reg_dest > 32 || reg_dest < 0){return false;}
            if(reg_stat.r_reg_status[reg_dest] != -1){
                return false;
            }
            break;
        case 1:
            if(reg_dest > 16 || reg_dest < 0){return false;}
            if(reg_stat.f_reg_status[reg_dest] != -1){
                return false;
            }
            break;

    }
    return true;
}

/*
  Checks for structural hazards.
*/
bool check_struct(struct func_status fu_status, unsigned int op){
    int i = 0;
    //All using integer unit
    if((unsigned int)op < 12 && (unsigned int)op >= 0){
        // 0 is integer unit
        if(fu_status.busy == true){
            return true;
        }
    }
    //Fpoint add and sub
    if((unsigned int)op == 12 || (unsigned int)op == 14){
        if(fu_status.busy == true){
            return true;
        }
    }
    // Fmult
    if((unsigned int)op == 13){
        if(fu_status.busy == true){
            return true;
        }
    }
    // LOAD/STORE
    if((unsigned int)op == 15 || (unsigned int)op == 16){
        if(fu_status.busy == true){
            return true;
        }
    }
    return false;
}

/*
  Gets the functional unit related to a certain op code.
*/
int get_functional_unit(unsigned int op){
    //All using integer unit
    if(op < 12 && op >= 0){
        // 0 is integer unit
            return 0;
    }
    //Fpoint add and sub
    if(op == 12 || op == 14){
            return 1;
    }
    // Fmult
    if(op == 13){
            return 2;
    }
    // LOAD/STORE
    if(op == 15 || op == 16){
            return 3;
    }
    return -1;
}

/*Description: Creates memory for the memory segments.
  Params: NONE
  Returns: void
*/
void make_memory(){
	// Calloc used to '0' all memory
    kernal_segment = malloc(MAX_SEG_SIZE);
    if(kernal_segment == NULL)
        exit(1);
    text_segment = malloc(MAX_SEG_SIZE);
    if(text_segment == NULL)
        exit(1);

}

/*Description: Loads the source code elements into simulated memory.
  Params: *filename -name of source code file
  Returns: void
*/
void parse_source_code(char *filename){
    FILE *fp;
    short data_load = 0;
    size_t length = 0;
    ssize_t read;
    int text_index = 0;
    char *token = NULL;
    char *line = NULL;
    fp = fopen(filename, "r");

    if(fp == NULL){
       printf("Could not open file.\n");
        exit (1);
    }
    while((read = getline(&line, &length, fp)) != -1){
            if(strcmp(line, "\n") != 0){
            line[strcspn(line, "\n")] = 0;
            //In data section of parse
                if(strcmp(line, ".data") == 0){
                     data_load = 1;
                continue;
                }
            //Exit data section of parse
                if(strcmp(line, ".text") == 0){
                     data_load = 0;
                     continue;
}
                if(data_load == 1){
                    load_data(line);
                }
                else{
                    load_text(line, &text_index);
                }
            }
    }
     instruction_count = text_index - 1;
}

/*Description: Loads data elements from source into simulated data segment.
  Params: *token - essentially the line that was read
  Returns: void
*/
void load_data(char* token){
    char *label = NULL;
    char *address = NULL;
    char *value = NULL;

    mem_addr addr = 0;
    mem_word val = 0;

    address = strtok(token, " \t");
    //Checking for a data of type string
    if(strcmp(address, ".float") == 0){
        address = strtok(NULL, " \t");
        addr = (mem_addr) strtol(address, (char **)NULL, 16);
        value = strtok(NULL, "\"") ;
        float newUm = strtof(value, NULL);
        write_mem(addr, newUm);
    }
    /* We are keeping up with the labels, add each label to table */
    else if(strcmp(address, ".label") == 0){
      label = strtok(NULL, " \t");
      strcpy(labels[last_label], label);
      address = strtok(NULL, " \t");
      label_addresses[last_label] = ((mem_addr) strtol(address, (char **)NULL, 16)) - TEXT_START;
      last_label += 1;
    }
    // We are keeping up with allocation, add each string allocation to table
    else if(strcmp(address, ".space") == 0){
      value = strtok(NULL, " \t");
      strcpy(string_allocation[last_string], value);
      address = strtok(NULL, " \t");
      mem_word length_of_string = (mem_word) strtol(address, (char **)NULL, 10);
      string_lengths[last_string] = length_of_string;
      string_addresses[last_string] = top_string_mem;
      last_string += 1;
      top_string_mem += length_of_string;
    }
    else{
      value = strtok(NULL, " \t");
      addr = (mem_addr) strtol(address, (char **)NULL, 16);
      val = (mem_word) strtol(value, (char **)NULL, 16);
      write_mem(addr, val);
    }
}

/*Description: Loads source code instructions into simulated text segment.
  Paramsi *index - program counter incremented for next instruction entry
  Returns: void
*/
void load_text(char* token, int *index){
    char *instr = strtok(token, " \t");
    unsigned int op_code = get_opCode(instr);
    int allocated;
    instruction new_instruction;
    mem_addr address = TEXT_START + *index;
    int label_index = 0;
    if((unsigned int)op_code == 10){
        struct instr_type1 instruction;
        instruction.op_code = op_code;
        new_instruction = type1_create(instruction);
        unsigned int new = (new_instruction >> 28) & 0x0000000f;
        write_instr(address, new_instruction);
        *index += 1;
    }
    // Type 1 Instructions
    if((op_code == 0) || (op_code == 3) || (op_code == 4) || ((unsigned int)op_code == 8) || ((unsigned int)op_code == 11)||
                (unsigned int)op_code == 12 || (unsigned int)op_code == 13 || (unsigned int)op_code == 14){
        struct instr_type1 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", ");
        instruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        if((unsigned int)op_code > 11){instruction.r_dest = (mem_word) strtol(instr + 2, (char **)NULL, 10);}
        instr = strtok(NULL, ", ");
        instruction.r_src = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        if((unsigned int)op_code > 11){instruction.r_src = (mem_word) strtol(instr + 2, (char **)NULL, 10);}
        instr = strtok(NULL, ", ");
        // BGE and BNE must find the label they are to branch to
        if((op_code == 3) || (op_code == 4)){
            label_index = find_label(instr);
            instruction.label = label_index;
        }
        else if((unsigned int)op_code > 11 && (unsigned int)op_code < 15){instruction.label = (mem_word) strtol(instr + 2, (char **)NULL, 10);}
        else if((unsigned int)op_code == 11){
        instruction.label = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        }
        else{
            instruction.label = (mem_word) strtol(instr, (char **)NULL, 10);
        }
        // Encode Instruction and Write to Memory
        new_instruction = type1_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
    // Type 2 Instructions
    if((op_code == 2) || (op_code == 5) || (op_code == 7)){
        struct instr_type2 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", ");
        instruction.r_target = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        // BEQZ Requires a label that references the instruction_segment
        if(op_code == 2){
            label_index = find_label(instr);
            instruction.label = (mem_word) label_index;
        }
        // LA needs to know if to load in an allocated address, or one
        // specified when declaring the string
        else if(op_code == 5){
            allocated = check_allocated(instr);
            if(allocated != -1){
                instruction.label = string_addresses[allocated];
            }
            else{
                instruction.label = (((mem_word) strtol(instr, (char **)NULL, 16)) - DATA_START);
            }
        }
        else {
            instruction.label = (mem_word) strtol(instr, (char **)NULL, 10);
        }
        new_instruction = type2_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
        }
    // B Requires a label that references the instruction_segment
    if(op_code == 1){
        struct instr_type3 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, " \t");
        label_index = find_label(instr);
        instruction.label = (mem_word) label_index;
        new_instruction = type3_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
    // SYSCALL
    if((unsigned int)op_code == 9){
        struct instr_type3 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, " \t");
        instruction.label = (mem_word) strtol(instr, (char **)NULL, 10);
        new_instruction = type3_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
    // LB requires a lot of complex parsing
    if(op_code == 6 || (unsigned int)op_code > 14){
        struct instr_type1 instruction;
        char reg[3];
        char offset[10];
        int reg_index;
        int close_par_index;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", "); // Reg_dest
        instruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        if((unsigned int)op_code > 14){instruction.r_dest = (mem_word) strtol(instr + 2, (char **)NULL, 10);}
        instr = strtok(NULL, ", "); // Second half
        reg_index = strcspn(instr, "$");
        close_par_index = strcspn(instr, ")");

        memcpy(reg, instr + reg_index + 1, (close_par_index - reg_index));
        instruction.r_src = strtol(reg, (char **)NULL, 10); // Reg_src
        memcpy(offset, instr, reg_index - 1);
        offset[(reg_index-1)] = '\0';
        instruction.label = (mem_word) strtol(offset, (char **)NULL, 10); // Offset
        new_instruction = type1_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
}

/*
	TYPE 1 instruction encoding: 4 bit OP code, 5 bit Rdest, 5 bit Rsrc, 18 bit Label
	Description: Encodes a type1 Instruction.
 	Params: struct instr_type1 instr - parsed values from source file
 	Returns: instruction - in type1 format
*/
instruction type1_create(struct instr_type1 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 27) | new_instruction;
    if(instr.op_code == 10) return new_instruction;
    new_instruction = (instr.r_dest << 22) | new_instruction;
    new_instruction = (instr.r_src << 17) | new_instruction;
    new_instruction = (instr.label & 0x0001ffff) | new_instruction;
    return new_instruction;

}

/* 	TYPE 2 instruction encoding: 4 bit OP code, 5 bit Rsrc, 23bit Label
	Description: Encodes a type2 Instruction.
 	Params: struct instr_type2 instr - parsed values from source file
 	Returns: instruction - in type2 format
*/
instruction type2_create(struct instr_type2 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 27) | new_instruction;
    new_instruction = (instr.r_target << 22) | new_instruction;
    new_instruction = (instr.label & 0x003fffff) | new_instruction;
    return new_instruction;
}

/* TYPE 3 instruction encoding: 4 bit OP code, 28 bit Label
	Description: Encodes a type3 Instruction.
 	Params: struct instr_type3 instr - parsed values from source file
 	Returns: instruction - in type3 format
*/
instruction type3_create(struct instr_type3 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 27) | new_instruction;
    new_instruction = (instr.label & 0x07ffffff) | new_instruction;
    return new_instruction;
}

/*Description: Gets the op code for an instruction for easier switch statement.
  Params: *instr - instruction that was parsed from source file
  Returns: instruction - opcode number or address for an instruction
*/
int get_opCode(char *instr){
    if(strcmp(instr, "addi") == 0) return 0;
    else if (strcmp(instr, "b") == 0) return 1;
    else if (strcmp(instr, "beqz") == 0) return 2;
    else if (strcmp(instr, "bge") == 0) return 3;
    else if (strcmp(instr, "bne") == 0) return 4;
    else if (strcmp(instr, "la") == 0) return 5;
    else if (strcmp(instr, "lb") == 0) return 6;
    else if (strcmp(instr, "li") == 0) return 7;
    else if (strcmp(instr, "subi") == 0) return 8;
    else if (strcmp(instr, "syscall") == 0) return 9;
    else if (strcmp(instr, "nop") == 0) return 10;
    else if (strcmp(instr, "add") == 0) return 11;
    else if (strcmp(instr, "fadd") == 0) return 12;
    else if (strcmp(instr, "fmult") == 0) return 13;
    else if (strcmp(instr, "fsub") == 0) return 14;
    else if (strcmp(instr, "ld") == 0) return 15;
    else if (strcmp(instr, "sd") == 0) return 16;
    else return -1;
}

/*Description: Compares to_search with labels in source file.
  Params: *char to_search - label value to look for
  Returns: int - the index of the label so the address can be looked up
*/
int find_label(char* to_search){
    int i;
    for(i = 0; i < last_label; i++){
        if(strcmp(to_search, labels[i]) == 0){
            return label_addresses[i];
        }
    }
    return -1;
}

/* For each of the reads and writes we first check to see if the address
    is less than its address top boundary and then the bottom.*/

/*Description: Read memory from the data_segment.
          Params: address - address to read from
          Returns: mem_word - returns the contents
*/
float read_mem(mem_addr address){
    if((address <  DATA_TOP) && (address >= DATA_START))
        return data_segment[(address - DATA_START)];
    else{
        printf("Read memory fail:\n");
        return 0.0;
    }
}

/*
      Description: Reads an instruction from memory.
      Params: address - address to read from, to_run - signaling last instruction
      Returns: if_id fetched - latch from fetch
*/
instruction fetch(mem_addr *address, int *to_run){
    instruction fetched;
    // Fetch and check to see if this instruction is out of bounds, if so we know we are at end of file
    if((*address <  TEXT_TOP) && (*address >= TEXT_START)){
        fetched = text_segment[(*address - TEXT_START)];
        printf("This is address from fetch: %d\n", (*address - TEXT_START));
        if((*address - TEXT_START) == 31){
            *to_run = 0;
        }
        *address += 1;
        return fetched;
    }
}

/*Description: Writes memory to the text_segment.
      Params: address - address to read from, instr- instruction to write
      Returns: void
*/
void write_instr(mem_addr address, instruction instr){

    if((address <  TEXT_TOP) && (address >= TEXT_START))
        text_segment[(address - TEXT_START)] = instr;
    else{
        printf("Write instruction fail:");
        return NULL;
    }
}

/*Description: Writes memory to the data_segment.
      Params: address - address to write to, value- string to write
      Returns: void
*/
void write_mem(mem_addr address, float value){
    if((address < DATA_TOP) && (address >= DATA_START)){
        data_segment[address - DATA_START] = value;
    }
    else{
        printf("Write memowry fail:");
        return NULL;
    }
}

/*
      Description: Decoded an instruction fetched from memory.
      Params: if_id to_decode - latch containing instruction
              mem_addr *pc - need the pc so we can branch
              exe_mem *exe_new - exe latch from prev clock cycle
              mem_wb *mem_new - mem_wb latch containing memory access from prev cycle
      Returns: id_exe decoded - latch with decoded instruction
*/
struct instruction_container decode(instruction to_decode){
    struct instruction_container instruction;
    unsigned int op_code = (to_decode >> 27) & 0x0000001f;
    mem_word dest;
    mem_word src;
    mem_word label;
    // Type 1
    if((op_code == 0) || (op_code == 3)
            || (op_code == 4) || (op_code == 6) || ((unsigned int)op_code == 8) || ((unsigned int) op_code == 11) ||
                ((unsigned int) op_code > 11)){
        instruction.type1.op_code = op_code;
        dest = (to_decode >> 22) & 0x0000001f;
        instruction.type1.r_dest = dest;
        src =  (to_decode >> 17) & 0x0000001f;
        instruction.type1.r_src = src;
        instruction.type1.label = (to_decode & 0x0001ffff);
        instruction.type = 1;
    }
    // Type 2
    else if((op_code == 2) || (op_code ==5) || (op_code == 7)){
        instruction.type2.op_code = op_code;
        dest = (to_decode >> 22) & 0x0000001f;
        instruction.type2.r_target = dest;
        instruction.type2.label = (to_decode & 0x0003ffff);
        instruction.type = 2;
        // BEQZ needs the value out of one register
        /*
        if(op_code == 2){
            decoded.op_a = REGISTER_FILE[instruction.type2.r_target];
        }
        */
    }
    // Type3
    else if((op_code == 1) || (op_code == 9)){
        instruction.type3.op_code = (mem_word) op_code;
        instruction.type3.label = (mem_word) (to_decode & 0x07ffffff);
        instruction.type = 3;
        //Branch label or System call
       // decoded.op_a = instruction.type3.label;
    }
    //NOP
    else if((unsigned int) op_code == 10){
        instruction.type1.op_code = (mem_word) op_code;
        instruction.type = 1;
    }
   return instruction;
}
/*Description: Completes the corresponding SYSCALL.
  Params: mem_word function - the system function
  Returns: instruction - opcode number or address for an instruction
*/
int syscall(mem_word function){
    // Read a string in
    int index = 0;
    if(function == 0){
        //int index  = get_index((REGISTER_FILE[0] - DATA_START));
        char buff[string_lengths[index]];
        scanf("%s", buff);
        memcpy((data_segment + string_addresses[index]), buff, strlen(buff)+ 1);
        data_segment[strlen(buff)] = '\0';
    }
    // Print a string
    if(function == 1){
        //int index  = (REGISTER_FILE[0] - DATA_START);
        printf("%s\n", &data_segment + index);
    }
    // Exit code
    if(function == 9){
        return 9;
    }
    // Prints a register value
    if(function == 6){
        //printf("Register %d is: %d\n", REGISTER_FILE[0], REGISTER_FILE[REGISTER_FILE[0]]);
    }
    return 0;
}

/*Description: Checks to see if the string name has been allocated.
  Params: *char match - string to match
  Returns: int - the index in the allocation table
*/
int check_allocated(char* match){
    int i;
    for(i = 0; i < last_string; i++){
        if(strcmp(string_allocation[i], match) == 0){
            return i;
        }
    }

    return -1;
}

/*Description: Gets the index of a string by address.
  Params: int match - address to match with string allocations
  Returns: int - the index in the allocation table
*/
int get_index(int match){
    int i;
    for(i = 0; i < last_string; i++){
        if(string_addresses[i] == match){
        return i;
        }
    }
    return -1;

}
