# TextDebugger

a text base debugger to analyse machine level code

Input: .mem files only    

To run program:
    ./debugger program.mem        //Start at the beginning of program.mem
    ./debugger program.mem 0x100  //Start at position 0x100 of program.mem
(reads command line arguments as hex)

Debugger instructions:
    quit/exit: terminates the debugger
    step: executes instruction at the current program counter
    run: starts executing until it hits a halt, a breakpoint, or an invalid instruction is found
    next: starts executing until it hist a return
    jump X: jumps to instruction at address X
    break X: adds a new breakpoint at address X
    delete X: deletes command at address X
    registers: prints current state of registers
    examine X: prints the current state of the memory at address X
sample test files located within testfiles/ folder
