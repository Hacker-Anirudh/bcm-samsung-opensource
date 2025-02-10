/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/broadcom/displays/lcd_ili9486.h
*
* Unless you and Broadcom execute a separate DISPCTRL_WRitten software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior DISPCTRL_WRitten consent.
*******************************************************************************/

/****************************************************************************
*
*  lcd_ili9486.h
*
*  PURPOSE:
*    This is the LCD-specific code for a ili9486 module.
*
*****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#ifndef __BCM_LCD_HX8357D_H
#define __BCM_LCD_HX8357D_H

/*  LCD command definitions
#define PIXEL_FORMAT_RGB565  0x55   // for MPU & RGB interface
#define PIXEL_FORMAT_RGB666  0x66   // for MPU & RGB interface
#define PIXEL_FORMAT_RGB888  0x77   // for MPU & RGB interface

#define RESET_SEQ 	{DISPCTRL_WR_CMND, 0x2A,0},\
	{DISPCTRL_WR_DATA, 0, (dev->col_start) >> 8},\
	{DISPCTRL_WR_DATA, 0, dev->col_start & 0xFF},\
	{DISPCTRL_WR_DATA, 0, (dev->col_end) >> 8},\
	{DISPCTRL_WR_DATA, 0, dev->col_end & 0xFF},\
	{DISPCTRL_WR_CMND, 0x2B,0},\
	{DISPCTRL_WR_DATA, 0, (dev->row_start) >> 8},\
	{DISPCTRL_WR_DATA, 0, dev->row_start & 0xFF},\
	{DISPCTRL_WR_DATA, 0, (dev->row_end) >> 8},\
	{DISPCTRL_WR_DATA, 0, dev->row_end & 0xFF},\
	{DISPCTRL_WR_CMND, 0x2C,0}
*/

#define PANEL_DTC			0x6bc010
#define PANEL_AUO_HX8357D	0x5b4c91
#define PANEL_SHARP			0x6b1c10

//const char *LCD_panel_name = "HX8357D LCD";

/* DISP DRV API - Display Info
static DISPDRV_INFO_T Disp_Info = {
	DISPLAY_TYPE_LCD_STD,	// DISPLAY_TYPE_T          type;          
	320,			// UInt32                  width;         
	480,			// UInt32                  height;        
	DISPDRV_FB_FORMAT_RGB888_U, // DISPDRV_FB_FORMAT_T     input_format; //@HW
	//DISPDRV_FB_FORMAT_RGB565,
	DISPLAY_BUS_SMI,	// DISPLAY_BUS_T           bus_type;
	4,			// Bpp; : !!! init may overwrite
};
*/

