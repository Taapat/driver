#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAX_LEN  512

static struct proc_dir_entry *proc_entry;
static char info[MAX_LEN] = "[ stb info ]\n\n  stb id :      09000b00:00c5f8\n  chip id :     1474f6094a5d1010\n  mac address : 24-00-0b-00-c5-f8";

ssize_t write_info(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    if (len > MAX_LEN)
    {
        printk(KERN_INFO "No space to write in /proc/sparkid!\n");
        return -1;
    }
    if (copy_from_user(info, buff, len))
        return -2;
    info[len-1] = 0;

    return len;
}

int read_info(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    if (off > 0)
    {
        *eof = 1;
        return 0;
    }

    return sprintf(page, "%s\n", info);
}

int init_module(void)
{
    proc_entry = create_proc_entry("sparkid", 0644, NULL);

    if (proc_entry == NULL)
    {
        printk(KERN_INFO "/proc/sparkid could not be created\n");
        return -1;
    }

    proc_entry->read_proc = read_info;
    proc_entry->write_proc = write_info;
    printk(KERN_INFO "/proc/sparkid created.\n");

    return 0;
}

void cleanup_module(void)
{
    remove_proc_entry("sparkid", NULL);
    printk(KERN_INFO "encrypt unloaded.\n");
}

