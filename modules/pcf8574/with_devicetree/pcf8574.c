#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>

#define COMPATIBLE "pcf8574sample"
#define DRIVER "PCF8574SAMPLE"

//driver needs device tree entry in order to work, overlay need to be loaded

static ssize_t pins_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned int pins;
	pins = i2c_smbus_read_byte(client);
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
	.name = COMPATIBLE,
	.attrs = pcf8574_attr,
};

static int pcf8574_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	err = device_create_file(&client->dev, &dev_attr_pins_state);
	if(err)
	{
		printk(KERN_ERR "%s: %s: cannot create sysfs entry(%i)\n", DRIVER, __func__, err);
		return err;
	}
	printk(KERN_INFO "%s: %s: device address 0x%X\n", DRIVER, __func__, client->addr);
	return 0;
}

static int pcf8574_remove(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_pins_state);
	printk(KERN_INFO "%s: %s: device address 0x%X\n", DRIVER, __func__, client->addr);
	return 0;
}

static const struct i2c_device_id pcf8574_id[] = {
	{COMPATIBLE, 0},
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

module_i2c_driver(pcf8574_i2c_driver);

MODULE_DEVICE_TABLE(i2c, pcf8574_id);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("PCF8574 IO expander driver");
MODULE_LICENSE("GPL");
