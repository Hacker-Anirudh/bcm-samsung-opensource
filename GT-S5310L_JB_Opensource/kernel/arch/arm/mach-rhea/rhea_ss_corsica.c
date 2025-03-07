/************************************************************************************************/
/*                                                                                              */
/*  Copyright 2010  Broadcom Corporation                                                        */
/*                                                                                              */
/*     Unless you and Broadcom execute a separate written software license agreement governing  */
/*     use of this software, this software is licensed to you under the terms of the GNU        */
/*     General Public License version 2 (the GPL), available at                                 */
/*                                                                                              */
/*          http://www.broadcom.com/licenses/GPLv2.php                                          */
/*                                                                                              */
/*     with the following added to such license:                                                */
/*                                                                                              */
/*     As a special exception, the copyright holders of this software give you permission to    */
/*     link this software with independent modules, and to copy and distribute the resulting    */
/*     executable under terms of your choice, provided that you also meet, for each linked      */
/*     independent module, the terms and conditions of the license of that module.              */
/*     An independent module is a module which is not derived from this software.  The special  */
/*     exception does not apply to any modifications of the software.                           */
/*                                                                                              */
/*     Notwithstanding the above, under no circumstances may you combine this software in any   */
/*     way with any other Broadcom software provided under a license other than the GPL,        */
/*     without Broadcom's express prior written consent.                                        */
/*                                                                                              */
/************************************************************************************************/

#include <linux/version.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/memory.h>
#include <linux/i2c.h>
#include <linux/i2c-kona.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <asm/gpio.h>
#include <mach/sdio_platform.h>
#ifdef CONFIG_GPIO_PCA953X
#include <linux/i2c/pca953x.h>
#endif
#ifdef CONFIG_TOUCHSCREEN_QT602240
#include <linux/i2c/qt602240_ts.h>
#endif
#ifdef CONFIG_REGULATOR_TPS728XX
#include <linux/regulator/tps728xx.h>
#endif
#include <mach/kona_headset_pd.h>
#include <mach/kona.h>
#include <mach/rhea.h>
#include <mach/pinmux.h>
#include <asm/mach/map.h>
#include <linux/power_supply.h>
#include <linux/mfd/bcm590xx/core.h>
#include <linux/mfd/bcm590xx/pmic.h>
#include <linux/mfd/bcm590xx/bcm59055_A0.h>
#include <linux/broadcom/bcm59055-power.h>
#include <linux/clk.h>
#include <linux/bootmem.h>
#include <plat/pi_mgr.h>
#include "common.h"
#ifdef CONFIG_DMAC_PL330
#include <mach/irqs.h>
#include <plat/pl330-pdata.h>
#include <linux/dma-mapping.h>
#endif
#include <linux/spi/spi.h>
#if defined (CONFIG_HAPTIC)
#include <linux/haptic.h>
#endif
#if defined (CONFIG_BMP18X)
#include <linux/bmp18x.h>
#endif
#if defined (CONFIG_AL3006)
#include <linux/al3006.h>
#endif
#if (defined(CONFIG_MPU_SENSORS_MPU6050A2) || defined(CONFIG_MPU_SENSORS_MPU6050B1))
#include <linux/mpu.h>
#endif
#define _RHEA_
#include <mach/comms/platform_mconfig.h>

#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))
#include <linux/broadcom/bcmbt_rfkill.h>
#endif

#ifdef CONFIG_GPS_IRQ
#include <linux/broadcom/gps.h>
#endif

#ifdef CONFIG_BCM_BT_LPM
#include <linux/broadcom/bcmbt_lpm.h>
#endif

#ifdef CONFIG_BCM_BZHW
#include <linux/broadcom/bcm_bzhw.h>
#endif

#if defined  (CONFIG_SENSORS_BMC150)
#include <linux/bst_sensor_common.h>
#endif

#if defined(CONFIG_BCMI2CNFC)
#include <linux/bcmi2cnfc.h>
#endif

#if defined  (CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) 
#include <linux/hscd_i2c_dev.h>
#endif

#if defined  (CONFIG_SENSORS_K3DH)
#include <linux/k3dh_dev.h>
#endif

#ifdef CONFIG_I2C_GPIO

#include <linux/i2c-gpio.h>
#endif
#ifdef CONFIG_FB_BRCM_RHEA
#include <video/kona_fb_boot.h>
#endif
#include <video/kona_fb.h>
#include <linux/pwm_backlight.h>

#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))
#include <linux/broadcom/bcmbt_rfkill.h>
#endif

#define SD_CARDDET_GPIO_PIN      123

#ifdef CONFIG_BCM_BT_LPM
#include <linux/broadcom/bcmbt_lpm.h>
#endif

#include <linux/ktd259b_bl.h>

#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <mach/rdb/brcm_rdb_uartb.h>

#include <linux/mfd/bcmpmu.h>
#include <plat/pi_mgr.h>

#include <media/soc_camera.h>
#include <mach/rdb/brcm_rdb_sysmap.h>
#include <mach/rdb/brcm_rdb_padctrlreg.h>
#include <linux/delay.h>
#ifdef CONFIG_KEYBOARD_BCM
#include <mach/bcm_keypad.h>
#endif
#ifdef CONFIG_KEYBOARD_EXPANDER
#include <linux/input/matrix_keypad.h>
#include <linux/input/stmpe1801.h>
#endif
#ifdef CONFIG_WD_TAPPER
#include <linux/broadcom/wd-tapper.h>
#endif


#ifdef CONFIG_RMI4_I2C
#include <linux/interrupt.h>
#include <linux/rmi.h>
#include <mach/gpio.h>


#define SYNA_TM2303 0x00000800

#define SYNA_BOARDS SYNA_TM2303 /* (SYNA_TM1333 | SYNA_TM1414) */
#define SYNA_BOARD_PRESENT(board_mask) (SYNA_BOARDS & board_mask)

struct syna_gpio_data {
	u16 gpio_number;
	char* gpio_name;
};

#define TOUCH_ON 1
#define TOUCH_OFF 0
#define TOUCH_POWER_GPIO 43 //PSJ

static int s2200_ts_power(int on_off)
{
	int retval;

	pr_info("%s: TS power change to %d.\n", __func__, on_off);
	retval = gpio_request(TOUCH_POWER_GPIO,"Touch_en");
	if (retval) {
		pr_err("%s: Failed to acquire power GPIO, code = %d.\n",
			 __func__, retval);
		return retval;
	}

	if (on_off == TOUCH_ON) {
		retval = gpio_direction_output(TOUCH_POWER_GPIO,1);
		if (retval) {
			pr_err("%s: Failed to set power GPIO to 1, code = %d.\n",
				__func__, retval);
			return retval;
	}
		gpio_set_value(TOUCH_POWER_GPIO,1);
	} else {
		retval = gpio_direction_output(TOUCH_POWER_GPIO,0);
		if (retval) {
			pr_err("%s: Failed to set power GPIO to 0, code = %d.\n",
				__func__, retval);
			return retval;
		}
		gpio_set_value(TOUCH_POWER_GPIO,0);
	}

	gpio_free(TOUCH_POWER_GPIO);
	msleep(200);
	return 0;
}

static int synaptics_touchpad_gpio_setup(void *gpio_data, bool configure)
{
	int retval=0;
	struct syna_gpio_data *data = gpio_data;

	pr_info("%s: RMI4 gpio configuration set to %d.\n", __func__,
		configure);

	if (configure) {
		retval = gpio_request(data->gpio_number, "rmi4_attn");
		if (retval) {
			pr_err("%s: Failed to get attn gpio %d. Code: %d.",
			       __func__, data->gpio_number, retval);
			return retval;
		}

		//omap_mux_init_signal(data->gpio_name, OMAP_PIN_INPUT_PULLUP);
		retval = gpio_direction_input(data->gpio_number);
		if (retval) {
			pr_err("%s: Failed to setup attn gpio %d. Code: %d.",
			       __func__, data->gpio_number, retval);
			gpio_free(data->gpio_number);
		}
	} else {
		pr_warn("%s: No way to deconfigure gpio %d.",
		       __func__, data->gpio_number);
	}

	return s2200_ts_power(configure);
}
#endif

#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
#include <linux/spa_power.h>
#endif

#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT

#include "board-rhea_ss_corsica-wifi.h"
extern int rhea_wifi_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id);

#endif

#define PMU_DEVICE_I2C_ADDR_0   0x08
#define PMU_IRQ_PIN           29

// keypad map
#define BCM_KEY_ROW_0  0
#define BCM_KEY_ROW_1  1
#define BCM_KEY_ROW_2  2
#define BCM_KEY_ROW_3  3
#define BCM_KEY_ROW_4  4
#define BCM_KEY_ROW_5  5
#define BCM_KEY_ROW_6  6
#define BCM_KEY_ROW_7  7

#define BCM_KEY_COL_0  0
#define BCM_KEY_COL_1  1
#define BCM_KEY_COL_2  2
#define BCM_KEY_COL_3  3
#define BCM_KEY_COL_4  4
#define BCM_KEY_COL_5  5
#define BCM_KEY_COL_6  6
#define BCM_KEY_COL_7  7

#if (defined(CONFIG_MFD_BCM59039) || defined(CONFIG_MFD_BCM59042))
struct regulator_consumer_supply hv6_supply[] = {
	{.supply = "vdd_sdxc"},
};

struct regulator_consumer_supply hv3_supply[] = {
	{.supply = "vdd_sdio"},
};
#endif

extern bool camdrv_ss_power(int cam_id,int bOn);
int configure_sdio_pullup(bool pull_up);

#ifdef CONFIG_MFD_BCMPMU
void __init board_pmu_init(void);
#endif

#ifdef CONFIG_MFD_BCM_PMU590XX
static int bcm590xx_event_callback(int flag, int param)
{
	int ret;
	printk("REG: pmu_init_platform_hw called \n") ;
	switch (flag) {
	case BCM590XX_INITIALIZATION:
		ret = gpio_request(PMU_IRQ_PIN, "pmu_irq");
		if (ret < 0) {
			printk(KERN_ERR "%s unable to request GPIO pin %d\n", __FUNCTION__, PMU_IRQ_PIN);
			return ret ;
		}
		gpio_direction_input(PMU_IRQ_PIN);
		break;
	case BCM590XX_CHARGER_INSERT:
		pr_info("%s: BCM590XX_CHARGER_INSERT\n", __func__);
		break;
	default:
		return -EPERM;
	}
	return 0 ;
}

