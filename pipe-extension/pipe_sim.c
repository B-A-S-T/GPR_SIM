/*
  Group 1
  Created October 2016 by Ian Thomas.

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

#define MAX_SEG_SIZE 33554428
#define NUM_STRINGS 10
#define STRING_SIZE 100
#define TEXT_START ((mem_word) 0x50000000)
#define DATA_START ((mem_word) 0x40000000)
#define KERNAL_START ((mem_word) 0x60000000)
#define DATA_TOP (TEXT_START -1)
#define TEXT_TOP (KERNAL_TOP -1)
#define KERNAL_TOP (((mem_word) (0x70000000)) -1)
#define NUM_REGISTERS 32
#define NUM_LABELS 50 // 50 Labels allowed
#define LABEL_SIZE 50 // Label string size =50

/*Global Variables:  */
instruction* text_segment;
char *data_segment;
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
mem_word REGISTER_FILE[NUM_REGISTERS]; // Keeping up with Registers
int instruction_count;

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
	We pakage the instruction types together to allow dynamic use during
	the execution process.
*/
struct instruction_container{
    struct instr_type1 type1;
    struct instr_type2 type2;
    struct instr_type3 type3;
    int type;
};

struct if_id{
    instruction instr;        
};

struct id_exe{
    unsigned int op_code;
    // which type of instruction
    struct instruction_container instruction;
    //values in registers
    int op_a;
    int op_b; 
};

struct exe_mem{
    unsigned int op_code; 
    mem_word ALU_out; // output
    int op_B; //used for store word
    mem_word r_dest;
};

struct mem_wb {
    unsigned int op_code;
    char MDR; // Data from memory
    int op_b; // where to load word
    mem_word ALU_out; // Type1/R-Type
    mem_word reg_dest; // where to write back    
};

/* Prototypes */
void make_memory();
void parse_source_code(char *filename);
void load_data(char* token);
void load_text(char* token, int *index);
int get_opCode(char* instr);
char read_mem(mem_addr address);
void write_instr(mem_addr address, instruction instr);
void write_mem(mem_addr address, char *value);
instruction type1_create(struct instr_type1 instr);
instruction type2_create(struct instr_type2 instr);
instruction type3_create(struct instr_type3 instr);
int find_label(char* to_search);
int alu(int op_a, int op_b, int op_code);
int check_allocated(char* match);
int syscall(mem_word function);
int get_index(int match);
struct if_id _if(mem_addr *address, int *to_run);
struct id_exe _id(struct if_id to_decode, mem_addr *pc, struct exe_mem *exe_new, struct mem_wb *mem_new);
struct exe_mem _exe(struct id_exe to_execute, struct exe_mem *old, struct mem_wb *new);
struct mem_wb _mem(struct exe_mem to_access);
void _wb(struct mem_wb to_write);

/*Main entry into simulator, one argument should be passed, the filename.*/
int main(int argc, char *argv[]){
    //if_id
    struct if_id if_id_new;
    struct if_id if_id_old;
    //id_exe
    struct id_exe id_exe_new;
    struct id_exe id_exe_old;
    //exe_mem
    struct exe_mem exe_mem_new;
    struct exe_mem exe_mem_old;
    //mem_wb
    struct mem_wb mem_wb_new;
    struct mem_wb mem_wb_old;

    make_memory();
    parse_source_code(argv[1]);
    instruction instr;
    int run = 1;
    int warm_up = 0;
    int cooldown = 0;
    int usermode = 1;
    mem_addr pc = TEXT_START; // PC
    float C = 0;
    float IC = 0;
    // Source files longer than or equal to 5 instructions
    //printf("Instruction count: %d\n", instruction_count);
    if(instruction_count >= 4){
        while(run){
            if(warm_up >= 0){
                if_id_old = if_id_new;
                if_id_new = _if(&pc, &run);
            }
            if(warm_up > 0){
                id_exe_old = id_exe_new;
                id_exe_new = _id(if_id_old, &pc, &exe_mem_new, &mem_wb_new);
            }
            if(warm_up > 1){
                exe_mem_old = exe_mem_new;
                exe_mem_new = _exe(id_exe_old, &exe_mem_old, &mem_wb_new);
            }
            if(warm_up > 2){
                mem_wb_old = mem_wb_new;
                mem_wb_new = _mem(exe_mem_old);
            }
            if(warm_up > 3){
                _wb(mem_wb_old);
            }
            warm_up += 1;
            }
            // Pushes the last few instructions through the pipeline
            id_exe_old = id_exe_new;
            while(cooldown < 3){
                if(cooldown < 1){
                    exe_mem_old = exe_mem_new;
                    exe_mem_new = _exe(id_exe_old, &exe_mem_old, &mem_wb_new);
                }
                if(cooldown < 2){
                    mem_wb_old = mem_wb_new;
                    mem_wb_new = _mem(exe_mem_old);
                }
                _wb(mem_wb_old);
                exe_mem_old = exe_mem_new;
                mem_wb_old = mem_wb_new;
                cooldown += 1;
            }
    }
    
/*
    printf("IC = %f\n C = %f\n Ratio = %f\n", IC, C, (8*IC)/C);
*/
    free(data_segment);
    free(kernal_segment);
    free(text_segment);
    return 0;
}

