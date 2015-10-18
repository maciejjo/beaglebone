#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/delay.h>

#define DRVNAME "A4988"
#define DIR_SIZE 4

#define PINS_AMOUNT 8

#define ENABLE_PIN 67
#define MS1_PIN 68
#define MS2_PIN 44
#define MS3_PIN 26
#define RESET_PIN 46
#define SLEEP_PIN 65
#define STEP_PIN 61
#define DIR_PIN 88

DEFINE_MUTEX(step_mutex);

static unsigned int enable = 1, sleep = 0, reset = 0, ustep = 1, steps = 0;
static char direction[DIR_SIZE];
static struct class *a4988_class;
int pins[] = {ENABLE_PIN, MS1_PIN, MS2_PIN, MS3_PIN, RESET_PIN, SLEEP_PIN, STEP_PIN, DIR_PIN};
int pins_init_val[] = {0, 0, 0, 0, 1, 1, 0, 0};
char *pins_names[] = {"enable", "ms1", "ms2", "ms3", "reset", "sleep", "step", "dir"};

/* show and store functions declarations */
static ssize_t enable_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", enable);
}
static ssize_t enable_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	if(tmp > 1)tmp = 1;
	enable = tmp;
	gpio_set_value(ENABLE_PIN, !enable);
	return count;
}

static ssize_t sleep_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", sleep);
}
static ssize_t sleep_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	if(tmp > 1)tmp = 1;
	sleep = tmp;
	gpio_set_value(SLEEP_PIN, !sleep);
	return count;
}

static ssize_t reset_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", reset);
}
static ssize_t reset_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	if(tmp > 1)tmp = 1;
	reset = tmp;
	gpio_set_value(RESET_PIN, !reset);
	return count;
}

static ssize_t ustep_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", ustep);
}
static ssize_t ustep_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	switch(tmp)
	{
		case 1:
			gpio_set_value(MS1_PIN, 0);
			gpio_set_value(MS2_PIN, 0);
			gpio_set_value(MS3_PIN, 0);
			break;
		case 2:
			gpio_set_value(MS1_PIN, 1);
			gpio_set_value(MS2_PIN, 0);
			gpio_set_value(MS3_PIN, 0);
			break;
		case 4:
			gpio_set_value(MS1_PIN, 0);
			gpio_set_value(MS2_PIN, 1);
			gpio_set_value(MS3_PIN, 0);
			break;
		case 8:
			gpio_set_value(MS1_PIN, 1);
			gpio_set_value(MS2_PIN, 1);
			gpio_set_value(MS3_PIN, 0);
			break;
		case 16:
			gpio_set_value(MS1_PIN, 1);
			gpio_set_value(MS2_PIN, 1);
			gpio_set_value(MS3_PIN, 1);
			break;
		default:
			return -EINVAL;
	}
	ustep = tmp;
	return count;
}

static ssize_t steps_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", steps);
}
static ssize_t steps_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp, i;
	if(mutex_trylock(&step_mutex))
	{
		sscanf(buf, "%u", &tmp);
		steps = tmp;
		/*wykonanie odpowiedniej ilości kroków*/
		for(i = 0; i < tmp; i++)
		{
			gpio_set_value(STEP_PIN, 1);
			mdelay(1);
			gpio_set_value(STEP_PIN, 0);
			mdelay(1);
			gpio_set_value(STEP_PIN, 1);
			mdelay(1);
			gpio_set_value(STEP_PIN, 0);
			msleep(1);
			if(i % 512 == 0)msleep(10);
			steps--;
		}
		mutex_unlock(&step_mutex);
		return count;
	}
	return -EBUSY;
}