#ifdef CONFIG_BATTERY_BCM59055
static struct bcm590xx_battery_pdata bcm590xx_battery_plat_data = {
        .batt_min_volt = 3200,
        .batt_max_volt = 4200,
        .batt_technology = POWER_SUPPLY_TECHNOLOGY_LION,
        .usb_cc = CURRENT_500_MA,
        .wac_cc = CURRENT_900_MA,
        /* 1500mA = 5400 coloumb
         * 1Ah = 3600 coloumb
        */
        .batt_max_capacity = 5400,
};
#endif

#ifdef CONFIG_REGULATOR_BCM_PMU59055
/* Regulator registration */
struct regulator_consumer_supply sim_supply[] = {
	{ .supply = "sim_vcc" },
	{ .supply = "simldo_uc" },
};

static struct regulator_init_data bcm59055_simldo_data = {
	.constraints = {
		.name = "simldo",
		.min_uV = 1300000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_VOLTAGE,
		.always_on = 1,
		.boot_on = 0,
	},
	.num_consumer_supplies = ARRAY_SIZE(sim_supply),
	.consumer_supplies = sim_supply,
};

static struct bcm590xx_regulator_init_data bcm59055_regulators[] =
{
	{BCM59055_SIMLDO, &bcm59055_simldo_data, BCM590XX_REGL_LPM_IN_DSM},
};

static struct bcm590xx_regulator_pdata bcm59055_regl_pdata = {
	.num_regulator	= ARRAY_SIZE(bcm59055_regulators),
	.init			= bcm59055_regulators,
	.default_pmmode = {
		[BCM59055_RFLDO]	= 0x00,
		[BCM59055_CAMLDO] 	= 0x00,
		[BCM59055_HV1LDO]	= 0x00,
		[BCM59055_HV2LDO]	= 0x00,
		[BCM59055_HV3LDO]	= 0x00,
		[BCM59055_HV4LDO]	= 0x00,
		[BCM59055_HV5LDO]	= 0x00,
		[BCM59055_HV6LDO]	= 0x00,
		[BCM59055_HV7LDO]	= 0x00,
		[BCM59055_SIMLDO]	= 0x00,
		[BCM59055_CSR]		= 0x00,
		[BCM59055_IOSR]		= 0x00,
		[BCM59055_SDSR]		= 0x00,
	},
};



/* Register userspace and virtual consumer for SIMLDO */
#ifdef CONFIG_REGULATOR_USERSPACE_CONSUMER
static struct regulator_bulk_data bcm59055_bd_sim = {
	.supply = "simldo_uc",
};

static struct regulator_userspace_consumer_data bcm59055_uc_data_sim = {
	.name = "simldo",
	.num_supplies = 1,
	.supplies = &bcm59055_bd_sim,
	.init_on = 0
};

static struct platform_device bcm59055_uc_device_sim = {
	.name = "reg-userspace-consumer",
	.id = BCM59055_SIMLDO,
	.dev = {
		.platform_data = &bcm59055_uc_data_sim,
	},
};
#endif
#ifdef CONFIG_REGULATOR_VIRTUAL_CONSUMER
static struct platform_device bcm59055_vc_device_sim = {
	.name = "reg-virt-consumer",
	.id = BCM59055_SIMLDO,
	.dev = {
		.platform_data = "simldo_uc"
	},
};
#endif
#endif

static const char *pmu_clients[] = {
#ifdef CONFIG_BCM59055_WATCHDOG
	"bcm59055-wdog",
#endif
	"bcmpmu_usb",
#ifdef CONFIG_INPUT_BCM59055_ONKEY
	"bcm590xx-onkey",
#endif
#ifdef CONFIG_BCM59055_FUELGAUGE
	"bcm590xx-fg",
#endif
#ifdef CONFIG_BCM59055_SARADC
	"bcm590xx-saradc",
#endif
#ifdef CONFIG_REGULATOR_BCM_PMU59055
	"bcm590xx-regulator",
#endif
#ifdef CONFIG_BCM59055_AUDIO
	"bcm590xx-audio",
#endif
#ifdef CONFIG_RTC_DRV_BCM59055
	"bcm59055-rtc",
#endif
#ifdef CONFIG_BATTERY_BCM59055
	"bcm590xx-power",
#endif
#ifdef CONFIG_BCM59055_ADC_CHIPSET_API
	"bcm59055-adc_chipset_api",
#endif
#ifdef CONFIG_BCMPMU_OTG_XCEIV
	"bcmpmu_otg_xceiv",
#endif
#ifdef CONFIG_BCM59055_SELFTEST
       "bcm59055-selftest",
#endif
};

static struct bcm590xx_platform_data bcm590xx_plat_data = {
#ifdef CONFIG_KONA_PMU_BSC_HS_MODE
	/*
	 * PMU in High Speed (HS) mode. I2C CLK is 3.25MHz
	 * derived from 26MHz input clock.
	 *
	 * Rhea: PMBSC is always in HS mode, i2c_pdata is not in use.
	 */
	.i2c_pdata = {ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_HS),},
#else
	.i2c_pdata = {ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_400K),},
#endif
	.pmu_event_cb = bcm590xx_event_callback,
#ifdef CONFIG_BATTERY_BCM59055
	.battery_pdata = &bcm590xx_battery_plat_data,
#endif
#ifdef CONFIG_REGULATOR_BCM_PMU59055
	.regl_pdata = &bcm59055_regl_pdata,
#endif
	.clients = pmu_clients,
	.clients_num = ARRAY_SIZE(pmu_clients),
};


static struct i2c_board_info __initdata pmu_info[] =
{
	{
		I2C_BOARD_INFO("bcm59055", PMU_DEVICE_I2C_ADDR_0 ),
		.irq = gpio_to_irq(PMU_IRQ_PIN),
		.platform_data  = &bcm590xx_plat_data,
	},
};
#endif

#ifdef CONFIG_KEYBOARD_BCM
/*!
 * The keyboard definition structure.
 */
struct platform_device bcm_kp_device = {
	.name = "bcm_keypad",
	.id = -1,
};

/*
 * Keymap for Corsica board
 */
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "Volume Up", KEY_VOLUMEUP},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "Home-Key", KEY_HOME},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "Volume Down", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "Music key", KEY_PLAY},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};


static struct bcm_keypad_platform_info bcm_keypad_data = {
	.row_num = 8,
	.col_num = 8,
	.keymap = newKeymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};
#endif

#if defined(CONFIG_BCMI2CNFC)

#define BCMBT_NFC_IRQ_GPIO	(99)       //NFC_IRQ
#define BCMBT_NFC_CLK_REQ_GPIO	(43)       //NFC_CLK_REQ
#define BCMBT_NFC_WAKE_GPIO	(88)       //NFC_WAKE
#define BCMBT_NFC_EN_GPIO	(25)       //NFC_EN
#define BCMBT_NFC_I2C_SDA_GPIO	(56)       //NFC_SDA
#define BCMBT_NFC_I2C_SCL_GPIO	(38)       //NFC_SCL

static int bcmi2cnfc_gpio_setup(void *);
static int bcmi2cnfc_gpio_clear(void *);
static struct bcmi2cnfc_i2c_platform_data bcmi2cnfc_pdata = {
	.irq_gpio = BCMBT_NFC_IRQ_GPIO,
	.en_gpio = BCMBT_NFC_EN_GPIO,
	.wake_gpio = BCMBT_NFC_WAKE_GPIO,
	.init = bcmi2cnfc_gpio_setup,
	.reset = bcmi2cnfc_gpio_clear,
	.i2c_pdata	= {ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_400K),},
};


static int bcmi2cnfc_gpio_setup(void *this)
{
	struct bcmi2cnfc_i2c_platform_data *p;
	p = (struct bcmi2cnfc_i2c_platform_data *) this;
	if (!p)
		return -1;
	pr_info("bcmi2cnfc_gpio_setup nfc en %d, wake %d, irq %d\n",
		p->en_gpio, p->wake_gpio, p->irq_gpio);

	gpio_request(p->irq_gpio, "nfc_irq");
	gpio_direction_input(p->irq_gpio);

	gpio_request(p->en_gpio, "nfc_en");
	gpio_direction_output(p->en_gpio, 1);

	gpio_request(p->wake_gpio, "nfc_wake");
	gpio_direction_output(p->wake_gpio, 0);

	return 0;
}
static int bcmi2cnfc_gpio_clear(void *this)
{
	struct bcmi2cnfc_i2c_platform_data *p;
	p = (struct bcmi2cnfc_i2c_platform_data *) this;
	if (!p)
		return -1;

	pr_info("bcmi2cnfc_gpio_clear nfc en %d, wake %d, irq %d\n",
		p->en_gpio, p->wake_gpio, p->irq_gpio);

	gpio_direction_output(p->en_gpio, 0);
	gpio_direction_output(p->wake_gpio, 1);
	gpio_free(p->en_gpio);
	gpio_free(p->wake_gpio);
	gpio_free(p->irq_gpio);

	return 0;
}

