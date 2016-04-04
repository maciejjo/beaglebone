#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/string.h>

/* common driver defines */
#define DRIVER_NAME "DS3231_SAMPLE"
/* it can't be called just ds3231 because ds1307 driver will probe it */
#define DS3231_DEV "ds3231_sample"
#define DS3231_ADDR 0x68
#define DS3231_BUS 1

#define UPPER_NIBBLE(x) ((x & 0xF0) >> 4)
#define LOWER_NIBBLE(x) (x & 0x0F)

/* DS3231 register defines */
#define SECOND_REG 0x00
#define MINUTE_REG 0x01
#define HOUR_REG 0x02
#define DAY_REG 0x03
#define DATE_REG 0x04
#define MONTH_REG 0x05
#define YEAR_REG 0x06

static struct i2c_board_info ds3231_board_info = {I2C_BOARD_INFO(DS3231_DEV, DS3231_ADDR)};
static struct i2c_client *ds3231_i2c_client;
static struct class *ds3231_class;

/* common functions */
static void write_reg(struct i2c_client *client, char reg, char val)
{
	char buf_wr[2];
	struct i2c_msg msg[] =
	{
		{
			.addr = client->addr,
			.flags = 0x00,
			.len = 2,
			.buf = buf_wr,
		},
	};
	buf_wr[0] = reg;
	buf_wr[1] = val;
	i2c_transfer(ds3231_i2c_client->adapter, msg, 1);
}

static char read_reg(struct i2c_client *client, char reg)
{
	char buf_rd[1];
	char buf_wr[1] = {reg};
	struct i2c_msg msg[] =
	{
		{
			.addr = client->addr,
			.flags = 0x00,
			.len = 1,
			.buf = buf_wr,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = buf_rd,
		},
	};
	i2c_transfer(client->adapter, msg, 2);
	return buf_rd[0];
}

/* sysfs attributes */
static ssize_t sec_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t sec_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute sec_attr = __ATTR(second, 0660, sec_show, sec_store);

static ssize_t min_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t min_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute min_attr = __ATTR(minute, 0660, min_show, min_store);

static ssize_t hour_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t hour_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute hour_attr = __ATTR(hour, 0660, hour_show, hour_store);

static ssize_t day_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t day_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute day_attr = __ATTR(day, 0660, day_show, day_store);

static ssize_t date_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t date_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute date_attr = __ATTR(date, 0660, date_show, date_store);

static ssize_t month_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t month_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute month_attr = __ATTR(month, 0660, month_show, month_store);

static ssize_t year_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t year_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static struct class_attribute year_attr = __ATTR(year, 0660, year_show, year_store);

static ssize_t sec_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%X", read_reg(ds3231_i2c_client, SECOND_REG));
}

static ssize_t sec_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int sec;

	sscanf(buf, "%X", &sec);
	if(UPPER_NIBBLE(sec) > 5 || LOWER_NIBBLE(sec) > 9)
		return -EINVAL;

	write_reg(ds3231_i2c_client, SECOND_REG, (char)sec);
	return count;
}

static ssize_t min_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%X", read_reg(ds3231_i2c_client, MINUTE_REG));
}

static ssize_t min_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int min;

	sscanf(buf, "%X", &min);
	if(UPPER_NIBBLE(min) > 5 || LOWER_NIBBLE(min) > 9)
		return -EINVAL;

	write_reg(ds3231_i2c_client, MINUTE_REG, (char)min);
	return count;
}

static ssize_t hour_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%X", read_reg(ds3231_i2c_client, HOUR_REG));
}

static ssize_t hour_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int hour;
	char upper, lower;

	sscanf(buf, "%X", &hour);
	upper = UPPER_NIBBLE(hour);
	lower = LOWER_NIBBLE(hour);
	if(upper > 2 || (upper == 2 && lower > 4) || lower > 9)
		return -EINVAL;

	write_reg(ds3231_i2c_client, HOUR_REG, (char)hour);
	return count;
}

static ssize_t day_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	char day;
	char day_name[16];
	day = read_reg(ds3231_i2c_client, DAY_REG);
	switch(day)
	{
		case 1:
			strcpy(day_name, "Monday");
			break;
		case 2:
			strcpy(day_name, "Tuseday");
			break;
		case 3:
			strcpy(day_name, "Wednesday");
			break;
		case 4:
			strcpy(day_name, "Thursday");
			break;
		case 5:
			strcpy(day_name, "Friday");
			break;
		case 6:
			strcpy(day_name, "Saturday");
			break;
		case 7:
			strcpy(day_name, "Sunday");
			break;
		default:
			strcpy(day_name, "Unkown");
	}
	return sprintf(buf, "%s", day_name);
}

