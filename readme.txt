================================================================================
		V M 0 1   T E C H N I C A L   R E F E R E N C E   M A N U A L
================================================================================

INTRODUCTION
--------------------------------------------------------------------------------
The VM01 is a stack-based virtual machine designed for high-performance 
execution of compiled assembly. This manual serves as the definitive guide to 
its architecture, instruction set, and system interfaces.

Architectural Focus: VM01 separates concerns into distinct memory segments: a 
strictly read-only Code segment (ROM) and a general-purpose Data segment (RAM) 
with support for heap allocation, global variables, and dedicated CPU registers.


1. ARCHITECTURE OVERVIEW
--------------------------------------------------------------------------------
The VM handles distinct memory spaces and native components to ensure code 
integrity and flexible data management.

  [C] CODE SEGMENT (ROM):   4,096 integers for instructions and strings.
                            Accessed internally via PC and CPEEK.
  
  [D] DATA SEGMENT (RAM):   8,192 integers for globals, stack frames, heap.
                            Accessed via PEEK/POKE.

  [S] OPERATIONAL STACK:    256-level depth for ALU operations.
  
  [#] CALL STACK:           256-level depth for function call returns.
  
  [R] NATIVE REGISTERS:     8 general-purpose CPU registers (R0 - R7)
                            for direct manipulation outside the stack.


2. BEGINNER'S CRASH COURSE
--------------------------------------------------------------------------------
A. Thinking in Stacks
If you're new to assembly languages, the VM01 is primarily a Stack Machine. 
Imagine a stack of heavy books. You can only put a new book on the top (PUSH), 
and you can only take a book off the top (POP). 

Let's say you want to add 5 and 3. You push the numbers first, then call ADD:
    PUSH 5      ; Stack is now: [5]
    PUSH 3      ; Stack is now: [5, 3]
    ADD         ; Pops 3 and 5, adds them, and pushes 8. Stack is now: [8]
    SYS 0       ; System Call 0 prints the top of the stack. Prints "8".
    HALT        ; Safely stop the program.

B. Variables (The Data Segment)
If you need to store things permanently, you use the .DATA section.
    .DATA
    my_score: 100       ; Create a variable named 'my_score' starting at 100

    .CODE
    PUSH my_score       ; Push memory address
    PUSH 500            ; Push new value
    POKE                ; POKE pops value (500) and address, saving it.

C. Using Native Registers (New)
The assembler natively recognizes eight available registers to fill with data.
You can bypass the stack for counters and temporary math using R0 - R7. 
    LDI R0 10           ; Load the Immediate value 10 into R0
    INCR R0             ; Increment R0 (now 11)


3. COMMAND-LINE USAGE
--------------------------------------------------------------------------------
The VM01 toolchain operates as a versatile environment capable of JIT execution,
AOT compilation, and deep binary introspection.

  Execute Mode (JIT or AOT):
    ./vm script.asm
    ./vm program.bin

  Compiler Mode (Output to binary):
    ./vm -c -o app.bin src.asm

  Debugger Mode (Trace execution state):
    ./vm -d src.asm

  Disassembler Mode:
    ./vm -dis program.bin


4. SYSTEM INTERFACE (SYSCALLS)
--------------------------------------------------------------------------------
The SYS instruction bridges code logic with the host OS. (Syntax: SYS <ID>)

 ID | NAME                 | STACK EFFECT            | DESCRIPTION
 -----------------------------------------------------------------------------
 0  | Print Integer        | [int] -> []             | Prints int to stdout
 1  | Print Character      | [char_code] -> []       | Prints ASCII char
 2  | Read Character       | [] -> [char_code]       | Blocks until 1 char input
 3  | Malloc (Alloc Mem)   | [size] -> [address]     | Returns dynamic mem ptr
 4  | Print Float          | [float_bits] -> []      | Prints bits as float
 5  | Print String (.DATA) | [data_addr] -> []       | Reads string from RAM
 6  | Exit VM              | [exit_code] -> []       | Halts; sets OS exit code
 7  | Random Integer       | [min, max] -> [val]     | Random int inclusive
 8  | Read Integer         | [] -> [int]             | Prompts user for int
 9  | Open File            | [addr, mode] -> [fd]    | Mode 0:R, 1:W, 2:A
 10 | Read File Bytes      | [fd, addr, sz] -> [cnt] | Reads to RAM, returns cnt
 11 | Write File Bytes     | [fd, addr, sz] -> [cnt] | Writes from RAM to File
 12 | Close File           | [fd] -> [success]       | Closes open FD (0-15)
 13 | Get System Time      | [] -> [unix_time]       | Returns Epoch timestamp
 14 | Sleep                | [milliseconds] -> []    | Pauses VM execution


5. ISA REFERENCE
--------------------------------------------------------------------------------
Below is the complete Instruction Set Architecture. 
'Ops' denotes how many inline operands follow the opcode (e.g., LDI takes 2).

 ID | MNEMONIC | OPS | STACK EFFECT           | DESCRIPTION
 -------------------------------------------------------------------------------
 0  | HALT     | 0   | [] -> []               | Safely terminate VM execution.
 1  | PUSH     | 1   | [] -> [value]          | Push literal int to stack.
 2  | POP      | 0   | [value] -> []          | Discard top stack element.
 3  | ADD      | 0   | [a, b] -> [a + b]      | Add top two integers.
 4  | SUB      | 0   | [a, b] -> [a - b]      | Subtract top from second top.
 5  | MUL      | 0   | [a, b] -> [a * b]      | Multiply top two integers.
 6  | DIV      | 0   | [a, b] -> [a / b]      | Divide second top by top.
 7  | MOD      | 0   | [a, b] -> [a % b]      | Modulo division.
 8  | AND      | 0   | [a, b] -> [a & b]      | Bitwise AND.
 9  | OR       | 0   | [a, b] -> [a | b]      | Bitwise OR.
 10 | XOR      | 0   | [a, b] -> [a ^ b]      | Bitwise XOR.
 11 | SHL      | 0   | [a, b] -> [a << b]     | Bitwise Shift Left.
 12 | SHR      | 0   | [a, b] -> [a >> b]     | Bitwise Shift Right.
 13 | PRN      | 0   | [value] -> []          | Legacy print (Use SYS 0).
 14 | JMP      | 1   | [] -> []               | Unconditional jump to address.
 15 | JZ       | 1   | [value] -> []          | Jump to address if zero (0).
 16 | JNZ      | 1   | [value] -> []          | Jump to address if not zero.
 17 | LOAD     | 1   | [] -> [value]          | Push value from Register (0-3).
 18 | STORE    | 1   | [value] -> []          | Pop value into Register (0-3).
 19 | CALL     | 1   | [] -> []               | Push PC to call stack, jump.
 20 | RET      | 0   | [] -> []               | Pop PC from call stack.
 21 | PEEK     | 0   | [address] -> [value]   | Read integer from Data RAM.
 22 | POKE     | 0   | [addr, val] -> []      | Write value into Data RAM.
 23 | SYS      | 1   | Varies                 | Invoke system call by ID.
 24 | STR      | 1   | [] -> [address]        | Define string, push ROM address.
 25 | CPEEK    | 0   | [address] -> [value]   | Read integer from Code ROM.
 26 | CPOKE    | 0   | [addr, val] -> []      | Write integer to Code ROM.
 27 | DUP      | 0   | [val] -> [val, val]    | Duplicate top stack element.
 28 | SWAP     | 0   | [a, b] -> [b, a]       | Swap top two stack elements.
 29 | EQ       | 0   | [a, b] -> [bool]       | Push 1 if a == b, else 0.
 30 | LT       | 0   | [a, b] -> [bool]       | Push 1 if a < b, else 0.
 31 | GT       | 0   | [a, b] -> [bool]       | Push 1 if a > b, else 0.
 32 | ENTER    | 1   | [] -> []               | Init stack frame w/ N locals.
 33 | LEAVE    | 0   | [] -> []               | Tear down current stack frame.
 34 | LDFP     | 1   | [] -> [value]          | Push offset from Frame Pointer.
 35 | STFP     | 1   | [value] -> []          | Pop to offset from Frame Ptr.
 36 | FADD     | 0   | [a, b] -> [a + b]      | Float addition.
 37 | FSUB     | 0   | [a, b] -> [a - b]      | Float subtraction.
 38 | FMUL     | 0   | [a, b] -> [a * b]      | Float multiplication.
 39 | FDIV     | 0   | [a, b] -> [a / b]      | Float division.
 40 | ITOF     | 0   | [int] -> [float]       | Cast Integer to Float bits.
 41 | FTOI     | 0   | [float] -> [int]       | Cast Float to Integer bits.
 42 | FEQ      | 0   | [a, b] -> [bool]       | Push 1 if a(float) == b, else 0.
 43 | FLT      | 0   | [a, b] -> [bool]       | Push 1 if a(float) < b, else 0.
 44 | FGT      | 0   | [a, b] -> [bool]       | Push 1 if a(float) > b, else 0.
 45 | NOT      | 0   | [val] -> [~val]        | Bitwise NOT inversion.
 46 | BCOPY    | 0   | [dst, src, len] -> []  | Block copy memory in RAM.
 47 | BRK      | 0   | [] -> []               | Trigger debugger breakpoint.
 48 | LDI      | 2   | [] -> []               | Load Immediate Val into Reg. (LDI R0 5)
 49 | MOVR     | 2   | [] -> []               | Copy Src to Dst. (MOVR R1 R0)
 50 | INCR     | 1   | [] -> []               | Increment Reg. (INCR R0)
 51 | DECR     | 1   | [] -> []               | Decrement Reg. (DECR R0)


6. EXAMPLES & BEST PRACTICES
--------------------------------------------------------------------------------

[Example 1: Function Frames]
Using ENTER and LEAVE ensures isolated local memory per call, preventing stack 
corruption in recursive calls.
    MY_FUNC:
        ENTER 1       ; Save old FP, allocate 1 local space
        LDFP 0        ; Load Arg 1 (offset 0 from FP)
        PUSH 1
        ADD
        STFP 1        ; Store in Local 1 (offset 1)
        LDFP 1
        SYS 0         ; Print result
        LEAVE         ; Clean frame
        RET

[Example 2: CPU Registers in Loops]
Using the new register commands prevents stack clutter during loop execution.
    .CODE
        LDI R0 5          ; Set loop counter to 5
    LOOP_START:
        ; ... do logic ...
        DECR R0           ; Decrement counter directly
        LOAD R0           ; Push counter to stack for JNZ check
        JNZ LOOP_START    ; If not zero, loop again
        HALT

[Example 3: File IO]
Writing text to a file using the System Call API.
    .DATA
        fname: "out.txt"
        content: "Hello File"
    .CODE
        PUSH fname
        PUSH 1            ; Mode 1 (Write)
        SYS 9             ; Open File. Stack now has [FD]
        DUP               ; Duplicate FD for later closing
        PUSH content      
        PUSH 10           ; Bytes to write
        SYS 11            ; Write File
        POP               ; Discard write_count
        SYS 12            ; Close File (using the FD we DUP'd)
        HALT