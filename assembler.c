#include "vm.h"

typedef struct {
    const char *mnemonic;
    Opcode opcode;
    int operand_count;
} InstructionDef;

InstructionDef instructions[] = {
    {"HALT",  	OP_HALT,  	0},
    {"PUSH",  	OP_PUSH,  	1},
    {"POP",   	OP_POP,   	0},
    {"ADD",   	OP_ADD,   	0},
    {"SUB",   	OP_SUB,   	0},
    {"MUL",   	OP_MUL,   	0},
    {"DIV",   	OP_DIV,   	0},
    {"MOD",   	OP_MOD,   	0},
    {"AND",   	OP_AND,   	0},
    {"OR",    	OP_OR,    	0},
    {"XOR",   	OP_XOR,   	0},
    {"SHL",   	OP_SHL,   	0},
    {"SHR",   	OP_SHR,   	0},
    {"PRN",   	OP_PRN,   	0},
    {"JMP",   	OP_JMP,   	1},
    {"JZ",    	OP_JZ,    	1},
    {"JNZ",   	OP_JNZ,   	1},
    {"LOAD",  	OP_LOAD,  	1}, 
    {"STORE", 	OP_STORE, 	1}, 
    {"CALL",  	OP_CALL,  	1},
    {"RET",   	OP_RET,   	0},
	{"PEEK",	OP_PEEK,	0},
	{"POKE",	OP_POKE,	0},
	{"SYS",		OP_SYS,		1},
	{"STR",		OP_STR,		1},
	{"CPEEK",	OP_CPEEK,	0},
	{"CPOKE",	OP_CPOKE,	0},
	{"DUP",     OP_DUP,     0},
    {"SWAP",    OP_SWAP,    0},
    {"EQ",      OP_EQ,      0},
    {"LT",      OP_LT,      0},
    {"GT",      OP_GT,      0},
    {"ENTER",   OP_ENTER,   1},
    {"LEAVE",   OP_LEAVE,   0},
    {"LDFP",    OP_LDFP,    1},
    {"STFP",    OP_STFP,    1},
	{"FADD",	OP_FADD,	0},
	{"FSUB",	OP_FSUB,	0},
	{"FMUL",	OP_FMUL,	0},
	{"FDIV",	OP_FDIV,	0},
	{"ITOF",	OP_ITOF,	0},
	{"FTOI",	OP_FTOI,	0},
	{"FEQ",     OP_FEQ,     0},
    {"FLT",     OP_FLT,     0},
    {"FGT",     OP_FGT,     0},
    {"NOT",     OP_NOT,     0},
    {"BCOPY",   OP_BCOPY,   0},
    {"BRK",     OP_BRK,     0},
	{"LDI",		OP_LDI,		2},
	{"MOVR",	OP_MOVR,	2},
	{"INCR",	OP_INCR,	1},
	{"DECR",	OP_DECR,	1},
    {NULL,    	0,			0}
};

