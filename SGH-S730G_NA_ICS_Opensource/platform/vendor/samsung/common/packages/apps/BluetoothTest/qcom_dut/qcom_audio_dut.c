/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2000-2001  Qualcomm Incorporated
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2009  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


//+ KJH 20100810 :: for android_set_aid_and_cap_k & log
/* init.rc
service bt_dut_cmd /system/bin/bcm_dut
    group bluetooth net_bt_admin
    disabled
    oneshot
*/
#include <sys/types.h>
#include <sys/socket.h>

#include <private/android_filesystem_config.h>
#include <sys/prctl.h>
#include <linux/capability.h>
#include "utils/Log.h"
//


//P1TF_BEGIN GA_BT(+neo. 2010.06.13.)
//to support DUT mode


//+ KJH 20100810 :: from android_bluez.c
/* Set UID to bluetooth w/ CAP_NET_RAW, CAP_NET_ADMIN and CAP_NET_BIND_SERVICE
 * (Android's init.rc does not yet support applying linux capabilities) */
void android_set_aid_and_cap_k() {
	LOGE("[GABT] %s + ",__FUNCTION__);
	prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
	setuid(AID_BLUETOOTH);

	struct __user_cap_header_struct header;
	struct __user_cap_data_struct cap;
	header.version = _LINUX_CAPABILITY_VERSION;
	header.pid = 0;
	cap.effective = cap.permitted = 1 << CAP_NET_RAW |
					1 << CAP_NET_ADMIN |
					1 << CAP_NET_BIND_SERVICE;
	cap.inheritable = 0;
	capset(&header, &cap);
	LOGE("[GABT] %s - ",__FUNCTION__);
}
//

static void hex_dump(char *pref, int width, unsigned char *buf, int len)
{
	register int i,n;

	for (i = 0, n = 1; i < len; i++, n++) {
		if (n == 1)
			LOGE("%s", pref);
		LOGE("[GABT] buf[%d] = %2.2X ", i, buf[i]);
		if (n == width) {
			LOGE("\n");
			n = 0;
		}
	}
	if (i && n!=1)
		LOGE("\n");
}

int main(int argc, char *argv[])
{
	int dev_id = -1;
	unsigned char buf[HCI_MAX_EVENT_SIZE];
	struct hci_filter flt;
	int len = 0;
	int dd = -1;
	uint16_t ocf;
	uint8_t ogf;
	int i = 0;
	hci_event_hdr *hdr;
	char* ptr;

	LOGE("[GABT] %s :: before : getuid=%d",__FUNCTION__, getuid());
	//android_set_aid_and_cap_k();
	LOGE("[GABT] %s :: after : getuid=%d",__FUNCTION__, getuid());
	if (dev_id < 0)
		dev_id = hci_get_route(NULL);

	if (dev_id < 0)
		 dev_id = 0;

	errno = 0;
	
	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		LOGE("[GABT] %s :: Hci Device open failed :: dev_id=%d, dd=%d",__FUNCTION__, dev_id, dd);
		printf("Hci Device open failed (dev_id = %d)", dev_id);
		perror("Hci Device open failed @");
		exit(EXIT_FAILURE);
	}

	/* Setup filter */
	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	hci_filter_all_events(&flt);
	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		LOGE("[GABT] %s :: HCI filter setup failed ",__FUNCTION__);
		perror("HCI filter setup failed");
		exit(EXIT_FAILURE);
	}
