#include "vm.h"

#if defined(_WIN32) || defined(WIN32)
	#include <windows.h>
#else
	#include <unistd.h>
#endif

void init_vm(VM *vm) {
    memset(vm->code, 0, sizeof(vm->code));
    memset(vm->stack, 0, sizeof(vm->stack));
    memset(vm->call_stack, 0, sizeof(vm->call_stack));
    memset(vm->registers, 0, sizeof(vm->registers));
    vm->pc = 0;
    vm->sp = -1;
    vm->csp = -1; // Initialize Call Stack Pointer
    vm->running = 1;
    vm->debug_mode = 0;
	vm->fp = 0;   // Frame Pointer
	// Set heap start to midpoint of data array (4096-8191 is heap)
	vm->heap_ptr = 4096;

	vm->exit_code = 0;
	for(int i = 0; i < 16; i++)
		vm->open_files[i] = NULL;
}

int run_vm(VM *vm) {
    while (vm->running && vm->pc < MEMORY_SIZE) {
        
        // Updated Debugging state dump
        if (vm->debug_mode) {
            printf("--- [TRACE] PC: %04d | SP: %03d | CSP: %03d | ", vm->pc, vm->sp, vm->csp);
            printf("R0:%d R1:%d R2:%d R3:%d R4:%d R5:%d R6:%d R7:%d | ",
                   vm->registers[0], vm->registers[1], vm->registers[2], vm->registers[3],
				   vm->registers[4], vm->registers[5], vm->registers[6], vm->registers[7]);
            if (vm->sp >= 0) printf("TOS: %d ---\n", vm->stack[vm->sp]);
            else printf("TOS: NULL ---\n");
            
            // Wait for user input to step forward
            printf("Press Enter to step (or 'q' to quit)...");
            char cmd = getchar();
            if (cmd == 'q') { vm->running = 0; break; }
            if (cmd != '\n') while (getchar() != '\n'); // flush buffer
        }

        int instr = vm->code[vm->pc++];
        switch (instr) {
            case OP_HALT:
                vm->running = 0;
                break;
            case OP_PUSH:
                if (vm->sp < STACK_SIZE - 1) {
                    vm->stack[++vm->sp] = vm->code[vm->pc++];
                } else {
                    fprintf(stderr, "Error: Stack Overflow\n");
                    return -1;
                }
                break;
            case OP_POP:
                if (vm->sp >= 0) {
                    vm->sp--;
                } else {
                    fprintf(stderr, "Error: Stack Underflow\n");
                    return -1;
                }
                break;
            case OP_ADD:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a + b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for ADD\n");
                    return -1;
                }
                break;
            case OP_SUB:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a - b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for SUB\n");
                    return -1;
                }
                break;
            case OP_PRN:
                if (vm->sp >= 0) {
					if(vm->debug_mode)
						printf("OUT: %d\n", vm->stack[vm->sp]);
					else
						printf("%d\n", vm->stack[vm->sp]);
                } else {
                    fprintf(stderr, "Error: Stack empty on PRN\n");
                    return -1;
                }
                break;
            case OP_JMP: {
                int dest = vm->code[vm->pc];
                if (dest >= 0 && dest < MEMORY_SIZE) vm->pc = dest;
                else { fprintf(stderr, "Error: JMP out of bounds (%d)\n", dest); return -1; }
                break;
            }
            case OP_JZ:
                if (vm->sp >= 0) {
                    int val = vm->stack[vm->sp--];
                    if (val == 0) {
                        int dest = vm->code[vm->pc];
                        if (dest >= 0 && dest < MEMORY_SIZE) vm->pc = dest;
                        else { fprintf(stderr, "Error: JZ out of bounds (%d)\n", dest); return -1; }
                    } else vm->pc++; 
                } else {
                    fprintf(stderr, "Error: Stack Underflow on JZ\n"); return -1;
                }
                break;
            case OP_JNZ:
                if (vm->sp >= 0) {
                    int val = vm->stack[vm->sp--];
                    if (val != 0) {
                        int dest = vm->code[vm->pc];
                        if (dest >= 0 && dest < MEMORY_SIZE) vm->pc = dest;
                        else { fprintf(stderr, "Error: JNZ out of bounds (%d)\n", dest); return -1; }
                    } else vm->pc++; 
                } else {
                    fprintf(stderr, "Error: Stack Underflow on JNZ\n"); return -1;
                }
                break;
            case OP_CALL:
                if (vm->csp < STACK_SIZE - 1) {
                    int dest = vm->code[vm->pc];
                    if (dest >= 0 && dest < MEMORY_SIZE) {
                        vm->call_stack[++vm->csp] = vm->pc + 1; 
                        vm->pc = dest; 
                    } else { fprintf(stderr, "Error: CALL out of bounds (%d)\n", dest); return -1; }
                } else {
                    fprintf(stderr, "Error: Call Stack Overflow\n"); return -1;
                }
                break;
            case OP_LOAD: {
                int reg_idx = vm->code[vm->pc++];
                if (reg_idx < 0 || reg_idx >= NUM_REGISTERS) {
                    fprintf(stderr, "Error: Invalid register index %d\n", reg_idx);
                    return -1;
                }
                if (vm->sp < STACK_SIZE - 1) {
                    vm->stack[++vm->sp] = vm->registers[reg_idx];
                } else {
                    fprintf(stderr, "Error: Stack Overflow on LOAD\n");
                    return -1;
                }
                break;
			}
            case OP_STORE: {
                int reg_idx = vm->code[vm->pc++];
                if (reg_idx < 0 || reg_idx >= NUM_REGISTERS) {
                    fprintf(stderr, "Error: Invalid register index %d\n", reg_idx);
                    return -1;
                }
                if (vm->sp >= 0) {
                    vm->registers[reg_idx] = vm->stack[vm->sp--];
                } else {
                    fprintf(stderr, "Error: Stack Underflow on STORE\n");
                    return -1;
                }
                break;
			}
            case OP_RET:
                if (vm->csp >= 0) {
                    // Restore the PC to the address saved on the call stack
                    vm->pc = vm->call_stack[vm->csp--];
                } else {
                    fprintf(stderr, "Error: Call Stack Underflow (RET without CALL)\n");
                    return -1;
                }
                break;
			case OP_MUL:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a * b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for MUL\n");
                    return -1;
                }
                break;
            case OP_DIV:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    if (b == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        return -1;
                    }
                    vm->stack[++vm->sp] = a / b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for DIV\n");
                    return -1;
                }
                break;
            case OP_MOD:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    if (b == 0) {
                        fprintf(stderr, "Error: Modulo by zero\n");
                        return -1;
                    }
                    vm->stack[++vm->sp] = a % b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for MOD\n");
                    return -1;
                }
                break;
            case OP_AND:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a & b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for AND\n");
                    return -1;
                }
                break;
            case OP_OR:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a | b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for OR\n");
                    return -1;
                }
                break;
            case OP_XOR:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a ^ b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for XOR\n");
                    return -1;
                }
                break;
            case OP_SHL:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a << b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for SHL\n");
                    return -1;
                }
                break;
            case OP_SHR:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = a >> b;
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for SHR\n");
                    return -1;
                }
                break;
			case OP_PEEK:
                if (vm->sp >= 0) {
                    int addr = vm->stack[vm->sp]; // Read address from TOS
                    if (addr >= 0 && addr < 8192) {
                        vm->stack[vm->sp] = vm->data[addr]; // Replace address with data
                    } else {
                        fprintf(stderr, "Error: PEEK Memory Access Violation at %d\n", addr);
                        return -1;
                    }
                } else {
                    fprintf(stderr, "Error: Stack Underflow on PEEK\n");
                    return -1;
                }
                break;
            case OP_POKE:
                if (vm->sp >= 1) {
                    int val = vm->stack[vm->sp--]; // Pop value
                    int addr = vm->stack[vm->sp--]; // Pop address
                    if (addr >= 0 && addr < 8192) {
                        vm->data[addr] = val; // Write to DMA
                    } else {
                        fprintf(stderr, "Error: POKE Memory Access Violation at %d\n", addr);
                        return -1;
                    }
                } else {
                    fprintf(stderr, "Error: Insufficient stack elements for POKE\n");
                    return -1;
                }
                break;
            case OP_SYS: {
                int sys_id = vm->code[vm->pc++]; // Read syscall ID operand
                switch (sys_id) {
                    case 0: // SYS 0: Print Integer (Replaces OP_PRN)
                        if (vm->sp >= 0) {
                            int val = vm->stack[vm->sp--];
                            if (vm->debug_mode) printf("OUT: %d\n", val);
                            else printf("%d\n", val);
                        } else {
                            fprintf(stderr, "Error: Stack empty on SYS 0\n");
                            return -1;
                        }
                        break;
                    case 1: // SYS 1: Print Character
                        if (vm->sp >= 0) {
                            putchar((char)vm->stack[vm->sp--]);
                        } else {
                            fprintf(stderr, "Error: Stack empty on SYS 1\n");
                            return -1;
                        }
                        break;
                    case 2: // SYS 2: Read Character
                        if (vm->sp < STACK_SIZE - 1) {
                            vm->stack[++vm->sp] = getchar();
                        } else {
                            fprintf(stderr, "Error: Stack Overflow on SYS 2\n");
                            return -1;
                        }
                        break;
					case 3: // SYS 3: MALLOC (Bump Allocator)
                        if (vm->sp >= 0) {
                            int size = vm->stack[vm->sp]; 
                            if (size <= 0) {
                                fprintf(stderr, "Error: Invalid allocation size (%d)\n", size);
                                return -1;
                            }
                            if (vm->heap_ptr + size < 8192) {
                                vm->stack[vm->sp] = vm->heap_ptr; 
                                vm->heap_ptr += size; 
                            } else {
                                fprintf(stderr, "Error: Out of Heap Memory\n");
                                return -1;
                            }
                        } else {
                            fprintf(stderr, "Error: Stack empty on SYS 3\n");
                            return -1;
                        }
                        break;
					case 4: // --- NEW: SYS 4: Print Float ---
                        if (vm->sp >= 0) {
                            union { int i; float f; } u;
                            u.i = vm->stack[vm->sp--]; // Pop bits as int
                            if (vm->debug_mode) printf("OUT: %f\n", u.f);
                            else printf("%f\n", u.f);  // Interpret and print as float
                        } else {
                            fprintf(stderr, "Error: Stack empty on SYS 4\n");
                            return -1;
                        }
                        break;
					case 5: // SYS 5: Print String from .DATA
                        if (vm->sp >= 0) {
                            int addr = vm->stack[vm->sp--]; // Pop address pointer
                            if (addr >= 0 && addr < 8192) {
                                int len = vm->data[addr]; // First byte is length
                                for (int i = 1; i <= len; i++) {
                                    putchar(vm->data[addr + i]);
                                }
                            } else {
                                fprintf(stderr, "Error: SYS 5 Invalid string address %d\n", addr);
                                return -1;
                            }
                        } else {
                            fprintf(stderr, "Error: Stack empty on SYS 5\n");
                            return -1;
                        }
                        break;
                    case 6: // SYS 6: Exit VM with Code
                        if (vm->sp >= 0) {
                            vm->exit_code = vm->stack[vm->sp--]; // Pop integer exit code
                            vm->running = 0;                     // Halt execution gracefully
                        } else {
                            fprintf(stderr, "Error: Stack empty on SYS 6\n");
                            return -1;
                        }
                        break;
                    case 7: // SYS 7: Random Number (min, max)
                        if (vm->sp >= 1) {
                            int max = vm->stack[vm->sp--];
                            int min = vm->stack[vm->sp--];
                            if (max < min) {
                                fprintf(stderr, "Error: SYS 7 max is less than min\n");
                                return -1;
                            }
                            int random_val = (rand() % (max - min + 1)) + min;
                            vm->stack[++vm->sp] = random_val;
                        } else {
                            fprintf(stderr, "Error: Insufficient stack elements for SYS 7\n");
                            return -1;
                        }
                        break;
                    case 8: // SYS 8: Read Integer
                        if (vm->sp < STACK_SIZE - 1) {
                            int val;
                            if (scanf("%d", &val) == 1) {
                                vm->stack[++vm->sp] = val;
                            } else {
                                // Flush invalid input buffer
                                while (getchar() != '\n'); 
                                vm->stack[++vm->sp] = 0; // Push 0 on parse failure
                            }
                        } else {
                            fprintf(stderr, "Error: Stack Overflow on SYS 8\n");
                            return -1;
                        }
                        break;
					case 9: // SYS 9: Open File
                        if (vm->sp >= 1) {
                            int mode_val = vm->stack[vm->sp--]; // 0 = read, 1 = write, 2 = append
                            int addr = vm->stack[vm->sp--];     // Address of filename string
                            
                            const char* mode_str = (mode_val == 1) ? "wb" : (mode_val == 2) ? "ab" : "rb";
                            
                            // Read string from .DATA
                            char filename[256] = {0};
                            int len = vm->data[addr];
                            for (int i = 0; i < len && i < 255; i++) {
                                filename[i] = (char)vm->data[addr + 1 + i];
                            }
                            
                            // Find open FD slot
                            int fd = -1;
                            for (int i = 0; i < 16; i++) {
                                if (vm->open_files[i] == NULL) { fd = i; break; }
                            }
                            
                            if (fd != -1) {
                                vm->open_files[fd] = fopen(filename, mode_str);
                                vm->stack[++vm->sp] = (vm->open_files[fd] != NULL) ? fd : -1;
                            } else {
                                vm->stack[++vm->sp] = -1; // No free file descriptors
                            }
                        } else return -1;
                        break;
                    case 10: // SYS 10: Read File (FD, Addr, Bytes)
                        if (vm->sp >= 2) {
                            int bytes = vm->stack[vm->sp--];
                            int addr = vm->stack[vm->sp--];
                            int fd = vm->stack[vm->sp--];
                            
                            if (fd >= 0 && fd < 16 && vm->open_files[fd] != NULL && addr + bytes < 8192) {
                                int read_count = 0;
                                for (int i = 0; i < bytes; i++) {
                                    int c = fgetc(vm->open_files[fd]);
                                    if (c == EOF) break;
                                    vm->data[addr + i] = c; // Store 8-bit char into 32-bit cell
                                    read_count++;
                                }
                                vm->stack[++vm->sp] = read_count;
                            } else vm->stack[++vm->sp] = -1; // Error
                        } else return -1;
                        break;
                    case 11: // SYS 11: Write File (FD, Addr, Bytes)
                        if (vm->sp >= 2) {
                            int bytes = vm->stack[vm->sp--];
                            int addr = vm->stack[vm->sp--];
                            int fd = vm->stack[vm->sp--];
                            
                            if (fd >= 0 && fd < 16 && vm->open_files[fd] != NULL && addr + bytes < 8192) {
                                int write_count = 0;
                                for (int i = 0; i < bytes; i++) {
                                    char c = (char)vm->data[addr + i]; // Cast 32-bit cell down to 8-bit char
                                    if (fputc(c, vm->open_files[fd]) != EOF) {
                                        write_count++;
                                    } else break;
                                }
                                vm->stack[++vm->sp] = write_count;
                            } else vm->stack[++vm->sp] = -1; // Error
                        } else return -1;
                        break;
                    case 12: // SYS 12: Close File
                        if (vm->sp >= 0) {
                            int fd = vm->stack[vm->sp--];
                            if (fd >= 0 && fd < 16 && vm->open_files[fd] != NULL) {
                                fclose(vm->open_files[fd]);
                                vm->open_files[fd] = NULL;
                                vm->stack[++vm->sp] = 1; // Success
                            } else vm->stack[++vm->sp] = 0; // Failure
                        } else return -1;
                        break;

                    case 13: // SYS 13: Get Epoch Time
                        if (vm->sp < STACK_SIZE - 1) {
                            vm->stack[++vm->sp] = (int)time(NULL);
                        } else return -1;
                        break;
					case 14: // SYS 14: Sleep (Milliseconds)
                        if (vm->sp >= 0) {
                            int ms = vm->stack[vm->sp--];
                            #if defined(_WIN32) || defined(WIN32)
                                Sleep(ms);
                            #else
                                usleep(ms * 1000); // Requires <unistd.h> which is already included
                            #endif
                        } else return -1;
                        break;
                    default:
                        fprintf(stderr, "Error: Unknown Syscall %d\n", sys_id);
                        return -1;
                }
                break;
            }
            case OP_STR: {
                int len = vm->code[vm->pc++]; 
                // Ensure length is valid and won't push PC out of bounds
                if (len < 0 || vm->pc + len > MEMORY_SIZE) {
                    fprintf(stderr, "Error: Corrupted STR length or OOB\n");
                    return -1;
                }
                int str_ptr = vm->pc;         
                vm->pc += (len + 1);          
                if (vm->sp < STACK_SIZE - 1) {
                    vm->stack[++vm->sp] = str_ptr; 
                } else {
                    fprintf(stderr, "Error: Stack Overflow on STR\n");
                    return -1;
                }
                break;
            }
            case OP_CPEEK:
                if (vm->sp >= 0) {
                    int addr = vm->stack[vm->sp];
                    if (addr >= 0 && addr < MEMORY_SIZE) {
                        vm->stack[vm->sp] = vm->code[addr]; // Read from ROM/Code
                    } else {
                        fprintf(stderr, "Error: CPEEK Memory Access Violation at %d\n", addr);
                        return -1;
                    }
                } else {
                    fprintf(stderr, "Error: Stack Underflow on CPEEK\n");
                    return -1;
                }
                break;
			case OP_CPOKE:
				if(vm->sp >= 1) {
					int val = vm->stack[vm->sp--]; // pop new opcode
					int addr = vm->stack[vm->sp--]; // pop instruction address

					if(addr >= 0 && addr < MEMORY_SIZE) {	// bounds checking against ROM
						vm->code[addr] = val;	// overwrite instruction memory
					} else {
						fprintf(stderr, "Error: CPOKE memory access violation at %d\n", addr);
						return -1;
					}
				} else {
					fprintf(stderr, "Error: Insufficient stack elements for CPOKE\n");
					return -1;
				}
				break;
			case OP_DUP:
                if (vm->sp >= 0 && vm->sp < STACK_SIZE - 1) {
                    int val = vm->stack[vm->sp];
                    vm->stack[++vm->sp] = val;
                } else return -1;
                break;
            case OP_SWAP:
                if (vm->sp >= 1) {
                    int tmp = vm->stack[vm->sp];
                    vm->stack[vm->sp] = vm->stack[vm->sp - 1];
                    vm->stack[vm->sp - 1] = tmp;
                } else return -1;
                break;
            case OP_EQ:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = (a == b) ? 1 : 0;
                } else return -1;
                break;
            case OP_LT:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = (a < b) ? 1 : 0;
                } else return -1;
                break;
            case OP_GT:
                if (vm->sp >= 1) {
                    int b = vm->stack[vm->sp--];
                    int a = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = (a > b) ? 1 : 0;
                } else return -1;
                break;
            case OP_ENTER: {
                int locals = vm->code[vm->pc++];
                if (vm->csp < STACK_SIZE - 1 && vm->sp + locals < STACK_SIZE) {
                    vm->call_stack[++vm->csp] = vm->fp; // Save old FP
                    vm->fp = vm->sp;                    // FP points to last arg
                    vm->sp += locals;                   // Allocate local space
                } else return -1;
                break;
            }
            case OP_LEAVE:
                if (vm->csp >= 0) {
                    vm->sp = vm->fp;                    // Deallocate locals
                    vm->fp = vm->call_stack[vm->csp--]; // Restore old FP
                } else return -1;
                break;
            case OP_LDFP: {
                int offset = vm->code[vm->pc++];
                int target = vm->fp + offset;
                if (target >= 0 && target < STACK_SIZE) {
                    if (vm->sp < STACK_SIZE - 1) {
                        vm->stack[++vm->sp] = vm->stack[target];
                    } else {
                        fprintf(stderr, "Error: Stack Overflow on LDFP\n"); return -1;
                    }
                } else {
                    fprintf(stderr, "Error: OOB Stack Read on LDFP (Index %d)\n", target); return -1;
                }
                break;
            }
            case OP_STFP: {
                int offset = vm->code[vm->pc++];
                int target = vm->fp + offset;
                if (target >= 0 && target < STACK_SIZE) {
                    if (vm->sp >= 0) {
                        vm->stack[target] = vm->stack[vm->sp--];
                    } else {
                        fprintf(stderr, "Error: Stack Underflow on STFP\n"); return -1;
                    }
                } else {
                    fprintf(stderr, "Error: OOB Stack Write on STFP (Index %d)\n", target); return -1;
                }
                break;
            }
			case OP_FADD:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b, res;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    res.f = a.f + b.f;
                    vm->stack[++vm->sp] = res.i;
                } else return -1;
                break;
            case OP_FSUB:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b, res;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    res.f = a.f - b.f;
                    vm->stack[++vm->sp] = res.i;
                } else return -1;
                break;
            case OP_FMUL:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b, res;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    res.f = a.f * b.f;
                    vm->stack[++vm->sp] = res.i;
                } else return -1;
                break;
            case OP_FDIV:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b, res;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    if (b.f == 0.0f) {
                        fprintf(stderr, "Error: Float Division by zero\n");
                        return -1;
                    }
                    res.f = a.f / b.f;
                    vm->stack[++vm->sp] = res.i;
                } else return -1;
                break;
            case OP_ITOF:
                if (vm->sp >= 0) {
                    union { int i; float f; } u;
                    u.f = (float)vm->stack[vm->sp]; // Convert actual int value to float
                    vm->stack[vm->sp] = u.i;        // Store bits
                } else return -1;
                break;
            case OP_FTOI:
                if (vm->sp >= 0) {
                    union { int i; float f; } u;
                    u.i = vm->stack[vm->sp];        // Read bits
                    vm->stack[vm->sp] = (int)u.f;   // Convert float value to int
                } else return -1;
                break;
			case OP_NOT:
                if (vm->sp >= 0) {
                    vm->stack[vm->sp] = ~vm->stack[vm->sp];
                } else return -1;
                break; 
            case OP_FEQ:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = (a.f == b.f) ? 1 : 0;
                } else return -1;
                break;
            case OP_FLT:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = (a.f < b.f) ? 1 : 0;
                } else return -1;
                break;
            case OP_FGT:
                if (vm->sp >= 1) {
                    union { int i; float f; } a, b;
                    b.i = vm->stack[vm->sp--];
                    a.i = vm->stack[vm->sp--];
                    vm->stack[++vm->sp] = (a.f > b.f) ? 1 : 0;
                } else return -1;
                break;
			case OP_BRK:
                printf("\n[BREAKPOINT HIT] PC: %04d\n", vm->pc - 1);
                vm->debug_mode = 1; // Force interactive debug mode on
                break;
            case OP_BCOPY: // Block copy (Dest, Src, Length)
                if (vm->sp >= 2) {
                    int len = vm->stack[vm->sp--];
                    int src = vm->stack[vm->sp--];
                    int dest = vm->stack[vm->sp--];
                    if (src >= 0 && dest >= 0 && src + len < 8192 && dest + len < 8192) {
                        memmove(&vm->data[dest], &vm->data[src], len * sizeof(int));
                    } else return -1; // Out of bounds
                } else return -1;
                break;
			case OP_LDI: {
                int reg_idx = vm->code[vm->pc++];
                int val = vm->code[vm->pc++];
                if (reg_idx >= 0 && reg_idx < NUM_REGISTERS) {
                    vm->registers[reg_idx] = val;
                } else {
                    fprintf(stderr, "Error: Invalid register index %d on LDI\n", reg_idx);
                    return -1;
                }
                break;
            }
            case OP_MOVR: {
                int dest_reg = vm->code[vm->pc++];
                int src_reg = vm->code[vm->pc++];
                if (dest_reg >= 0 && dest_reg < NUM_REGISTERS && src_reg >= 0 && src_reg < NUM_REGISTERS) {
                    vm->registers[dest_reg] = vm->registers[src_reg];
                } else {
                    fprintf(stderr, "Error: Invalid register index on MOVR\n");
                    return -1;
                }
                break;
            }
            case OP_INCR: {
                int reg_idx = vm->code[vm->pc++];
                if (reg_idx >= 0 && reg_idx < NUM_REGISTERS) {
                    vm->registers[reg_idx]++;
                } else {
                    fprintf(stderr, "Error: Invalid register index %d on INCR\n", reg_idx);
                    return -1;
                }
                break;
            }
            case OP_DECR: {
                int reg_idx = vm->code[vm->pc++];
                if (reg_idx >= 0 && reg_idx < NUM_REGISTERS) {
                    vm->registers[reg_idx]--;
                } else {
                    fprintf(stderr, "Error: Invalid register index %d on DECR\n", reg_idx);
                    return -1;
                }
                break;
            }
            default: // <-- unknown opcode fallback
                fprintf(stderr, "Unknown opcode %d at index %d\n", instr, vm->pc - 1);
                return -1;
        }
    }
	// cleanup unclosed files when VM terminates
	for(int i = 0; i < 16; i++) {
		if(vm->open_files[i] != NULL) {
			fclose(vm->open_files[i]);
			vm->open_files[i] = NULL;
		}
	}

    return 0;
}

