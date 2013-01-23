#ifndef __RTKTS_SYSCALLS_H_
#define __RTKTS_SYSCALLS_H_

#include <asm/unistd.h>
#include <linux/syscalls.h>

void **syscall_table;
inline unsigned long**
find_syscall_table(void){
        unsigned long **table;
        unsigned long ptr;

        for(ptr = 0xc0000000; ptr <=0xd0000000; ptr+=sizeof(void *))
        {
                table = (unsigned long **) ptr;
                if(table[__NR_close] == (unsigned long *)sys_close)
                {
                        return &(table[0]);
                }

        }
        printk("Not found\n");
        return NULL;

}

#endif
