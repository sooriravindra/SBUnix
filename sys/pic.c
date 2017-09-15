#include <sys/pic.h>
#include <sys/kprintf.h>
#define ICW1_ICW4 0x01      /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

static inline void
outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
    /* There's an outb %al, $imm8  encoding, for compile-time constant port
     * numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i"
     * constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we
     * had the port number a wider C type */
}

static inline uint8_t
inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void
io_wait()
{
        __asm__ __volatile__ ( "jmp 1f\n\t"
                   "1:jmp 2f\n\t"
                   "2:" );
}

void
pic_init()
{
 	outb( 0x20,0x11);

        outb( 0xa0,0x11);

        /*Remaps IRQ0-IRQ7 to 0x20-0x27 in interrupt vector table*/

        outb( 0x21,0x20); 

        /*Remaps IRQ8-IRQ15 to 0x28-0x2F in interrupt vector table*/

        outb( 0xa1,0x28);

        /*PIC2 is connected to PIC1 via IRQ2*/

        outb( 0x21,0x04);

        outb( 0xa1,0x02);

        /*Enables 8086/88 mode*/

        outb( 0x21,0x01);

        outb( 0xa1,0x01);

        /*Disables all interrupts from IRQ0-IRQ7*/

        outb( 0x21,0xfd);

        /*Disables all interrupts from IRQ8-IRQ15*/

        outb( 0xa1,0xff);
}

struct_idt_entry pic_get_idt_entry(void * address)
{
    long addr = (long) address;
    struct_idt_entry member;
    member.offset_1 = ( addr & 0xffff );
    member.offset_2 = ( (addr >> 16) & 0xffff);
    member.offset_3 = ( (addr >> 32) & 0xffffffff);
    member.selector = 8;
    member.ist = 0;
    member.zero = 0;
    member.type_attr = 0x8E;
    return member;
}

void fake_isr() {
    static int i =0;
    __asm__(
   "push %rax;"
   "push %rbx;"
   "push %rcx;"
   "push %rdx;"
   "push %rsi;"
   "push %rdi;"
   "push %rbp;"
);
    signalme('0'+i++);
    kprintf_("okkkk");
        outb(PIC1_COMMAND, PIC_EOI);
     __asm__(
   "pop %rbp;"
   "pop %rdi;"
   "pop %rsi;"
   "pop %rdx;"
   "pop %rcx;"
   "pop %rbx;"
   "pop %rax;"
   "add $0x8,%rsp;"
    "iretq;"
    );
}

static inline void lidt(void* base, uint16_t size)
{   // This function works in 32 and 64bit mode
    struct {
        uint16_t length;
        uint64_t    base;
    } __attribute__((packed)) IDTR = { size, (uint64_t)base };
 
    __asm__ ( "lidt %0;"
              "sti;"
              : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void register_idt() {
    int i;
    for (i=0; i<256; i++) {
        idt[i] = pic_get_idt_entry((void*)fake_isr);
    }
    lidt(idt, sizeof(struct_idt_entry) * 256 - 1);
    kprintf_("this %d",sizeof(struct_idt_entry));
}


#if 0
void pic_indicate_eoi(unsigned char irq)
    {
        outb(PIC1_COMMAND, PIC_EOI);
        if (irq > 7) {
            outb(PIC2_COMMAND, PIC_EOI);
        }
    }
#endif
