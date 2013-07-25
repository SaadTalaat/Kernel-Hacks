
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <asm/errno.h>
#include <asm/unistd.h>
#include <linux/mman.h>
#include <asm/proto.h>
#include <asm/delay.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/sched.h>
static struct file_operations chdir_ops;
asmlinkage long (*real_chdir)(const char __user *filename);
void (*syscall_handler)(void);
long unsigned int orig_reg;

void
fake_syscall_dispatcher(void){
	/* steps:
	 *	1- reverse the function prolouge
	 *	2- store the GP-registers/FLAGS
	 *	3- do [Nice] things
	 *	4- restore GP-registers/FLAGS
	 * 	5- call system call
	 */
	__asm__ __volatile__ (
		"mov %rbp,%rsp\n"
		"pop %rbp\n");

	__asm__ __volatile__ (
		"push %rsp\n"
		"push %rax\n"
		"push %rbp\n"
		"push %rdi\n"
		"push %rsi\n"
		"push %rdx\n"
		"push %rcx\n"
		"push %rbx\n"
		"push %r8\n"
		"push %r9\n"
		"push %r10\n"
		"push %r11\n"
		"push %r12\n"
		"push %r15\n"
		);
        // Hook Goes here.
	__asm__ __volatile__(

		"\tpop %%r15\n"
		"\tpop %%r12\n"
		"\tpop %%r11\n"
		"\tpop %%r10\n"
		"\tpop %%r9\n"
		"\tpop %%r8\n"
		"\tpop %%rbx\n"
		"\tpop %%rcx\n"
		"\tpop %%rdx\n"
		"\tpop %%rsi\n"
		"\tpop %%rdi\n"
		"\tpop %%rbp\n"
		"\tpop %%rax\n"
		"\tpop %%rsp\n"
		"\tjmp *%0\n"
		:: "m"(syscall_handler));

}

int __init
chdir_init(void){

	unsigned int low = 0, high = 0, lo=0;
	long unsigned int address;

	rdmsr(0xC0000082,low,high);
	printk("Low:%x\tHigh:%x\n", low,high);
	address = 0;
	address |= high;
	address = address << 32;
	address |= low;
	orig_reg = address;

	printk("Syscall Handler: %lx\n", address);
	syscall_handler = (void (*)(void)) address;

	lo = (unsigned int) (((unsigned long)fake_syscall_dispatcher)
                 & 0xFFFFFFFF);
	printk("Lo: %x\tHi:%x\n", lo,high);

	asm volatile ("wrmsr" :: "c"(0xC0000082), "a"(lo),
                 "d"(high) : "memory");

	return 0;
}

void __exit
chdir_cleanup(void){
	printk("Exit\n");

	asm volatile ("wrmsr" :: "c"(0xC0000082),
          "a"((unsigned int) (orig_reg & 0xFFFFFFFF)),
          "d"(0xffffffff) : "memory");
	return;
}

static struct file_operations chdir_ops= {
	.owner 	= THIS_MODULE,
};
module_init(chdir_init);
module_exit(chdir_cleanup);
MODULE_LICENSE("GPL");