int save_bytecode(VM *vm, int code_size, int data_size, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    
    int magic = VM_MAGIC;
    fwrite(&magic, sizeof(int), 1, f);
    fwrite(&code_size, sizeof(int), 1, f);
    fwrite(&data_size, sizeof(int), 1, f); 

    // Write Data and Code lumps
    fwrite(vm->code, sizeof(int), code_size, f);
    fwrite(vm->data, sizeof(int), data_size, f); 
    // Write Symbol Table lump
    fwrite(&vm->symbol_count, sizeof(int), 1, f);
    fwrite(vm->symbols, sizeof(Label), vm->symbol_count, f);

    fclose(f);
    return 0;
}

int load_bytecode(VM *vm, const char *filename, int *code_size_out) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;

    int magic = 0, code_size = 0, data_size = 0;
    if (fread(&magic, sizeof(int), 1, f) != 1 || magic != VM_MAGIC) { fclose(f); return -1; }
    if (fread(&code_size, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    if (fread(&data_size, sizeof(int), 1, f) != 1) { fclose(f); return -1; } 

    // --- NEW: Strict Bounds Checking ---
    if (code_size < 0 || code_size > MEMORY_SIZE) {
        fprintf(stderr, "Fatal: Invalid code size in binary (%d).\n", code_size);
        fclose(f); return -1; 
    }
    if (data_size < 0 || data_size > 8192) {
        fprintf(stderr, "Fatal: Invalid data size in binary (%d).\n", data_size);
        fclose(f); return -1; 
    }

    if (fread(vm->code, sizeof(int), code_size, f) != code_size) { fclose(f); return -1; }
    if (fread(vm->data, sizeof(int), data_size, f) != data_size) { fclose(f); return -1; } 
    
    // Read Symbol Table safely
    if (fread(&vm->symbol_count, sizeof(int), 1, f) == 1) {
        // --- NEW: Symbol Bound Check ---
        if (vm->symbol_count < 0 || vm->symbol_count > MAX_LABELS) {
            fprintf(stderr, "Fatal: Invalid symbol count in binary (%d).\n", vm->symbol_count);
            fclose(f); return -1;
        }
        fread(vm->symbols, sizeof(Label), vm->symbol_count, f);
    } else {
        vm->symbol_count = 0; 
    }
    
    *code_size_out = code_size;
    fclose(f);
    return 0;
}