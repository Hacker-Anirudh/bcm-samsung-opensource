 /*
 * S6E63M0 AMOLED LCD panel driver.
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 * Modified by JuneSok Lee <junesok.lee@samsung.com>
 *
 * Derived from drivers/video/broadcom/lcd/smart_dimming_s6e63m0_dsi.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <video/kona_fb.h>
#include <linux/broadcom/mobcom_types.h>
#include "dispdrv_common.h"

#undef dev_dbg
#define dev_dbg dev_info

#define MAX_BRIGHTNESS		255
#define DEFAULT_BRIGHTNESS		190
#define DEFAULT_GAMMA_LEVEL	17 /*190cd*/

#define ELVSS_SET_START_IDX 2
#define ELVSS_SET_END_IDX 5

#define ID_VALUE_M2			0xA4
#define ID_VALUE_SM2			0xB4
#define ID_VALUE_SM2_1			0xB6

#define ESD_OPERATION
#define BL_POSTPONE
#define BL_WAIT_TIME 1000*2
#define DURATION_TIME 5000 /* 3000ms */
#define BOOT_WAIT_TIME 1000*30 /* 30sec */

extern char *get_seq(DISPCTRL_REC_T *rec);
extern void panel_write(UInt8 *buff);
extern int panel_read(UInt8 reg, UInt8 *rxBuff, UInt8 buffLen);
  extern void kona_fb_esd_hw_reset(void);
  extern int kona_fb_obtain_lock(void);
  extern int kona_fb_release_lock(void);

extern struct device *lcd_dev;

  extern int real_level;

struct hx8357_dsi_lcd {
	struct device	*dev;
	struct mutex	lock;
	unsigned int	current_brightness;
	unsigned int	bl;	
	bool			panel_awake;			
	u8*			lcd_id;	
#ifdef ESD_OPERATION
	unsigned int	esd_enable;
		  struct delayed_work esd_work;
#endif
#ifdef BL_POSTPONE
		  struct delayed_work bl_work;	
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	earlysuspend;
#endif
};

#define PANEL_ID_MAX		3
struct hx8357_dsi_lcd *lcd = NULL;
u8 gPanelID[PANEL_ID_MAX];

int panel_read_id(void)
{
	u8 *pPanelID = gPanelID;	
	int ret=0;	
	pr_info("%s\n", __func__);	

	/* Read panel ID*/
	ret = panel_read(0xDA, pPanelID, 1); 
	if(ret<0)
		return ret;
	printk("[LCD] gPanelID1 = 0x%02X\n", pPanelID[0]);
	ret = panel_read(0xDB, pPanelID+1, 1);
	if(ret<0)
		return -1;	
	printk("[LCD] gPanelID2 = 0x%02X\n", pPanelID[1]);
	panel_read(0xDC, pPanelID+2, 1);	
	printk("[LCD] gPanelID3 = 0x%02X\n", pPanelID[2]);
	return 0;
}
  
  int panel_check_id(void)
  {
	  int ret=0;  
	  u8 rdnumed;
	  u8 rdidic[3]={0,0,0};
	  u8 exp_rdidic[3] = {0x5B, 0xBC, 0x91};
	  pr_info("%s\n", __func__);  

 //     kona_fb_obtain_lock();

       panel_read(0x0A, &rdnumed, 1);
 	  printk("[LCD] READ STATE = 0x%02X\n", rdnumed);
      
	  if(rdnumed != 0x9C){
		  pr_err("%s : error in read_data(0x0A) = 0x%02X \n", __func__, rdidic[0]);
		  ret = -1;
//		goto esd_detection;
		} 
//	esd_detection:
//		  kona_fb_release_lock();
	      pr_info(" %s : finished %d \n", __func__, ret);
	  
		  return ret;

}
EXPORT_SYMBOL(panel_read_id);

void panel_initialize(char *init_seq)
{
	u8 *pPanelID = gPanelID;
	
	pr_info("%s\n", __func__);	

	panel_write(init_seq);	

	//panel_read_id();
}
EXPORT_SYMBOL(panel_initialize);

static ssize_t show_lcd_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[20];
	  sprintf(temp, "INH_5BBC91\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, show_lcd_info, NULL);


#ifdef ESD_OPERATION
void hx8357_esd_recovery(void)
{
	  printk("%s\n", __func__);	  
	kona_fb_esd_hw_reset();
	
	/* To Do : LCD Reset & Initialize */
}
static void esd_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
       
	pr_info(" lcd %s \n", __func__);
		
	if (lcd->esd_enable /* && !battpwroff_charging*/) {
		mutex_lock(&lcd->lock);		
		if(panel_check_id() == -1)
{
				hx8357_esd_recovery();
				msleep(10);
		}
		schedule_delayed_work(dwork, msecs_to_jiffies(DURATION_TIME));		
		mutex_unlock(&lcd->lock);		
	}
	
}
#endif	/*ESD_OPERATION*/

