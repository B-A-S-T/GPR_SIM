/*
  Group 1
  Created September 2016 by Ian Thomas with the help of Lanxin Ma.

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
#define NUM_LABELS 50
#define LABEL_SIZE 50

/*Global Variables:  */
instruction* text_segment;
//mem_word *data_segment;
char *data_segment;
mem_word *kernal_segment;
mem_addr data_limit;
char labels[NUM_LABELS][LABEL_SIZE];
int label_addresses[NUM_LABELS];
int last_label = 0;
int last_string = 0;
int top_string_mem = 1500;
char string_allocation[NUM_STRINGS][STRING_SIZE];
int string_addresses[NUM_STRINGS];
int string_lengths[NUM_STRINGS];
mem_word REGISTER_FILE[NUM_REGISTERS];

struct instr_type1{
    unsigned int op_code;
    mem_word r_dest;
    mem_word r_src;
    mem_word label;
};
struct instr_type2{
    mem_word op_code;
    mem_word r_target;
    mem_word label;
};
struct instr_type3{
    unsigned int op_code;
    mem_word label;
};

struct instruction_container{
    struct instr_type1 type1;
    struct instr_type2 type2;
    struct instr_type3 type3;
    int type;
};


/* Prototypes */
void make_memory();
void parse_source_code(char *filename);
void load_data(char* token);
void load_text(char* token, int *index);
int get_opCode(char* instr);
char read_mem(mem_addr address);
instruction read_inst(mem_addr address);
void write_instr(mem_addr address, instruction instr);
void write_mem(mem_addr address, char *value);
void add(mem_word* accum, mem_addr operand);
void mult(mem_word* accum, mem_addr operand);
instruction type1_create(struct instr_type1 instr);
instruction type2_create(struct instr_type2 instr);
instruction type3_create(struct instr_type3 instr);
int find_label(char* to_search);
struct instruction_container decode(int to_decode);
int alu(struct instruction_container instr, int op_code);
int check_allocated(char* match);
int syscall(mem_word function);
int get_index(int match);