DISPCTRL_REC_T power_on_seq_hx8357d_smi[] =
{

	/* + Initializing Setting 1 */
	{DISPCTRL_WR_CMND, 0xB9, 0}, 
	{DISPCTRL_WR_DATA, 0, 0xFF},
	{DISPCTRL_WR_DATA, 0, 0x83},
	{DISPCTRL_WR_DATA, 0, 0x57},

	{DISPCTRL_WR_CMND, 0xB0, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x77},
	{DISPCTRL_WR_DATA, 0, 0x01},

	{DISPCTRL_WR_CMND, 0xB1, 0},
	{DISPCTRL_WR_DATA, 0, 0x00},
	{DISPCTRL_WR_DATA, 0, 0x14},
	{DISPCTRL_WR_DATA, 0, 0x1E},
	{DISPCTRL_WR_DATA, 0, 0x1E},
	{DISPCTRL_WR_DATA, 0, 0xC3},
	{DISPCTRL_WR_DATA, 0, 0x44},
	
	{DISPCTRL_WR_CMND, 0xB3, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x00},
	{DISPCTRL_WR_DATA, 0, 0x00},
	{DISPCTRL_WR_DATA, 0, 0x06},
	{DISPCTRL_WR_DATA, 0, 0x06},
		
	{DISPCTRL_WR_CMND, 0xB4, 0},
	{DISPCTRL_WR_DATA, 0, 0x22},
	{DISPCTRL_WR_DATA, 0, 0x40},
	{DISPCTRL_WR_DATA, 0, 0x00},
	{DISPCTRL_WR_DATA, 0, 0x2A},
	{DISPCTRL_WR_DATA, 0, 0x2A},
	{DISPCTRL_WR_DATA, 0, 0x09},
	{DISPCTRL_WR_DATA, 0, 0x93},

	/* + Power Setting Sequence */
    {DISPCTRL_WR_CMND, 0xB6, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x45},

	{DISPCTRL_WR_CMND, 0xC0, 0},
	{DISPCTRL_WR_DATA, 0, 0x6C},
	{DISPCTRL_WR_DATA, 0, 0x6C},
	{DISPCTRL_WR_DATA, 0, 0x00},
	{DISPCTRL_WR_DATA, 0, 0x3C},
	{DISPCTRL_WR_DATA, 0, 0xC4},
	{DISPCTRL_WR_DATA, 0, 0x08},
	
    {DISPCTRL_WR_CMND, 0xC2, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x00},
	{DISPCTRL_WR_DATA, 0, 0x13},
	{DISPCTRL_WR_DATA, 0, 0x00},

    {DISPCTRL_WR_CMND, 0xCC, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x09},

	{DISPCTRL_WR_CMND, 0xE3, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x28},
    {DISPCTRL_WR_DATA, 0, 0x20},
    /* - Power Setting Sequence */

    /* + One Gamma Setting */
    {DISPCTRL_WR_CMND, 0xE0, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x0E},	//	1
    {DISPCTRL_WR_DATA, 0, 0x0D},	//	2
    {DISPCTRL_WR_DATA, 0, 0x19},	//	3
    {DISPCTRL_WR_DATA, 0, 0x29},	//	4
	{DISPCTRL_WR_DATA, 0, 0x32},	//	5
    {DISPCTRL_WR_DATA, 0, 0x4A},	//	6
    {DISPCTRL_WR_DATA, 0, 0x54},	//	7
    {DISPCTRL_WR_DATA, 0, 0x5C},	//	8
	{DISPCTRL_WR_DATA, 0, 0x3E},	//	9
	{DISPCTRL_WR_DATA, 0, 0x38},	//	10
	{DISPCTRL_WR_DATA, 0, 0x32},	//	11
	{DISPCTRL_WR_DATA, 0, 0x2B},	//	12
	{DISPCTRL_WR_DATA, 0, 0x27},	//	13
    {DISPCTRL_WR_DATA, 0, 0x23},	//	14
	{DISPCTRL_WR_DATA, 0, 0x1F},	//	15
	{DISPCTRL_WR_DATA, 0, 0x18},	//	16
	{DISPCTRL_WR_DATA, 0, 0x00},	//	17
    {DISPCTRL_WR_DATA, 0, 0x0D},	//	18
	{DISPCTRL_WR_DATA, 0, 0x19},	//	19
	{DISPCTRL_WR_DATA, 0, 0x29},	//	20
	{DISPCTRL_WR_DATA, 0, 0x32},	//	21
    {DISPCTRL_WR_DATA, 0, 0x4A},	//	22
    {DISPCTRL_WR_DATA, 0, 0x54},	//	23
	{DISPCTRL_WR_DATA, 0, 0x5D},	//	24
    {DISPCTRL_WR_DATA, 0, 0x3E},	//	25
	{DISPCTRL_WR_DATA, 0, 0x38},	//	26
	{DISPCTRL_WR_DATA, 0, 0x33},	//	27
    {DISPCTRL_WR_DATA, 0, 0x2B},	//	28
	{DISPCTRL_WR_DATA, 0, 0x27},	//	29
	{DISPCTRL_WR_DATA, 0, 0x23},	//	30
    {DISPCTRL_WR_DATA, 0, 0x20},	//	31
 	{DISPCTRL_WR_DATA, 0, 0x00},	//	32
    {DISPCTRL_WR_DATA, 0, 0x00},	//	33
	{DISPCTRL_WR_DATA, 0, 0x01},	//	34
	/* - One Gamma Setting */

	/* + Three Gamma Setting */
    {DISPCTRL_WR_CMND, 0xC1, 0}, 
    {DISPCTRL_WR_DATA, 0, 0x01},	//	1
    /* R Settings */
	{DISPCTRL_WR_DATA, 0, 0x00},	//	2
    {DISPCTRL_WR_DATA, 0, 0x0A},	//	3
    {DISPCTRL_WR_DATA, 0, 0x11},	//	4
	{DISPCTRL_WR_DATA, 0, 0x1C},	//	5
    {DISPCTRL_WR_DATA, 0, 0x24},	//	6
    {DISPCTRL_WR_DATA, 0, 0x2C},	//	7
    {DISPCTRL_WR_DATA, 0, 0x35},	//	8
	{DISPCTRL_WR_DATA, 0, 0x40},	//	9
	{DISPCTRL_WR_DATA, 0, 0x48},	//	10
	{DISPCTRL_WR_DATA, 0, 0x53},	//	11
	{DISPCTRL_WR_DATA, 0, 0x57},	//	12
	{DISPCTRL_WR_DATA, 0, 0x5D},	//	13
    {DISPCTRL_WR_DATA, 0, 0x66},	//	14
	{DISPCTRL_WR_DATA, 0, 0x70},	//	15
	{DISPCTRL_WR_DATA, 0, 0x78},	//	16
	{DISPCTRL_WR_DATA, 0, 0x82},	//	17
    {DISPCTRL_WR_DATA, 0, 0x8A},	//	18
	{DISPCTRL_WR_DATA, 0, 0x92},	//	19
	{DISPCTRL_WR_DATA, 0, 0x9B},	//	20
	{DISPCTRL_WR_DATA, 0, 0xA2},	//	21
    {DISPCTRL_WR_DATA, 0, 0xAB},	//	22
    {DISPCTRL_WR_DATA, 0, 0xB0},	//	23
	{DISPCTRL_WR_DATA, 0, 0xB9},	//	24
    {DISPCTRL_WR_DATA, 0, 0xC3},	//	25
	{DISPCTRL_WR_DATA, 0, 0xCB},	//	26
	{DISPCTRL_WR_DATA, 0, 0xD1},	//	27
    {DISPCTRL_WR_DATA, 0, 0xDB},	//	28
	{DISPCTRL_WR_DATA, 0, 0xE0},	//	29
	{DISPCTRL_WR_DATA, 0, 0xE9},	//	30
    {DISPCTRL_WR_DATA, 0, 0xEF},	//	31
 	{DISPCTRL_WR_DATA, 0, 0xF3},	//	32
    {DISPCTRL_WR_DATA, 0, 0xF8},	//	33
	{DISPCTRL_WR_DATA, 0, 0xFF},	//	34
	/* G Settings */
	{DISPCTRL_WR_DATA, 0, 0x00},	//	35
    {DISPCTRL_WR_DATA, 0, 0x08},	//	36
    {DISPCTRL_WR_DATA, 0, 0x10},	//	37
    {DISPCTRL_WR_DATA, 0, 0x18},	//	38
    {DISPCTRL_WR_DATA, 0, 0x20},	//	39
	{DISPCTRL_WR_DATA, 0, 0x28},	//	40
    {DISPCTRL_WR_DATA, 0, 0x30},	//	41
    {DISPCTRL_WR_DATA, 0, 0x38},	//	42
    {DISPCTRL_WR_DATA, 0, 0x40},	//	43
	{DISPCTRL_WR_DATA, 0, 0x48},	//	44
	{DISPCTRL_WR_DATA, 0, 0x50},	//	45
	{DISPCTRL_WR_DATA, 0, 0x58},	//	46
	{DISPCTRL_WR_DATA, 0, 0x60},	//	47
	{DISPCTRL_WR_DATA, 0, 0x68},	//	48
	{DISPCTRL_WR_DATA, 0, 0x70},	//	49
	{DISPCTRL_WR_DATA, 0, 0x78},	//	50
    {DISPCTRL_WR_DATA, 0, 0x80},	//	51
	{DISPCTRL_WR_DATA, 0, 0x88},	//	52
	{DISPCTRL_WR_DATA, 0, 0x90},	//	53
	{DISPCTRL_WR_DATA, 0, 0x98},	//	54
    {DISPCTRL_WR_DATA, 0, 0xA0},	//	55
	{DISPCTRL_WR_DATA, 0, 0xA8},	//	56
	{DISPCTRL_WR_DATA, 0, 0xB0},	//	57
	{DISPCTRL_WR_DATA, 0, 0xB8},	//	58
    {DISPCTRL_WR_DATA, 0, 0xC0},	//	59
    {DISPCTRL_WR_DATA, 0, 0xC8},	//	60
	{DISPCTRL_WR_DATA, 0, 0xD0},	//	61
    {DISPCTRL_WR_DATA, 0, 0xD8},	//	62
	{DISPCTRL_WR_DATA, 0, 0xE0},	//	63
	{DISPCTRL_WR_DATA, 0, 0xE8},	//	64
    {DISPCTRL_WR_DATA, 0, 0xF0},	//	65
	{DISPCTRL_WR_DATA, 0, 0xF8},	//	66
	{DISPCTRL_WR_DATA, 0, 0xFF},	//	67
    /* B Settings */
	{DISPCTRL_WR_DATA, 0, 0x00},	//	68
	{DISPCTRL_WR_DATA, 0, 0x00},	//	69
    {DISPCTRL_WR_DATA, 0, 0x09},	//	70
    {DISPCTRL_WR_DATA, 0, 0x13},	//	71
    {DISPCTRL_WR_DATA, 0, 0x18},	//	72
	{DISPCTRL_WR_DATA, 0, 0x1F},	//	73
    {DISPCTRL_WR_DATA, 0, 0x26},	//	74
    {DISPCTRL_WR_DATA, 0, 0x2E},	//	75
    {DISPCTRL_WR_DATA, 0, 0x36},	//	76
	{DISPCTRL_WR_DATA, 0, 0x3C},	//	77
	{DISPCTRL_WR_DATA, 0, 0x43},	//	78
	{DISPCTRL_WR_DATA, 0, 0x4B},	//	79
	{DISPCTRL_WR_DATA, 0, 0x55},	//	80
	{DISPCTRL_WR_DATA, 0, 0x5E},	//	81
    {DISPCTRL_WR_DATA, 0, 0x64},	//	82
	{DISPCTRL_WR_DATA, 0, 0x6C},	//	83
	{DISPCTRL_WR_DATA, 0, 0x74},	//	84
	{DISPCTRL_WR_DATA, 0, 0x7D},	//	85
    {DISPCTRL_WR_DATA, 0, 0x84},	//	86
	{DISPCTRL_WR_DATA, 0, 0x8B},	//	87
	{DISPCTRL_WR_DATA, 0, 0x91},	//	88
	{DISPCTRL_WR_DATA, 0, 0x98},	//	89
    {DISPCTRL_WR_DATA, 0, 0xA0},	//	90
    {DISPCTRL_WR_DATA, 0, 0xA8},	//	91
	{DISPCTRL_WR_DATA, 0, 0xAF},	//	92
    {DISPCTRL_WR_DATA, 0, 0xB5},	//	93
	{DISPCTRL_WR_DATA, 0, 0xBE},	//	94
	{DISPCTRL_WR_DATA, 0, 0xC3},	//	95
    {DISPCTRL_WR_DATA, 0, 0xCB},	//	96
	{DISPCTRL_WR_DATA, 0, 0xD1},	//	97
	{DISPCTRL_WR_DATA, 0, 0xD8},	//	98
    {DISPCTRL_WR_DATA, 0, 0xF8},	//	99
 	{DISPCTRL_WR_DATA, 0, 0xFF},	//	100
	/* - Three Gamma Setting */

	/* + Initializing Setting 2 */
	{DISPCTRL_WR_CMND, 0xEA, 0}, 
    {DISPCTRL_WR_DATA, 0, 0x03}, 

    {DISPCTRL_WR_CMND, 0x3A, 0}, 
	{DISPCTRL_WR_DATA, 0, 0x66},
	
	{DISPCTRL_WR_CMND, 0x36, 0}, 
	{DISPCTRL_WR_DATA, 0, 0xD0},
	
	{DISPCTRL_WR_CMND, 0x35, 0},
    /* - Initializing Setting 2 */

	{DISPCTRL_WR_CMND, 0x11, 0},	/* Sleep Out */
	{DISPCTRL_SLEEP_MS, 0, 140},   	

	/* + Display On */
	{DISPCTRL_WR_CMND, 0x29, 0}, 
    {DISPCTRL_LIST_END, 0, 0}
    /* - Display On */
};


