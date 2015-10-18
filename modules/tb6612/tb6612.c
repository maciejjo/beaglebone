#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/string.h>

#define DRVNAME "TB6612"

#define AIN1  66
#define AIN2  69
#define BIN1  47
#define BIN2  27
#define STDBY 45
#define PWMA   6 /* P8_13 */
#define PWMB   5 /* P8_19 */
#define MOTOR_PWM_PERIOD 10000000
#define MOTOR_PWM_DUTY_MUL 100000
#define MODE_SIZE 5

static unsigned int motora_speed = 0, motorb_speed = 0;
static char motora_mode[MODE_SIZE], motorb_mode[MODE_SIZE];
static unsigned char standby = 1;
static struct class *tb6612_class;
static struct pwm_device *motora_pwm;
static struct pwm_device *motorb_pwm;

/* show and store functions declarations */
static ssize_t motora_speed_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t motora_speed_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static ssize_t motora_mode_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t motora_mode_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);

static ssize_t motorb_speed_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t motorb_speed_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);
static ssize_t motorb_mode_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t motorb_mode_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);

static ssize_t tb6612_standby_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t tb6612_standby_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count);

/* attributes */
static struct class_attribute motora_speed_attr = __ATTR(motora_speed, 0660, motora_speed_show, motora_speed_store);
static struct class_attribute motorb_speed_attr = __ATTR(motorb_speed, 0660, motorb_speed_show, motorb_speed_store);
static struct class_attribute motora_mode_attr = __ATTR(motora_mode, 0660, motora_mode_show, motora_mode_store);
static struct class_attribute motorb_mode_attr = __ATTR(motorb_mode, 0660, motorb_mode_show, motorb_mode_store);
static struct class_attribute tb6612_standby_attr = __ATTR(standby, 0660, tb6612_standby_show, tb6612_standby_store);

static ssize_t motora_speed_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", motora_speed);
}

static ssize_t motora_speed_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	if(tmp < 0 || tmp > 100)return -EINVAL;
	motora_speed = tmp;
	pwm_config(motora_pwm, motora_speed * MOTOR_PWM_DUTY_MUL, MOTOR_PWM_PERIOD);
	return count;
}

static ssize_t motorb_speed_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%u", motorb_speed);
}

static ssize_t motorb_speed_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	if(tmp < 0 || tmp > 100)return -EINVAL;
	motorb_speed = tmp;
	pwm_config(motorb_pwm, motorb_speed * MOTOR_PWM_DUTY_MUL, MOTOR_PWM_PERIOD);
	return count;
}


static ssize_t motora_mode_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", motora_mode);
}

static ssize_t motora_mode_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int len;
	len = strlen(buf);
	if(buf[len - 1] == '\n')len--;
	if(len > 4)return -EINVAL;
	if(strncmp(buf, "stop", len) != 0 && strncmp(buf, "cw", len) != 0 && strncmp(buf, "ccw", len) != 0)return -EINVAL;
	memset(motora_mode, '\0', MODE_SIZE);
	strncpy(motora_mode, buf, len);
	if(strcmp(motora_mode, "stop") == 0){
		gpio_set_value(AIN1, 0);
		gpio_set_value(AIN2, 0);
	}
	else if(strcmp(motora_mode, "cw") == 0){
		gpio_set_value(AIN1, 0);
		gpio_set_value(AIN2, 1);
	}
	else if(strcmp(motora_mode, "ccw") == 0){
		gpio_set_value(AIN1, 1);
		gpio_set_value(AIN2, 0);
	}
	else return -EINVAL;
	return count;
}

static ssize_t motorb_mode_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", motorb_mode);
}

static ssize_t motorb_mode_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	int len;
	len = strlen(buf);
	if(buf[len - 1] == '\n')len--;
	if(len > 4)return -EINVAL;
	if(strncmp(buf, "stop", len) != 0 && strncmp(buf, "cw", len) != 0 && strncmp(buf, "ccw", len) != 0)return -EINVAL;
	memset(motorb_mode, '\0', MODE_SIZE);
	strncpy(motorb_mode, buf, len);
	if(strcmp(motorb_mode, "stop") == 0){
		gpio_set_value(BIN1, 0);
		gpio_set_value(BIN2, 0);
	}
	else if(strcmp(motorb_mode, "cw") == 0){
		gpio_set_value(BIN1, 0);
		gpio_set_value(BIN2, 1);
	}
	else if(strcmp(motorb_mode, "ccw") == 0){
		gpio_set_value(BIN1, 1);
		gpio_set_value(BIN2, 0);
	}
	else return -EINVAL;
	return count;

}


static ssize_t tb6612_standby_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%i", standby);
}

static ssize_t tb6612_standby_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int tmp;
	sscanf(buf, "%u", &tmp);
	if(tmp > 1)tmp = 1;
	standby = tmp;
	gpio_set_value(STDBY, !standby);
	return count;
}