Token* tokenize(const char *filename, int *token_count_out) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    int capacity = 1024;
    Token *tokens = malloc(capacity * sizeof(Token));
    if (!tokens) { fclose(f); return NULL; }

    int count = 0;
    int current_line = 1;
    char c;

    while ((c = fgetc(f)) != EOF) {
        // Expand token array if we are nearing the limit
        if (count >= capacity - 2) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(Token));
        }

        if (c == '\n') { current_line++; continue; }
        if (isspace(c)) continue;

        if (c == ';') { 
            while ((c = fgetc(f)) != '\n' && c != EOF);
            if (c == '\n') current_line++;
            continue;
        }

		// 1. Character Literal Detection
        if (c == '\'') {
            c = fgetc(f);
            int val = c;
            if (c == '\\') { // Handle basic escapes
                c = fgetc(f);
                if (c == 'n') val = '\n';
                else if (c == '0') val = '\0';
            }
            c = fgetc(f); // Consume closing quote
            tokens[count].type = TOKEN_NUMBER;
            tokens[count].value = val;
            tokens[count].line = current_line;
            count++;
            continue;
        }

        // 2. String Literal Detection
        if (c == '"') { 
            int i = 0;
            tokens[count].line = current_line;
            c = fgetc(f);
            while (c != '"' && c != EOF && i < 254) {
                if (c == '\\') { 
                    c = fgetc(f);
                    if (c == EOF) break; // Prevent hanging on EOF
                    if (c == 'n') c = '\n';
                    else if (c == 't') c = '\t';
                    else if (c == '"') c = '"'; // Allow escaped quotes
                }
                tokens[count].text[i++] = c;
                c = fgetc(f);
            }
            
            // --- Safely discard excess characters without crashing ---
            if (c != '"' && c != EOF) {
                while(c != '"' && c != EOF) c = fgetc(f);
                fprintf(stderr, "Warning: String truncated on line %d (exceeded 254 chars)\n", current_line);
            }
            
            tokens[count].text[i] = '\0';
            tokens[count].type = TOKEN_STRING;
            count++;
            continue;
        }

        // --- RESTORED & UPGRADED: Identifier & Keyword Detection ---
        if (isalpha(c) || c == '_' || c == '.') {
            int i = 0;
            tokens[count].line = current_line;
            do {
                tokens[count].text[i++] = toupper(c);
                c = fgetc(f);
            } while ((isalnum(c) || c == '_' || c == '.') && c != EOF && i < 63);
            tokens[count].text[i] = '\0';

            if (c == ':') {
                tokens[count].type = TOKEN_LABEL_DEF;
            } else {
                // USABILITY FEATURE: Native Register Aliases
                if		(strcmp(tokens[count].text, "R0") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 0; }
                else if (strcmp(tokens[count].text, "R1") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 1; }
                else if (strcmp(tokens[count].text, "R2") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 2; }
                else if (strcmp(tokens[count].text, "R3") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 3; }
				else if (strcmp(tokens[count].text, "R4") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 4; }
				else if (strcmp(tokens[count].text, "R5") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 5; }
				else if (strcmp(tokens[count].text, "R6") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 6; }
				else if (strcmp(tokens[count].text, "R7") == 0) { tokens[count].type = TOKEN_NUMBER; tokens[count].value = 7; }
                else tokens[count].type = TOKEN_IDENTIFIER;
                
                if (c != EOF) ungetc(c, f);
            }
            count++;
            continue;
        } 
        // --- END OF RESTORED BLOCK ---

        if (isdigit(c) || c == '-') {
            int sign = 1;
            tokens[count].line = current_line;
            if (c == '-') { sign = -1; c = fgetc(f); }
            
            // Check for Hex (0x...)
            if (c == '0') {
                char next_c = fgetc(f);
                if (next_c == 'x' || next_c == 'X') {
                    int val = 0;
                    c = fgetc(f);
                    while (isxdigit(c) && c != EOF) {
                        val = val * 16 + (isdigit(c) ? (c - '0') : (toupper(c) - 'A' + 10));
                        c = fgetc(f);
                    }
                    tokens[count].type = TOKEN_NUMBER;
                    tokens[count].value = val * sign;
                    count++;
                    if (c != EOF) ungetc(c, f);
                    continue;
                } else {
                    ungetc(next_c, f); // Put back if not hex
                }
            }
            
            // Standard Decimal fallback
            int val = 0;
            while (isdigit(c) && c != EOF) {
                val = val * 10 + (c - '0');
                c = fgetc(f);
            }
            tokens[count].type = TOKEN_NUMBER;
            tokens[count].value = val * sign;
            count++;
            if (c != EOF) ungetc(c, f);
        } else {
			fprintf(stderr, "Lexical error on line '%d': Unrecognized Character '%c'\n", current_line, c);
			fclose(f);
			free(tokens);
			return NULL;
		}

    }
    fclose(f);
    tokens[count].type = TOKEN_EOF;
    tokens[count].line = current_line;
    *token_count_out = count;
    return tokens;
}