/*Description: Creates memory for the memory segments.
  Params: NONE
  Returns: void
*/
void make_memory(){
	// Calloc used to '0' all memory
    data_segment = calloc(MAX_SEG_SIZE, 1);
    if(data_segment == NULL)
        exit(1);
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
    if(strcmp(address, ".asciiz") == 0){
        address = strtok(NULL, " \t");
        addr = (mem_addr) strtol(address, (char **)NULL, 16);
        value = strtok(NULL, "\"") ;
        write_mem(addr, value);
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
  Params: *index - program counter incremented for next instruction entry
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
    if((op_code == 0) || (op_code == 3) || (op_code == 4) || ((unsigned int)op_code == 8) || ((unsigned int)op_code == 11)){
        struct instr_type1 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", ");
        instruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        instruction.r_src = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        // BGE and BNE must find the label they are to branch to
        if((op_code == 3) || (op_code == 4)){
            label_index = find_label(instr);
            instruction.label = label_index;
        }
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
    if(op_code == 6){
        struct instr_type1 instruction;
        char reg[3];
        char offset[10];
        int reg_index;
        int close_par_index;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", "); // Reg_dest
        instruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
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
    new_instruction = (instr.op_code << 28) | new_instruction;
    if(instr.op_code == 10) return new_instruction;
    new_instruction = (instr.r_dest << 23) | new_instruction;
    new_instruction = (instr.r_src << 18) | new_instruction;
    new_instruction = (instr.label & 0x001fffff) | new_instruction;
    return new_instruction;

}

/* 	TYPE 2 instruction encoding: 4 bit OP code, 5 bit Rsrc, 23bit Label
	Description: Encodes a type2 Instruction.
 	Params: struct instr_type2 instr - parsed values from source file
 	Returns: instruction - in type2 format
*/ 
instruction type2_create(struct instr_type2 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.r_target << 23) | new_instruction;
    new_instruction = (instr.label & 0x007fffff) | new_instruction;
    return new_instruction;
}

/* TYPE 3 instruction encoding: 4 bit OP code, 28 bit Label
	Description: Encodes a type3 Instruction.
 	Params: struct instr_type3 instr - parsed values from source file
 	Returns: instruction - in type3 format
*/ 
instruction type3_create(struct instr_type3 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.label & 0x0fffffff) | new_instruction;
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
char read_mem(mem_addr address){
    if((address <  DATA_TOP) && (address >= DATA_START))
        return data_segment[(address - DATA_START)];
    else{
        printf("Read memory fail:\n");
        return NULL;
    }
}