/*Main entry into simulator, one argument should be passed, the filename.*/
int main(int argc, char *argv[]){

    struct instruction_container instruc;
    int call = 0;
    make_memory();
    parse_source_code(argv[1]);
    instruction instr;
    int usermode = 1;
    int computes;
    mem_addr pc = TEXT_START;
    int type;
    float C = 0;
    float IC = 0;
    unsigned int op_code;

    while(usermode){
        instr = read_inst(pc);
        instruc = decode(instr);
        type = instruc.type;
        if(type == 1) op_code = instruc.type1.op_code;
        if(type == 2) op_code = instruc.type2.op_code;
        if(type == 3) op_code = instruc.type3.op_code;
        pc += 1;
        switch((unsigned int)op_code){
            case 0:
                alu(instruc, instruc.type1.op_code);
                C += 6;
                break;
            case 1:
                pc = TEXT_START + instruc.type3.label;
                pc += 1;
                C += 4;
                break;
            case 2:
                if(alu(instruc, instruc.type2.op_code) == 2){
                    pc = TEXT_START + instruc.type2.label;
                    pc += 1;
                    C += 5;
                }
                break;
            case 3:
                if(alu(instruc, instruc.type1.op_code) == 3){
                    pc = TEXT_START + instruc.type1.label;
                    pc += 1;
                    C += 5;
                }
                break;
            case 4:
                if(alu(instruc, instruc.type1.op_code) == 4){
                    pc = TEXT_START + instruc.type1.label;
                    pc += 1;
                    C += 5;
                }
                break;
            case 5:
                REGISTER_FILE[instruc.type2.r_target] = DATA_START + instruc.type2.label;
                C += 5;
                break;
            case 6:
                REGISTER_FILE[instruc.type1.r_dest] = 
                            read_mem((REGISTER_FILE[instruc.type1.r_src] 
                            + instruc.type1.label));
                C += 6;
                break;
            case 7:
                REGISTER_FILE[instruc.type2.r_target] = instruc.type2.label;
                C += 3;
                break;
            case 8:
                alu(instruc, instruc.type1.op_code);
                C += 6;
                break;
            case 9:
                if(instruc.type3.label == 9){
                usermode = 0; 
                }
                C += 8;
                syscall(instruc.type3.label);
                break;
        }
        IC += 1;
    }
    printf("IC = %f\n C = %f\n Ratio = %f\n", IC, C, (8*IC)/C);
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
    int *text_index = 0;
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
    else if(strcmp(address, ".label") == 0){
      label = strtok(NULL, " \t");
      strcpy(labels[last_label], label);
      address = strtok(NULL, " \t");
      label_addresses[last_label] = ((mem_addr) strtol(address, (char **)NULL, 16)) - TEXT_START;
      last_label += 1;
    }
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
    
    if((op_code == 0) || (op_code == 3) || (op_code == 4) || ((unsigned int)op_code == 8)){
        struct instr_type1 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", ");
        instruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        instruction.r_src = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        if((op_code == 3) || (op_code == 4)){
            label_index = find_label(instr);
            instruction.label = label_index;
        }
        else{
            instruction.label = (mem_word) strtol(instr, (char **)NULL, 10);
        }
        new_instruction = type1_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }

    if((op_code == 2) || (op_code == 5) || (op_code == 7)){
        struct instr_type2 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", ");
        instruction.r_target = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        if(op_code == 2){
            printf("Branching: %s\n", instr);
            label_index = find_label(instr);
            printf("Branching: %d\n", label_index);
            instruction.label = (mem_word) label_index;
        }
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

    if((unsigned int)op_code == 9){
        struct instr_type3 instruction;
        instruction.op_code = op_code;
        instr = strtok(NULL, " \t");
        instruction.label = (mem_word) strtol(instr, (char **)NULL, 10);
        new_instruction = type3_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }

    if(op_code == 6){
        struct instr_type1 instruction;
        char reg[3];
        char offset[10];
        int reg_index;
        instruction.op_code = op_code;
        instr = strtok(NULL, ", ");
        instruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        instr = strtok(NULL, ", ");
        reg_index = strcspn(instr, "$");
        memcpy(reg, instr + reg_index + 1, strlen(instr) - reg_index -2);
        instruction.r_src = strtol(reg, (char **)NULL, 10);
        memcpy(offset, instr, reg_index - 1);
        offset[(reg_index-1)] = '\0';
        instruction.label = (mem_word) strtol(offset, (char **)NULL, 10);
        new_instruction = type1_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }

}
/* TYPE 1 instruction encoding: 4 bit OP code, 5 bit Rdest, 5 bit Rsrc, 18 bit Label */

instruction type1_create(struct instr_type1 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.r_dest << 23) | new_instruction;
    new_instruction = (instr.r_src << 18) | new_instruction;
    new_instruction = (instr.label & 0x001fffff) | new_instruction;
    return new_instruction;

}
/* TYPE 2 instruction encoding: 4 bit OP code, 5 bit Rsrc, 23bit Label */
instruction type2_create(struct instr_type2 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.r_target << 23) | new_instruction;
    new_instruction = (instr.label & 0x007fffff) | new_instruction;
    return new_instruction;
}