static struct i2c_board_info __initdata bcmi2cnfc[] = {
	{
	 I2C_BOARD_INFO("bcmi2cnfc", 0x1FA),
	 .flags = I2C_CLIENT_TEN,
	 .platform_data = (void *)&bcmi2cnfc_pdata,
	 .irq = gpio_to_irq(BCMBT_NFC_IRQ_GPIO),
	 },

};
static struct i2c_gpio_platform_data rhea_nfc_i2c_gpio_platdata = {
        .sda_pin                = BCMBT_NFC_I2C_SDA_GPIO,
        .scl_pin                = BCMBT_NFC_I2C_SCL_GPIO,
        .udelay                 = 1,
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

#endif

#ifdef CONFIG_GPIO_PCA953X

#if defined(CONFIG_MACH_RHEA_RAY_EDN1X) || defined(CONFIG_MACH_RHEA_RAY_EDN2X) || defined(CONFIG_MACH_RHEA_SS) 
#define GPIO_PCA953X_GPIO_PIN      121 /* Configure pad MMC1DAT4 to GPIO74 */
#define GPIO_PCA953X_2_GPIO_PIN      122 /* Configure ICUSBDM pad to GPIO122 */
#else
#define GPIO_PCA953X_GPIO_PIN      74 /* Configure pad MMC1DAT4 to GPIO74 */
#endif

#define SENSOR_0_GPIO_PWRDN		12
#define SENSOR_0_GPIO_RST		33


#define SENSOR_1_GPIO_PWRDN		13
#define SENSOR_1_CLK			"dig_ch0_clk"


static int pca953x_platform_init_hw(struct i2c_client *client,
		unsigned gpio, unsigned ngpio, void *context)
{
	int rc;
	rc = gpio_request(GPIO_PCA953X_GPIO_PIN, "gpio_expander");
	if (rc < 0)
	{
		printk(KERN_ERR "unable to request GPIO pin %d\n", GPIO_PCA953X_GPIO_PIN);
		return rc;
	}
	gpio_direction_input(GPIO_PCA953X_GPIO_PIN);

	/*init sensor gpio here to be off */
	rc = gpio_request(SENSOR_0_GPIO_PWRDN, "CAM_STANDBY0");
	if (rc < 0)
		printk(KERN_ERR "unable to request GPIO pin %d\n", SENSOR_0_GPIO_PWRDN);

	gpio_direction_output(SENSOR_0_GPIO_PWRDN, 0);
	gpio_set_value(SENSOR_0_GPIO_PWRDN, 0);

	rc = gpio_request(SENSOR_0_GPIO_RST, "CAM_RESET0");
	if (rc < 0)
		printk(KERN_ERR "unable to request GPIO pin %d\n", SENSOR_0_GPIO_RST);

	gpio_direction_output(SENSOR_0_GPIO_RST, 0);
	gpio_set_value(SENSOR_0_GPIO_RST, 0);


	rc = gpio_request(SENSOR_1_GPIO_PWRDN, "CAM_STANDBY1");
	if (rc < 0)
		printk(KERN_ERR "unable to request GPIO pin %d\n", SENSOR_1_GPIO_PWRDN);

	gpio_direction_output(SENSOR_1_GPIO_PWRDN, 0);
	gpio_set_value(SENSOR_1_GPIO_PWRDN, 0);

	return 0;
}

static int pca953x_platform_exit_hw(struct i2c_client *client,
		unsigned gpio, unsigned ngpio, void *context)
{
	gpio_free(GPIO_PCA953X_GPIO_PIN);
	return 0;
}

static struct pca953x_platform_data board_expander_info = {
	.i2c_pdata	= { ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_100K), },
	.gpio_base	= 33, //Temporay value KONA_MAX_GPIO,
	.irq_base	= gpio_to_irq(33),
	.setup		= pca953x_platform_init_hw,
	.teardown	= pca953x_platform_exit_hw,
};

static struct i2c_board_info __initdata pca953x_info[] = {
	{
		I2C_BOARD_INFO("pca9539", 0x74),
		.irq = gpio_to_irq(GPIO_PCA953X_GPIO_PIN),
		.platform_data = &board_expander_info,
	},
};
#ifdef CONFIG_MACH_RHEA_RAY_EDN1X
/* Expander #2 on RheaRay EDN1X*/
static int pca953x_2_platform_init_hw(struct i2c_client *client,
		unsigned gpio, unsigned ngpio, void *context)
{
	int rc;
	rc = gpio_request(GPIO_PCA953X_2_GPIO_PIN, "gpio_expander_2");
	if (rc < 0)
	{
		printk(KERN_ERR "unable to request GPIO pin %d\n", GPIO_PCA953X_2_GPIO_PIN);
		return rc;
	}
	gpio_direction_input(GPIO_PCA953X_2_GPIO_PIN);
	return 0;
}

static int pca953x_2_platform_exit_hw(struct i2c_client *client,
		unsigned gpio, unsigned ngpio, void *context)
{
	gpio_free(GPIO_PCA953X_2_GPIO_PIN);
	return 0;
}

static struct pca953x_platform_data board_expander_2_info = {
	.i2c_pdata	= { ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_100K), },
	.gpio_base	= 33; //Temporay value  KONA_MAX_GPIO+16,
	.irq_base	= gpio_to_irq(33),
	.setup		= pca953x_2_platform_init_hw,
	.teardown	= pca953x_2_platform_exit_hw,
};

static struct i2c_board_info __initdata pca953x_2_info[] = {
	{
		I2C_BOARD_INFO("pca9539", 0x75),
		.irq = gpio_to_irq(GPIO_PCA953X_2_GPIO_PIN),
		.platform_data = &board_expander_2_info,
	},
};
#endif
#endif /* CONFIG_GPIO_PCA953X */

#define TSP_INT_GPIO_PIN      (91)

#if defined (CONFIG_TOUCHSCREEN_BT403_ZANIN)
static struct i2c_board_info __initdata zinitix_i2c_devices[] = {
	{
		I2C_BOARD_INFO("zinitix_isp", 0x50),
	},
	{
		I2C_BOARD_INFO("Zinitix_tsp", 0x20),
		.irq = gpio_to_irq(TSP_INT_GPIO_PIN),
	},
};
#endif

#if defined (CONFIG_TOUCHSCREEN_F761)
static struct i2c_board_info __initdata silabs_i2c_devices[] = {
	{
				I2C_BOARD_INFO("silabs-f760", 0x20),
				.irq = gpio_to_irq(TSP_INT_GPIO_PIN),
	},
};
#endif

#if defined (CONFIG_TOUCHSCREEN_S2200)

int pins_to_gpio (bool to_gpio)
{
/*	if (to_gpio) {
		gpio_tlmm_config(GPIO_CFG(TSP_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(TSP_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(TSP_INT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(GPIO_CFG(TSP_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(TSP_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(TSP_INT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
*/	
	return 0;
}
static struct s2200_ts_platform_data s2200_ts_pdata = {
	.max_x		= 240,
	.max_y		= 320,
	.gpio_sda	= 89,
	.gpio_scl	= 90,
	.gpio_int	= 91,
//	.gpio_vdd_en	= ,
	.pins_to_gpio	= pins_to_gpio,
};

static struct i2c_board_info __initdata synaptics_i2c_devices[] = {
	{
		I2C_BOARD_INFO("synaptics_s2200_ts", 0x20),
		.irq = gpio_to_irq(TSP_INT_GPIO_PIN),
		.platform_data = &s2200_ts_pdata,
	},
};

#if 0
#define AXIS_ALIGNMENT

#define TM1414_ADDR 0x20
// Line below is for two device testing only!
#define TM1414_ATTN	 91

struct syna_gpio_data {
	u16 gpio_number;
	char* gpio_name;
};

static struct syna_gpio_data tm1414_gpiodata = {
	.gpio_number = TM1414_ATTN,
	.gpio_name = "mcbsp1_fsr.gpio_157",
};

#define OMAP_PULL_ENA			(1 << 3)
#define OMAP_PULL_UP			(1 << 4)
#define OMAP_ALTELECTRICALSEL		(1 << 5)

#define OMAP_PIN_INPUT_PULLUP		(OMAP_PULL_ENA | OMAP_INPUT_EN \
						| OMAP_PULL_UP)

static int synaptics_touchpad_gpio_setup(void *gpio_data, bool configure)
{
	int retval=0;
	struct syna_gpio_data *data = gpio_data;

	if (configure) {
		retval = gpio_request(data->gpio_number, "rmi4_attn");
		if (retval) {
			pr_err("%s: Failed to get attn gpio %d. Code: %d.",
			       __func__, data->gpio_number, retval);
			return retval;
		}

		//omap_mux_init_signal(data->gpio_name, OMAP_PIN_INPUT_PULLUP);
		retval = gpio_direction_input(data->gpio_number);
		if (retval) {
			pr_err("%s: Failed to setup attn gpio %d. Code: %d.",
			       __func__, data->gpio_number, retval);
			gpio_free(data->gpio_number);
		}
	} else {
		pr_warn("%s: No way to deconfigure gpio %d.",
		       __func__, data->gpio_number);
	}

	return retval;
}

static struct rmi_device_platform_data tm1414_platformdata = {
	.driver_name = "rmi-generic",
	.sensor_name = "TM1414",
	.attn_gpio = TM1414_ATTN,
	.attn_polarity = RMI_ATTN_ACTIVE_LOW,
	.gpio_data = &tm1414_gpiodata,
	.gpio_config = synaptics_touchpad_gpio_setup,
	.reset_delay_ms = 100,
	.axis_align = {
		.flip_x = true,
	},
};

static struct i2c_board_info bus2_i2c_devices[] = {
     {
         I2C_BOARD_INFO("rmi-generic", TM1414_ADDR),
        .platform_data = &tm1414_platformdata,
     },
};
#endif
#endif

#if defined (CONFIG_RMI4_I2C)
#if SYNA_BOARD_PRESENT(SYNA_TM2303)
	/* tm2303 has four buttons.
	 */

#define AXIS_ALIGNMENT { }

#define TM2303_ADDR 0x20
#define TM2303_ATTN 91
static unsigned char tm2303_f1a_button_codes[] = {KEY_MENU, KEY_BACK};

static int tm2303_post_suspend(void *pm_data) {
	pr_info("%s: RMI4 callback.\n", __func__);
	return s2200_ts_power(TOUCH_OFF);
}

static int tm2303_pre_resume(void *pm_data) {
	pr_info("%s: RMI4 callback.\n", __func__);
	return s2200_ts_power(TOUCH_ON);
}

static struct rmi_f1a_button_map tm2303_f1a_button_map = {
	.nbuttons = ARRAY_SIZE(tm2303_f1a_button_codes),
	.map = tm2303_f1a_button_codes,
};

static struct syna_gpio_data tm2303_gpiodata = {
	.gpio_number = TM2303_ATTN,
	.gpio_name = "sdmmc2_clk.gpio_130",
};

static struct rmi_device_platform_data tm2303_platformdata = {
	.driver_name = "rmi_generic",
	.attn_gpio = TM2303_ATTN,
	.attn_polarity = RMI_ATTN_ACTIVE_LOW,
	.reset_delay_ms = 250,
	.gpio_data = &tm2303_gpiodata,
	.gpio_config = synaptics_touchpad_gpio_setup,
	.axis_align = AXIS_ALIGNMENT,
	.f1a_button_map = &tm2303_f1a_button_map,
	.post_suspend = tm2303_post_suspend,
	.pre_resume = tm2303_pre_resume,
	.f11_type_b = true,	
};

static struct i2c_board_info __initdata synaptics_i2c_devices[] = {
	{
         I2C_BOARD_INFO("rmi_i2c", TM2303_ADDR),
        .platform_data = &tm2303_platformdata,
	},
};

#endif /* TM2303 */
#endif /* RMI4_I2C */

