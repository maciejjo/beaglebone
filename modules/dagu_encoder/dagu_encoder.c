#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>

#define DRVNAME			"DAGU ENCODER"
#define PIN_A			66
#define PIN_B			69
#define SIGS_PER_ROT	192
#define CIRCUMFERENCE	204204	/* in micrometers */
#define DIST_PER_SIG	1064	/* in micrometers */

static struct class *encoder_class;
atomic_t sigs_a = ATOMIC_INIT(0);
atomic_t sigs_b = ATOMIC_INIT(0);
atomic_t rots_a = ATOMIC_INIT(0);
atomic_t rots_b = ATOMIC_INIT(0);

/* show and store functions declarations */
static ssize_t distance_a_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	unsigned long long distance;
	distance = atomic_read(&rots_a) * CIRCUMFERENCE + atomic_read(&sigs_a) * DIST_PER_SIG;	/* distance micrometers */
	return sprintf(buf, "%llu", distance);
}

static ssize_t distance_b_show(struct class *cls, struct class_attribute *attr, char *buf)
{
	unsigned long long distance;
	distance = atomic_read(&rots_a) * CIRCUMFERENCE + atomic_read(&sigs_a) * DIST_PER_SIG;	/* distance in micrometers */
	return sprintf(buf, "%llu", distance);
}

static ssize_t reset_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
	atomic_set(&sigs_a, 0);
	atomic_set(&sigs_b, 0);
	atomic_set(&rots_a, 0);
	atomic_set(&rots_b, 0);
	return count;
}

/* attributes */
static struct class_attribute distance_a_attr = __ATTR(distance_a, 0660, distance_a_show, NULL);
static struct class_attribute distance_b_attr = __ATTR(distance_b, 0660, distance_b_show, NULL);
static struct class_attribute reset_attr = __ATTR(reset, 0660, NULL, reset_store);

/* interrupt service routines */
static irq_handler_t pin_a_irq(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	atomic_inc(&sigs_a);
	if(atomic_read(&sigs_a) == SIGS_PER_ROT){
		atomic_inc(&rots_a);
		atomic_set(&sigs_a, 0);
	}
	return (irq_handler_t)IRQ_HANDLED;
}

static irq_handler_t pin_b_irq(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	atomic_inc(&sigs_b);
	if(atomic_read(&sigs_b) == SIGS_PER_ROT){
		atomic_inc(&rots_b);
		atomic_set(&sigs_b, 0);
	}
	return (irq_handler_t)IRQ_HANDLED;
}

static int __init encoder_init(void)
{
	/* create entries in sysfs */
	encoder_class = class_create(THIS_MODULE, "dagu");
	if(encoder_class == NULL){
		printk(KERN_ERR "%s: Cannot create entry in sysfs", DRVNAME);
		return -1;
	}
	
	if(class_create_file(encoder_class, &distance_a_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err1;
	}
	
	if(class_create_file(encoder_class, &distance_b_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err2;
	}
	
	if(class_create_file(encoder_class, &reset_attr) != 0){
		printk(KERN_ERR "%s: Cannot create sysfs attribute\n", DRVNAME);
		goto err3;
	}
	
	/* gpio initialization */
	if(gpio_request(PIN_A, "pin_a") != 0){
		printk(KERN_ERR "%s: Cannot request gpio\n", DRVNAME);
		goto err4;
	}
	gpio_direction_input(PIN_A);
	gpio_export(PIN_A, false);
	
	if(gpio_request(PIN_B, "pin_b") != 0){
		printk(KERN_ERR "%s: Cannot request gpio\n", DRVNAME);
		goto err5;
	}
	gpio_direction_input(PIN_B);
	gpio_export(PIN_B, false);
	
	if(request_irq(gpio_to_irq(PIN_A), (irq_handler_t)pin_a_irq, IRQF_TRIGGER_RISING, "pin_a_irq", NULL) != 0){
		printk(KERN_ERR "%s: Cannot request interrupt\n", DRVNAME);
		goto err6;
	}
	if(request_irq(gpio_to_irq(PIN_B), (irq_handler_t)pin_b_irq, IRQF_TRIGGER_RISING, "pin_b_irq", NULL) != 0){
		printk(KERN_ERR "%s: Cannot request interrupt\n", DRVNAME);
		goto err7;
	}
	
	printk(KERN_INFO "%s: Module loaded\n", DRVNAME);
	return 0;
	
	err7:
	free_irq(gpio_to_irq(PIN_A), NULL);
	err6:
	gpio_unexport(PIN_B);
	err5:
	gpio_unexport(PIN_A);
	gpio_free(PIN_A);
	err4:
	class_remove_file(encoder_class, &reset_attr);
	err3:
	class_remove_file(encoder_class, &distance_b_attr);
	err2:
	class_remove_file(encoder_class, &distance_a_attr);
	err1:
	class_destroy(encoder_class);
	return -1;
}

static void __exit encoder_exit(void)
{
	free_irq(gpio_to_irq(PIN_B), NULL);
	free_irq(gpio_to_irq(PIN_A), NULL);
	gpio_unexport(PIN_B);
	gpio_free(PIN_B);
	gpio_unexport(PIN_A);
	gpio_free(PIN_A);
	class_remove_file(encoder_class, &reset_attr);
	class_remove_file(encoder_class, &distance_b_attr);
	class_remove_file(encoder_class, &distance_a_attr);
	class_destroy(encoder_class);
	printk(KERN_INFO "%s: Module unloaded\n", DRVNAME);
}

module_init(encoder_init);
module_exit(encoder_exit);

MODULE_AUTHOR("Adam Olek");
MODULE_DESCRIPTION("Dagu encoder driver");
MODULE_LICENSE("GPL");
