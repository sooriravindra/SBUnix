.globl timer_isr_asm, kb_isr_asm
.extern timer_isr, kb_isr 

push_regs:

    // Push all regs onto stack
    push %rsi
    push %rdi
    push %rbp
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    /* Since we changed the stack we need to push the return address 
     * which was pushed onto the stack when this function was called. 
     * First read it from stack, then push. 120 = 8(size) * 15(num of regs) */
    movq 120(%rsp), %rax
    push %rax

    retq

pop_regs:

    /* 
     * The stack top has return address. We move it to the the location that 
     * will be the stack top once we pop all registers. This allows to safely 
     * return to the place that this function was called from.
     */
    pop %rax
    movq %rax,120(%rsp)

    // Now we pop all regs
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    pop %rbp
    pop %rdi
    pop %rsi
    retq

timer_isr_asm:
    callq push_regs
    callq timer_isr
    callq pop_regs
    iretq

kb_isr_asm:
    callq push_regs
    callq kb_isr
    callq pop_regs
    iretq

.end
