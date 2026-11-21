// SPDX-License-Identifier: GPL-2.0
/*

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/thermal.h>
#include <linux/uaccess.h>

/* ---------- sched_assist_scene ---------- */

#define SCENE_SET_FLAG		0x80	/* val has this bit: set bits, else clear */

static unsigned int scene_val;
static DEFINE_SPINLOCK(scene_lock);

static ssize_t scene_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	unsigned int val;

	/* Bad input is ignored silently on purpose (avoids log spam). */
	if (kstrtouint_from_user(buf, count, 0, &val))
		return count;

	spin_lock(&scene_lock);
	if (!val)
		scene_val = 0;
	else if (val & SCENE_SET_FLAG)
		scene_val |= val & ~SCENE_SET_FLAG;
	else
		scene_val &= ~val;
	spin_unlock(&scene_lock);

	return count;
}

static ssize_t scene_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	char kbuf[16];
	int len;

	len = scnprintf(kbuf, sizeof(kbuf), "0x%x\n", READ_ONCE(scene_val));
	return simple_read_from_buffer(buf, count, ppos, kbuf, len);
}

static const struct proc_ops scene_ops = {
	.proc_read	= scene_read,
	.proc_write	= scene_write,
};

/* ---------- generic dummy node ---------- */

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

static const struct proc_ops dummy_ops = {
	.proc_read	= dummy_read,
	.proc_write	= dummy_write,
};

/* ---------- /proc/shell-temp ---------- */

#define SHELL_TEMP_MAX		120000	/* millidegree C */

/*
 * Shell temp = body/casing temperature, so use board sensors only.
 * Battery is excluded on purpose (its sensor reads garbage).
 */
static const char * const shell_tz_names[] = {
	"quiet-therm-usr", "quiet-therm", "xo-therm",
};

/* Returns 0 and fills *temp from the first sane sensor, else -ENODATA. */
static int shell_temp_get(int *temp)
{
	int i, t;

	for (i = 0; i < ARRAY_SIZE(shell_tz_names); i++) {
		struct thermal_zone_device *tz;

		tz = thermal_zone_get_zone_by_name(shell_tz_names[i]);
		if (IS_ERR(tz))
			continue;
		if (!thermal_zone_get_temp(tz, &t) && t > 0 && t < SHELL_TEMP_MAX) {
			*temp = t;
			return 0;
		}
	}

	return -ENODATA;	/* never fake a value */
}

static ssize_t shell_temp_read(struct file *file, char __user *buf,
			       size_t count, loff_t *ppos)
{
	char kbuf[16];
	int temp, len, ret;

	ret = shell_temp_get(&temp);
	if (ret)
		return ret;

	len = scnprintf(kbuf, sizeof(kbuf), "%d\n", temp);
	return simple_read_from_buffer(buf, count, ppos, kbuf, len);
}

static const struct proc_ops shell_temp_ops = {
	.proc_read	= shell_temp_read,
	.proc_write	= dummy_write,
};

/* ---------- /proc/devinfo/ufs ---------- */

static ssize_t ufs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	static const char info[] =
		"Device version:\t\tUFS2.2\nDevice manufacture:\tSKhynix\n";

	return simple_read_from_buffer(buf, count, ppos, info, sizeof(info) - 1);
}

static const struct proc_ops ufs_ops = {
	.proc_read	= ufs_read,
};

/* ---------- node tables ---------- */

/* Parents must come before children. */
static const char * const stub_dirs[] = {
	"oplus_scheduler",
	"oplus_scheduler/sched_assist",
	"jank_info",
	"jank_info/cpu_jank_info",
	"oplus_healthinfo",
	"devinfo",
	"oplus_mem",
	"oplus_binder",
	"oplus_cl",
	"oplus_afs_config",
};

static const struct stub_node {
	const char *path;
	umode_t mode;
	const struct proc_ops *ops;
} stub_nodes[] = {
	{ "oplus_scheduler/sched_assist/sched_assist_scene", 0666, &scene_ops },
	{ "oplus_scheduler/sched_assist/sched_impt_task",    0666, &dummy_ops },
	{ "oplus_scheduler/sched_assist/ux_task",            0666, &dummy_ops },
	{ "oplus_scheduler/sched_assist/im_flag",            0666, &dummy_ops },
	{ "oplus_scheduler/sched_assist/im_flag_app",        0666, &dummy_ops },
	{ "jank_info/cpu_jank_info/task_track_enable",       0666, &dummy_ops },
	{ "oplus_healthinfo/cpu_loading",                    0666, &dummy_ops },
	{ "devinfo/ufs",                                     0444, &ufs_ops },
	{ "oplus_mem/memory_monitor",                        0666, &dummy_ops },
	{ "oplus_binder/ux_flag",                            0666, &dummy_ops },
	{ "oplus_cl/cl_glthread",                            0666, &dummy_ops },
	{ "oplus_afs_config/afs_config",                     0666, &dummy_ops },
	{ "shell-temp",                                      0666, &shell_temp_ops },
	{ "theiaPwkReport",                                  0666, &dummy_ops },
};

static int __init oplus_sched_stub_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stub_dirs); i++)
		if (!proc_mkdir(stub_dirs[i], NULL))
			pr_warn("failed to create /proc/%s\n", stub_dirs[i]);

	for (i = 0; i < ARRAY_SIZE(stub_nodes); i++)
		if (!proc_create(stub_nodes[i].path, stub_nodes[i].mode, NULL,
				 stub_nodes[i].ops))
			pr_warn("failed to create /proc/%s\n", stub_nodes[i].path);

	pr_info("initialized\n");
	return 0;
}

static void __exit oplus_sched_stub_exit(void)
{
	int i;

	for (i = ARRAY_SIZE(stub_nodes) - 1; i >= 0; i--)
		remove_proc_entry(stub_nodes[i].path, NULL);

	for (i = ARRAY_SIZE(stub_dirs) - 1; i >= 0; i--)
		remove_proc_entry(stub_dirs[i], NULL);
}

module_init(oplus_sched_stub_init);
module_exit(oplus_sched_stub_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Oplus procfs compatibility stub for GKI 5.15");