static ssize_t day_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	char day_buf[count];
	int i, day;
	if(buf[count - 1] == '\n')
	{
		strncpy(day_buf, buf, count - 1);
		day_buf[count - 1] = '\0';
	}
	else
		strcpy(day_buf, buf);

	/*to lower case */
	for(i = 0; i < count; i++)
		if(day_buf[i] >= 65 && day_buf[i] <= 90)day_buf[i] += 32;

	if(strcmp(day_buf, "monday") == 0)day = 1;
	else if(strcmp(day_buf, "tuseday") == 0)day = 2;
	else if(strcmp(day_buf, "wednesday") == 0)day = 3;
	else if(strcmp(day_buf, "thursday") == 0)day = 4;
	else if(strcmp(day_buf, "friday") == 0)day = 5;
	else if(strcmp(day_buf, "saturday") == 0)day = 6;
	else if(strcmp(day_buf, "sunday") == 0)day = 7;
	else return -EINVAL;

	write_reg(ds3231_i2c_client, DAY_REG, (char)day);
	return count;
}

static ssize_t date_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%X", read_reg(ds3231_i2c_client, DATE_REG));
}

static ssize_t date_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int date;
	char upper, lower;

	sscanf(buf, "%X", &date);
	upper = UPPER_NIBBLE(date);
	lower = LOWER_NIBBLE(date);
	if(upper > 3 || (upper == 3 && lower > 1) || lower > 9 || date == 0)
		return -EINVAL;

	write_reg(ds3231_i2c_client, DATE_REG, (char)date);
	return count;
}

static ssize_t month_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	char month;
	char month_name[16];
	month = read_reg(ds3231_i2c_client, MONTH_REG);
	switch(month)
	{
		case 0x01:
			strcpy(month_name, "January");
			break;
		case 0x02:
			strcpy(month_name, "February");
			break;
		case 0x03:
			strcpy(month_name, "March");
			break;
		case 0x04:
			strcpy(month_name, "April");
			break;
		case 0x05:
			strcpy(month_name, "May");
			break;
		case 0x06:
			strcpy(month_name, "June");
			break;
		case 0x07:
			strcpy(month_name, "July");
			break;
		case 0x08:
			strcpy(month_name, "August");
			break;
		case 0x09:
			strcpy(month_name, "September");
			break;
		case 0x10:
			strcpy(month_name, "October");
			break;
		case 0x11:
			strcpy(month_name, "November");
			break;
		case 0x12:
			strcpy(month_name, "December");
			break;
		default:
			strcpy(month_name, "Unkown");
	}
	return sprintf(buf, "%s", month_name);
}

static ssize_t month_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	char month_buf[count];
	int i, month;
	if(buf[count - 1] == '\n')
	{
		strncpy(month_buf, buf, count - 1);
		month_buf[count - 1] = '\0';
	}
	else
		strcpy(month_buf, buf);

	/*to lower case */
	for(i = 0; i < count; i++)
		if(month_buf[i] >= 65 && month_buf[i] <= 90)month_buf[i] += 32;

	printk(KERN_INFO "%s: %s", DRIVER_NAME, month_buf);
	if(strcmp(month_buf, "januart") == 0)month = 0x01;
	else if(strcmp(month_buf, "february") == 0)month = 0x02;
	else if(strcmp(month_buf, "march") == 0)month = 0x03;
	else if(strcmp(month_buf, "april") == 0)month = 0x04;
	else if(strcmp(month_buf, "may") == 0)month = 0x05;
	else if(strcmp(month_buf, "june") == 0)month = 0x06;
	else if(strcmp(month_buf, "july") == 0)month = 0x07;
	else if(strcmp(month_buf, "august") == 0)month = 0x08;
	else if(strcmp(month_buf, "september") == 0)month = 0x09;
	else if(strcmp(month_buf, "october") == 0)month = 0x10;
	else if(strcmp(month_buf, "november") == 0)month = 0x11;
	else if(strcmp(month_buf, "december") == 0)month = 0x12;
	else return -EINVAL;

	write_reg(ds3231_i2c_client, MONTH_REG, (char)month);
	return count;
	return count;
}

