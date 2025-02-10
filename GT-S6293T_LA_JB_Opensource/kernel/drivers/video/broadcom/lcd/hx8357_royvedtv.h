/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/broadcom/displays/HX8357.h
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
*  HX8357.h
*
*  PURPOSE:
*    This is the LCD-specific code for a HX8357 module.
*
*****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#ifndef __HX8357_H__
#define __HX8357_H__

#include "display_drv.h"
#include "lcd.h"

//  LCD command definitions
#define PIXEL_FORMAT_RGB565	0x05   // for 16 bits
#define PIXEL_FORMAT_RGB666	0x06   // for 18 bits
#define PIXEL_FORMAT_RGB888	0x07   // for 24 bits

#define MAX_BRIGHTNESS		255
#define DEFAULT_BRIGHTNESS	160
#define DEFAULT_GAMMA_LEVEL	14/*180cd*/
#define MAX_GAMMA_VALUE 	24
#define MAX_GAMMA_LEVEL		25

#define DIM_BL 20
#define MIN_BL 30
#define MAX_BL 255

#define GAMMA_INDEX_NOT_SET	0xFFFF

typedef enum {
	HX8357_CMD_NOP					= 0x00,
	HX8357_CMD_SLEEP_IN				= 0x10,		
	HX8357_CMD_SLEEP_OUT				= 0x11,	
	HX8357_CMD_DISP_ON				= 0x29,
	HX8357_CMD_DISP_OFF				= 0x28,
	HX8357_CMD_PIXEL					= 0x3A,
	HX8357_CMD_TEON					= 0x35,
	HX8357_CMD_DISP_DIRECTION		= 0x36,	
	HX8357_CMD_PWR_CTL				= 0xB1,
	HX8357_CMD_SET_DSIP				= 0xB2,		
	HX8357_CMD_RGB_SET				= 0xB3,	
	HX8357_CMD_DISP_INVERSION		= 0xB4,	
	HX8357_CMD_BGP_VOL				= 0xB5,
	HX8357_CMD_VCOM					= 0xB6,		
	HX8357_CMD_ENABLE_CMD			= 0xB9,
	HX8357_CMD_MIPI_CMD				= 0xBA,
	HX8357_CMD_GOA_CTL				= 0xD5,
	HX8357_CMD_SRC_OPT				= 0xC0,	
	HX8357_CMD_SRC_DGC_OFF			= 0xC1,		
	HX8357_CMD_SETEQ				= 0xC2,			
	HX8357_CMD_SRC					= 0xC6,	
	HX8357_CMD_SET_CLOCK				= 0xCB,	
	HX8357_CMD_PANEL					= 0xCC,	
	HX8357_CMD_GAMMA					= 0xE0,	
	HX8357_CMD_EQ						= 0xE3,	
	HX8357_CMD_SET_IMAGEI				= 0xE9,
	HX8357_CMD_MESSI					= 0xEA,
	HX8357_CMD_BL					= 0x53,
} HX8357_CMD_T;