#if 0
	/* sleep mode disable */
	ogf = 0x3f;
	ocf = 0x0027;
	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x00;
	buf[7] = 0x00;
	buf[8] = 0x00;
	buf[9] = 0x00;
	buf[10] = 0x00;
	buf[11] = 0x00;
	len = 12;

	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		LOGE("[GABT] %s :: Send failed 1",__FUNCTION__);
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		LOGE("[GABT] %s :: Read failed 1",__FUNCTION__);
		perror("Read failed");
		exit(EXIT_FAILURE);
	}
	LOGE("[GABT] sleep mode disable -");


	/* Set Event Filter */
	ogf = 0x03;
	ocf = 0x0005;
	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x02; //Filter_Type          , 0x02 : Connection Setup.
	buf[1] = 0x00; //Filter_Condition_Type, 0x00 : Allow Connections from all devices.
	buf[2] = 0x02; //Condition
	len = 3;

	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		LOGE("[GABT] %s :: Send failed 2 ",__FUNCTION__);
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		LOGE("[GABT] %s :: Read failed 2 ",__FUNCTION__);
		perror("Read failed");
		exit(EXIT_FAILURE);
	}
	LOGE("[GABT] Set Event Filter -");

	/* Write Scan Enable */
	ogf = 0x03;
	ocf = 0x001a;	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x03; //Scan_Enable, 0x03 : Inquiry Scan enabled.
	               //                    Page Scan enabled.
	len = 1;
	
	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		LOGE("[GABT] %s :: Send failed 3 ",__FUNCTION__);
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		LOGE("[GABT] %s :: Read failed 3 ",__FUNCTION__);
		perror("Read failed");
		exit(EXIT_FAILURE);
	}
	LOGE("[GABT] Write Scan Enable -");
#endif

	/* Enable Device Under Test Mode */
	ogf = 0x06;
	ocf = 0x0003;	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x00;
	len = 0;

	LOGE("< HCI Command: ogf 0x%02x, ocf 0x%04x, plen %d\n", ogf, ocf, len);
	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		LOGE("[GABT] %s :: Send failed 4 ",__FUNCTION__);
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		LOGE("[GABT] %s :: Read failed 4 ",__FUNCTION__);
		perror("Read failed");
		exit(EXIT_FAILURE);
	}	
      
      	hdr = (void *)(buf + 1);
	ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
	len -= (1 + HCI_EVENT_HDR_SIZE);

	LOGE("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
	
	hex_dump("  ", 20, ptr, len); 
	LOGE("[GABT] Enable Device Under Test Mode -");

	////////////////////////////////////////////////////////////////////
	/* Set Event Filter */
	ogf = 0x03;
	ocf = 0x0005;
	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x02; //Filter_Type          , 0x02 : Connection Setup.
	buf[1] = 0x00; //Filter_Condition_Type, 0x00 : Allow Connections from all devices.
	buf[2] = 0x02; //Condition
	len = 3;

	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		perror("Read failed");
		exit(EXIT_FAILURE);
	}

	/* Write Scan Enable */
	ogf = 0x03;
	ocf = 0x001a;	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x03; //Scan_Enable, 0x03 : Inquiry Scan enabled.
	                    //                    Page Scan enabled.
	len = 1;
	
	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		perror("Read failed");
		exit(EXIT_FAILURE);
	}

	/* Write Authentication Enable*/
	ogf = 0x03;
	ocf = 0x0020;	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x00; //Authentication_Enable, 0x00: Authentication not required. Default.
	len = 1;
	
	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		perror("Read failed");
		exit(EXIT_FAILURE);
	}
	
	/* Write Encrytion Mode*/
	ogf = 0x03;
	ocf = 0x0022;	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x00; //Encryption_Mode, 0x00 : Encryption not required.
	len = 1;
	
	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		perror("Read failed");
		exit(EXIT_FAILURE);
	}
	
	/////////////////////////////////////////////////////////////////////////
	/* Send Remote Loopback Mode */
	ogf = 0x06;
	ocf = 0x0002;	
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x02;
	len = 1;

	LOGE("< HCI Command: ogf 0x%02x, ocf 0x%04x, plen %d\n", ogf, ocf, len);
	if (hci_send_cmd(dd, ogf, ocf, len, buf) < 0) {
		LOGE("[GABT] %s :: Send failed 4 ",__FUNCTION__);
		perror("Send failed");
		exit(EXIT_FAILURE);
	}

	len = read(dd, buf, sizeof(buf));
	if (len < 0) {
		LOGE("[GABT] %s :: Read failed 4 ",__FUNCTION__);
		perror("Read failed");
		exit(EXIT_FAILURE);
	}	
      
      	hdr = (void *)(buf + 1);
	ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
	len -= (1 + HCI_EVENT_HDR_SIZE);

	LOGE("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
	
	hex_dump("  ", 20, ptr, len); 
	LOGE("[GABT] Enable Bluetooth Audio Test Mode");	
	/////////////////////////////////////////////////////////////////////////

	hci_close_dev(dd);
	
	LOGE("[GABT] %s :: EXIT ",__FUNCTION__);
	return 0;
}
//P1TF_END
