#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

static struct proc_dir_entry *oplus_sched_dir;
static struct proc_dir_entry *sched_assist_dir;
static struct proc_dir_entry *jank_info_dir;
static struct proc_dir_entry *cpu_jank_dir;
static struct proc_dir_entry *oplus_health_dir;
static struct proc_dir_entry *devinfo_dir;

static unsigned int sched_assist_scene_val = 0;

/* 1. Handler untuk sched_assist_scene (Bitmask RMW) */
static ssize_t scene_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	char kbuf;
	unsigned int val = 0;

	if (count >= sizeof(kbuf))
		return -EINVAL;
	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;
	kbuf[count] = '\0';

	if (kstrtouint(kbuf, 0, &val) == 0) {
		if (val == 0)
			sched_assist_scene_val = 0;
		else if (val & 0x80)
			sched_assist_scene_val |= (val & ~0x80);
		else
			sched_assist_scene_val &= ~val;
	}
	return count;
}

static ssize_t scene_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	char kbuf;
	int len = snprintf(kbuf, sizeof(kbuf), "0x%x\n", sched_assist_scene_val);
	return simple_read_from_buffer(buf, count, ppos, kbuf, len);
}

static const struct proc_ops scene_fops = {
	.proc_read  = scene_read,
	.proc_write = scene_write,
};

/* 2. Handler Generik (Terima semua write, kembalikan "0\n" saat dibaca) */
static ssize_t dummy_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	return count;
}

static ssize_t dummy_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(buf, count, ppos, "0\n", 2);
}

static const struct proc_ops dummy_fops = {
	.proc_read  = dummy_read,
	.proc_write = dummy_write,
};

/* 3. Handler khusus /proc/devinfo/ufs */
static ssize_t ufs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	const char *info = "Device version:\t\tUFS2.2\nDevice manufacture:\tSAMSUNG\n";
	return simple_read_from_buffer(buf, count, ppos, info, strlen(info));
}

static const struct proc_ops ufs_fops = {
	.proc_read = ufs_read,
};

static int __init oplus_sched_stub_init(void)
{
	/* /proc/oplus_scheduler/sched_assist/* */
	oplus_sched_dir = proc_mkdir("oplus_scheduler", NULL);
	if (oplus_sched_dir) {
		sched_assist_dir = proc_mkdir("sched_assist", oplus_sched_dir);
		if (sched_assist_dir) {
			proc_create("sched_assist_scene", 0666, sched_assist_dir, &scene_fops);
			proc_create("sched_impt_task", 0666, sched_assist_dir, &dummy_fops);
			proc_create("ux_task", 0666, sched_assist_dir, &dummy_fops);
		}
	}

	/* /proc/jank_info/cpu_jank_info/task_track_enable */
	jank_info_dir = proc_mkdir("jank_info", NULL);
	if (jank_info_dir) {
		cpu_jank_dir = proc_mkdir("cpu_jank_info", jank_info_dir);
		if (cpu_jank_dir)
			proc_create("task_track_enable", 0666, cpu_jank_dir, &dummy_fops);
	}

	/* /proc/oplus_healthinfo/cpu_loading */
	oplus_health_dir = proc_mkdir("oplus_healthinfo", NULL);
	if (oplus_health_dir)
		proc_create("cpu_loading", 0666, oplus_health_dir, &dummy_fops);

	/* /proc/devinfo/ufs */
	devinfo_dir = proc_mkdir("devinfo", NULL);
	if (devinfo_dir)
		proc_create("ufs", 0444, devinfo_dir, &ufs_fops);

	pr_info("oplus_sched_stub: all performance nodes ready\n");
	return 0;
}

static void __exit oplus_sched_stub_exit(void)
{
	remove_proc_subtree("oplus_scheduler", NULL);
	remove_proc_subtree("jank_info", NULL);
	remove_proc_subtree("oplus_healthinfo", NULL);
	remove_proc_subtree("devinfo", NULL);
}

module_init(oplus_sched_stub_init);
module_exit(oplus_sched_stub_exit);

MODULE_LICENSE("GPL");
