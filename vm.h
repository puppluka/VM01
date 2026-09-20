#ifndef VM_H
#define VM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MEMORY_SIZE 4096
#define STACK_SIZE 256
#define NUM_REGISTERS 8
#define MAX_LABELS 128

#define VM_MAGIC 0x31304D56		// 'VM01' in Little Endian

typedef enum {
    OP_HALT = 0,
	OP_PUSH,
	OP_POP,
	OP_ADD,
    OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
    OP_AND,
	OP_OR,
	OP_XOR,
	OP_SHL,
    OP_SHR,
	OP_PRN,
	OP_JMP,
	OP_JZ,  
    OP_JNZ,
	OP_LOAD,
	OP_STORE,
	OP_CALL,  
    OP_RET,
	OP_PEEK,
	OP_POKE,
	OP_SYS,
	OP_STR,
	OP_CPEEK,
	OP_CPOKE,
	OP_DUP,
	OP_SWAP,
	OP_EQ,
	OP_LT,
	OP_GT,
	OP_ENTER,
	OP_LEAVE,
	OP_LDFP,
	OP_STFP,
	OP_FADD,
	OP_FSUB,
	OP_FMUL,
	OP_FDIV,
	OP_ITOF,
	OP_FTOI,
	OP_FEQ,
	OP_FLT,
	OP_FGT,
	OP_NOT,
	OP_BCOPY,
	OP_BRK,
	OP_LDI,
	OP_MOVR,
	OP_INCR,
	OP_DECR,
} Opcode;

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER, 
    TOKEN_NUMBER,
    TOKEN_LABEL_DEF,
	TOKEN_STRING
} VMTokenType;

typedef struct {
    VMTokenType type;
    char text[256];
    int value;
    int line; 
} Token;

typedef struct {
    char name[64];
    int address;
    int is_data; // 1 for .DATA, 0 for .CODE
} Label;

// Update the VM struct
typedef struct {
    int code[MEMORY_SIZE];
    int stack[STACK_SIZE];
    int sp;     
    int call_stack[STACK_SIZE]; 
    int csp;                    
    int registers[NUM_REGISTERS]; 
    int pc;     
    int fp;         
    int running;
    int debug_mode; 
    
    int data[8192]; 
    int heap_ptr;   

    Label symbols[MAX_LABELS];
    int symbol_count;

	int exit_code;
	FILE *open_files[16]; // max open files at once
} VM;

Token* tokenize(const char *filename, int *token_count_out);
Token* process_includes(Token *tokens, int *count);
int assemble(Token *tokens, VM *vm, int *data_size_out);
void disassemble(VM *vm, int code_size);

void init_vm(VM *vm);
int run_vm(VM *vm);
int save_bytecode(VM *vm, int code_size, int data_size, const char *filename);
int load_bytecode(VM *vm, const char *filename, int *code_size_out);

#endif	// VM_H
