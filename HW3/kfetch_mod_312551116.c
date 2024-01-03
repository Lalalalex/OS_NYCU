 #include <linux/atomic.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/kernel.h>
#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/types.h> 
#include <linux/uaccess.h>
#include <linux/version.h> 
#include <asm/errno.h> 
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/sched.h>
#include <linux/utsname.h>
#include <asm/page.h>
#include <linux/ktime.h>
#include <linux/sysinfo.h>
#include "kfetch.h"

static int device_open(struct inode *, struct file *); 
static int device_release(struct inode *, struct file *); 
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *); 

#define SUCCESS 0 
#define DEVICE_NAME "kfetch"
#define BUF_LEN 80

static int major;
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
};
int mask[KFETCH_NUM_INFO] = {1, 1, 1, 1, 1, 1};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
static char msg[BUF_LEN + 1];
static struct class *cls; 
static struct file_operations chardev_fops = { .read = device_read, .write = device_write, .open = device_open, .release = device_release,}; 
static int __init chardev_init(void) {
	major = register_chrdev(0, DEVICE_NAME, &chardev_fops); 
    if (major < 0) { 
        pr_alert("Registering char device failed with %d\n", major); 
        return major; 
    } 
    pr_info("I was assigned major number %d.\n", major); 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
    cls = class_create(DEVICE_NAME); 
#else 
    cls = class_create(THIS_MODULE, DEVICE_NAME); 
#endif 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 
    pr_info("Device created on /dev/%s\n", DEVICE_NAME); 
    return SUCCESS; 
} 
 
static void __exit chardev_exit(void){ 
    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls); 
    unregister_chrdev(major, DEVICE_NAME); 
} 

static int device_open(struct inode *inode, struct file *file){ 
    static int counter = 0; 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 
    sprintf(msg, "I already told you %d times Hello world!\n", counter++); 
    try_module_get(THIS_MODULE); 
    return SUCCESS; 
} 

static int device_release(struct inode *inode, struct file *file) { 
    atomic_set(&already_open, CDEV_NOT_USED); 
    module_put(THIS_MODULE); 
    return SUCCESS; 
} 

void get_kernel_release(char *info_buffer, int info_length){
	strncat(info_buffer, "Kernel:   ", info_length);
	strncat(info_buffer, utsname() -> release, info_length);
}

void get_cpu_model_name(char *info_buffer, int info_length){
	unsigned int cpu = 0;
	struct cpuinfo_x86 *c;
	for_each_online_cpu(cpu){
		c = &cpu_data(cpu);
		strncat(info_buffer, "CPU:      ", info_length);
		strncat(info_buffer, c -> x86_model_id, info_length);
		break;
	}
}

void get_cpu_cores(char *info_buffer, int info_length){
	char online_cpu[30];
	char all_cpu[30];
	snprintf(online_cpu, sizeof(online_cpu), "%d",num_online_cpus());
	snprintf(all_cpu, sizeof(online_cpu), "%d", nr_cpu_ids);
	strncat(info_buffer, "CPUs:     \0", info_length);
	strncat(info_buffer, online_cpu, info_length);
	strncat(info_buffer, " / ", info_length);
	strncat(info_buffer, all_cpu, info_length);
}

void get_mem_info(char *info_buffer, int info_length){
	struct sysinfo sysInfo;
	si_meminfo(&sysInfo);

	unsigned long int totalMem = (sysInfo.totalram << (PAGE_SHIFT - 10)) / 1024;
	unsigned long int freeMem = (sysInfo.freeram << (PAGE_SHIFT - 10)) / 1024;

	char totalMemStr[30];
	char freeMemStr[30];
	
	snprintf(totalMemStr, sizeof(totalMemStr), "%d", totalMem);
	snprintf(freeMemStr, sizeof(freeMemStr), "%d", freeMem);

	strncat(info_buffer, "Mem:      \0", info_length);
	strncat(info_buffer, freeMemStr, info_length);
	strncat(info_buffer, " MB / ", info_length);
	strncat(info_buffer, totalMemStr, info_length);
	strncat(info_buffer, " MB", info_length);
}

void get_processes(char *info_buffer, int info_length){
	struct task_struct *task;
	size_t counter = 0;
	for_each_process(task){
		counter = counter + 1;
	}
	char processes[30];
	snprintf(processes, sizeof(processes), "%d", counter);
	strncat(info_buffer, "Procs:    \0", info_length);
	strncat(info_buffer, processes, info_length);
}