DISPCTRL_REC_T power_off_seq_hx8357d_smi[] =
{
	{DISPCTRL_WR_CMND, 0x28,0},
	{DISPCTRL_SLEEP_MS, 0, 40},
	{DISPCTRL_WR_CMND, 0x10,0}, 
	{DISPCTRL_SLEEP_MS, 0, 140},
    {DISPCTRL_LIST_END, 0, 0}
};

DISPCTRL_REC_T sleep_in_seq_hx8357d_smi[] =
{
	{DISPCTRL_WR_CMND, 0x28, 0},
	{DISPCTRL_SLEEP_MS, 0, 40},
	{DISPCTRL_WR_CMND, 0x10, 0},
	{DISPCTRL_SLEEP_MS, 0, 140},
    {DISPCTRL_LIST_END, 0, 0}
};

DISPCTRL_REC_T sleep_out_seq_hx8357d_smi[] =
{
	{DISPCTRL_WR_CMND, 0x11,0},
	{DISPCTRL_SLEEP_MS, 0, 140},
	/* + Display On */
	{DISPCTRL_WR_CMND, 0x29, 0}, 
    {DISPCTRL_LIST_END, 0, 0}
};

DISPCTRL_REC_T display_on_seq_hx8357d_smi[] =
{
	{DISPCTRL_WR_CMND, 0x29,0},
	//{DISPCTRL_SLEEP_MS, 0, 100},
    {DISPCTRL_LIST_END, 0, 0}
};

DISPCTRL_REC_T display_off_seq_hx8357d_smi[] =
{
	{DISPCTRL_WR_CMND, 0x28,0},
	{DISPCTRL_SLEEP_MS, 0, 40},
	{DISPCTRL_WR_CMND, 0x10, 0},
	{DISPCTRL_SLEEP_MS, 0, 140},
    {DISPCTRL_LIST_END, 0, 0}
};

#endif /* __BCM_LCD_HX8357D_H */