#if defined(CONFIG_TOUCHSCREEN_IST30XX)

#define TSP_INT 91

static struct i2c_board_info imagis_i2c_devices[] = {
	{
		I2C_BOARD_INFO("sec_touch", 0x50),
		.irq	= gpio_to_irq(TSP_INT),
	},
};
#endif	

#if defined  (CONFIG_SENSORS_BMC150)
static struct bosch_sensor_specific bss_bma2x2 = {
	.name = "bma2x2" ,
        .place = 5,
};

static struct bosch_sensor_specific bss_bmm050 = {
	.name = "bmm050" ,
        .place = 5,
};
#endif

#if defined  (CONFIG_SENSORS_K3DH)
static struct k3dh_platform_data k3dh_platform_data = {	
	.orientation = {	
	1, 0, 0,
	0, -1, 0,		
	0, 0, -1},
};
#endif

#if defined  (CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) 
static struct hscd_i2c_platform_data hscd_i2c_platform_data = {	
	.orientation = {	
		0, 1, 0,
		1, 0, 0,			
		0, 0, -1},
};
#endif


static struct i2c_board_info __initdata rhea_ss_i2cgpio1_board_info[] = {

#if defined  (CONFIG_SENSORS_K3DH)
	{		
		I2C_BOARD_INFO("k3dh", 0x19),
		.platform_data = &k3dh_platform_data,                        	
	},
#endif

#if defined  (CONFIG_SENSORS_HSCDTD006A) || defined(CONFIG_SENSORS_HSCDTD008A) 	
	{
		I2C_BOARD_INFO("hscd_i2c", 0x0c),
		.platform_data = &hscd_i2c_platform_data,
	},
#endif

};

#ifdef CONFIG_KEYBOARD_EXPANDER

#define GPOIO_EXPANDER_INT 91 //SPI0TXD

static struct stmpe_keypad_platform_data stmpe1801_keypad_data = {
	.debounce_ms    = 64,
	.scan_count     = 8,
	.no_autorepeat  = true,
	.keymap_data    = &stmpe1801_keymap_data,
};

static struct stmpe_platform_data stmpe1801_data = {
	.id				=	1,
	//.blocks			=	STMPE_BLOCK_GPIO| STMPE_BLOCK_KEYPAD,
	//.irq_trigger   	 	= 	IRQF_TRIGGER_FALLING,
	//.irq_base       		= 	gpio_to_irq(GPOIO_EXPANDER_INT),
	.keypad			= 	&stmpe1801_keypad_data,
	.autosleep   	=	 true,
	.autosleep_timeout = 	1024,
};

static struct i2c_board_info __initdata rhea_ss_i2cgpio2_board_info[] = {

	{
		I2C_BOARD_INFO("stmpe1801", 0x40),
		.irq = gpio_to_irq(GPOIO_EXPANDER_INT),
		.platform_data = &stmpe1801_data,
	},

};
#endif

#if defined(CONFIG_STEREO_SPEAKER)// IVORY_AMP
static struct i2c_board_info __initdata rhea_ss_i2cgpio3_board_info[] = {
	{
		I2C_BOARD_INFO("tpa2026", 0x58),
	},
};
#endif


#ifdef CONFIG_TOUCHSCREEN_QT602240
#ifdef CONFIG_GPIO_PCA953X
#define QT602240_INT_GPIO_PIN      (121)
#else
#define QT602240_INT_GPIO_PIN      74 /* skip expander chip */
#endif
static int qt602240_platform_init_hw(void)
{
	int rc;
	rc = gpio_request(QT602240_INT_GPIO_PIN, "ts_qt602240");
	if (rc < 0)
	{
		printk(KERN_ERR "unable to request GPIO pin %d\n", QT602240_INT_GPIO_PIN);
		return rc;
	}
	gpio_direction_input(QT602240_INT_GPIO_PIN);

	return 0;
}

static void qt602240_platform_exit_hw(void)
{
	gpio_free(QT602240_INT_GPIO_PIN);
}

static struct qt602240_platform_data qt602240_platform_data = {
	.i2c_pdata	= { ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_100K), },
	.x_line		= 15,
	.y_line		= 11,
	.x_size		= 1023,
	.y_size		= 1023,
	.x_min		= 90,
	.y_min		= 90,
	.x_max		= 0x3ff,
	.y_max		= 0x3ff,
	.max_area	= 0xff,
	.blen		= 33,
	.threshold	= 70,
	.voltage	= 2700000,              /* 2.8V */
	.orient		= QT602240_DIAGONAL_COUNTER,
	.init_platform_hw = qt602240_platform_init_hw,
	.exit_platform_hw = qt602240_platform_exit_hw,
};

static struct i2c_board_info __initdata qt602240_info[] = {
	{
		I2C_BOARD_INFO("qt602240_ts", 0x4a),
		.platform_data = &qt602240_platform_data,
		.irq = gpio_to_irq(QT602240_INT_GPIO_PIN),
	},
};
#endif /* CONFIG_TOUCHSCREEN_QT602240 */
#ifdef CONFIG_BMP18X
static struct i2c_board_info __initdata bmp18x_info[] =
{
	{
		I2C_BOARD_INFO("bmp18x", 0x77 ),
		/*.irq = */
	},
};
#endif
#ifdef CONFIG_AL3006
#ifdef CONFIG_GPIO_PCA953X
	#define AL3006_INT_GPIO_PIN		(121) //@Fixed me Temporay value
#else
	#define AL3006_INT_GPIO_PIN		122	/*  skip expander chip */
#endif
static int al3006_platform_init_hw(void)
{
	int rc;
	rc = gpio_request(AL3006_INT_GPIO_PIN, "al3006");
	if (rc < 0)
	{
		printk(KERN_ERR "unable to request GPIO pin %d\n", AL3006_INT_GPIO_PIN);
		return rc;
	}
	gpio_direction_input(AL3006_INT_GPIO_PIN);

	return 0;
}

static void al3006_platform_exit_hw(void)
{
	gpio_free(AL3006_INT_GPIO_PIN);
}

static struct al3006_platform_data al3006_platform_data = {
	.i2c_pdata	= { ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_100K), },
	.init_platform_hw = al3006_platform_init_hw,
	.exit_platform_hw = al3006_platform_exit_hw,
};

static struct i2c_board_info __initdata al3006_info[] =
{
	{
		I2C_BOARD_INFO("al3006", 0x1d ),
		.platform_data = &al3006_platform_data,
		.irq = gpio_to_irq(AL3006_INT_GPIO_PIN),
	},
};
#endif
#if (defined(CONFIG_MPU_SENSORS_MPU6050A2) || defined(CONFIG_MPU_SENSORS_MPU6050B1))
static struct mpu_platform_data mpu6050_data={
	.int_config = 0x10,
	.orientation =
		{ 0,1,0,
		  1,0,0,
		  0,0,-1},
	.level_shifter = 0,

	.accel = {
		 /*.get_slave_descr = mpu_get_slave_descr,*/
		.adapt_num = 2,
		.bus = EXT_SLAVE_BUS_SECONDARY,
		.address = 0x38,
		.orientation = 
			{ 0,1,0,
			  1,0,0,
			  0,0,-1},
	},
	.compass = {
		 /*.get_slave_descr = compass_get_slave_descr,*/
		.adapt_num = 2,
		.bus = EXT_SLAVE_BUS_PRIMARY,
		.address = (0x50>>1),
		.orientation =
			{ 0,1,0,
			  1,0,0,
			  0,0,-1},
	},
};
static struct i2c_board_info __initdata mpu6050_info[] =
{
	{
		I2C_BOARD_INFO("mpu6050", 0x68),
		 /*.irq = */
		.platform_data = &mpu6050_data,
	},
};
#endif

#ifdef CONFIG_KONA_HEADSET_MULTI_BUTTON

#define HS_IRQ		gpio_to_irq(31)
#define HSB_IRQ		BCM_INT_ID_AUXMIC_COMP1
#define HSI_IRQ		BCM_INT_ID_AUXMIC_COMP2
#define HSR_IRQ 	   BCM_INT_ID_AUXMIC_COMP2_INV

/*
 * Default table used if the platform does not pass one
 */ 
static unsigned int  rheass_button_adc_values [3][2] = 
{
	/* SEND/END Min, Max*/
        {0,    115},
	/* Volume Up  Min, Max*/
        {116,    235},
	/* Volue Down Min, Max*/
        {236,   520},
};

static struct kona_headset_pd headset_data = {
	/* GPIO state read is 0 on HS insert and 1 for
	 * HS remove
	 */

	.hs_default_state = 1,
	/*
	 * Because of the presence of the resistor in the MIC_IN line.
	 * The actual ground is not 0, but a small offset is added to it.
	 * This needs to be subtracted from the measured voltage to determine the
	 * correct value. This will vary for different HW based on the resistor
	 * values used.
	 *
	 * What this means to Rhearay?
	 * From the schematics looks like there is no such resistor put on
	 * Rhearay. That means technically there is no need to subtract any extra load
	 * from the read Voltages. On other HW, if there is a resistor present
	 * on this line, please measure the load value and put it here.
	 */
	.phone_ref_offset = 0,

	/*
	 * Inform the driver whether there is a GPIO present on the board to
	 * detect accessory insertion/removal _OR_ should the driver use the
	 * COMP1 for the same.
	 */
	.gpio_for_accessory_detection = 1,

	/*
	 * Pass the board specific button detection range
	 */
	.button_adc_values = rheass_button_adc_values,

	/* ldo for MICBIAS */
	.ldo_id = "hv1",

};

