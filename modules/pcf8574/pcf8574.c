#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

#define DRIVER_NAME "PCF8574_sample"
#define PCF8574_DEV "pcf8574_sample"
#define PCF8574_ADDR 0x38
#define PCF8574_BUS 1

static struct i2c_board_info pcf8574_board_info = {I2C_BOARD_INFO(PCF8574_DEV, PCF8574_ADDR)};
static struct i2c_client *pcf8574_i2c_client;
static struct class *pcf8574_class;

static dev_t first;
static unsigned int count = 1;
static struct cdev *pcf8574_cdev;

static int pcf8574_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk(KERN_INFO "%s: %s registered for device at address 0x%X\n", DRIVER_NAME, client->name, client->addr);
	return 0;
}

static int pcf8574_remove(struct i2c_client *client)
{
	printk(KERN_INFO "%s unregistered for device at address 0x%X\n", client->name, client->addr);
	return 0;
}

static const struct i2c_device_id pcf8574_id[] = {
	{PCF8574_DEV, 0},
	{},
};

static struct i2c_driver pcf8574_i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.id_table = pcf8574_id,
	.probe = pcf8574_probe,
	.remove = pcf8574_remove,
};

static int pcf8574_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int pcf8574_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t pcf8574_read(struct file *file, char __user *buf, size_t lbuf, loff_t *ppos)
{
	char buf_rd[1];
	char to_user[8];
	int nbytes;
	struct i2c_msg msg[] =
	{
		{
			.addr = pcf8574_i2c_client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf_rd,
		},
	};
	i2c_transfer(pcf8574_i2c_client->adapter, msg, 1);
	sprintf(to_user, "%X", (int)buf_rd[0]);
	nbytes = copy_to_user(buf, to_user, lbuf);
	printk(KERN_INFO "%s: %i %s %i\n", DRIVER_NAME, (int)buf_rd[0], to_user, nbytes);
	return 0;
}

static ssize_t pcf8574_write(struct file *file, const char __user *buf, size_t lbuf, loff_t *ppos)
{
	char buf_wr[1];
	char from_user[8];
	unsigned int val;
	int nbytes;
	struct i2c_msg msg[] =
	{
		{
			.addr = pcf8574_i2c_client->addr,
			.flags = 0x00,
			.len = 1,
			.buf = buf_wr,
		},
	};
	nbytes = copy_from_user(from_user, buf, lbuf);
	sscanf(buf, "%X", &val);
	buf_wr[0] = (char)val;
	i2c_transfer(pcf8574_i2c_client->adapter, msg, 1);
	printk(KERN_INFO "%s: %s %i %i", DRIVER_NAME, from_user, val, nbytes);
	return lbuf;
}

static const struct file_operations pcf8574_fops = {
	.owner = THIS_MODULE,
	.read = pcf8574_read,
	.write = pcf8574_write,
	.open = pcf8574_open,
	.release = pcf8574_release,
};

int __init pcf8574_init(void)
{
	struct i2c_adapter *adapter;

	/* i2c registering */
	if(i2c_add_driver(&pcf8574_i2c_driver)){
		printk(KERN_ERR "%s: i2c_add_driver failed\n", DRIVER_NAME);
		goto err1;
	}

	adapter = i2c_get_adapter(PCF8574_BUS);
	if(adapter == NULL){
		printk(KERN_ERR "%s: i2c_get_adapter for bus %i failed\n", DRIVER_NAME, PCF8574_BUS);
		goto err2;
	}

	pcf8574_i2c_client = i2c_new_device(adapter, &pcf8574_board_info);
	if(pcf8574_i2c_client == NULL){
		printk(KERN_ERR "%s: i2c_new_device failed\n", DRIVER_NAME);
		goto err3;
	}

	/* character device registering */
	if(alloc_chrdev_region(&first, 0, count, PCF8574_DEV) < 0)
	{
		printk(KERN_ERR "%s: alloc_chrdev_region failed\n", DRIVER_NAME);
		goto err4;
	}

	if(!(pcf8574_cdev = cdev_alloc()))
	{
		printk(KERN_ERR "%s: cdev_alloc() failed\n", DRIVER_NAME);
		goto err5;
	}

	cdev_init(pcf8574_cdev, &pcf8574_fops);
	if(cdev_add(pcf8574_cdev, first, count) < 0)
	{
		printk(KERN_ERR "cdev_add() failed\n");
		goto err6;
	}

	pcf8574_class = class_create(THIS_MODULE, PCF8574_DEV);
	if(pcf8574_class == NULL)
	{
		printk(KERN_INFO "%s: class_create() failed\n", DRIVER_NAME);
		goto err6;
	}
	device_create(pcf8574_class, NULL, first, NULL, PCF8574_DEV);

	i2c_put_adapter(adapter);
	printk(KERN_INFO "%s: registered\n", DRIVER_NAME);
	return 0;

	err6:
	cdev_del(pcf8574_cdev);
	err5:
	unregister_chrdev_region(first, count);
	err4:
	i2c_unregister_device(pcf8574_i2c_client);
	err3:
	i2c_put_adapter(adapter);
	err2:
	i2c_del_driver(&pcf8574_i2c_driver);
	err1:
	return -1;
}

void __exit pcf8574_exit(void)
{
	device_destroy(pcf8574_class, first);
	class_destroy(pcf8574_class);
	if(pcf8574_cdev)cdev_del(pcf8574_cdev);
	unregister_chrdev_region(first, count);
	
	i2c_unregister_device(pcf8574_i2c_client);
	i2c_del_driver(&pcf8574_i2c_driver);
	printk(KERN_INFO "%s: unregistered\n", DRIVER_NAME);
}

module_init(pcf8574_init);
module_exit(pcf8574_exit);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("PCF8574 GPIO expander driver");
MODULE_LICENSE("GPL");
