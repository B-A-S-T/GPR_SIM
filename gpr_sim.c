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
#define LABEL_SIZE 15

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
char strings[NUM_STRINGS][STRING_SIZE];
mem_word REGISTER_FILE[NUM_REGISTERS];

struct instr_type1{
    mem_word op_code;
    mem_word r_dest;
    mem_word r_src;
    mem_word _label;
};
struct instr_type2{
    mem_word op_code;
    mem_word r_target;
    mem_word label;
};
struct instr_type3{
    mem_word op_code;
    mem_word label;
};


/* Prototypes */
void make_memory();
void parse_source_code(char *filename);
void load_data(char* token);
void load_text(char* token, int *index);
instruction get_opCode(char* instr);
mem_word read_mem(mem_addr address);
instruction read_inst(mem_addr address);
void write_instr(mem_addr address, instruction instr);
void write_mem(mem_addr address, char *value);
void add(mem_word* accum, mem_addr operand);
void mult(mem_word* accum, mem_addr operand);
instruction type1_create(struct instr_type1 instr);
instruction type2_create(struct instr_type2 instr);
instruction type3_create(struct instr_type3 instr);

/*Main entry into simulator, one argument should be passed, the filename.*/
int main(int argc, char *argv[]){

    make_memory();
    parse_source_code(argv[1]);
    printf("FIRST CHAR: %c\n", data_segment[0]);
    printf("LABEL AT:  %d\n", label_addresses[0]);
/*
    instruction instr;
    int usermode = 1;
    mem_word to_load = 0;
    mem_addr to_store = 0;
    mem_addr to_add = 0;
    mem_addr to_mult = 0;
    mem_addr pc = TEXT_START;
    mem_addr to_print;

    while(usermode){
        instr = read_inst(pc);
        pc += 1;
        switch(instr){
            case 0:
                to_load = (mem_word) read_inst(pc);
                accumulator = read_mem(to_load);
                pc += 1;
                break;
            case 1:
                to_store = (mem_addr) read_inst(pc);
                write_mem(to_store, accumulator);
                pc += 1;
                break;
            case 2:
                to_add = (mem_addr) read_inst(pc);
                add(&accumulator, to_add);
                pc +=1;
                break;
            case 3:
                to_mult = (mem_addr) read_inst(pc);
                mult(&accumulator, to_mult);
                pc +=1;
                break;
            case 4:
                usermode = 0;
                printf("%d\n", accumulator);
                break;
            case 5:
                to_print = (mem_addr) read_inst(pc);
                if(to_print >= last_string || to_print < 0){
                    printf("Failed to print that string: Wrong Index");
                }
                printf("%s", strings[to_print]);
                pc += 1;
                break;
        }
    }
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

    data_segment = malloc(MAX_SEG_SIZE);
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
        printf("String: %s, Address: %s\n", value, address);
        write_mem(addr, value);
    }
    else if(strcmp(address, ".label") == 0){
      label = strtok(NULL, " \t");
      strcpy(labels[last_label], label);
      address = strtok(NULL, " \t");
      label_addresses[last_label] = ((mem_addr) strtol(address, (char **)NULL, 16)) - DATA_START;
      last_label += 1;
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
    mem_word op_code = get_opCode(instr);;
    instruction new_instruction;
    mem_addr address - TEXT_START + *index;
    
    if((op_code == 0) || (op_code == 3) || (op_code == 4) || (op_code = 8)){
        struct instr_type1 instruction;
        instruction.op_code = op_code;
        strtok(NULL, ", ");
        istruction.r_dest = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        strtok(NULL, ", ");
        istruction.r_src = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        strtok(NULL, ", ");
        istruction._label = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        new_instruction = type1_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
    if((op_code == 2) || (op_code == 5) || (op_code == 7)){
        struct instr_type2 instruction;
        instruction.op_code = op_code;
        strtok(NULL, ", ");
        istruction.target = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        strtok(NULL, ", ");
        istruction._label = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        new_instruction = type2_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
    if(op_code == 1){
        struct instr_type3 instruction;
        instruction.op_code = op_code;
        strtok(NULL, ", ");
        istruction._label = (mem_word) strtol(instr + 1, (char **)NULL, 10);
        new_instruction = type2_create(instruction);
        write_instr(address, new_instruction);
        *index += 1;
    }
    while(instr != NULL){
        op_code = get_opCode(instr);
        mem_addr address = TEXT_START + *index;
        write_instr(address, op_code);
        *index += 1;
        instr = strtok(NULL, " \t");
    }

}
/* TYPE 1 instruction encoding: 4 bit OP code, 5 bit Rdest, 5 bit Rsrc, 18 bit Label */

instruction type1_create(struct instr_type1 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.r_dest << 23) | new_instruction;
    new_instruction = (instr.r_src << 17) | new_instruction;
    new_instruction = (instr._label & 0x0003ffff) | new_instruction;
    return new_instruction;

}
/* TYPE 2 instruction encoding: 4 bit OP code, 5 bit Rsrc, 23bit Label */
instruction type2_create(struct instr_type2 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.r_src << 23) | new_instruction;
    new_instruction = (instr.label & 0x007FFFFF) | new_instruction;
    return new_instruction;
}

/* TYPE 3 instruction encoding: 4 bit OP code, 28 bit Label */
instruction type3_create(struct instr_type3 instr){
    instruction new_instruction = 0;
    new_instruction = (instr.op_code << 28) | new_instruction;
    new_instruction = (instr.label & 0x0FFFFFFF) | new_instruction;
    return new_instruction;
}

/*Description: Gets the op code for an instruction for easier switch statement.
  Params: *instr - instruction that was parsed from source file
  Returns: instruction - opcode number or address for an instruction
*/
instruction get_opCode(char *instr){
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
    else return (instruction) strtol(instr, (char **)NULL, 16);
}


/* For each of the reads and writes we first check to see if the address
    is less than its address top boundary and then the bottom.*/
/*Description: Read memory from the data_segment.
          Params: address - address to read from
          Returns: mem_word - returns the contents
*/
mem_word read_mem(mem_addr address){
    if((address <  DATA_TOP) && (address >= DATA_START))
        return data_segment[(address - DATA_START)];
    else{
        printf("We couldn't access that address check addresses avaliable\n");
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
        printf("We couldn't access that address check addresses avaliable\n");
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
        printf("We couldn't access that address check address avaliable\n");
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
        printf("We couldn't access that address check addresses avaliable\n");
        return NULL;
    }
}

/*Description: Adds value that is in the accumulator and a value from
                a memory location.
      Params: *accum- pointer to the accumulator, addr of the operand to add
      Returns: void
*/
void add(mem_word* accum, mem_addr operand){
    mem_word quotient = read_mem(operand);
    *accum = quotient + *accum;
}

/*Description: Multiplies value that is in the accumulator and a value from
                a memory location.
      Params: *accum- pointer to the accumulator, addr of the operand to multiply
      Returns: void
*/
void mult(mem_word* accum, mem_addr operand){

    mem_word product = read_mem(operand);
    *accum = product * (*accum);
}