static ssize_t year_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%X", read_reg(ds3231_i2c_client, YEAR_REG));
}

static ssize_t year_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int year;

	sscanf(buf, "%X", &year);
	if(UPPER_NIBBLE(year) > 9 || LOWER_NIBBLE(year) > 9)
		return -EINVAL;

	write_reg(ds3231_i2c_client, YEAR_REG, (char)year);
	return count;
}

static int ds3231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk(KERN_INFO "%s: %s registered for device at address 0x%X\n", DRIVER_NAME, client->name, client->addr);
	return 0;
}

static int ds3231_remove(struct i2c_client *client)
{
	printk(KERN_INFO "%s unregistered for device at address 0x%X", client->name, client->addr);
	return 0;
}

static const struct i2c_device_id ds3231_id[] = {
	{DS3231_DEV, 0},
	{},
};

static struct i2c_driver ds3231_i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.id_table = ds3231_id,
	.probe = ds3231_probe,
	.remove = ds3231_remove,
};

int __init ds3231_init(void)
{
	int ret;
	struct i2c_adapter *adapter;

	/* i2c registering */
	ret = i2c_add_driver(&ds3231_i2c_driver);
	if(ret){
		printk(KERN_ERR "%s: i2c_add_driver failed\n", DRIVER_NAME);
		goto err1;
	}

	adapter = i2c_get_adapter(DS3231_BUS);
	if(adapter == NULL){
		printk(KERN_ERR "%s: i2c_get_adapter fo bus %i failed\n", DRIVER_NAME, DS3231_BUS);
		ret = -1;
		goto err2;
	}

	ds3231_i2c_client = i2c_new_device(adapter, &ds3231_board_info);
	if(ds3231_i2c_client == NULL){
		printk(KERN_ERR "%s: i2c_new_device failed\n", DRIVER_NAME);
		ret = -1;
		goto err3;
	}

	/* class creation */
	ds3231_class = class_create(THIS_MODULE, "ds3231");
	if(ds3231_class == NULL){
		printk(KERN_ERR "%s: Cannot create entry in sysfs", DRIVER_NAME);
		ret = -1;
		goto err3;
	}

	if(class_create_file(ds3231_class, &sec_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err4;
	}

	if(class_create_file(ds3231_class, &min_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err5;
	}

	if(class_create_file(ds3231_class, &hour_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err6;
	}

	if(class_create_file(ds3231_class, &day_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err7;
	}

	if(class_create_file(ds3231_class, &date_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err8;
	}

	if(class_create_file(ds3231_class, &month_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err9;
	}

	if(class_create_file(ds3231_class, &year_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRIVER_NAME);
		goto err10;
	}

	i2c_put_adapter(adapter);
	printk(KERN_INFO "%s: module load succeeded\n", DRIVER_NAME);
	return 0;

	err10:
	class_remove_file(ds3231_class, &month_attr);
	err9:
	class_remove_file(ds3231_class, &date_attr);
	err8:
	class_remove_file(ds3231_class, &day_attr);
	err7:
	class_remove_file(ds3231_class, &hour_attr);
	err6:
	class_remove_file(ds3231_class, &min_attr);
	err5:
	class_remove_file(ds3231_class, &sec_attr);
	err4:
	class_destroy(ds3231_class);
	err3:
	i2c_put_adapter(adapter);
	err2:
	i2c_del_driver(&ds3231_i2c_driver);
	err1:
	return ret;
}

void __exit ds3231_exit(void)
{
	class_remove_file(ds3231_class, &year_attr);
	class_remove_file(ds3231_class, &month_attr);
	class_remove_file(ds3231_class, &date_attr);
	class_remove_file(ds3231_class, &day_attr);
	class_remove_file(ds3231_class, &hour_attr);
	class_remove_file(ds3231_class, &min_attr);
	class_remove_file(ds3231_class, &sec_attr);
	class_destroy(ds3231_class);
	if(ds3231_i2c_client != NULL){
		i2c_unregister_device(ds3231_i2c_client);
		ds3231_i2c_client = NULL;
	}
	i2c_del_driver(&ds3231_i2c_driver);
	printk(KERN_INFO "%s: module unload succeeded\n", DRIVER_NAME);
}

module_init(ds3231_init);
module_exit(ds3231_exit);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("DS3231 driver");
MODULE_LICENSE("GPL");