Token* process_includes(Token *tokens, int *count) {
    int i = 0;
    int include_limit = 100; // Safeguard against runaway include loops (.INCLUDE <codefile>)
    
    while (i < *count) {
        if (tokens[i].type == TOKEN_IDENTIFIER && strcmp(tokens[i].text, ".INCLUDE") == 0) {
            
            if (i + 1 >= *count || tokens[i+1].type != TOKEN_STRING) {
                fprintf(stderr, "Error: .INCLUDE requires a string filename (e.g., .INCLUDE \"math.asm\").\n");
                free(tokens);
                return NULL;
            }
            
            if (--include_limit < 0) {
                fprintf(stderr, "Error: Maximum include limit (100) reached. Check for infinite loops.\n");
                free(tokens);
                return NULL;
            }
            
            const char *inc_filename = tokens[i+1].text;
            int inc_count = 0;
            Token *inc_tokens = tokenize(inc_filename, &inc_count);
            
            if (!inc_tokens) {
                fprintf(stderr, "Error: Failed to open or tokenize included file '%s'\n", inc_filename);
                free(tokens);
                return NULL;
            }
            
            // We are replacing 2 tokens (.INCLUDE and "filename") with inc_count tokens
            int new_count = *count - 2 + inc_count;
            
            // +1 to ensure space for the TOKEN_EOF at the very end
            Token *new_tokens = realloc(tokens, (new_count + 1) * sizeof(Token));
            if (!new_tokens) {
                fprintf(stderr, "Error: Out of memory during include parsing.\n");
                free(inc_tokens);
                free(tokens);
                return NULL;
            }
            tokens = new_tokens;
            
            // Shift the remaining tokens (and the EOF token) to the right
            int remainder = *count - (i + 2) + 1; 
            memmove(&tokens[i + inc_count], &tokens[i + 2], remainder * sizeof(Token));
            
            // Splice in the included tokens (omitting the included file's EOF token)
            memcpy(&tokens[i], inc_tokens, inc_count * sizeof(Token));
            
            free(inc_tokens);
            *count = new_count;
            
            // Note: We do NOT increment 'i' here. 
            // This allows the loop to immediately process the newly spliced tokens,
            // meaning an included file can contain its own .INCLUDE directives!
            
        } else {
            i++;
        }
    }
    return tokens;
}

int assemble(Token *tokens, VM *vm, int *data_size_out) {
    vm->symbol_count = 0; // Initialize
    int pc = 0, dc = 0;
    int in_data = 0; 

    // --- PASS 1: Build the Symbol Table ---
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type == TOKEN_IDENTIFIER) {
            if (strcmp(tokens[i].text, ".DATA") == 0) { in_data = 1; continue; }
            if (strcmp(tokens[i].text, ".CODE") == 0) { in_data = 0; continue; }
        }

        if (tokens[i].type == TOKEN_LABEL_DEF) {
			if (vm->symbol_count >= MAX_LABELS) {
				fprintf(stderr, "ERROR -- assemble(): MAX num of labels (%d) exceeded.\n", MAX_LABELS);
				return -1;
			}
            strcpy(vm->symbols[vm->symbol_count].name, tokens[i].text);
            vm->symbols[vm->symbol_count].address = in_data ? dc : pc; 
            vm->symbols[vm->symbol_count].is_data = in_data;
            vm->symbol_count++;
        } else if (in_data) {
            if (tokens[i].type == TOKEN_NUMBER) dc++;
            else if (tokens[i].type == TOKEN_STRING) dc += strlen(tokens[i].text) + 2;
            else if (tokens[i].type == TOKEN_IDENTIFIER && strcmp(tokens[i].text, ".RES") == 0) {
                dc += tokens[++i].value; // Advance Data Counter by requested size
            }
        } else if (tokens[i].type == TOKEN_IDENTIFIER) {
            for (int j = 0; instructions[j].mnemonic != NULL; j++) {
                if (strcmp(tokens[i].text, instructions[j].mnemonic) == 0) {
                    if (instructions[j].opcode == OP_STR) {
                        pc++; i++;
                        if (tokens[i].type == TOKEN_STRING) pc += 1 + strlen(tokens[i].text) + 1; 
                    } else {
                        pc++; // count opcode
                        if (instructions[j].operand_count > 0) {
							pc += instructions[j].operand_count;
							i += instructions[j].operand_count;
						}
                    }
                    break;
                }
            }
        }
    }

    // --- PASS 2: Code & Data Emission ---
    pc = 0; dc = 0; in_data = 0;
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type == TOKEN_LABEL_DEF) continue;

        if (tokens[i].type == TOKEN_IDENTIFIER && strcmp(tokens[i].text, ".DATA") == 0) { in_data = 1; continue; }
        if (tokens[i].type == TOKEN_IDENTIFIER && strcmp(tokens[i].text, ".CODE") == 0) { in_data = 0; continue; }

        if (in_data) {
            if (tokens[i].type == TOKEN_NUMBER) {
                vm->data[dc++] = tokens[i].value;
            } else if (tokens[i].type == TOKEN_STRING) {
                int len = strlen(tokens[i].text);
                vm->data[dc++] = len; 
                for (int s = 0; s <= len; s++) vm->data[dc++] = tokens[i].text[s]; 
            } else if (tokens[i].type == TOKEN_IDENTIFIER && strcmp(tokens[i].text, ".RES") == 0) {
                int res_size = tokens[++i].value;
                for (int r = 0; r < res_size; r++) vm->data[dc++] = 0; // Fill with zeroes
            }
            continue;
        }

        if (tokens[i].type == TOKEN_IDENTIFIER) {
			if (pc >= MEMORY_SIZE - 256) {
				fprintf(stderr, "ERROR -- assemble(): Code Segment (ROM) limit exceeded.\n");
				return -1;
			}
            int found = 0;
            for (int j = 0; instructions[j].mnemonic != NULL; j++) {
                if (strcmp(tokens[i].text, instructions[j].mnemonic) == 0) {
                    if (instructions[j].opcode == OP_STR) {
                        vm->code[pc++] = OP_STR;
                        i++; 
                        int len = strlen(tokens[i].text);
                        vm->code[pc++] = len;
                        for (int s = 0; s <= len; s++) vm->code[pc++] = tokens[i].text[s];
                        found = 1; break;
                    }

                    vm->code[pc++] = instructions[j].opcode;
                    found = 1;

					if (instructions[j].operand_count > 0) {
						for (int op = 0; op < instructions[j].operand_count; op++) {
							if(tokens[i+1].type == TOKEN_EOF) {
								fprintf(stderr, "Error: Missing operands for instruction %s\n", instructions[j].mnemonic);
								return -1;
							}
							i++;
							if (tokens[i].type == TOKEN_NUMBER) {
								vm->code[pc++] = tokens[i].value;
							} else if (tokens[i].type == TOKEN_IDENTIFIER) {
								int resolved = -1;
								for (int l = 0; l < vm->symbol_count; l++) {
									if (strcmp(tokens[i].text, vm->symbols[l].name) == 0) {
										resolved = vm->symbols[l].address; break;
									}
								}
								if (resolved != -1) vm->code[pc++] = resolved;
								else { fprintf(stderr, "Unresolved label: %s\n", tokens[i].text); return -1; }
							}
						}
					}
                }
            }
            if (!found) { fprintf(stderr, "Unknown instruction: %s\n", tokens[i].text); return -1; }
        }
    }
    
    vm->code[pc] = OP_HALT;
    *data_size_out = dc;
    return pc + 1; 
}