static int __init tb6612_init(void)
{
	/* create entried in sysfs */
	tb6612_class = class_create(THIS_MODULE, "tb6612");
	if(tb6612_class == NULL){
		printk(KERN_ERR "%s: Cannot create entry in sysfs", DRVNAME);
		return -1;
	}
	
	if(class_create_file(tb6612_class, &motora_speed_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err1;
	}
	
	if(class_create_file(tb6612_class, &motorb_speed_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err2;
	}
	if(class_create_file(tb6612_class, &motora_mode_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err3;
	}
	if(class_create_file(tb6612_class, &motorb_mode_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err4;
	}
	if(class_create_file(tb6612_class, &tb6612_standby_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err5;
	}
	
	/* configure gpio and pwm outputs */
	/* Motor A direction regulation */
	gpio_request(AIN1, "motora_in1");
	gpio_direction_output(AIN1, 0);
	gpio_export(AIN1, 0);

	gpio_request(AIN2, "motora_in2");
	gpio_direction_output(AIN2, 0);
	gpio_export(AIN2, 0);

	/* Motor B direction regulation */
	gpio_request(BIN1, "motorb_in1");
	gpio_direction_output(BIN1, 0);
	gpio_export(BIN1, 0);

	gpio_request(BIN2, "motorb_in2");
	gpio_direction_output(BIN2, 0);
	gpio_export(BIN2, 0);

	/* Standby regulation */
	gpio_request(STDBY, "standby");
	gpio_direction_output(STDBY, !standby);
	gpio_export(STDBY, 0);

	/* Motor A speed regulation */
	motora_pwm = pwm_request(PWMA, "motora_pwm");
	if(motora_pwm == NULL)
	{
		printk(KERN_ERR "%s: Cannot use PWM output P8_13\n", DRVNAME);
		goto err6;
	}
	pwm_config(motora_pwm, motora_speed * MOTOR_PWM_DUTY_MUL, MOTOR_PWM_PERIOD);
	pwm_set_polarity(motora_pwm, PWM_POLARITY_NORMAL);
	pwm_enable(motora_pwm);
	
	/* Motor B speed regulation */
	motorb_pwm = pwm_request(PWMB, "motorb_pwm");
	if(motorb_pwm == NULL)
	{
		printk(KERN_ERR "%s: Cannot use PWM output P8_19\n", DRVNAME);
		goto err7;
	}
	pwm_config(motorb_pwm, motorb_speed * MOTOR_PWM_DUTY_MUL, MOTOR_PWM_PERIOD);
	pwm_set_polarity(motorb_pwm, PWM_POLARITY_NORMAL);
	pwm_enable(motorb_pwm);
	
	/* current mode of motors  */
	strcpy(motora_mode, "stop");
	strcpy(motorb_mode, "stop");

	return 0;
	err7:
	pwm_config(motora_pwm, 0, 0);
	pwm_disable(motora_pwm);
	pwm_free(motora_pwm);
	err6:
	gpio_set_value(STDBY, 0);
	gpio_unexport(STDBY);
	gpio_free(STDBY);
	gpio_set_value(BIN2, 0);
	gpio_unexport(BIN2);
	gpio_free(BIN2);
	gpio_set_value(BIN1, 0);
	gpio_unexport(BIN1);
	gpio_free(BIN1);
	gpio_set_value(AIN2, 0);
	gpio_unexport(AIN2);
	gpio_free(AIN2);
	gpio_set_value(AIN1, 0);
	gpio_unexport(AIN1);
	gpio_free(AIN1);
	class_remove_file(tb6612_class, &tb6612_standby_attr);
	err5:
	class_remove_file(tb6612_class, &motorb_mode_attr);
	err4:
	class_remove_file(tb6612_class, &motora_mode_attr);
	err3:
	class_remove_file(tb6612_class, &motorb_speed_attr);
	err2:
	class_remove_file(tb6612_class, &motora_speed_attr);
	err1:
	class_destroy(tb6612_class);
	return -1;
}

static void __exit tb6612_exit(void)
{
	pwm_config(motorb_pwm, 0, 0);
	pwm_disable(motorb_pwm);
	pwm_free(motorb_pwm);

	pwm_config(motora_pwm, 0, 0);
	pwm_disable(motora_pwm);
	pwm_free(motora_pwm);

	gpio_set_value(STDBY, 0);
	gpio_unexport(STDBY);
	gpio_free(STDBY);

	gpio_set_value(BIN2, 0);
	gpio_unexport(BIN2);
	gpio_free(BIN2);

	gpio_set_value(BIN1, 0);
	gpio_unexport(BIN1);
	gpio_free(BIN1);

	gpio_set_value(AIN2, 0);
	gpio_unexport(AIN2);
	gpio_free(AIN2);

	gpio_set_value(AIN1, 0);
	gpio_unexport(AIN1);
	gpio_free(AIN1);

	class_remove_file(tb6612_class, &tb6612_standby_attr);
	class_remove_file(tb6612_class, &motorb_mode_attr);
	class_remove_file(tb6612_class, &motora_mode_attr);
	class_remove_file(tb6612_class, &motorb_speed_attr);
	class_remove_file(tb6612_class, &motora_speed_attr);
	class_destroy(tb6612_class);
}

module_init(tb6612_init);
module_exit(tb6612_exit);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("TB6612 H-bridge driver");
MODULE_LICENSE("GPL");
