#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/thermal.h>

static struct proc_dir_entry *oplus_sched_dir;
static struct proc_dir_entry *sched_assist_dir;
static struct proc_dir_entry *jank_info_dir;
static struct proc_dir_entry *cpu_jank_dir;
static struct proc_dir_entry *oplus_health_dir;
static struct proc_dir_entry *devinfo_dir;
static struct proc_dir_entry *oplus_mem_dir;
static struct proc_dir_entry *oplus_binder_dir;
static struct proc_dir_entry *oplus_cl_dir;

static unsigned int sched_assist_scene_val = 0;

static ssize_t scene_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	unsigned int val = 0;
	int ret;

	ret = kstrtouint_from_user(buf, count, 0, &val);
	if (ret == 0) {
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
	char *kbuf;
	int len;
	ssize_t ret;

	kbuf = kasprintf(GFP_KERNEL, "0x%x\n", sched_assist_scene_val);
	if (!kbuf)
		return -ENOMEM;

	len = strlen(kbuf);
	ret = simple_read_from_buffer(buf, count, ppos, kbuf, len);
	kfree(kbuf);
	return ret;
}

static const struct proc_ops scene_fops = {
	.proc_read  = scene_read,
	.proc_write = scene_write,
};

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

static ssize_t temp_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct thermal_zone_device *tz;
	int temp = 35000;
	char *kbuf;
	ssize_t ret;

	tz = thermal_zone_get_zone_by_name("quiet-therm");
	if (IS_ERR(tz))
		tz = thermal_zone_get_zone_by_name("battery");
	if (!IS_ERR(tz))
		thermal_zone_get_temp(tz, &temp);

	kbuf = kasprintf(GFP_KERNEL, "%d\n", temp);
	if (!kbuf)
		return -ENOMEM;

	ret = simple_read_from_buffer(buf, count, ppos, kbuf, strlen(kbuf));
	kfree(kbuf);
	return ret;
}

static const struct proc_ops temp_fops = {
	.proc_read  = temp_read,
	.proc_write = dummy_write,
};

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
	oplus_sched_dir = proc_mkdir("oplus_scheduler", NULL);
	if (oplus_sched_dir) {
		sched_assist_dir = proc_mkdir("sched_assist", oplus_sched_dir);
		if (sched_assist_dir) {
			proc_create("sched_assist_scene", 0666, sched_assist_dir, &scene_fops);
			proc_create("sched_impt_task", 0666, sched_assist_dir, &dummy_fops);
			proc_create("ux_task", 0666, sched_assist_dir, &dummy_fops);
			proc_create("im_flag", 0666, sched_assist_dir, &dummy_fops);
			proc_create("im_flag_app", 0666, sched_assist_dir, &dummy_fops);
		}
	}

	jank_info_dir = proc_mkdir("jank_info", NULL);
	if (jank_info_dir) {
		cpu_jank_dir = proc_mkdir("cpu_jank_info", jank_info_dir);
		if (cpu_jank_dir)
			proc_create("task_track_enable", 0666, cpu_jank_dir, &dummy_fops);
	}

	oplus_health_dir = proc_mkdir("oplus_healthinfo", NULL);
	if (oplus_health_dir)
		proc_create("cpu_loading", 0666, oplus_health_dir, &dummy_fops);

	devinfo_dir = proc_mkdir("devinfo", NULL);
	if (devinfo_dir)
		proc_create("ufs", 0444, devinfo_dir, &ufs_fops);

	oplus_mem_dir = proc_mkdir("oplus_mem", NULL);
	if (oplus_mem_dir)
		proc_create("memory_monitor", 0666, oplus_mem_dir, &dummy_fops);

	oplus_binder_dir = proc_mkdir("oplus_binder", NULL);
	if (oplus_binder_dir)
		proc_create("ux_flag", 0666, oplus_binder_dir, &dummy_fops);

	oplus_cl_dir = proc_mkdir("oplus_cl", NULL);
	if (oplus_cl_dir)
		proc_create("cl_glthread", 0666, oplus_cl_dir, &dummy_fops);

	proc_create("shell-temp", 0666, NULL, &temp_fops);
	proc_create("theiaPwkReport", 0666, NULL, &dummy_fops);

	pr_info("oplus_sched_stub: all oplus nodes ready\n");
	return 0;
}

static void __exit oplus_sched_stub_exit(void)
{
	remove_proc_subtree("oplus_scheduler", NULL);
	remove_proc_subtree("jank_info", NULL);
	remove_proc_subtree("oplus_healthinfo", NULL);
	remove_proc_subtree("devinfo", NULL);
	remove_proc_subtree("oplus_mem", NULL);
	remove_proc_subtree("oplus_binder", NULL);
	remove_proc_subtree("oplus_cl", NULL);
	remove_proc_entry("shell-temp", NULL);
	remove_proc_entry("theiaPwkReport", NULL);
}

module_init(oplus_sched_stub_init);
module_exit(oplus_sched_stub_exit);

MODULE_LICENSE("GPL");
