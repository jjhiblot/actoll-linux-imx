/*
 * GPIO Reset Controller driver
 *
 * Copyright 2013 Philipp Zabel, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>

static struct of_device_id gpio_init_dt_ids[] = {
	{ .compatible = "gpio-init" },
	{ }
};

MODULE_DEVICE_TABLE(of, gpio_init_dt_ids);

static int gpio_init_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	enum of_gpio_flags flags;
	int gpio, value;
	int i, ret;
	s32 delay_ms;

	printk("bingking gpio_init_probe \n");
	if (!np)
		return -ENODEV;

	for (i = 0;i < 7; i++) 
	{
		gpio = of_get_named_gpio_flags(np, "init-gpios", i, &flags);
		value = (flags == OF_GPIO_ACTIVE_LOW)? 1:0;
	        if(!gpio_is_valid(gpio))
		{
			printk("invalid init-gpio index=%d\n", i);
       			break;
		}

		ret = devm_gpio_request(&pdev->dev, gpio, "init-gpio");
		if (ret) 
		{
			printk("can't get gpio %d\n", gpio);
			continue;
		}
                printk("Get gpio %d value %d\n", gpio,value);		
		gpio_direction_output(gpio, value);
		gpio_free(gpio);
	}
    
        for (i = 0;i < 1; i++)
        {
                gpio = of_get_named_gpio(np, "reset-gpios", i);
                if(!gpio_is_valid(gpio))
                {
                        printk("invalid reset-gpios index=%d\n", i);
                        break;
                }

                ret = devm_gpio_request(&pdev->dev, gpio, "reset-gpio");
                if (ret)
                {
                        printk("can't get reset gpios %d\n", gpio);
                        continue;
                }

                if(of_property_read_u32_index(np, "reset-delay-ms", i, &delay_ms))
                {
                        printk("invalid delay-ms index=%d\n", i);
                        continue;
                }

                gpio_direction_output(gpio, 0);
                msleep(delay_ms);
                gpio_direction_output(gpio, 1);

                printk("Get reset gpio %d\n", gpio);
                printk("Delay_ms %d\n", delay_ms);
        }

    	return 0;
}

static int gpio_init_remove(struct platform_device *pdev)
{
	return 0;
}



static struct platform_driver gpio_init_driver = {
	.probe = gpio_init_probe,
	.remove = gpio_init_remove,
	.driver = {
		.name = "gpio-init",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(gpio_init_dt_ids),
	},
};

static int __init gpio_init_init(void)
{
	printk("bingking gpio_init_init \n");
	return platform_driver_register(&gpio_init_driver);
}
device_initcall(gpio_init_init);


static void __exit gpio_init_exit(void)
{
	platform_driver_unregister(&gpio_init_driver);
}
module_exit(gpio_init_exit);


MODULE_AUTHOR("Philipp Zabel <p.zabel@pengutronix.de>");
MODULE_DESCRIPTION("gpio init controller");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio-init");