/* TYPE 3 instruction encoding: 4 bit OP code, 28 bit Label */
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
    if(strcmp(instr, "ADDI") == 0) return 0;
    else if (strcmp(instr, "B") == 0) return 1;
    else if (strcmp(instr, "BEQZ") == 0) return 2;
    else if (strcmp(instr, "BGE") == 0) return 3;
    else if (strcmp(instr, "BNE") == 0) return 4;
    else if (strcmp(instr, "LA") == 0) return 5;
    else if (strcmp(instr, "LB") == 0) return 6;
    else if (strcmp(instr, "LI") == 0) return 7;
    else if (strcmp(instr, "SUBI") == 0) return 8;
    else if (strcmp(instr, "SYSCALL") == 0) return 9;
    else return -1;
}
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
instruction read_inst(mem_addr address){
    if((address <  TEXT_TOP) && (address >= TEXT_START)){
        return text_segment[(address - TEXT_START)];
}
    else{
        printf("Read instruction fail:");
        return NULL;
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
      Params: address - address to read from, instr- data to write
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

struct instruction_container decode(instruction to_decode){
    struct instruction_container instruction;
    unsigned int op_code = (to_decode >> 28) & 0x0000000f;
    mem_word dest;
    mem_word src;
    mem_word label;

    if((op_code == 0) || (op_code == 3) 
            || (op_code == 4) || (op_code == 6) || ((unsigned int)op_code == 8)){
        instruction.type1.op_code = op_code;
        dest = (to_decode >> 23) & 0x0000001f;
        instruction.type1.r_dest = dest;
        src =  (to_decode >> 18) & 0x0000001f;
        instruction.type1.r_src = src;
        instruction.type1.label = (to_decode & 0x0003ffff);
        instruction.type = 1;
    }
    else if((op_code == 2) || (op_code ==5) || (op_code == 7)){
        instruction.type2.op_code = op_code;
        dest = (to_decode >> 23) & 0x0000001f;
        instruction.type2.r_target = dest;
        instruction.type2.label = (to_decode & 0x0007ffff);
        instruction.type = 2;
    }
    else if((op_code == 1) || (op_code == 9)){
        instruction.type3.op_code = (mem_word) op_code;
        instruction.type3.label = (mem_word) (to_decode & 0x0fffffff);
        instruction.type = 3;
    }
    return instruction;
}

int alu(struct instruction_container instr, int op_code){
    if(op_code == 0){
        REGISTER_FILE[instr.type1.r_dest] = REGISTER_FILE[instr.type1.r_src]
                                + instr.type1.label;
        return 0;
    }

    if(op_code == 2){
        if(REGISTER_FILE[instr.type2.r_target] == 0){
            return 2;
        }
        else return -1;
    }

    if(op_code == 3){
        if(REGISTER_FILE[instr.type1.r_dest] >= REGISTER_FILE[instr.type1.r_src]){
            return 3;
        }
        else return -1;
    }

    if(op_code == 4){
        if(REGISTER_FILE[instr.type1.r_dest] != REGISTER_FILE[instr.type1.r_src]){
            return 4;
        }
        else return -1;
    }

    if(op_code == 7){
        REGISTER_FILE[instr.type1.r_dest] = REGISTER_FILE[instr.type1.r_src]
                                - instr.type1.label;
        return 0;
    }
    if(op_code == 8){
        REGISTER_FILE[instr.type1.r_dest] = REGISTER_FILE[instr.type1.r_src]
                                - instr.type1.label;
        return 0;
    }
    return -1;
    
}

int syscall(mem_word function){
    
    if(function == 0){
        int index  = get_index((REGISTER_FILE[0] - DATA_START));
        char buff[string_lengths[index]];     
        scanf("%s", buff);
        memcpy((data_segment + string_addresses[index]), buff, strlen(buff)+ 1);
        data_segment[strlen(buff)] = '\0';
    }
    if(function == 1){
        int index  = (REGISTER_FILE[0] - DATA_START);
        printf("%s\n", data_segment + index);
    }
    return 0;
    if(function == 9){
        return 9;
    }
    
}
int check_allocated(char* match){
    int i;
    for(i = 0; i < last_string; i++){
        if(strcmp(string_allocation[i], match) == 0){
            return i;
        }
    }

    return -1;
}
int get_index(int match){
    int i;
    for(i = 0; i < last_string; i++){
        if(string_addresses[i] == match){
        return i;
        }
    }
    return -1;

}
