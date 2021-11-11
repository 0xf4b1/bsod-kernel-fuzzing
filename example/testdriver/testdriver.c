#include <linux/module.h>
#include <linux/pci.h>

#define SOME_OPERATION 0

int a;

void generated(unsigned int input);

long int ioctl(struct file *file, unsigned int cmd, unsigned long i_arg) {
  int arg_cmd;
  unsigned int data;

  arg_cmd = _IOC_NR(cmd);
  copy_from_user(&data, (int *)i_arg, sizeof(int));

  switch (arg_cmd) {
    case SOME_OPERATION:
        generated(data);
        break;
  }
  return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = ioctl,
    .compat_ioctl = ioctl,
};

static int __init testdriver_init_module(void) {
  return register_chrdev(195, "testdriver", &fops);
}

module_init(testdriver_init_module);
MODULE_LICENSE("GPL");
