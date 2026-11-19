#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

static struct proc_dir_entry *oplus_sched_dir;
static struct proc_dir_entry *sched_assist_dir;
static unsigned int sched_assist_scene_val = 0;

static ssize_t sched_assist_scene_write(struct file *file, const char __user *buf,
					size_t count, loff_t *ppos)
{
	char kbuf[32];
	unsigned int val = 0;

	if (count >= sizeof(kbuf))
		return -EINVAL;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';

	if (kstrtouint(kbuf, 0, &val) == 0) {
		if (val == 0) {
			sched_assist_scene_val = 0;
		} else if (val & 0x80) {
			sched_assist_scene_val |= (val & ~0x80);
		} else {
			sched_assist_scene_val &= ~val;
		}
	}

	return count;
}

static ssize_t sched_assist_scene_read(struct file *file, char __user *buf,
				       size_t count, loff_t *ppos)
{
	char kbuf[32];
	int len;

	len = snprintf(kbuf, sizeof(kbuf), "0x%x\n", sched_assist_scene_val);
	return simple_read_from_buffer(buf, count, ppos, kbuf, len);
}

static const struct proc_ops sched_assist_scene_fops = {
	.proc_read  = sched_assist_scene_read,
	.proc_write = sched_assist_scene_write,
};

static int __init oplus_sched_stub_init(void)
{
	oplus_sched_dir = proc_mkdir("oplus_scheduler", NULL);
	if (!oplus_sched_dir)
		return -ENOMEM;

	sched_assist_dir = proc_mkdir("sched_assist", oplus_sched_dir);
	if (!sched_assist_dir) {
		remove_proc_entry("oplus_scheduler", NULL);
		return -ENOMEM;
	}

	if (!proc_create("sched_assist_scene", 0666, sched_assist_dir, &sched_assist_scene_fops)) {
		remove_proc_entry("sched_assist", oplus_sched_dir);
		remove_proc_entry("oplus_scheduler", NULL);
		return -ENOMEM;
	}

	pr_info("oplus_sched_stub: node ready\n");
	return 0;
}

static void __exit oplus_sched_stub_exit(void)
{
	remove_proc_entry("sched_assist_scene", sched_assist_dir);
	remove_proc_entry("sched_assist", oplus_sched_dir);
	remove_proc_entry("oplus_scheduler", NULL);
}

module_init(oplus_sched_stub_init);
module_exit(oplus_sched_stub_exit);

MODULE_LICENSE("GPL");