void disassemble(VM *vm, int code_size) {
    printf("--- DISASSEMBLY ---\n");
    int pc = 0;
    while (pc < code_size) {
        int instr = vm->code[pc];
        printf("%04d: ", pc);
        pc++;
        int found = 0;
        for (int i = 0; instructions[i].mnemonic != NULL; i++) {
            if (instructions[i].opcode == instr) {
                printf("%s", instructions[i].mnemonic);
                
                if (instructions[i].operand_count > 0) {
                    for (int op = 0; op < instructions[i].operand_count; op++) {
                        int operand = vm->code[pc++];
                        const char* matched_symbol = NULL;
                        int is_data_symbol = 0;
                        
                        // Check if this instruction usually takes a label
                        int can_have_label = (instr == OP_PUSH || instr == OP_JMP || 
                                              instr == OP_JZ || instr == OP_JNZ || 
                                              instr == OP_CALL);
                        
                        if (can_have_label) {
                            for (int l = 0; l < vm->symbol_count; l++) {
                                if (vm->symbols[l].address == operand) {
                                    matched_symbol = vm->symbols[l].name;
                                    is_data_symbol = vm->symbols[l].is_data; 
                                    break;
                                }
                            }
                        }
                        
                        // Print the operand
                        if (matched_symbol && is_data_symbol) {
                            printf(" [%d]\t\t; global address of '%s'", operand, matched_symbol);
                        } else if (matched_symbol) {
                            printf(" <%d>\t\t; label '%s'", operand, matched_symbol);
                        } else {
                            printf(" %d", operand);
                        }
                    }
                } else if (instr == OP_STR) {
                    int len = vm->code[pc++];
                    printf(" \"");
                    for(int j = 0; j < len; j++) printf("%c", vm->code[pc++]);
                    printf("\"");
                    pc++; 
                }
                found = 1; break;
            }
        }
        if (!found) printf("UNKNOWN (%d)", instr);
        printf("\n");
    }
}