/*Description: Read memory from the text_segment.
      Params: address - address to read from
      Returns: insrtuction - returns the contents
*/
struct if_id _if(mem_addr *address, int *to_run){
    struct if_id fetched;
    if((*address <  TEXT_TOP) && (*address >= TEXT_START) && ((*address - TEXT_START) <= instruction_count)){
        fetched.instr = text_segment[(*address - TEXT_START)];
        *address += 1;
        return fetched;
    }
    else{
        *to_run = 0;
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
void write_mem(mem_addr address, char *value){
    if((address < DATA_TOP) && (address >= DATA_START))
        memcpy(data_segment + (address - DATA_START), value, strlen(value));
    else{
        printf("Write memowry fail:");
        return NULL;
    }
}

/*Description: Decodes an instruction.
  Params: instruction to_decode - that was stored in memory
  Returns: struct instruction_container - container with the correct
  		   instruction inside.
*/
struct id_exe _id(struct if_id to_decode, mem_addr *pc, struct exe_mem *exe_new, struct mem_wb *mem_new){
    struct instruction_container instruction;
    struct id_exe decoded;
    unsigned int op_code = (to_decode.instr >> 28) & 0x0000000f;
    mem_word dest;
    mem_word src;
    mem_word label;
    // Type 1
    if((op_code == 0) || (op_code == 3) 
            || (op_code == 4) || (op_code == 6) || ((unsigned int)op_code == 8) || ((unsigned int) op_code == 11)){
        instruction.type1.op_code = op_code;
        dest = (to_decode.instr >> 23) & 0x0000001f;
        instruction.type1.r_dest = dest;
        src =  (to_decode.instr >> 18) & 0x0000001f;
        instruction.type1.r_src = src;
        instruction.type1.label = (to_decode.instr & 0x0003ffff);
        instruction.type = 1;
        decoded.instruction = instruction;
        // ADD needs the values in two registers
        if((unsigned int) op_code == 11){
            decoded.op_a = REGISTER_FILE[instruction.type1.r_src];
            decoded.op_b = REGISTER_FILE[instruction.type1.label];
        }
        // BGE and BNE need values out of two registers
        else if((op_code == 3) || (op_code == 4)){
            decoded.op_a = REGISTER_FILE[instruction.type1.r_dest];
            decoded.op_b = REGISTER_FILE[instruction.type1.r_src];
        }
        // all other just need one register value and a label/immediate
        // ADDI:0----LB:6----SUBI:8
        else{
            decoded.op_a = REGISTER_FILE[instruction.type1.r_src];
            decoded.op_b = instruction.type1.label;
        }
    }
    // Type 2
    else if((op_code == 2) || (op_code ==5) || (op_code == 7)){
        instruction.type2.op_code = op_code;
        dest = (to_decode.instr >> 23) & 0x0000001f;
        instruction.type2.r_target = dest;
        instruction.type2.label = (to_decode.instr & 0x0007ffff);
        instruction.type = 2;
        decoded.instruction = instruction;
        // BEQZ needs the value out of one register
        if(op_code == 2){
            decoded.op_a = REGISTER_FILE[instruction.type2.r_target];
        }
    }
    // Type3
    else if((op_code == 1) || (op_code == 9)){
        instruction.type3.op_code = (mem_word) op_code;
        instruction.type3.label = (mem_word) (to_decode.instr & 0x0fffffff);
        instruction.type = 3;
        decoded.instruction = instruction;
        //Branch label or System call 
        decoded.op_a = instruction.type3.label;
    }
    //NOP
    else if((unsigned int) op_code == 10){
        instruction.type1.op_code = (mem_word) op_code;
        instruction.type = 1;
        decoded.instruction = instruction;
    }

    switch((unsigned int) op_code){
        case 0:
            // ADDI Hazard where the source register is being written this clock cycle
            if(decoded.instruction.type1.r_src == (*mem_new).reg_dest){
                decoded.op_a = (*mem_new).ALU_out;
                // LB reads from memory so we have to use MDR instead of ALU_OUT
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            break; 
        case 2:
            // BEQZ Hazard where the target is to be written this clock cycle
            if(decoded.instruction.type2.r_target == (*mem_new).reg_dest){
                decoded.op_a = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            // Hazard where the target was calculated last clock cycle cycle in exe
            if(decoded.instruction.type2.r_target == (*exe_new).r_dest){
                decoded.op_a = (*exe_new).ALU_out;
            }
            break; 
        case 3:
            // BGE Hazard where the src or dest is to be written this clock cycle
            if(decoded.instruction.type1.r_dest == (*mem_new).reg_dest){
                decoded.op_a = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            if(decoded.instruction.type1.r_src == (*mem_new).reg_dest){
                decoded.op_b = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            // Hazard where the src or dest was calculated last clock cycle in exe
            if(decoded.instruction.type1.r_dest == (*exe_new).r_dest){
                decoded.op_a = (*exe_new).ALU_out;
            }
            if(decoded.instruction.type1.r_src == (*exe_new).r_dest){
                decoded.op_b = (*exe_new).ALU_out;
            }
            break; 
        case 4:
            // BNE Hazard where the src or dest is to be written this clock cycle
            if(decoded.instruction.type1.r_dest == (*mem_new).reg_dest){
                decoded.op_a = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            if(decoded.instruction.type1.r_src == (*mem_new).reg_dest){
                decoded.op_b = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_b = (*mem_new).MDR;
                }
            }
            // Hazard where the src or dest was calcuated last clock cycle in exe
            if(decoded.instruction.type1.r_dest == (*exe_new).r_dest){
                decoded.op_a = (*exe_new).ALU_out;
            }
            if(decoded.instruction.type1.r_src == (*exe_new).r_dest){
                decoded.op_b = (*exe_new).ALU_out;
            }
            break; 
        case 8:
            // SUB source register is to be written this clock cycle
            if(decoded.instruction.type1.r_src == (*mem_new).reg_dest){
                decoded.op_a = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            break; 
        case 11:
            // ADD source or destination register is to be written this clock cycle
            if(decoded.instruction.type1.r_src == (*mem_new).reg_dest){
                decoded.op_a = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_a = (*mem_new).MDR;
                }
            }
            if(decoded.instruction.type1.label == (*mem_new).reg_dest){
                decoded.op_b = (*mem_new).ALU_out;
                if((*mem_new).op_code == 6){
                    decoded.op_b = (*mem_new).MDR;
                }
            }
            break;
        case 6:
            // LB the source register is to be written to this clock cycle
            if(decoded.instruction.type1.r_src == (*mem_new).reg_dest){
                // Grab the value out of that will be loaded into that register
                decoded.op_a = (*mem_new).ALU_out;
            }
            break;
        /*case 0:
            break; 
        case 0:
            break; 
        case 0:
            break; 
        case 0:
            break; 
        case 0:
            break; 
*/
        }
    
    /*
        Branches will start here.
        We have to update branches in the Decode stage.
    */
    // Unconditional Branch
    if(op_code == 1){
        *pc = TEXT_START + decoded.op_a;
        *pc += 1;
    }
    // Other Types of Branches
    else if((op_code == 2) || (op_code == 3) || (op_code == 4)){
        // BEQZ is a type 2 so we have to keep separate
        if(op_code == 2){
            // Only requires one operand
            if(alu(decoded.op_a, 0, instruction.type2.op_code) == 2){
                *pc = TEXT_START + instruction.type2.label;
                *pc += 1;
            }
        }
        // BGE and BNE are both type 1 instructions
        if((op_code == 3) || (op_code == 4)){
            int branch = alu(decoded.op_a, decoded.op_b, instruction.type1.op_code);
            if((branch == 4) || (branch == 3)){
                *pc = TEXT_START + instruction.type1.label;
                *pc += 1;
            }
        }
    }
    // Return the id_exe latch
    return decoded;
}
struct exe_mem _exe(struct id_exe to_execute, struct exe_mem *old, struct mem_wb *new){
        unsigned int op_code;
        struct exe_mem exe_latch;
        int type = to_execute.instruction.type;
        if(type == 1) op_code = to_execute.instruction.type1.op_code;
        if(type == 2) op_code = to_execute.instruction.type2.op_code;
        if(type == 3) op_code = to_execute.instruction.type3.op_code;
        exe_latch.op_code = op_code;
        if((unsigned int) op_code == 4 || (unsigned int) op_code == 2 || (unsigned int) op_code == 1 
                    || (unsigned int) op_code == 3 || (unsigned int) op_code == 10){
            exe_latch.r_dest = -1;
        }
        printf("Op_code: %lu\n", exe_latch.op_code);
        // AddI and SUBI MEM HAzArds
        if((exe_latch.op_code == 0) || (unsigned int)exe_latch.op_code == 8){
            if(to_execute.instruction.type1.r_src == (*new).reg_dest){
                to_execute.op_a = (*new).ALU_out;
                if((*new).op_code == 6){
                    to_execute.op_a = (*new).MDR;
                }
            }
        }
        // ADD MEM HAZARD
        if((unsigned int)exe_latch.op_code == 11){
            //printf("MEM HAZARDS\n");
            if(to_execute.instruction.type1.r_src == (*new).reg_dest){
                to_execute.op_a = (*new).ALU_out;
                if((*new).op_code == 6){
                    to_execute.op_a = (*new).MDR;
                }
            }
            if(to_execute.instruction.type1.label == (*new).reg_dest){
                to_execute.op_b= (*new).ALU_out;
                if((*new).op_code == 6){
                    to_execute.op_b = (*new).MDR;
                }
            }
            /*if(REGISTER_FILE[to_execute.instruction.type1.label] != to_execute.op_b){
                to_execute.op_b = REGISTER_FILE[to_execute.instruction.type1.label];
            }*/
        }
        // LB MEM HAZARD
        if(exe_latch.op_code == 6){
            if(to_execute.instruction.type1.r_src == (*new).reg_dest){
                to_execute.op_a = (*new).ALU_out;
            }
            if(to_execute.instruction.type1.r_src == (*old).r_dest){
                to_execute.op_a = (*old).ALU_out;
            }

        }
        // ADDI and SUBI EXE HAZARDS
        if((exe_latch.op_code == 0) || (unsigned int)exe_latch.op_code == 8){
            if(to_execute.instruction.type1.r_src == (*old).r_dest){
                to_execute.op_a = (*old).ALU_out;
            }
        }
        // ADD EXE HAZARD
        if((unsigned int)exe_latch.op_code == 11){
            //printf("EXE HAZARDS\n");
            if(to_execute.instruction.type1.r_src == (*old).r_dest){
                to_execute.op_a = (*old).ALU_out;
            }
            if(to_execute.instruction.type1.label == (*old).r_dest){
                to_execute.op_b= (*old).ALU_out;
            }
        }
    /* Branches already done might or might not need this
        if((to_execute.op_code == 3) || (unsigned int)to_execute.op_code == 4){
            if(to_execute.instruction.type2.r_src == *new.reg_dest){
                to_execute.op_b = *new.ALU_out;
            }
            if(to_execute.instruction.type1.r_dest == *new.reg_dest){
                to_execute.op_a = *new.ALU_out;
            }
        }
*/
        switch((unsigned int)op_code){
            case 0: // ADDI
                exe_latch.ALU_out = alu(to_execute.op_a, to_execute.op_b, exe_latch.op_code);
                //Destination foward
                exe_latch.r_dest = to_execute.instruction.type1.r_dest;
                printf("ALU: %d, DEST: %d\n", exe_latch.ALU_out, exe_latch.r_dest);
                break;
            case 5: // LA
                // Destination
                exe_latch.r_dest = to_execute.instruction.type2.r_target;
                //Offset
                exe_latch.ALU_out = to_execute.instruction.type2.label + DATA_START;
                printf("ALU: %d, DEST: %d\n", exe_latch.ALU_out - DATA_START, exe_latch.r_dest);
                break;
            case 6: // LB
                // Offset for target memory address
                exe_latch.ALU_out = to_execute.op_a + to_execute.op_b;
                // Destionation register
                exe_latch.r_dest = to_execute.instruction.type1.r_dest;
                printf("ALU: %d, DEST: %d\n", exe_latch.ALU_out, exe_latch.r_dest);
                break;
            case 7: // LI
                exe_latch.ALU_out = to_execute.instruction.type2.label;
                exe_latch.r_dest = to_execute.instruction.type2.r_target;
                printf("ALU: %d, DEST: %d\n", exe_latch.ALU_out, exe_latch.r_dest);
                break;
            case 8: // SUBI
                exe_latch.ALU_out = alu(to_execute.op_a, to_execute.op_b, exe_latch.op_code);
                //Destination foward
                exe_latch.r_dest = to_execute.instruction.type1.r_dest;
                printf("ALU: %d, DEST: %d\n", exe_latch.ALU_out, exe_latch.r_dest);
                break;
            case 9: // SYSCALL
                printf("SYSCALL NUM: %d\n", to_execute.op_a);
                syscall(to_execute.op_a);
                break;
            case 11:
                exe_latch.ALU_out = alu(to_execute.op_a, to_execute.op_b, 0);
                exe_latch.r_dest = to_execute.instruction.type1.r_dest;
                printf("IN EXE__________--ALU: %d, DEST: %d\n", exe_latch.ALU_out, exe_latch.r_dest);
                break;
        }
        return exe_latch;
}

struct mem_wb _mem(struct exe_mem to_access){
    struct mem_wb to_write_back;
    to_write_back.op_code = to_access.op_code;
    to_write_back.ALU_out = to_access.ALU_out;
    to_write_back.reg_dest = to_access.r_dest;
    if((unsigned int) to_access.op_code == 6){
        to_write_back.MDR = read_mem(to_write_back.ALU_out);
    }
    return to_write_back;
}

void  _wb(struct mem_wb to_write){
        // write backs
      switch((unsigned int) to_write.op_code){
        case 0:
            REGISTER_FILE[to_write.reg_dest] = to_write.ALU_out;
            break;
        case 5:
            REGISTER_FILE[to_write.reg_dest] = to_write.ALU_out;
            break;
        case 6:
            REGISTER_FILE[to_write.reg_dest] = to_write.MDR;
            break;
        case 7:
            REGISTER_FILE[to_write.reg_dest] = to_write.ALU_out;
            break;
        case 8:
            REGISTER_FILE[to_write.reg_dest] = to_write.ALU_out;
            break;
        case 11:
            REGISTER_FILE[to_write.reg_dest] = to_write.ALU_out;
            break;
    }
}

/*Description: Does standard alu calculations.
  Params: struct instruction_container - instruction, int op_code - alu function code
  Returns: returns the value of op code if correctly completes.
*/
int alu(int op_a, int op_b, int op_code){
	// Addi
    if(op_code == 0){
        printf("Op a: %d, Op b: %d\n", op_a, op_b);
        return (op_a + op_b);
    }
    // Check if equals 0
    if(op_code == 2){
        if(op_a == 0){
            return 2;
        }
        else return -1;
    }
    // Check if greater than or equal
    if(op_code == 3){
        if(op_a >= op_b){
            return 3;
        }
        else return -1;
    }
    // Check for inequality
    if(op_code == 4){
        if(op_a != op_b){
            return 4;
        }
        else return -1;
    }
    // Sub
    if(op_code == 8){
        printf("Op A: %d\n", op_a);
        return (op_a - op_b);
    }
    return -1;
    
}

/*Description: Completes the corresponding SYSCALL.
  Params: mem_word function - the system function
  Returns: instruction - opcode number or address for an instruction
*/
int syscall(mem_word function){
    // Read a string in
    if(function == 0){
        int index  = get_index((REGISTER_FILE[0] - DATA_START));
        char buff[string_lengths[index]];     
        scanf("%s", buff);
        memcpy((data_segment + string_addresses[index]), buff, strlen(buff)+ 1);
        data_segment[strlen(buff)] = '\0';
    }
    // Print a string
    if(function == 1){
        int index  = (REGISTER_FILE[0] - DATA_START);
        printf("%s\n", data_segment + index);
    }
    return 0;
    // Exit code
    if(function == 9){
        return 9;
    }
    
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