void get_uptime(char *info_buffer, int info_length){
	s64 uptime;
	uptime = ktime_divns(ktime_get_coarse_boottime(), NSEC_PER_SEC);
	char uptimeStr[30];
	snprintf(uptimeStr, sizeof(uptimeStr), "%d", uptime/60);
	strncat(info_buffer, "Uptime:   ", info_length);
	strncat(info_buffer, uptimeStr, info_length);
	strncat(info_buffer, " mins\0", info_length);
}

void get_sysinfo(char *info_buffer, int infoID){
	size_t info_length = sizeof(info_buffer) - strlen(info_buffer) - 1;
	switch(infoID){
		case 0:
			get_kernel_release(info_buffer, info_length);
			break;
		case 1:
			get_cpu_model_name(info_buffer, info_length);
			break;
		case 2:
			get_cpu_cores(info_buffer, info_length);
			break;
		case 3:
			get_mem_info(info_buffer, info_length);
			break;
		case 4:
			get_processes(info_buffer, info_length);
			break;
		case 5:
			get_uptime(info_buffer, info_length);
			break;
		default:
			break;
	}
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
	char info_buffer[1000];
    size_t info_length = sizeof(info_buffer) - strlen(info_buffer) - 1;
	memset(info_buffer, 0, sizeof(info_buffer) - 1);
	strncat(info_buffer, "                   \0", info_length);
	strncat(info_buffer, utsname() -> nodename, info_length);
	strncat(info_buffer, "\n", info_length);
	strncat(info_buffer, "        .-.        ", info_length);
	for(int i = 0;i < strlen(utsname() -> nodename); i++){
		strncat(info_buffer, "-", info_length);
	}
	strncat(info_buffer, "\n", info_length);

	int id = 0;
	strncat(info_buffer, "       (.. |       ", info_length);
	while(id < 6){
		if(mask[id] == 1){
			get_sysinfo(info_buffer, id);
			id = id + 1;
			break;
		}
		id = id + 1;
	}
	strncat(info_buffer, "\n", info_length);

	strncat(info_buffer,"        <>  |       ", info_length);
	while(id < 6){
		if(mask[id] == 1){
			get_sysinfo(info_buffer, id);
			id = id + 1;
			break;
		}
		id = id + 1;
	}
	strncat(info_buffer, "\n", info_length);

	strncat(info_buffer, "      / --- \\      ", info_length);
	while(id < 6){
		if(mask[id] == 1){
			get_sysinfo(info_buffer, id);
			id = id + 1;
			break;
		}
		id = id + 1;
	}
	strncat(info_buffer, "\n", info_length);

	strncat(info_buffer, "     ( |   | |     ", info_length);
	while(id < 6){
		if(mask[id] == 1){
			get_sysinfo(info_buffer, id);
			id = id + 1;
			break;
		}
		id = id + 1;
	}
	strncat(info_buffer, "\n", info_length);

	strncat(info_buffer, "   |\\\\_)___/\\)/\\   ", info_length);
	while(id < 6){
		if(mask[id] == 1){
			get_sysinfo(info_buffer, id);
			id = id + 1;
			break;
		}
		id = id + 1;
	}
	strncat(info_buffer, "\n", info_length);

	strncat(info_buffer, "  <__)------(__/   ", info_length);
	while(id < 6){
		if(mask[id] == 1){
			get_sysinfo(info_buffer, id);
			id = id + 1;
			break;
		}
		id = id + 1;
	}
	strncat(info_buffer, "\n", info_length);
	
	ssize_t ret = copy_to_user(buffer, info_buffer, strlen(info_buffer));
    return strlen(info_buffer); 
} 

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *off) { 
	int maskInfo;
	for(int i = 0;i < KFETCH_NUM_INFO;i++){
		mask[i] = 0;
	}
	if(copy_from_user(&maskInfo, buffer, length)){
		pr_alert("Failed to copy data from user.\n");
	}
	if(maskInfo & KFETCH_RELEASE){
		mask[0] = 1;	
	}
	if(maskInfo & KFETCH_NUM_CPUS){
		mask[2] = 1;
	}
	if(maskInfo & KFETCH_CPU_MODEL){
		mask[1] = 1;
	}
	if(maskInfo & KFETCH_MEM){
		mask[3] = 1;
	}
	if(maskInfo & KFETCH_UPTIME){
		mask[5] = 1;
	}
	if(maskInfo & KFETCH_NUM_PROCS){
		mask[4] = 1;
	}
    return 0;
} 

module_init(chardev_init); 
module_exit(chardev_exit); 
MODULE_LICENSE("GPL");