static ssize_t direction_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", direction);
}
static ssize_t direction_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	size_t len;
	len = strlen(buf);
	if(buf[len - 1] == '\n')len--;
	if(len > DIR_SIZE)return -EINVAL;
	if(strncmp(buf, "cw", len) != 0 && strncmp(buf, "ccw", len) != 0)return -EINVAL;
	memset(direction, '\0', DIR_SIZE);
	strncpy(direction, buf, len);
	/*ustawienie wyjść*/
	if(strcmp(direction, "cw") == 0)gpio_set_value(DIR_PIN, 0);
	else if(strcmp(direction, "ccw") == 0)gpio_set_value(DIR_PIN, 1);
	else return -EINVAL;
	return count;
}

/* attributes */
static struct class_attribute enable_attr = __ATTR(enable, 0660, enable_show, enable_store);
static struct class_attribute sleep_attr = __ATTR(sleep, 0660, sleep_show, sleep_store);
static struct class_attribute reset_attr = __ATTR(reset, 0660, reset_show, reset_store);
static struct class_attribute ustep_attr = __ATTR(ustep, 0660, ustep_show, ustep_store);
static struct class_attribute steps_attr = __ATTR(steps, 0660, steps_show, steps_store);
static struct class_attribute direction_attr = __ATTR(direction, 0660, direction_show, direction_store);

static int __init a4988_init(void)
{
	int i, fail;
	/* create entries in sysfs */
	a4988_class = class_create(THIS_MODULE, "a4988");
	if(a4988_class == NULL){
		printk(KERN_ERR "%s: Cannot create entry in sysfs", DRVNAME);
		return -1;
	}
	
	if(class_create_file(a4988_class, &enable_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err1;
	}
	
	if(class_create_file(a4988_class, &sleep_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err2;
	}
	if(class_create_file(a4988_class, &reset_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err3;
	}
	if(class_create_file(a4988_class, &ustep_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err4;
	}
	if(class_create_file(a4988_class, &steps_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err5;
	}
	if(class_create_file(a4988_class, &direction_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err6;
	}
	for(i = 0; i < PINS_AMOUNT; i++)
	{
		if(gpio_request(pins[i], pins_names[i]) != 0){
			printk(KERN_ERR "%s: GPIO pin %i cannot be requested\n", DRVNAME, ENABLE_PIN);
			fail = i;
			goto err7;
		}
		gpio_direction_output(pins[i], pins_init_val[i]);
		gpio_export(pins[i], 0);
	}
	strcpy(direction, "cw");
	printk(KERN_INFO "%s: Module loaded\n", DRVNAME);
	return 0;
	err7:
	for(i = 0; i < fail; i++)
	{
		gpio_set_value(pins[i], 0);
		gpio_unexport(pins[i]);
		gpio_free(pins[i]);
	}
	class_remove_file(a4988_class, &direction_attr);
	err6:
	class_remove_file(a4988_class, &steps_attr);
	err5:
	class_remove_file(a4988_class, &ustep_attr);
	err4:
	class_remove_file(a4988_class, &reset_attr);
	err3:
	class_remove_file(a4988_class, &sleep_attr);
	err2:
	class_remove_file(a4988_class, &enable_attr);
	err1:
	class_destroy(a4988_class);
	return -1;
}

static void __exit a4988_exit(void)
{	
	int i;
	for(i = 0; i < PINS_AMOUNT; i++)
	{
		gpio_set_value(pins[i], 0);
		gpio_unexport(pins[i]);
		gpio_free(pins[i]);
	}
	class_remove_file(a4988_class, &direction_attr);
	class_remove_file(a4988_class, &steps_attr);
	class_remove_file(a4988_class, &ustep_attr);
	class_remove_file(a4988_class, &reset_attr);
	class_remove_file(a4988_class, &sleep_attr);
	class_remove_file(a4988_class, &enable_attr);
	class_destroy(a4988_class);
	printk(KERN_INFO "%s: Module unloaded\n", DRVNAME);
}

module_init(a4988_init);
module_exit(a4988_exit);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("A4988 steppermotor driver");
MODULE_LICENSE("GPL");