__initdata struct DSI_COUNTER hx8357_timing[] = {
	/* LP Data Symbol Rate Calc - MUST BE FIRST RECORD */
	{"ESC2LP_RATIO", DSI_C_TIME_ESC2LPDT, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0x0000003F, 1, 1},
	/* SPEC:  min =  100[us] + 0[UI] */
	/* SET:   min = 1000[us] + 0[UI]                             <= */
	{"HS_INIT", DSI_C_TIME_HS, 0,
		0, 100000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"HS_WAKEUP", DSI_C_TIME_HS, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"LP_WAKEUP", DSI_C_TIME_ESC, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 1, 1},
	/* SPEC:  min = 0[ns] +  8[UI] */
	/* SET:   min = 0[ns] + 12[UI]                               <= */
	{"HS_CLK_PRE", DSI_C_TIME_HS, 0,
		0, 0, 8, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 38[ns] + 0[UI]   max= 95[ns] + 0[UI] */
	/* SET:   min = 48[ns] + 0[UI]   max= 95[ns] + 0[UI]         <= */
	{"HS_CLK_PREPARE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 48, 0, 0, 0, 95, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 262[ns] + 0[UI] */
	/* SET:   min = 262[ns] + 0[UI]                              <= */
	{"HS_CLK_ZERO", DSI_C_TIME_HS, 0,
		0, 300, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min =  60[ns] + 52[UI] */
	/* SET:   min =  70[ns] + 52[UI]                             <= */
	{"HS_CLK_POST", DSI_C_TIME_HS, 0,
		0, 60, 52, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min =  60[ns] + 0[UI] */
	/* SET:   min =  70[ns] + 0[UI]                              <= */
	{"HS_CLK_TRAIL", DSI_C_TIME_HS, 0,
		0, 70, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min =  50[ns] + 0[UI] */
	/* SET:   min =  60[ns] + 0[UI]                              <= */
	{"HS_LPX", DSI_C_TIME_HS, 0,
		0, 50, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 40[ns] + 4[UI]      max= 85[ns] + 6[UI] */
	/* SET:   min = 50[ns] + 4[UI]      max= 85[ns] + 6[UI]      <= */
	{"HS_PRE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 55, 4, 0, 0, 85, 6, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 105[ns] + 6[UI] */
	/* SET:   min = 105[ns] + 6[UI]                              <= */
	{"HS_ZERO", DSI_C_TIME_HS, 0,
		0, 145, 10, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	/* SET:   min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	{"HS_TRAIL", DSI_C_TIME_HS, DSI_C_MIN_MAX_OF_2,
		0, 70, 4, 70, 8, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 100[ns] + 0[UI] */
	/* SET:   min = 110[ns] + 0[UI]                              <= */
	{"HS_EXIT", DSI_C_TIME_HS, 0,
		0, 100, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* min = 50[ns] + 0[UI] */
	/* LP esc counters are speced in LP LPX units.
	   LP_LPX is calculated by chal_dsi_set_timing
	   and equals LP data clock */
	{"LPX", DSI_C_TIME_ESC, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
	/* min = 4*[Tlpx]  max = 4[Tlpx], set to 4 */
	{"LP-TA-GO", DSI_C_TIME_ESC, 0,
		4, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
	/* min = 1*[Tlpx]  max = 2[Tlpx], set to 2 */
	{"LP-TA-SURE", DSI_C_TIME_ESC, 0,
		2, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
	/* min = 5*[Tlpx]  max = 5[Tlpx], set to 5 */
	{"LP-TA-GET", DSI_C_TIME_ESC, 0,
		5, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
};

__initdata DISPCTRL_REC_T hx8357_scrn_on[] = {
	{DISPCTRL_WR_CMND, HX8357_CMD_DISP_ON},
	{DISPCTRL_SLEEP_MS, 100},	
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T hx8357_scrn_off[] = {
	{DISPCTRL_WR_CMND, HX8357_CMD_DISP_OFF},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T hx8357_slp_in[] = {
	{DISPCTRL_WR_CMND, HX8357_CMD_DISP_OFF},
	{DISPCTRL_SLEEP_MS, 100},
	{DISPCTRL_WR_CMND, HX8357_CMD_SLEEP_IN},
	{DISPCTRL_SLEEP_MS, 120},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T hx8357_slp_out[] = {
	{DISPCTRL_WR_CMND, HX8357_CMD_SLEEP_OUT},
	{DISPCTRL_SLEEP_MS, 120},
	{DISPCTRL_WR_CMND, HX8357_CMD_DISP_ON},
	{DISPCTRL_SLEEP_MS, 100},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T hx8357_init_panel_vid[] = {
	{DISPCTRL_WR_CMND, HX8357_CMD_SLEEP_OUT},
	{DISPCTRL_SLEEP_MS, 120},
  {DISPCTRL_WR_CMND, HX8357_CMD_ENABLE_CMD},
	{DISPCTRL_WR_DATA, 0xFF},		
	{DISPCTRL_WR_DATA, 0x83},
	{DISPCTRL_WR_DATA, 0x57},

	{DISPCTRL_WR_CMND, HX8357_CMD_PWR_CTL},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x14},
	{DISPCTRL_WR_DATA, 0x15},
	{DISPCTRL_WR_DATA, 0x15},	
	{DISPCTRL_WR_DATA, 0xC6},
	{DISPCTRL_WR_DATA, 0x21},
	{DISPCTRL_WR_DATA, 0x54},

	{DISPCTRL_WR_CMND, HX8357_CMD_RGB_SET},
	{DISPCTRL_WR_DATA, 0x43},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x06},
	{DISPCTRL_WR_DATA, 0x06},

	{DISPCTRL_WR_CMND, HX8357_CMD_DISP_INVERSION},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x40},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x2A},	
	{DISPCTRL_WR_DATA, 0x2A},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x40},

	{DISPCTRL_WR_CMND, HX8357_CMD_BGP_VOL},
	{DISPCTRL_WR_DATA, 0x0B},
	{DISPCTRL_WR_DATA, 0x0B},
	{DISPCTRL_WR_DATA, 0x66},	

	{DISPCTRL_WR_CMND, HX8357_CMD_PANEL},
	{DISPCTRL_WR_DATA, 0x05},//0x02

	{DISPCTRL_WR_CMND, HX8357_CMD_SRC_OPT},
	{DISPCTRL_WR_DATA, 0x25},
	{DISPCTRL_WR_DATA, 0x25},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x38},	
	{DISPCTRL_WR_DATA, 0x08},
	{DISPCTRL_WR_DATA, 0x08},

	{DISPCTRL_WR_CMND, HX8357_CMD_SETEQ},
	{DISPCTRL_WR_DATA, 0x00},	
	{DISPCTRL_WR_DATA, 0x08},
	{DISPCTRL_WR_DATA, 0x04},	

	{DISPCTRL_WR_CMND, HX8357_CMD_EQ},
	{DISPCTRL_WR_DATA, 0x05},
	{DISPCTRL_WR_DATA, 0x05},

	{DISPCTRL_WR_CMND, HX8357_CMD_PIXEL},
	{DISPCTRL_WR_DATA, 0x77},

	{DISPCTRL_WR_CMND, HX8357_CMD_SET_IMAGEI},
	{DISPCTRL_WR_DATA, 0x20},

	{DISPCTRL_WR_CMND, HX8357_CMD_TEON},

	{DISPCTRL_WR_CMND, HX8357_CMD_GAMMA},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x06},
	{DISPCTRL_WR_DATA, 0x0F},
	{DISPCTRL_WR_DATA, 0x1E},	
	{DISPCTRL_WR_DATA, 0x2A},
	{DISPCTRL_WR_DATA, 0x40},
	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x53},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x40},		
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x32},
	{DISPCTRL_WR_DATA, 0x2E},
	{DISPCTRL_WR_DATA, 0x2A},	
	{DISPCTRL_WR_DATA, 0x26},
	{DISPCTRL_WR_DATA, 0x01},
	
	{DISPCTRL_WR_DATA, 0x04},
	{DISPCTRL_WR_DATA, 0x06},
	{DISPCTRL_WR_DATA, 0x0F},		
	{DISPCTRL_WR_DATA, 0x1E},
	{DISPCTRL_WR_DATA, 0x2A},
	{DISPCTRL_WR_DATA, 0x3F},
	{DISPCTRL_WR_DATA, 0x4B},	
	{DISPCTRL_WR_DATA, 0x53},
	{DISPCTRL_WR_DATA, 0x45},
	{DISPCTRL_WR_DATA, 0x40},
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x31},
	{DISPCTRL_WR_DATA, 0x2F},	
	{DISPCTRL_WR_DATA, 0x2A},
	{DISPCTRL_WR_DATA, 0x25},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x00},	
	{DISPCTRL_WR_DATA, 0x01},
	
	{DISPCTRL_WR_CMND, HX8357_CMD_DISP_ON},
	{DISPCTRL_SLEEP_MS, 60},
	{DISPCTRL_LIST_END, 0},
};

void hx8357_winset(char *msgData, DISPDRV_WIN_t *p_win)
{
	return;
}

__initdata struct lcd_config hx8357_cfg = {
	.name = "HX8357",
	.mode_supp = LCD_VID_ONLY,
	.phy_timing = &hx8357_timing[0],
	.max_lanes = 1,
	.max_hs_bps = 400000000,
	.max_lp_bps = 10000000,
	.phys_width = 45,
	.phys_height = 70,
	.init_cmd_seq = NULL,
	.init_vid_seq = &hx8357_init_panel_vid[0],
	.slp_in_seq = &hx8357_slp_in[0],
	.slp_out_seq = &hx8357_slp_out[0],
	.scrn_on_seq = &hx8357_scrn_on[0],
	.scrn_off_seq = &hx8357_scrn_off[0],
	.id_seq = NULL,
	.verify_id = false,	
	.updt_win_fn = hx8357_winset,
	.updt_win_seq_len = 0,
	.vid_cmnds = false,
	.vburst = true,
	.cont_clk = false,
	.hs = 8,
	.hbp = 60,
	.hfp = 60,
	.vs = 1,
	.vbp = 20,
	.vfp = 20,
};

#endif /* __HX8357_H__ */

