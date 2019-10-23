# TextDebugger

a text base debugger to analyse machine level code

Input: .mem files only <br/>    

To run program: <br/> 
    ./debugger program.mem        //Start at the beginning of program.mem <br/> 
    ./debugger program.mem 0x100  //Start at position 0x100 of program.mem <br/> 
(reads command line arguments as hex)
 <br/> 
Debugger instructions: <br/> 
    quit/exit: terminates the debugger <br/> 
    step: executes instruction at the current program counter <br/> 
    run: starts executing until it hits a halt, a breakpoint, or an invalid instruction is found <br/> 
    next: starts executing until it hist a return <br/> 
    jump X: jumps to instruction at address X <br/> 
    break X: adds a new breakpoint at address X <br/> 
    delete X: deletes command at address X <br/> 
    registers: prints current state of registers <br/> 
    examine X: prints the current state of the memory at address X <br/> 
sample test files located within testfiles/ folder