#ifdef BL_POSTPONE
void bl_work_func(void)
{
	int limit=19;
	
	pr_info(" %s function entered\n", __func__);

    real_level = limit;
	gpio_set_value(24,1); /* backlight on */
	udelay(100);
	for(;limit>0;limit--) {
		udelay(2);
		gpio_set_value(24,0);
		udelay(2);
		gpio_set_value(24,1);
	}	
	cancel_delayed_work_sync(&(lcd->bl_work));
}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
static void hx8357_dsi_early_suspend(struct early_suspend *earlysuspend)
{
  int ret;
  struct hx8357_dsi_lcd *lcd = container_of(earlysuspend, struct hx8357_dsi_lcd, earlysuspend);

  pr_info(" %s function entered\n", __func__);

#ifdef ESD_OPERATION
		lcd->esd_enable = 0;
	  cancel_delayed_work_sync(&(lcd->esd_work));
	  dev_dbg(lcd->dev,"disable esd operation\n");
#endif		  

	if (lcd->panel_awake) {
			lcd->panel_awake = false;		
			dev_dbg(lcd->dev, "panel goes to sleep\n");	
	}	
}

static void hx8357_dsi_late_resume(struct early_suspend *earlysuspend)
{
  struct hx8357_dsi_lcd *lcd = container_of(earlysuspend, struct hx8357_dsi_lcd, earlysuspend);

  pr_info(" %s function entered\n", __func__);

	if (!lcd->panel_awake) {
			lcd->panel_awake = true;
			 dev_info(lcd->dev, "panel is ready\n"); 		 
	}

//	  mutex_lock(&lcd->lock); 
	
	/* To Do : Resume LCD Device */
	
//	  mutex_unlock(&lcd->lock); 		  
	
#ifdef ESD_OPERATION
		lcd->esd_enable = 1;
		  schedule_delayed_work(&(lcd->esd_work), msecs_to_jiffies(DURATION_TIME));   
		  dev_dbg(lcd->dev, "enable esd operation : %d\n", lcd->esd_enable);
#endif	

}
#endif

static int hx8357_panel_probe(struct platform_device *pdev)
{
	int ret = 0;

	lcd = kzalloc(sizeof(struct hx8357_dsi_lcd), GFP_KERNEL);
	if (!lcd)
		return -ENOMEM;

	lcd->dev = &pdev->dev;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->lcd_id = gPanelID;
	lcd->current_brightness	= 255;
	lcd->panel_awake = true;	

	dev_info(lcd->dev, "%s function entered\n", __func__);			
	
	{
		int n = 0;

		dev_info(lcd->dev, "panelID : [0x%02X], [0x%02X], [0x%02X]\n", gPanelID[0], gPanelID[1], gPanelID[2]);		

	}	
	
	platform_set_drvdata(pdev, lcd);	

	mutex_init(&lcd->lock);

#ifdef CONFIG_LCD_CLASS_DEVICE
	ret = device_create_file(lcd_dev, &dev_attr_lcd_type);
	if (ret < 0)
		printk("Failed to add lcd_type sysfs entries, %d\n",	__LINE__);		
#endif			
	
#ifdef ESD_OPERATION
	lcd->esd_enable = 1;		
	  INIT_DELAYED_WORK(&(lcd->esd_work), esd_work_func);
	  schedule_delayed_work(&(lcd->esd_work), msecs_to_jiffies(BOOT_WAIT_TIME));    
#endif	
	
#ifdef CONFIG_HAS_EARLYSUSPEND
  lcd->earlysuspend.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
  lcd->earlysuspend.suspend = hx8357_dsi_early_suspend;
  lcd->earlysuspend.resume  = hx8357_dsi_late_resume;
  register_early_suspend(&lcd->earlysuspend);
#endif	

#ifdef BL_POSTPONE
	if(gPanelID[0] == 0x5B){
	  INIT_DELAYED_WORK(&(lcd->bl_work), bl_work_func);
	  schedule_delayed_work(&(lcd->bl_work), msecs_to_jiffies(BL_WAIT_TIME));   
		}
#endif
	return 0;
}

static int hx8357_panel_remove(struct platform_device *pdev)
{
	struct hx8357_dsi_lcd *lcd = platform_get_drvdata(pdev);

	dev_dbg(lcd->dev, "%s function entered\n", __func__);

#ifdef ESD_OPERATION	
	cancel_delayed_work_sync(&(lcd->esd_work));	
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
  unregister_early_suspend(&lcd->earlysuspend);
#endif

	kfree(lcd);

	return 0;
}

static struct platform_driver hx8357_panel_driver = {
	.driver		= {
		.name	= "HX8357",
		.owner	= THIS_MODULE,
	},
	.probe		= hx8357_panel_probe,
	.remove		= hx8357_panel_remove,
};
static int __init hx8357_panel_init(void)
{
	return platform_driver_register(&hx8357_panel_driver);
}

static void __exit hx8357_panel_exit(void)
{
	platform_driver_unregister(&hx8357_panel_driver);
}

late_initcall_sync(hx8357_panel_init);
module_exit(hx8357_panel_exit);

MODULE_DESCRIPTION("hx8357 panel control driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hx8357");