static struct resource board_headset_resource[] = {
	{	/* For AUXMIC */
		.start = AUXMIC_BASE_ADDR,
		.end = AUXMIC_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{	/* For ACI */
		.start = ACI_BASE_ADDR,
		.end = ACI_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{	/* For GPIO Headset IRQ */
		.start = HS_IRQ,
		.end = HS_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	{	/* For Headset insert IRQ */
		.start = HSI_IRQ,
		.end = HSI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	{	/* For Headset remove IRQ */
		.start = HSR_IRQ,
		.end = HSR_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	{	/* COMP1 for Button*/
		.start = HSB_IRQ,
		.end = HSB_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device headset_device = {
	.name = "konaaciheadset",
	.id = -1,
	.resource = board_headset_resource,
	.num_resources	= ARRAY_SIZE(board_headset_resource),
	.dev	=	{
		.platform_data = &headset_data,
	},
};
#endif /* CONFIG_KONA_HEADSET_MULTI_BUTTON */

#ifdef CONFIG_DMAC_PL330
static struct kona_pl330_data rhea_pl330_pdata =	{
	/* Non Secure DMAC virtual base address */
	.dmac_ns_base = KONA_DMAC_NS_VA,
	/* Secure DMAC virtual base address */
	.dmac_s_base = KONA_DMAC_S_VA,
	/* # of PL330 dmac channels 'configurable' */
	.num_pl330_chans = 8,
	/* irq number to use */
	.irq_base = BCM_INT_ID_RESERVED184,
	/* # of PL330 Interrupt lines connected to GIC */
	.irq_line_count = 8,
};

static struct platform_device pl330_dmac_device = {
	.name = "kona-dmac-pl330",
	.id = 0,
	.dev = {
		.platform_data = &rhea_pl330_pdata,
		.coherent_dma_mask  = DMA_BIT_MASK(64),
	},
};
#endif

/*
 * SPI board info for the slaves
 */
static struct spi_board_info spi_slave_board_info[] __initdata = {
#if 0 //def CONFIG_SPI_SPIDEV
	{
	 .modalias = "spidev",	/* use spidev generic driver */
	 .max_speed_hz = 13000000,	/* use max speed */
	 .bus_num = 0,		/* framework bus number */
	 .chip_select = 0,	/* for each slave */
	 .platform_data = NULL,	/* no spi_driver specific */
	 .irq = 0,		/* IRQ for this device */
	 .mode = SPI_MODE_1,	/* SPI mode 1 */
	 },
#endif
	/* TODO: adding more slaves here */
};

#if defined (CONFIG_HAPTIC_SAMSUNG_PWM)
void haptic_gpio_setup(void)
{
	/* Board specific configuration like pin mux & GPIO */
}

static struct haptic_platform_data haptic_control_data = {
	/* Haptic device name: can be device-specific name like ISA1000 */
	.name = "pwm_vibra",
	/* PWM interface name to request */
	.pwm_name = "kona_pwmc:4",
	/* Invalid gpio for now, pass valid gpio number if connected */
	.gpio = ARCH_NR_GPIOS,
	.setup_pin = haptic_gpio_setup,
};

struct platform_device haptic_pwm_device = {
	.name   = "samsung_pwm_haptic",
	.id     = -1,
	.dev	=	 {	.platform_data = &haptic_control_data,}
};

#endif /* CONFIG_HAPTIC_SAMSUNG_PWM */

#ifdef CONFIG_BCM_SS_VIBRA
struct platform_device bcm_vibrator_device = {
	.name = "vibrator", 
	.id = 0,
	.dev = {
		.platform_data="hv4",
   },
};
#endif

static struct resource board_sdio1_resource[] = {
	[0] = {
		.start = SDIO1_BASE_ADDR,
		.end = SDIO1_BASE_ADDR + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_SDIO0,
		.end = BCM_INT_ID_SDIO0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource board_sdio2_resource[] = {
	[0] = {
		.start = SDIO2_BASE_ADDR,
		.end = SDIO2_BASE_ADDR + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_SDIO1,
		.end = BCM_INT_ID_SDIO1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource board_sdio3_resource[] = {
	[0] = {
		.start = SDIO3_BASE_ADDR,
		.end = SDIO3_BASE_ADDR + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_SDIO_NAND,
		.end = BCM_INT_ID_SDIO_NAND,
		.flags = IORESOURCE_IRQ,
	},
};

#ifdef CONFIG_MACH_RHEA_RAY_EDN2XT
static struct resource board_sdio4_resource[] = {
	[0] = {
		.start = SDIO4_BASE_ADDR,
		.end = SDIO4_BASE_ADDR + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_SDIO_MMC,
		.end = BCM_INT_ID_SDIO_MMC,
		.flags = IORESOURCE_IRQ,
	},
};
#endif

static struct sdio_platform_cfg board_sdio_param[] = {
	{ /* SDIO1 */
		.id = 0,
		.data_pullup = 0,
		.cd_gpio = SD_CARDDET_GPIO_PIN,
		.devtype = SDIO_DEV_TYPE_SDMMC,
		.flags = KONA_SDIO_FLAGS_DEVICE_REMOVABLE,
		.peri_clk_name = "sdio1_clk",
		.ahb_clk_name = "sdio1_ahb_clk",
		.sleep_clk_name = "sdio1_sleep_clk",
		.peri_clk_rate = 48000000,
                /* vdd_sdc regulator: needed to support UHS SD cards */
                .vddo_regulator_name = "vdd_sdio",
	 	/*The SD controller regulator*/
	 	.vddsdxc_regulator_name = "vdd_sdxc",
        .configure_sdio_pullup = configure_sdio_pullup,
	},
	{ /* SDIO2 */
		.id = 1,
		.data_pullup = 0,
		.is_8bit = 1,
		.devtype = SDIO_DEV_TYPE_EMMC,
		.flags = KONA_SDIO_FLAGS_DEVICE_NON_REMOVABLE ,
		.peri_clk_name = "sdio2_clk",
		.ahb_clk_name = "sdio2_ahb_clk",
		.sleep_clk_name = "sdio2_sleep_clk",
		.peri_clk_rate = 52000000,
	},
	{ /* SDIO3 */
		.id = 2,
		.data_pullup = 0,
		.devtype = SDIO_DEV_TYPE_WIFI,
		.wifi_gpio = {
			.reset		= 70,		/* WLAN_REG_ON : GPIO70 */
			.reg		= -1,
			.host_wake	= 14,		/* WLAN_HOST_WAKE : GPIO14 */
			.shutdown	= -1,
		},
		.flags = KONA_SDIO_FLAGS_DEVICE_REMOVABLE,
		.peri_clk_name = "sdio3_clk",
		.ahb_clk_name = "sdio3_ahb_clk",
		.sleep_clk_name = "sdio3_sleep_clk",
		.peri_clk_rate = 48000000,
#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
		.register_status_notify=rhea_wifi_status_register,
#endif
		},
};

static struct platform_device board_sdio1_device = {
	.name = "sdhci",
	.id = 0,
	.resource = board_sdio1_resource,
	.num_resources   = ARRAY_SIZE(board_sdio1_resource),
	.dev      = {
		.platform_data = &board_sdio_param[0],
	},
};

static struct platform_device board_sdio2_device = {
	.name = "sdhci",
	.id = 1,
	.resource = board_sdio2_resource,
	.num_resources   = ARRAY_SIZE(board_sdio2_resource),
	.dev      = {
		.platform_data = &board_sdio_param[1],
	},
};

static struct platform_device board_sdio3_device = {
	.name = "sdhci",
	.id = 2,
	.resource = board_sdio3_resource,
	.num_resources   = ARRAY_SIZE(board_sdio3_resource),
	.dev      = {
		.platform_data = &board_sdio_param[2],
	},
};


/* Common devices among all the Rhea boards (Rhea Ray, Rhea Berri, etc.) */
static struct platform_device *board_sdio_plat_devices[] __initdata = {
	&board_sdio2_device,
	&board_sdio3_device,
	&board_sdio1_device,
};

void dump_rhea_pin_config(struct pin_config *debug_pin_config)
{

	printk("%s-drv_sth:%d, input_dis:%d, slew_rate_ctrl:%d, pull_up:%d, pull_dn:%d, hys_en:%d, sel:%d\n"
				,__func__
				,debug_pin_config->reg.b.drv_sth
				,debug_pin_config->reg.b.input_dis
				,debug_pin_config->reg.b.slew_rate_ctrl
				,debug_pin_config->reg.b.pull_up
				,debug_pin_config->reg.b.pull_dn
				,debug_pin_config->reg.b.hys_en
				,debug_pin_config->reg.b.sel);

}


int configure_sdio_pullup(bool pull_up)
{
    int ret=0;
	char i;
	struct pin_config new_pin_config;

	pr_err("%s, Congifure Pin with pull_up:%d\n",__func__,pull_up);
	
	new_pin_config.name = PN_SDCMD;

	ret = pinmux_get_pin_config(&new_pin_config);
	if(ret){
		printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
		return ret;
	}
	
	printk("%s-Before setting pin with new setting\n",__func__);
	dump_rhea_pin_config(&new_pin_config);	
	if(pull_up){
		new_pin_config.reg.b.pull_up =PULL_UP_ON;
		new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
	}
	else{
		new_pin_config.reg.b.pull_up =PULL_UP_OFF;
		new_pin_config.reg.b.pull_dn =PULL_DN_ON;
	}
		
	ret = pinmux_set_pin_config(&new_pin_config);
	if(ret){
		pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
		return ret;
	}
		
	ret = pinmux_get_pin_config(&new_pin_config);
	if(ret){
		pr_err("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
		return ret;
	}
	printk("%s-after setting pin with new setting\n",__func__);
	dump_rhea_pin_config(&new_pin_config);

	for(i=0;i<4;i++){
		new_pin_config.name = (PN_SDDAT0+i);	
		ret = pinmux_get_pin_config(&new_pin_config);
		if(ret){
			printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
			return ret;
		}
		printk("%s-Before setting pin with new setting\n",__func__);
		dump_rhea_pin_config(&new_pin_config);	
		if(pull_up){
			new_pin_config.reg.b.pull_up =PULL_UP_ON;
			new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
		}
		else{
			new_pin_config.reg.b.pull_up =PULL_UP_OFF;
			new_pin_config.reg.b.pull_dn =PULL_DN_ON;
		}
		
		ret = pinmux_set_pin_config(&new_pin_config);
		if(ret){
			pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
			return ret;
		}
		
		ret = pinmux_get_pin_config(&new_pin_config);
		if(ret){
			pr_err("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
			return ret;
		}
		printk("%s-after setting pin with new setting\n",__func__);
		dump_rhea_pin_config(&new_pin_config);
	}	

	return ret;
}

void __init board_add_sdio_devices(void)
{
	platform_add_devices(board_sdio_plat_devices, ARRAY_SIZE(board_sdio_plat_devices));
}

#ifdef CONFIG_BACKLIGHT_PWM

static struct platform_pwm_backlight_data bcm_backlight_data = {
/* backlight */
	.pwm_name 	= "kona_pwmc:4",
	.max_brightness = 32,   /* Android calibrates to 32 levels*/
	.dft_brightness = 32,
//#ifdef CONFIG_MACH_RHEA_RAY_EDN1X
	.polarity       = 1,    /* Inverted polarity */
//#endif
	.pwm_period_ns 	=  5000000,
};

static struct platform_device bcm_backlight_devices = {
	.name 	= "pwm-backlight",
	.id 	= 0,
	.dev 	= {
		.platform_data  =       &bcm_backlight_data,
	},
};

#else

static struct platform_ktd259b_backlight_data bcm_ktd259b_backlight_data = {
	.max_brightness = 255,
	.dft_brightness = 143,
	.ctrl_pin = 95,
};


static struct platform_device bcm_backlight_devices = {
    .name           = "panel",
	.id 		= -1,
	.dev 	= {
		.platform_data  =       &bcm_ktd259b_backlight_data,
	},
	
};

#endif /*CONFIG_BACKLIGHT_PWM */




static struct platform_device touchkeyled_device = {
	.name 		= "touchkey-led",
	.id 		= -1,
};

#if defined (CONFIG_REGULATOR_TPS728XX)
#if defined(CONFIG_MACH_RHEA_RAY) || defined(CONFIG_MACH_RHEA_RAY_EDN1X) \
	|| defined(CONFIG_MACH_RHEA_FARADAY_EB10) \
	|| defined(CONFIG_MACH_RHEA_DALTON) || defined(CONFIG_MACH_RHEA_RAY_EDN2X) || defined(CONFIG_MACH_RHEA_SS) \
	|| defined(CONFIG_MACH_RHEA_RAY_DEMO) || defined(CONFIG_MACH_RHEA_SS_ZANIN)
#define GPIO_SIM2LDO_EN		86
#endif
#ifdef CONFIG_GPIO_PCA953X
#define GPIO_SIM2LDOVSET	(KONA_MAX_GPIO + 7)
#endif
#define TPS728XX_REGL_ID        (BCM59055_MAX_LDO + 0)
struct regulator_consumer_supply sim2_supply[] = {
	{ .supply = "sim2_vcc" },
	{ .supply = "sim2ldo_uc" },
};

static struct regulator_init_data tps728xx_regl_initdata = {
	.constraints = {
		.name = "sim2ldo",
		.min_uV = 1300000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
			REGULATOR_CHANGE_VOLTAGE,
		.always_on = 0,
		.boot_on = 0,
	},
	.num_consumer_supplies = ARRAY_SIZE(sim2_supply),
	.consumer_supplies = sim2_supply,
};

struct tps728xx_plat_data tps728xx_data = {
	.gpio_vset	= GPIO_SIM2LDOVSET,
	.gpio_en	= GPIO_SIM2LDO_EN,
	.vout0		= 1800000,
	.vout1		= 3000000,
	.initdata	= &tps728xx_regl_initdata,
};

struct platform_device tps728xx_device = {
	.name = "tps728xx-regulator",
	.id = -1,
	.dev	=	{
		.platform_data = &tps728xx_data,
	},
};

/* Register userspace and virtual consumer for SIM2LDO */
#ifdef CONFIG_REGULATOR_USERSPACE_CONSUMER
static struct regulator_bulk_data tps728xx_bd_sim2 = {
	.supply = "sim2ldo_uc",
};

static struct regulator_userspace_consumer_data tps728xx_uc_data_sim2 = {
	.name = "sim2ldo",
	.num_supplies = 1,
	.supplies = &tps728xx_bd_sim2,
	.init_on = 0
};

static struct platform_device tps728xx_uc_device_sim2 = {
	.name = "reg-userspace-consumer",
	.id = TPS728XX_REGL_ID,
	.dev = {
		.platform_data = &tps728xx_uc_data_sim2,
	},
};
#endif
#ifdef CONFIG_REGULATOR_VIRTUAL_CONSUMER
static struct platform_device tps728xx_vc_device_sim2 = {
	.name = "reg-virt-consumer",
	.id = TPS728XX_REGL_ID,
	.dev = {
		.platform_data = "sim2ldo_uc"
	},
};
#endif
#endif /* CONFIG_REGULATOR_TPS728XX*/




#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))

#define BCMBT_VREG_GPIO       (100)
#define BCMBT_N_RESET_GPIO    (92)
#define BCMBT_AUX0_GPIO        (-1)   /* clk32 */
#define BCMBT_AUX1_GPIO        (-1)    /* UARTB_SEL */

static struct bcmbt_rfkill_platform_data board_bcmbt_rfkill_cfg = {
        .vreg_gpio = BCMBT_VREG_GPIO,
        .n_reset_gpio = BCMBT_N_RESET_GPIO,
        .aux0_gpio = BCMBT_AUX0_GPIO,  /* CLK32 */
        .aux1_gpio = BCMBT_AUX1_GPIO,  /* UARTB_SEL, probably not required */
};

static struct platform_device board_bcmbt_rfkill_device = {
        .name = "bcmbt-rfkill",
        .id = -1,
        .dev =
        {
                .platform_data=&board_bcmbt_rfkill_cfg,
        },
};
#endif

#ifdef CONFIG_BCM_BZHW
#define GPIO_BT_WAKE 7
#define GPIO_HOST_WAKE 34
static struct bcm_bzhw_platform_data bcm_bzhw_data = {
        .gpio_bt_wake = GPIO_BT_WAKE,
        .gpio_host_wake = GPIO_HOST_WAKE,
};

static struct platform_device board_bcm_bzhw_device = {
        .name = "bcm_bzhw",
        .id = -1,
        .dev = {
                .platform_data = &bcm_bzhw_data,
                },
};
#endif

#ifdef CONFIG_BCM_BT_LPM
#define GPIO_BT_WAKE   7 
#define GPIO_HOST_WAKE 34

static struct bcm_bt_lpm_platform_data brcm_bt_lpm_data = {
        .gpio_bt_wake = GPIO_BT_WAKE,
        .gpio_host_wake = GPIO_HOST_WAKE,
};

static struct platform_device board_bcmbt_lpm_device = {
        .name = "bcmbt-lpm",
        .id = -1,
        .dev =
        {
                .platform_data=&brcm_bt_lpm_data,
        },
};
#endif

#ifdef CONFIG_GPS_IRQ

#define GPIO_GPS_HOST_WAKE 124 

static struct gps_platform_data gps_hostwake_data= {
        .gpio_interrupt = GPIO_GPS_HOST_WAKE,
	.i2c_pdata  = {ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_400K),},
};

static struct i2c_board_info __initdata gpsi2c[] = {
	{
	I2C_BOARD_INFO("gpsi2c", 0x1FA),
	.flags = I2C_CLIENT_TEN,
	.platform_data = (void *)&gps_hostwake_data,
	.irq = gpio_to_irq(GPIO_GPS_HOST_WAKE),
	},
};

static struct platform_device gps_hostwake= {
        .name = "gps-hostwake",
        .id = -1,
        .dev =
        {
                .platform_data=&gps_hostwake_data,
        },
};
#endif

#ifdef CONFIG_SOC_CAMERA
//@HW
#define SR200PC20M_I2C_ADDRESS (0x40>>1) //sensor address in 2-wire serial bus(write) 
//#define SR030PC50_I2C_ADDRESS (0x60>>1)


//struct i2c_slave_platform_data rhea_cam_pdata = { ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_400K), };


static struct i2c_board_info rhea_i2c_camera[] = {
	{
		I2C_BOARD_INFO("camdrv_ss", SR200PC20M_I2C_ADDRESS),
	},
};
/*
static struct i2c_board_info rhea_i2c_camera_sub[] = {
	{
		I2C_BOARD_INFO("camdrv_ss_sub", SR030PC50_I2C_ADDRESS),
	},
};
*/

static int rhea_camera_power(struct device *dev, int on)
{
		
	printk("rhea_camera_power %d\n",on);
	if(!camdrv_ss_power(0,(bool)on))
	{
		  printk("%s,camdrv_ss_power failed for MAIN CAM!! \n", __func__);
		return -1;
	}
	return 0;

}

static int rhea_camera_reset(struct device *dev)
{
	/* reset the camera gpio */
	
	return 0;
}
	
static struct v4l2_subdev_sensor_interface_parms sr200pc20m_if_params = {
	.if_type = V4L2_SUBDEV_SENSOR_SERIAL,
	.if_mode = V4L2_SUBDEV_SENSOR_MODE_SERIAL_CSI2,
	.orientation = V4L2_SUBDEV_SENSOR_ORIENT_90,
	.facing = V4L2_SUBDEV_SENSOR_BACK,
	.parms.serial = {
		.lanes = 1,
		.channel = 0,
		.phy_rate = 0,
		.pix_clk = 0,
        .hs_term_time = 0x7,
        .hs_settle_time = 0x5
	},
};


static struct soc_camera_link iclink_sr200pc20m = {
	.bus_id		= 0,
	
	.board_info	= &rhea_i2c_camera[0],
	.i2c_adapter_id	= 0,
	.module_name	= "camdrv_ss",
	.power		= &rhea_camera_power,
	.reset		= &rhea_camera_reset,
	.priv		= &sr200pc20m_if_params,
};

static struct platform_device rhea_camera = {
	.name	= "soc-camera-pdrv",
	.id		= 0,
	.dev	= {
		.platform_data = &iclink_sr200pc20m,
	},
};
#endif // CONFIG_SOC_CAMERA

#if 0
static struct v4l2_subdev_sensor_interface_parms sr030pc50_if_params = {
	.if_type = V4L2_SUBDEV_SENSOR_SERIAL,
	.if_mode = V4L2_SUBDEV_SENSOR_MODE_SERIAL_CSI2,
    .orientation =V4L2_SUBDEV_SENSOR_LANDSCAPE,
    .facing = V4L2_SUBDEV_SENSOR_FRONT,
	.parms.serial = {
		.lanes = 1,
		.channel = 1,
		.phy_rate = 0,
		.pix_clk = 0,
		.hs_term_time = 0x7
	},
};
static struct soc_camera_link iclink_sr030pc50 = {
	.bus_id		= 0,
	
	.board_info	= &rhea_i2c_camera_sub[0],
	.i2c_adapter_id	= 0,
	.module_name	= "camdrv_ss_sub",
	.power		= &rhea_camera_power_sub,
	.reset		= &rhea_camera_reset_sub,
	.priv		= &sr030pc50_if_params,
};

static struct platform_device rhea_camera_sub = {
	.name	= "soc-camera-pdrv",
	.id		= 1,
	.dev	= {
		.platform_data = &iclink_sr030pc50,
	},
};
#endif


#ifdef CONFIG_WD_TAPPER
static struct wd_tapper_platform_data wd_tapper_data = {
  /* Set the count to the time equivalent to the time-out in seconds
* required to pet the PMU watchdog to overcome the problem of reset in
* suspend*/
  .count = 300,
   .lowbattcount = 30,
   .verylowbattcount = 1,
   .ch_num = 1,
   .name = "aon-timer",
};

static struct platform_device wd_tapper = {
   .name = "wd_tapper",
   .id = 0,
   .dev = {
      .platform_data = &wd_tapper_data,
   },
};
#endif

/* Rhea Ray specific platform devices */
static struct platform_device *rhea_ray_plat_devices[] __initdata = {
#ifdef CONFIG_KEYBOARD_BCM
	&bcm_kp_device,
#endif
#ifdef CONFIG_KONA_HEADSET_MULTI_BUTTON
	&headset_device,
#endif
#ifdef CONFIG_BCM_SS_VIBRA
	&bcm_vibrator_device,
#endif
#ifdef CONFIG_DMAC_PL330
	&pl330_dmac_device,
#endif
#ifdef CONFIG_HAPTIC_SAMSUNG_PWM
	&haptic_pwm_device,
#endif
	&bcm_backlight_devices,
/* TPS728XX device registration */
#ifdef CONFIG_REGULATOR_TPS728XX
	&tps728xx_device,
#endif
#if 0
#ifdef CONFIG_FB_BRCM_RHEA
	&rhea_ss_smi_display_device,	
#endif
#endif

#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))
    &board_bcmbt_rfkill_device,
#endif

#ifdef CONFIG_BCM_BZHW
        &board_bcm_bzhw_device,
#endif

#ifdef CONFIG_BCM_BT_LPM
    &board_bcmbt_lpm_device,
#endif
#ifdef CONFIG_SOC_CAMERA
	&rhea_camera,
#endif
#if 0 // NO SUB CAMERA on Crosica
	&rhea_camera_sub,
#endif

#ifdef CONFIG_GPS_IRQ
        &gps_hostwake,
#endif
#ifdef CONFIG_WD_TAPPER
   &wd_tapper,
#endif

        &touchkeyled_device
};

/* Add all userspace regulator consumer devices here */
#ifdef CONFIG_REGULATOR_USERSPACE_CONSUMER
struct platform_device *rhea_ray_userspace_consumer_devices[] __initdata = {
#if defined(CONFIG_REGULATOR_BCM_PMU59055) && defined(CONFIG_MFD_BCM_PMU590XX)
	&bcm59055_uc_device_sim,
#endif
#ifdef CONFIG_REGULATOR_TPS728XX
	&tps728xx_uc_device_sim2,
#endif
};
#endif

/* Add all virtual regulator consumer devices here */
#ifdef CONFIG_REGULATOR_VIRTUAL_CONSUMER
struct platform_device *rhea_ray_virtual_consumer_devices[] __initdata = {
#if defined(CONFIG_REGULATOR_BCM_PMU59055) && defined(CONFIG_MFD_BCM_PMU590XX)
	&bcm59055_vc_device_sim,
#endif
#ifdef CONFIG_REGULATOR_TPS728XX
	&tps728xx_vc_device_sim2,
#endif
};
#endif


/* Rhea Ray specific i2c devices */
static void __init rhea_ray_add_i2c_devices (void)
{
	/* 59055 on BSC - PMU */
#ifdef CONFIG_MFD_BCM_PMU590XX
	i2c_register_board_info(2,
			pmu_info,
			ARRAY_SIZE(pmu_info));
#endif
#ifdef CONFIG_GPIO_PCA953X
	i2c_register_board_info(1,
			pca953x_info,
			ARRAY_SIZE(pca953x_info));
#ifdef CONFIG_MACH_RHEA_RAY_EDN1X
	i2c_register_board_info(1,
			pca953x_2_info,
			ARRAY_SIZE(pca953x_2_info));
#endif
#endif

#if defined(CONFIG_GPS_IRQ)
	i2c_register_board_info(1, gpsi2c, ARRAY_SIZE(gpsi2c));
#endif

#ifdef CONFIG_TOUCHSCREEN_QT602240
	i2c_register_board_info(1,
			qt602240_info,
			ARRAY_SIZE(qt602240_info));
#endif
#ifdef CONFIG_BMP18X_I2C
	i2c_register_board_info(1,
			bmp18x_info,
			ARRAY_SIZE(bmp18x_info));
#endif
#ifdef CONFIG_AL3006
	i2c_register_board_info(1,
			al3006_info,
			ARRAY_SIZE(al3006_info));
#endif
#if (defined(CONFIG_MPU_SENSORS_MPU6050A2) || defined(CONFIG_MPU_SENSORS_MPU6050B1))
	i2c_register_board_info(1,
			mpu6050_info,
			ARRAY_SIZE(mpu6050_info));
#endif
#if defined (CONFIG_TOUCHSCREEN_F761)
	i2c_register_board_info(0x3, silabs_i2c_devices,
				ARRAY_SIZE(silabs_i2c_devices)); //PSJ
#endif

#if defined (CONFIG_RMI4_I2C)
	i2c_register_board_info(0x3, synaptics_i2c_devices,
				ARRAY_SIZE(synaptics_i2c_devices)); //PSJ
#endif

#if defined (CONFIG_TOUCHSCREEN_IST30XX)
	i2c_register_board_info(0x3, imagis_i2c_devices,
				ARRAY_SIZE(imagis_i2c_devices)); //PSJ

#endif

	i2c_register_board_info(0x4, rhea_ss_i2cgpio1_board_info,
				ARRAY_SIZE(rhea_ss_i2cgpio1_board_info));
#if defined(CONFIG_BCMI2CNFC)
	i2c_register_board_info(0x5, bcmi2cnfc, ARRAY_SIZE(bcmi2cnfc));
#endif
#ifdef CONFIG_KEYBOARD_EXPANDER
i2c_register_board_info(0x6, rhea_ss_i2cgpio2_board_info,
				ARRAY_SIZE(rhea_ss_i2cgpio2_board_info));
#endif
#if defined(CONFIG_STEREO_SPEAKER)// IVORY_AMP
i2c_register_board_info(0x7, rhea_ss_i2cgpio3_board_info,
				ARRAY_SIZE(rhea_ss_i2cgpio3_board_info));
#endif

}

static int __init rhea_ray_add_lateInit_devices (void)
{
	board_add_sdio_devices();
#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT

	printk(KERN_ERR "Calling WLAN_INIT!\n");

	rhea_wlan_init();
	printk(KERN_ERR "DONE WLAN_INIT!\n");
#endif
	return 0;
}

static void __init rhea_ray_reserve(void)
{
	board_common_reserve();
}



/* uart platform data */

#define KONA_UART0_PA	UARTB_BASE_ADDR
#define KONA_UART1_PA	UARTB2_BASE_ADDR
#define KONA_UART2_PA	UARTB3_BASE_ADDR

#define KONA_UART_FCR_UART0	(UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_11)
#define KONA_UART_FCR_UART1	(UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10)
#define KONA_UART_FCR_UART2	(UART_FCR_ENABLE_FIFO | UART_FCR_R_TRIG_10)

#define KONA_8250PORT(name, clk)				\
{								\
	.membase    = (void __iomem *)(KONA_##name##_VA),	\
	.mapbase    = (resource_size_t)(KONA_##name##_PA),	\
	.irq	    = BCM_INT_ID_##name,			\
	.uartclk    = 26000000,					\
	.regshift   = 2,				\
	.iotype	    = UPIO_DWAPB,			\
	.type	    = PORT_16550A,			\
	.flags	    = UPF_BOOT_AUTOCONF | UPF_FIXED_TYPE | UPF_SKIP_TEST | \
						UPF_LOW_LATENCY, \
	.private_data = (void __iomem *)((KONA_##name##_VA) + \
						UARTB_USR_OFFSET), \
	.clk_name = clk,	\
	.fcr = KONA_UART_FCR_##name, \
}

static struct plat_serial8250_port board_uart_data[] = {
	KONA_8250PORT(UART0, "uartb_clk"),
	KONA_8250PORT(UART1, "uartb2_clk"),
	KONA_8250PORT(UART2, "uartb3_clk"),
	{
	 .flags = 0,
	 },
};

/* For Samsung long duration testing wake_lock must be hold once UART 
 * cable is connected, so that system won't go to sleep. 
 * uas_notify_init() will register for JIG_UART insertion and removal and
 * hold and release the wake_lock accordingly
*/
struct notifier_block nb[2];
#ifdef CONFIG_HAS_WAKELOCK
static struct wake_lock jig_uart_wl;
#endif

#ifdef CONFIG_KONA_PI_MGR
static struct pi_mgr_qos_node qos_node;
#endif

extern int bcmpmu_get_uas_sw_grp(void);
extern int musb_info_handler(struct notifier_block *nb, unsigned long event, void *para);
static int uas_jig_uart_handler(struct notifier_block *nb,
		unsigned long event, void *para)
{
	switch(event) {
	case BCMPMU_JIG_EVENT_UART:
		pr_info("%s: BCMPMU_JIG_EVENT_UART uart_connected\n", __func__);
#ifdef CONFIG_HAS_WAKELOCK
		if (wake_lock_active(&jig_uart_wl) == 0)
			wake_lock(&jig_uart_wl);
#endif

#ifdef CONFIG_KONA_PI_MGR
		pi_mgr_qos_request_update(&qos_node, 0);
#endif
		musb_info_handler(NULL, 0, 1);
		break;

	case BCMPMU_USB_EVENT_ID_CHANGE:
	default:
		pr_info("%s: UART JIG SW GRP %d\n",
			__func__, bcmpmu_get_uas_sw_grp());
		if(bcmpmu_get_uas_sw_grp() != UAS_SW_GRP_UART_JIG) {
#ifdef CONFIG_HAS_WAKELOCK
			if (wake_lock_active(&jig_uart_wl))
				wake_unlock(&jig_uart_wl);
#endif
		musb_info_handler(NULL, 0, 0);
#ifdef CONFIG_KONA_PI_MGR
		pi_mgr_qos_request_update(&qos_node, PI_MGR_QOS_DEFAULT_VALUE);
#endif
		}
		break;
	}
	return 0;
}

void uas_jig_force_sleep(void)
{
	pr_info("%s: UART JIG SW GRP=%d\n",
		__func__, bcmpmu_get_uas_sw_grp());

#ifdef CONFIG_HAS_WAKELOCK
	if (wake_lock_active(&jig_uart_wl)) {
		wake_unlock(&jig_uart_wl);
		pr_info("UART JIG Force\n");
	}
#endif
#ifdef CONFIG_KONA_PI_MGR
	pi_mgr_qos_request_update(&qos_node, PI_MGR_QOS_DEFAULT_VALUE);
#endif

}




static int __init uas_notify_init(void)
{
	int ret = 0;
	pr_info("uas_notify_init: STARTED\n");
#ifndef CONFIG_SEC_MAKE_LCD_TEST
	nb[0].notifier_call = uas_jig_uart_handler;
	nb[1].notifier_call = uas_jig_uart_handler;
	ret = bcmpmu_add_notifier(BCMPMU_JIG_EVENT_UART, &nb[0]);
	ret |= bcmpmu_add_notifier(BCMPMU_USB_EVENT_ID_CHANGE, &nb[1]);
	if (ret) {
		pr_info("%s: failed to register for JIG UART notification\n",
				__func__);
		return -1;
	}
#ifdef CONFIG_KONA_PI_MGR
	ret=pi_mgr_qos_add_request(&qos_node, "jig_uart", PI_MGR_PI_ID_ARM_SUB_SYSTEM,
					PI_MGR_QOS_DEFAULT_VALUE);
	if (ret)
	{
		pr_info("%s: failed to add jig_uart to qos\n",__func__);
		return -1;
	}
#endif 
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&jig_uart_wl, WAKE_LOCK_SUSPEND, "jig_uart_wake");
#endif
	if (bcmpmu_get_uas_sw_grp() == UAS_SW_GRP_UART_JIG) {
		pr_info("uas_notify_init: JIG UART CONNECTED\n");
#ifdef CONFIG_HAS_WAKELOCK
		if (wake_lock_active(&jig_uart_wl) == 0)
			wake_lock(&jig_uart_wl);
#endif
		musb_info_handler(NULL, 0, 1);
#ifdef CONFIG_KONA_PI_MGR
		ret=pi_mgr_qos_request_update(&qos_node, 0);
		if (ret)
		{
			pr_info("%s: failed to request update qos_node\n",__func__);
			return -1;
		}
#endif

	}
#endif
	
	return 0;
}

late_initcall(uas_notify_init);



#define TSP_SDA 89
#define TSP_SCL 90
#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
        .sda_pin    = TSP_SDA,
        .scl_pin    = TSP_SCL,
		//.sda_is_open_drain = true,
		//.scl_is_open_drain = true,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device touch_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 0x3,
        .dev        = {
            .platform_data  = &touch_i2c_gpio_data,
        },
};

#define SENSOR_SDA 15
#define SENSOR_SCL 32

static struct i2c_gpio_platform_data sensor_i2c_gpio_data = {
        .sda_pin    = SENSOR_SDA,
        .scl_pin    = SENSOR_SCL,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device sensor_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 0x4,
        .dev        = {
                .platform_data  = &sensor_i2c_gpio_data,
        },
};


#ifdef CONFIG_KEYBOARD_EXPANDER

#define KEY_EXPANDER_SCL 33
#define KEY_EXPANDER_SDA 34

static struct i2c_gpio_platform_data key_i2c_gpio_data = {
        .sda_pin    = KEY_EXPANDER_SDA,
        .scl_pin    = KEY_EXPANDER_SCL,
        .udelay  = 3,
        .timeout = 100,
};

static struct platform_device key_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 0x6,
        .dev        = {
            .platform_data  = &key_i2c_gpio_data,
        },
};
#endif

#if defined(CONFIG_STEREO_SPEAKER)// IVORY_AMP
#define AMP_SDA 119
#define AMP_SCL 121

static struct i2c_gpio_platform_data amp_i2c_gpio_data = {
        .sda_pin    = AMP_SDA,
        .scl_pin    = AMP_SCL,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device amp_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 0x7,
        .dev        = {
                .platform_data  = &amp_i2c_gpio_data,
        },
};
#endif


static struct platform_device *gpio_i2c_devices[] __initdata = {

#if defined(CONFIG_I2C_GPIO)
	&touch_i2c_gpio_device,
	&sensor_i2c_gpio_device,	
#if defined(CONFIG_BCMI2CNFC)
	&rhea_nfc_i2c_gpio_device,
#endif /* CONFIG_BCMI2CNFC */
	#ifdef CONFIG_KEYBOARD_EXPANDER
	&key_i2c_gpio_device,
	#endif
#if defined(CONFIG_STEREO_SPEAKER)// IVORY_AMP
	&amp_i2c_gpio_device,	
#endif

#endif
};

#endif	

#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
// +++ for board files, it may contain changeable values
struct spa_temp_tb batt_temp_tb[]=
{
	{869, -300},            /* -30 */
	{769, -200},			/* -20 */
	{635, -100},                    /* -10 */
	{574, -50},				/* -5 */
	{509,   0},                    /* 0   */
	{376,  100},                    /* 10  */
	{321,  100},                    /* 15  */
	{277,  200},                    /* 20  */
	{237,  250},                    /* 25  */
	{200,  300},                    /* 30  */
	{166,  300},                    /* 35  */
	{139,  400},                    /* 40  */
	{98 ,  500},                    /* 50  */
	{82 ,  500},                    /* 55  */
	{68 ,  600},                    /* 60  */
	{54 ,  650},                    /* 65  */
	{46 ,  700},            /* 70  */
	{34 ,  800},            /* 80  */
};

struct spa_power_data spa_power_pdata=
{
	.charger_name = "bcm59039_charger",
	.eoc_current=80, // Ivory 0.3 H/W
	.recharge_voltage=4140,
	.charging_cur_usb=450,
	.charging_cur_wall=650,
	#if defined(CONFIG_SPA_SUPPLEMENTARY_CHARGING)
	.backcharging_time = 40, //mins
	.recharging_eoc = 60, // mA
	#endif
	.suspend_temp_hot=600,
	.recovery_temp_hot=400,
	.suspend_temp_cold=-50,
	.recovery_temp_cold=0,
	.batt_temp_tb=&batt_temp_tb[0],
	.batt_temp_tb_len=ARRAY_SIZE(batt_temp_tb),
};
EXPORT_SYMBOL(spa_power_pdata);

static struct platform_device spa_power_device=
{
	.name = "spa_power",
	.id=-1,
	.dev.platform_data = &spa_power_pdata,
};
static struct platform_device spa_ps_device=
{
	.name = "spa_ps",
	.id=-1,
};
// --- for board files
#endif


static void __init rhea_ray_add_platform_data(void)
{
/* override the platform data in common.c */
	board_serial_device.dev.platform_data = board_uart_data;
}

/* All Rhea Ray specific devices */
static void __init rhea_ray_add_devices(void)
{
#ifdef CONFIG_KEYBOARD_BCM
	bcm_kp_device.dev.platform_data = &bcm_keypad_data;
#endif
	platform_add_devices(rhea_ray_plat_devices, ARRAY_SIZE(rhea_ray_plat_devices));

	rhea_ray_add_i2c_devices();
#ifdef CONFIG_REGULATOR_USERSPACE_CONSUMER
	platform_add_devices(rhea_ray_userspace_consumer_devices, ARRAY_SIZE(rhea_ray_userspace_consumer_devices));
#endif
#ifdef CONFIG_REGULATOR_VIRTUAL_CONSUMER
	platform_add_devices(rhea_ray_virtual_consumer_devices, ARRAY_SIZE(rhea_ray_virtual_consumer_devices));
#endif

#if defined(CONFIG_I2C_GPIO)
	
	platform_add_devices(gpio_i2c_devices, ARRAY_SIZE(gpio_i2c_devices));
	
#endif

#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
	platform_device_register(&spa_power_device);
	platform_device_register(&spa_ps_device);
#endif

	spi_register_board_info(spi_slave_board_info,
				ARRAY_SIZE(spi_slave_board_info));
}
#ifndef CONFIG_KONA_ATAG_DT
static void __init board_config_default_gpio(void)
{
	gpio_request(70, "WLAN_REG_ON"); 
	gpio_direction_output(70,0); 
	gpio_free(70);

	gpio_request(100, "BT_REG_ON"); 
	gpio_direction_output(100,0); 
	gpio_free(100);
	
	gpio_request(28, "GPS_EN"); 
	gpio_direction_output(28,0); 
	gpio_free(28);
	
	gpio_request(25, "NFC_EN"); 
	gpio_direction_output(25,0); 
	gpio_free(25);
	
	
}
#endif
#ifdef CONFIG_FB_BRCM_RHEA
/*
 *   	     KONA FRAME BUFFER DISPLAY DRIVER PLATFORM CONFIG
 */ 
struct kona_fb_platform_data konafb_devices[] __initdata = {
	{
		.dispdrv_name  = "ILI9341", 
		.dispdrv_entry = DISPDRV_GetFuncTable,
		.parms = {
			.w0 = {
				.bits = { 
					.boot_mode  = 0,  	  
					.bus_type   = RHEA_BUS_SMI,  
					.bus_no     = RHEA_BUS_0,
					.bus_ch     = RHEA_BUS_CH_1,
					.bus_width  = RHEA_BUS_WIDTH_16,
					.te_input   = RHEA_TE_IN_0_LCD,
					.col_mode_i = RHEA_CM_I_XRGB888,	  
					.col_mode_o = RHEA_CM_O_RGB888, 
				},	
			
			},
			.w1 = {
				.bits = { 
					.api_rev  = RHEA_LCD_BOOT_API_REV,
					.lcd_rst0 = 41,  	  
				}, 
			},
		},
	},
};

#include "rhea_fb_init.c"
#endif

void __init board_init(void)
{
#ifndef CONFIG_KONA_ATAG_DT
	board_config_default_gpio();
#endif

//	rhea_ray_add_platform_data();

	board_add_common_devices();
	rhea_ray_add_devices();
#ifdef CONFIG_FB_BRCM_RHEA
	/* rhea_fb_init.c */
	konafb_init();
#endif
	return;
}

void __init board_map_io(void)
{
	/* Map machine specific iodesc here */

	rhea_map_io();
}

late_initcall(rhea_ray_add_lateInit_devices);


MACHINE_START(RHEA, "rhea_ss_corsica")
	.map_io = board_map_io,
	.init_irq = kona_init_irq,
	.timer  = &kona_timer,
	.init_machine = board_init,
	.reserve = rhea_ray_reserve
MACHINE_END
