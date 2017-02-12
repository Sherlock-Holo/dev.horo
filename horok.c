#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/netpoll.h>

#define M_MISC_MAJOR 0
#define FEED "feed"
#define WASH "wash"
#define EIDLEN 13

struct horo_data {
	int clean;
	int hunger
};

struct horo_data Horo;

static struct miscdevice misc;

char kbuf[4096];

static ssize_t horo_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offp)
{
	return simple_read_from_buffer(buf, count, offp, kbuf, strlen(kbuf) + 1);
}

static int call_horoproxy(const char *cmd) {
	struct subprocess_info *sub_info;
	char *argv[] = {"/usr/local/bin/horoproxy", cmd, NULL};
	static char *envp[] = {
		"HOME=/",
		"TERM=linux",
		NULL
	};
	sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_ATOMIC, NULL, NULL, NULL);
	if (sub_info == NULL) return -ENOMEM;
	return call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
}

static ssize_t horo_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offp)
{
	char kbuf[20] = "";
	int ret;

	memset(kbuf, 0, sizeof(kbuf));
	pr_info("MARK");
	ret = simple_write_to_buffer(kbuf, sizeof(kbuf), offp, buf, count);
	if (ret < 0)
		return ret;
	pr_info("Kbuf: %s", kbuf);
	ret = call_horoproxy(kbuf);
	if(ret == 0)
		return count;
	return ret;
}

// horo_ioctl is for store the result to buffer
// later will show the result when userspace call horo_read
static long horo_ioctl(struct file *filp, unsigned int op, 
		unsigned long paramp) 
{
	memset(kbuf, 0, sizeof(kbuf));
	sprintf(kbuf, "%s", (char *)paramp);
	pr_info("IOCTL Recv Message: %s\n", (char *)paramp);
	return 0;
}

const struct file_operations misc_fops = {
	.read = horo_read,
	.owner = THIS_MODULE,
	.write = horo_write,
	.unlocked_ioctl = horo_ioctl,
};

int horo_init(void)
{
	int ret;

	pr_info("Start init misc device horo\n");
	misc.minor = MISC_DYNAMIC_MINOR;
	misc.mode = 0666;
	misc.name = "horo";
	misc.fops = &misc_fops;
	ret = misc_register(&misc);
	Horo.hunger = 100;
	Horo.clean = 0;
	if (ret < 0) {
		pr_err("Cannot register misc horo\n");
		return ret;
	}
	return 0;
}

void horo_exit(void)
{
	pr_info("Start de-init misc device horo\n");
	misc_deregister(&misc);
}


module_init(horo_init);
module_exit(horo_exit)

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("VOID001<tg @VOID001>");