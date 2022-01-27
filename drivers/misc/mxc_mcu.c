#include <linux/i2c.h>
#include <linux/string.h> 
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/bcd.h>
#define DRV_VERSION "0.0.3"
#define DEV_NAME  "mcu_i2c"
#define MAX_MCU_BUF 32
struct mcu_driver{
	struct device *dev;
	struct class *class;
	struct i2c_client* mcu_client;
	char mcu_buff[MAX_MCU_BUF];
};

typedef enum
{
        REG_VERSION,
        REG_POWER_OFF,
        REG_DAT0,
        REG_DAT1,
        REG_DAT2,
        REG_DAT3,
        REG_DAT4,
        REG_DAT5,
        REG_DAT6,
        REG_DAT7,
        REG_START,
        REG_MODEL_H,
        REG_MODEL_L,
        REG_SERIAL_1,
        REG_SERIAL_2,
        REG_SERIAL_3,
        REG_SERIAL_4,
        REG_SERIAL_5,
        REG_SERIAL_6,
        REG_SERIAL_7,
        REG_SERIAL_8,
}REGS;



static int mcu_read(struct i2c_client* client, char* dat, int len){
	struct i2c_msg msgs[2];
	char addrbuf[2]={0,0};
	char buf[MAX_MCU_BUF+2];
	int ret;

	if(len > MAX_MCU_BUF)
		len = MAX_MCU_BUF;

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = addrbuf;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = buf;

	ret = i2c_transfer(client->adapter,msgs,sizeof(msgs)/sizeof(struct i2c_msg));
	memcpy(dat, buf+2, len);
	return ret;
}

static int mcu_write(struct i2c_client* client, char *dat, int len){
	char buf[MAX_MCU_BUF+1];
	int ret;
	struct i2c_msg msgs[]={
		{client->addr, 0, len, buf},
	};
	printk("mcu_write ----------------\n");
	
	if(len > MAX_MCU_BUF)
		len = MAX_MCU_BUF;
	memcpy(buf+1, dat, len);
	ret = i2c_transfer(client->adapter,msgs,sizeof(msgs)/sizeof(struct i2c_msg));
	return ret;
}

struct mcu_driver *gDrv = NULL;
void mcu_poweroff(void){
	struct i2c_client* mcu_client = gDrv->mcu_client;
        char databuf[2];
        char addrbuf[2];
	int ret;
        struct i2c_msg msgs[]={
                {mcu_client->addr,0,1,addrbuf},
                {mcu_client->addr,I2C_M_RD,1,databuf},
        };
	char buff[MAX_MCU_BUF];

        addrbuf[0] = 0xa1;
        ret = i2c_transfer(mcu_client->adapter,msgs,sizeof(msgs)/sizeof(struct i2c_msg));
	mcu_read(gDrv->mcu_client, buff, sizeof(buff));
	buff[REG_POWER_OFF] = 0xaa;
	mcu_write(gDrv->mcu_client, buff, sizeof(buff));
}
EXPORT_SYMBOL(mcu_poweroff);
static int f_read_proc(struct file *file, char *buf,char **start,off_t pos,int count,
		int *eof,void *data)
{
	struct mcu_driver *drv = PDE_DATA(file_inode(file));
	 int model_id, len=0, i;
        char buff[256];
	int ret;
        if(data == NULL)
                return 0;
        ret = mcu_read(drv->mcu_client, drv->mcu_buff, sizeof(drv->mcu_buff));
	if(ret < 0){
		printk(KERN_ERR "mcu i2c io err=%d \n", ret);
	return 0;
	}
	model_id = ((drv->mcu_buff[REG_MODEL_H])<<8)|(drv->mcu_buff[REG_MODEL_L]);
	len += sprintf(buff+len,"firmware_ver = V%02X\n", drv->mcu_buff[REG_VERSION]);
	len += sprintf(buff+len,"board_id = %d\n", model_id);
	len += sprintf(buff+len,"SN = ");
	for(i=0; i<8; i++)
		len += sprintf(buff+len,"%02X", drv->mcu_buff[REG_SERIAL_1+i]);
	len += sprintf(buff+len,"\n");
        len = strlen(buff)+1;
        return sprintf(buf,"%s", buff);
}

static const struct file_operations mcu_proc_fops = {
        .owner          = THIS_MODULE,
        .read           = f_read_proc,
	.llseek 	= default_llseek,
};
static void create_proc(char* name, void* data){
        struct proc_dir_entry *entry;
		entry= proc_create_data(name, S_IRUGO|S_IWUGO, NULL, &mcu_proc_fops, data);
	if (!entry){
		printk("create_proc error \n");
	} 
}

static int  mxc_mcu_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct mcu_driver* drv = NULL;
	int ret;
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
		return -ENODEV;
	printk("\n mcu_i2c_probe ...\n");	

        drv = kmalloc(sizeof(struct mcu_driver), GFP_KERNEL);
        if(!drv){
                printk(KERN_ERR "unable kzalloc mcu_driver\n");
                return -1;
}

        ret = mcu_read(client, drv->mcu_buff, sizeof(drv->mcu_buff));
        if(ret < 0){
                printk(KERN_ERR "mcu i2c io err=%d \n", ret);
                kfree(drv);
                return -2;
}

	create_proc("mcu", drv);
	drv->mcu_client = client;
	gDrv = drv;
	i2c_set_clientdata(client, drv);

	return 0;
}

static const struct i2c_device_id mxc_mcu_i2c_id[] = {
	{ DEV_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, mxc_mcu_i2c_id);
static struct of_device_id mxc_mcu_dt_ids[] = {
	{ .compatible = "mxc,mcu" },
	{ }
};
static struct i2c_driver mxc_mcu_i2c_driver = {
	.driver = {
		   .name = DEV_NAME,
		   },
	.probe = mxc_mcu_i2c_probe,
	.id_table = mxc_mcu_i2c_id,

	.driver = {
		.name	= "mcu_i2c",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mxc_mcu_dt_ids),
	},
};

static int __init mxc_mcu_i2c_init(void)
{
	return i2c_add_driver(&mxc_mcu_i2c_driver);
}

static void __exit mxc_mcu_i2c_exit(void)
{
	i2c_del_driver(&mxc_mcu_i2c_driver);
}


module_init(mxc_mcu_i2c_init);
module_exit(mxc_mcu_i2c_exit);

MODULE_AUTHOR("wuwenbing <wuwenbing@norco.com.cn> 18038196330");
MODULE_DESCRIPTION("norco mcu dirver");
MODULE_LICENSE("GPL");
