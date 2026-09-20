#include "vm.h"

// Helper function to check if a file has a .bin extension
int is_binary_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;
    return strcmp(dot, ".bin") == 0;
}

int main(int argc, char **argv) {
    int debug_mode = 0;
    int compile_only = 0;
	int disassemble_only = 0;
    char *source_file = NULL;
    char *output_file = "out.bin"; // Default output name

    // 1. Expanded Command-Line Parser
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            compile_only = 1; // Do not run, just compile
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires an output filename.\n");
                return 1;
            }
		} else if (strcmp(argv[i], "-dis") == 0) {
			disassemble_only = 1;
        } else if (argv[i][0] != '-') {
            source_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!source_file) {
        fprintf(stderr, "Usage: %s [-d] [-c] [-o out.bin] <file.asm | file.bin>\n", argv[0]);
        return 1;
    }

    VM vm;
    init_vm(&vm);
    vm.debug_mode = debug_mode;

	// Execution Routing updates
    int code_size = 0;
    int data_size = 0;

	if (is_binary_file(source_file)) {
        if (load_bytecode(&vm, source_file, &code_size) < 0) return 1;
    } else {
        int token_count = 0;
        Token *tokens = tokenize(source_file, &token_count);
        
        if (!tokens) return 1; // Catch if the initial file failed to load
        
        // --- Run the Include Preprocessor ---
        tokens = process_includes(tokens, &token_count);
        if (!tokens) return 1; // Catch if an include failed
        
        // Pass the fully merged token stream to the assembler
        code_size = assemble(tokens, &vm, &data_size);
        free(tokens); 

        if (code_size < 0) return 1;

        if (compile_only) {
            save_bytecode(&vm, code_size, data_size, output_file);
            return 0; 
        }
    }

    if (disassemble_only) {
        disassemble(&vm, code_size);
        return 0;
    }

	srand((unsigned int)time(NULL));

    if (run_vm(&vm) < 0) {
        // Post-Mortem Crash Dump
        fprintf(stderr, "\n=== VM CRASH DUMP ===\n");
        fprintf(stderr, "PC: %04d | SP: %03d | CSP: %03d | FP: %03d\n", 
                vm.pc > 0 ? vm.pc - 1 : 0, vm.sp, vm.csp, vm.fp);
        fprintf(stderr, "Registers: R0:%d R1:%d R2:%d R3:%d R4:%d R5:%d R6:%d R7:%d\n",
                vm.registers[0], vm.registers[1], vm.registers[2], vm.registers[3],
				vm.registers[4], vm.registers[5], vm.registers[6], vm.registers[7]);
        fprintf(stderr, "Data Stack (Top 3):\n");
        for (int i = 0; i < 3 && vm.sp - i >= 0; i++) {
            fprintf(stderr, "  [%03d] %d\n", vm.sp - i, vm.stack[vm.sp - i]);
        }
        fprintf(stderr, "=====================\n");
        return 1;
    }

    // Allow SYS 6 to pass its exitcode value back to the OS
    return vm.exit_code;
}