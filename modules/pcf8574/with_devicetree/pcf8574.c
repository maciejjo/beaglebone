#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/fs.h>

#define PCF8574 "pcf8574sample"
#define DRIVER "PCF8574SAMPLE"

//driver needs device tree entry in order to work, overlay need to be loaded

struct pcf8574_device
{
	dev_t cdev;
	struct device *dev;
	char name[16];
};

static struct class *pcf8574_class;

static ssize_t pins_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int pins;
	pins = i2c_smbus_read_byte(client);
	if(pins < 0)
	{
		printk(KERN_ERR "%s: %s: i2c_smbus_read_byte failed(%i)\n", DRIVER, __func__, pins);
		return pins;
	}
	printk(KERN_INFO "%s: %s: addr: 0x%X\n", DRIVER, __func__, client->addr);
	return sprintf(buf, "%X", pins);
}

static ssize_t pins_state_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned int pins;
	int ret;

	sscanf(buf, "%X", &pins);
	if(pins < 0 || pins > 0xFF)
	{
		printk(KERN_ERR "%s: %s: invalid argument\n", DRIVER, __func__);
		return -EINVAL;
	}

	ret = i2c_smbus_write_byte(client, (unsigned char)pins);
	if(ret)
	{
		printk(KERN_ERR "%s: %s: i2c_smbus_write_byte failed(%i)\n", DRIVER, __func__, ret);
		return ret;
	}
	return count;
}

static DEVICE_ATTR(pins_state, S_IWUSR | S_IRUGO, pins_state_show, pins_state_store);

static struct attribute *pcf8574_attr[] = {
	&dev_attr_pins_state.attr,
	NULL,
};

static const struct attribute_group pcf8574_attr_group = {
	.name = PCF8574,
	.attrs = pcf8574_attr,
};

static int pcf8574_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	struct pcf8574_device *pcf8574_dev;
	static int number = 0;

	pcf8574_dev = kzalloc(sizeof(struct pcf8574_device), GFP_KERNEL);
	if(!pcf8574_dev)
	{
		printk(KERN_ERR "%s: %s: cannot allocate memory\n", DRIVER, __func__);
		return -1;
	}

	sprintf(pcf8574_dev->name, "pcf8574_%i", number++);
	err = alloc_chrdev_region(&pcf8574_dev->cdev, 0, 1, pcf8574_dev->name);
	if(err)
	{
		printk(KERN_ERR "%s: %s: alloc_chrdev_region failed(%i)\n", DRIVER, __func__, err);
		goto err1;
	}

	pcf8574_dev->dev = device_create(pcf8574_class, &client->dev, pcf8574_dev->cdev, NULL, pcf8574_dev->name);
	if(IS_ERR(pcf8574_dev->dev))
	{
		printk(KERN_ERR "%s: %s: cannot create device\n", DRIVER, __func__);
		err = -1;
		goto err2;
	}

	err = device_create_file(pcf8574_dev->dev, &dev_attr_pins_state);
	if(err)
	{
		printk(KERN_ERR "%s: %s: cannot create sysfs entry(%i)\n", DRIVER, __func__, err);
		goto err3;
	}

	i2c_set_clientdata(client, pcf8574_dev);

	printk(KERN_INFO "%s: %s: device address 0x%X\n", DRIVER, __func__, client->addr);
	return 0;

	err3:
	device_destroy(pcf8574_class, pcf8574_dev->cdev);
	err2:
	unregister_chrdev_region(pcf8574_dev->cdev, 1);
	err1:
	kfree(pcf8574_dev);
	return err;
}

static int pcf8574_remove(struct i2c_client *client)
{
	struct pcf8574_device *pcf8574_dev = i2c_get_clientdata(client);

	device_remove_file(pcf8574_dev->dev, &dev_attr_pins_state);
	device_destroy(pcf8574_class, pcf8574_dev->cdev);
	unregister_chrdev_region(pcf8574_dev->cdev, 1);
	kfree(pcf8574_dev);
	i2c_set_clientdata(client, NULL);
	printk(KERN_INFO "%s: %s: device address 0x%X\n", DRIVER, __func__, client->addr);
	return 0;
}

static const struct i2c_device_id pcf8574_id[] = {
	{PCF8574, 0},
	{},
};

static struct i2c_driver pcf8574_i2c_driver = {
	.driver = {
		.name = DRIVER,
		.owner = THIS_MODULE,
	},
	.id_table = pcf8574_id,
	.probe = pcf8574_probe,
	.remove = pcf8574_remove,
};

static int __init pcf8574_init(void)
{
	int err;

	pcf8574_class = class_create(THIS_MODULE, PCF8574);
	if(!pcf8574_class)
	{
		printk(KERN_ERR "%s: %s: cannot create class\n", DRIVER, __func__);
		return -1;
	}

	err = i2c_add_driver(&pcf8574_i2c_driver);
	if(err)
	{
		printk(KERN_ERR "%s: %s: cannot add i2c driver(%i)\n", DRIVER, __func__, err);
		class_destroy(pcf8574_class);
		return err;
	}

	return 0;
}

static void __exit pcf8574_exit(void)
{
	i2c_del_driver(&pcf8574_i2c_driver);
	class_destroy(pcf8574_class);
}

module_init(pcf8574_init);
module_exit(pcf8574_exit);

MODULE_DEVICE_TABLE(i2c, pcf8574_id);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("PCF8574 IO expander driver");
MODULE_LICENSE("GPL");
