/*
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2010 ST-Ericsson SA
 *  Copyright (C) 2011 Tieto Poland
 *
 *  Author: Waldemar Rymarkiewicz <waldemar.rymarkiewicz@tieto.com>
 *          for ST-Ericsson
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <glib.h>
#include <gdbus.h>

#include "log.h"
#include "sap.h"

#include <ril.h>
#include <secril-client.h>
#include <Oem_ril_sap.h>
//#include <oemril.h>

#define SAP_SEC_IFACE "org.bluez.SimAccessSec"
#define SAP_SEC_PATH "/org/bluez/sapsec"

enum {
	SIM_DISCONNECTED= 0x00,
	SIM_CONNECTED	= 0x01,
	SIM_POWERED_OFF	= 0x02,
	SIM_MISSING	= 0x03
};

static DBusConnection *connection = NULL;

static int sim_card_conn_status = SIM_DISCONNECTED;
static void *sap_data = NULL;  /* SAP server private data.*/
static gboolean ongoing_call_status = FALSE;
static int max_msg_size_supported = 512;

/*ril related variables*/
static HRilClient ril_client = NULL;
static RilOnComplete req_handler = NULL;
static RilOnUnsolicited unsol_handler = NULL;
static RilOnError err_handler = NULL;
static gboolean ril_connected = FALSE;
static char err_buf[128];

void com_samsung_ril_client_sap_handle_request(int id);
static void sendSapReq(char cmd, char msgId);
static void sendSapApduReq(unsigned short apdu_len, unsigned char  *apdu_req);
static int onReqComplete(HRilClient client, const void *data, size_t datalen);
static int onUnsol(HRilClient client, const void *data, size_t datalen);
static int onError(void *data, int error);
static int handleSapConnectRes(ril_sap_res_connect *rsp);
static int handleSapAtrRes(ril_sap_res_transfer_atr *rsp);
static int handleSapApduRes(ril_sap_res_transfer_apdu *rsp);
static int handleSapStatusNoti(unsol_sap_status *noti);
static int handleSapStatusRes(ril_sap_res_sap_status *rsp);
static int handleSapSimPowerRes(ril_sap_res_sim_power *rsp);
static int handleSapReaderStatusRes(ril_sap_res_card_reader_status *rsp);


/*I had to go for this variable, do I have a better choice?*/
void *sap_device_needed;

void sap_connect_req(void *sap_device, uint16_t maxmsgsize)
{
    DBG("");
    sap_device_needed = sap_device;

    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_CONNECT_EVT);
}

void sap_disconnect_req(void *sap_device, uint8_t linkloss)
{
    DBG("");
    sap_device_needed = sap_device;

    // must be called for SIM RESET
    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_DISCONNECT_EVT);

    if (linkloss) {
        DBG("Link Loss!!!");
        return;
    }
}

void sap_transfer_apdu_req(void *sap_device, struct sap_parameter *param)
{
    DBG("");

    sap_device_needed = sap_device;

    sendSapApduReq(param->len,param->val);
}

void sap_transfer_atr_req(void *sap_device)
{
    DBG("");
    sap_device_needed = sap_device;
    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_ATR_EVT);
}

void sap_power_sim_off_req(void *sap_device)
{
    DBG("");
    sap_device_needed = sap_device;
    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_OFF_EVT);
}

void sap_power_sim_on_req(void *sap_device)
{
    DBG("");
    sap_device_needed = sap_device;
    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_ON_EVT);
}

void sap_reset_sim_req(void *sap_device)
{
    DBG("");
    sap_device_needed = sap_device;
    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_RESET_EVT);
}

void sap_transfer_card_reader_status_req(void *sap_device)
{
    DBG("");
    sap_device_needed = sap_device;
    com_samsung_ril_client_sap_handle_request(SAPS_RIL_SIM_CARD_READER_STATUS_EVT);
}

void sap_set_transport_protocol_req(void *sap_device,
    struct sap_parameter *param)
{
    DBG("");
    sap_device_needed = sap_device;
    sap_transport_protocol_rsp(sap_device, SAP_RESULT_NOT_SUPPORTED);
}

gboolean com_samsung_ril_client_sap_connect()
{
    int err;

    DBG("");

    if (ril_client != NULL || req_handler != NULL || unsol_handler != NULL ||
        err_handler != NULL || ril_connected) {
        DBG("Oops, already connected!!!");
        DBG("ril_client    = 0x%p", ril_client);
        DBG("req_handler   = 0x%p", req_handler);
        DBG("unsol_handler = 0x%p", unsol_handler);
        DBG("err_handler   = 0x%p", err_handler);
        DBG("ril_connected = %s", ril_connected ? "true" : "false");
        return FALSE;
    }

   ril_client = OpenClient_RILD();
    if (ril_client == NULL) {
        DBG("Failed to OpenClient_RILD()");
        return FALSE;
    }

    err = RegisterRequestCompleteHandler(ril_client, REQ_OEM_HOOK_RAW, onReqComplete);
    if (err == RIL_CLIENT_ERR_SUCCESS) {
        req_handler = onReqComplete;
    }
    else {
        DBG("Failed to RegisterRequestCompleteHandler(), err = %d", err);
        com_samsung_ril_client_sap_disconnect();
        return FALSE;
    }

    err = RegisterUnsolicitedHandler(ril_client, RIL_UNSOL_SAP, onUnsol);
    if (err == RIL_CLIENT_ERR_SUCCESS) {
        unsol_handler = onUnsol;
    }
    else {
        DBG("Failed to RegisterUnsolicitedHandler(), err = %d", err);
        com_samsung_ril_client_sap_disconnect();
        return FALSE;
    }

    err = RegisterErrorCallback(ril_client, onError, err_buf);
    if (err == RIL_CLIENT_ERR_SUCCESS) {
        err_handler = onError;
    }
    else {
        DBG("Failed to RegisterErrorCallback(), err = %d", err);
        com_samsung_ril_client_sap_disconnect();
        return FALSE;
    }

    err = Connect_RILD(ril_client);
    if (err == RIL_CLIENT_ERR_SUCCESS) {
        ril_connected = TRUE;
    }
    else {
        DBG("Failed to Connect_RILD(), err = %d", err);
        com_samsung_ril_client_sap_disconnect();
        return FALSE;
    }

    DBG("SAP driver RIL client connected, ril_client = 0x%p", ril_client);
    return TRUE;
}


/*Request response handler. The response handler is invoked in the client task context.
* Return is 0 or error code. */
static int onReqComplete(HRilClient client, const void *data, size_t datalen)
{
    const char *c = (const char *) data;
    oem_ril_sap_hdr *hdr = (oem_ril_sap_hdr *) data;

    DBG("datalen = %d, data = %02x %02x %02x %02x %02x %02x %02x %02x",
          datalen, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);

    if (hdr->func_id != OEM_FUNCTION_ID_SAP) {
        DBG("Not a SAP request. id = %02x, skip it...", hdr->func_id);
        return RIL_CLIENT_ERR_INVAL;
    }

   DBG("SAP cmd. id = %02x, skip it...", hdr->cmd);


    switch (hdr->cmd) {
    case OEM_SAP_CONNECT:
        return handleSapConnectRes((ril_sap_res_connect *) &c[4]);
    case OEM_SAP_TRANSFER_ATR:
        return handleSapAtrRes((ril_sap_res_transfer_atr *) &c[4]);
    case OEM_SAP_STATUS:
        return handleSapStatusRes((ril_sap_res_sap_status *) &c[4]);
    case OEM_SAP_READER_STATUS:
        return handleSapReaderStatusRes((ril_sap_res_card_reader_status *) &c[4]);
    case OEM_SAP_SIM_POWER:
        return handleSapSimPowerRes((ril_sap_res_sim_power *) &c[4]);
    case OEM_SAP_TRANSFER_APDU:
        return handleSapApduRes((ril_sap_res_transfer_apdu *) &c[4]);
    /*case OEM_SAP_SET_PROTOCOL:
        return handleSetProtocolRes((ril_sap_res_transport_protocol *) &c[4]);
    */

    default:
        DBG("Unknown SAP response. datalen = %d, data = %02x %02x %02x %02x",
             datalen, c[0], c[1], c[2], c[3]);
        break;

    }

    return 0;
}

/* Unsolicited response handler. The response handler is invoked in the client task context.
 * Return is 0 or error code.
 */
static int onUnsol(HRilClient client, const void *data, size_t datalen)
{
    const char *c = (const char *) data;
    oem_ril_sap_hdr *hdr = (oem_ril_sap_hdr *) data;

    DBG("datalen = %d, data = %02x %02x %02x %02x %02x",
          datalen, c[0], c[1], c[2], c[3], c[4]);

    if (hdr->func_id != OEM_FUNCTION_ID_SAP) {
        DBG("Not a SAP request. id = %02x, skip it...", hdr->func_id);
        return RIL_CLIENT_ERR_INVAL;
    }

    switch (hdr->cmd) {
   // case OEM_SAP_CONNECT:
      //  return handleSapConnectNoti((unsol_sap_connect *) &c[4]);
    case OEM_SAP_STATUS:
        return handleSapStatusNoti((unsol_sap_status *) &c[4]);
    default:
        DBG("Unknown SAP unsolicited response. datalen = %d, data = %02x %02x %02x %02x %02x",
               datalen, c[0], c[1], c[2], c[3], c[4]);
        break;
    }

    return 0;
}


/* Error handler. Return is 0 or error code. */
static int onError(void *data, int error)
{
    const char *c = (const char *) data;

    DBG("##########################################");
    DBG("error = %d, data = %02x %02x %02x %02x",
         error, c[0], c[1], c[2], c[3]);
    DBG("##########################################");
    return 0;
}

void com_samsung_ril_client_sap_handle_request(int id)
{
    DBG("service_sap_SAPService %d id: ", id);

    switch(id){
    // connect Req ..
    case SAPS_RIL_SIM_CONNECT_EVT:
        DBG("SAPS_RIL_SIM_CONNECT_EVT ");
        sendSapReq(OEM_SAP_CONNECT, OEM_SAP_CONNECT_REQ);
        break;
    // disconnect Req ..
    case SAPS_RIL_SIM_DISCONNECT_EVT:
        DBG("SAPS_RIL_SIM_DISCONNECT_EVT ");
        sendSapReq(OEM_SAP_CONNECT, OEM_SAP_DISCONNECT_REQ);
        break;
    // Transfer ATR Req ..
    case SAPS_RIL_SIM_ATR_EVT:
        DBG("SAPS_RIL_SIM_ATR_EVT ");
        sendSapReq(OEM_SAP_TRANSFER_ATR, OEM_SAP_TRANSFER_ATR_REQ);
        break;
    // SIM OFF ..
    case SAPS_RIL_SIM_OFF_EVT:
        DBG("SAPS_RIL_SIM_OFF_EVT ");
        sendSapReq(OEM_SAP_SIM_POWER, OEM_SAP_POWER_SIM_OFF_REQ);
        break;
    // SIM ON..
    case SAPS_RIL_SIM_ON_EVT:
        DBG("SAPS_RIL_SIM_ON_EVT ");
        sendSapReq(OEM_SAP_SIM_POWER, OEM_SAP_POWER_SIM_ON_REQ);
        break;
    // SIM RESET ..
    case SAPS_RIL_SIM_RESET_EVT:
        DBG("SAPS_RIL_SIM_RESET_EVT ");
        sendSapReq(OEM_SAP_SIM_POWER, OEM_SAP_RESET_SIM_REQ);
        break;
    // CARD READER STATUS ..
    case SAPS_RIL_SIM_CARD_READER_STATUS_EVT:
        DBG("SAPS_RIL_SIM_CARD_READER_STATUS_EVT ");
        sendSapReq(OEM_SAP_READER_STATUS, OEM_SAP_TRANSFER_CARD_READER_STATUS_REQ);
        break;
    //Transfer APDU req ..
    case SAPS_RIL_SIM_APDU_EVT:
        DBG("%s: SAPS_RIL_SIM_APDU_EVT ");
        //sendSapApduReq(req_len,apdu_req);
        break;
    // Unknown req..
    default:
        DBG("Oops: unknown event ID %d", id);
        break;
    }

}


static void sendSapReq(char cmd, char msgId)
{
    int  ret;
    int  len = 5;
    int  retry_count = 0;
    char data[8];

    DBG("");
    DBG("cmd = %d, msgId = %d", cmd, msgId);
    DBG("ril_client    = 0x%p", ril_client);
    DBG("req_handler   = 0x%p", req_handler);
    DBG("unsol_handler = 0x%p", unsol_handler);
    DBG("err_handler   = 0x%p", err_handler);

    data[0] = OEM_FUNCTION_ID_SAP;
    data[1] = cmd;
    data[2] = 0;
    data[3] = len;
    data[4] = msgId;

    do {
        if (retry_count != 0) {
            DBG("cmd/msgId (%d/%d) retry_count = %d",
                 cmd, msgId, retry_count);
        }
        ret = InvokeOemRequestHookRaw(ril_client, data, len);
    } while (ret == RIL_CLIENT_ERR_AGAIN && retry_count++ < MAX_RIL_RETRY);

    if (ret == RIL_CLIENT_ERR_SUCCESS) {
        DBG("cmd/msgId (%d/%d) done! retry_count = %d",
             cmd, msgId, retry_count);
    }
    else if (ret == RIL_CLIENT_ERR_AGAIN) {
        DBG("cmd/msgId (%d/%d) retried more than %d times. Give up...",
             cmd, msgId, MAX_RIL_RETRY);
    }
    else {
        DBG("cmd/MSGiD (%d/%d) failed. ret = %d", cmd, msgId, ret);
    }
}


// Send an SAP APDU request.
static void sendSapApduReq(unsigned short apdu_len, unsigned char  *apdu_req)
{
    int  ret;
    int  len = apdu_len + 2 + 4;
    int  retry_count = 0;
    char data[MAX_MSG_SIZE + 8];
    ril_sap_req_transfer_apdu *apdu;

    DBG("#################################################");
    DBG("apdu_len = %d", apdu_len);
    DBG("#################################################");


    if (apdu_len > MAX_MSG_SIZE) {
        DBG("APDU apdu_len (%d) > MAX_MSG_SIZE (%d)",apdu_len, MAX_MSG_SIZE);
        return;
    }


    data[0] = OEM_FUNCTION_ID_SAP;
    data[1] = OEM_SAP_TRANSFER_APDU;
    data[2] = (len >> 8) & 0xFF;
    data[3] = len & 0xFF;

    apdu = (ril_sap_req_transfer_apdu *) &data[4];
    apdu->apdu_len = apdu_len;
    memcpy(apdu->apdu, apdu_req, apdu_len);

// Samsung SWP Bluetooth feature
/*{
    char reqdata[100] ={0,};
    int i;
    for(i=0;( i< apdu->apdu_len && i < 18); i++){
        sprintf(&reqdata[i*5], "0x%02x ", apdu->apdu[i]);
    }
    LOGI("SJ APDU req: %s", reqdata);
    }*/
// End Samsung SWP Bluetooth feature


    do {
        if (retry_count != 0) {
            DBG("Send APDU req retry_count = %d", retry_count);
        }
        ret = InvokeOemRequestHookRaw(ril_client, data, len);
    } while (ret == RIL_CLIENT_ERR_AGAIN && retry_count++ < MAX_RIL_RETRY);

    if (ret == RIL_CLIENT_ERR_SUCCESS) {
        DBG("Send APDU req done! retry_count = %d", retry_count);
    }else if (ret == RIL_CLIENT_ERR_AGAIN) {
        DBG("Send APDU req retried more than %d times. Give up...",MAX_RIL_RETRY);
    }else {
        DBG("Send APDU req failed. ret = %d", ret);
    }
}


/* Request response handler. The response handler is invoked in the client task context.
 * Return is 0 or error code. Handle the SAP Connect command response */
static int handleSapConnectRes(ril_sap_res_connect *rsp)
{

    DBG("msg_id = %d, connection_status = %d, max_msg_size = %d",
        rsp->msg_id, rsp->connection_status, rsp->max_msg_size);

    switch (rsp->msg_id) {
        case OEM_SAP_CONNECT_RESP:
            switch (rsp->connection_status) {
                case OEM_SAP_CONNECT_OK:
                    DBG("SIM card connected ok. max_msg_size = %d",rsp->max_msg_size);
                    break;
                case OEM_SAP_CONNECT_UNABLE_ESTABLISH:
                case OEM_SAP_CONNECT_NOT_SUPPORT_MAX_SIZE:
                case OEM_SAP_CONNECT_TOO_SMALL_MAX_SIZE:
                    DBG("SIM card connection failed, connection_status = %d",
                        rsp->connection_status);
                    DBG("Send gracefully disconnect command to BTA");

                    break;

                default:
                    DBG("Unsupported SIM card connection_status = %d",rsp->connection_status);
                    break;
                }
                break;
        case OEM_SAP_DISCONNECT_RESP:
            DBG("SIM card disconnection, connection_status = %d",rsp->connection_status);
        break;
    }

    if(rsp->msg_id == OEM_SAP_CONNECT_RESP){
        sap_connect_rsp(sap_device_needed, rsp->connection_status,
            rsp->max_msg_size);
        sap_reset_sim_req(sap_device_needed);
        sap_status_ind(sap_device_needed, SAP_STATUS_CHANGE_CARD_RESET);
    } else if(rsp->msg_id == OEM_SAP_DISCONNECT_RESP){
        sap_disconnect_rsp(sap_device_needed);
    }

    return 0;
}

/* Handle the SAP ATR transfer command response */
static int handleSapAtrRes(ril_sap_res_transfer_atr *rsp)
{
    DBG("result_code = %d, atr_len = %d", rsp->result_code, rsp->atr_len);

    int i;
    for(i=0; i< rsp->atr_len; i++){
        DBG("@@@@ ATR req: %d", rsp->atr[i]);
    }

    sap_transfer_atr_rsp(sap_device_needed, rsp->result_code,
        (uint8_t*)rsp->atr, rsp->atr_len);
    return 0;
}

/* Handle the SAP APDU transfer command response */
static int handleSapApduRes(ril_sap_res_transfer_apdu *rsp)
{
    DBG("result_code = %d, res_apdu_len = %d",
        rsp->result_code, rsp->res_apdu_len);

    int i;
    for(i=0; (i< rsp->res_apdu_len); i++){
        DBG("@@@@ APDU req: %d", rsp->res_apdu[i]);
    }

    int ret = -3;
    ret = sap_transfer_apdu_rsp(sap_device_needed, rsp->result_code,
        (uint8_t*)rsp->res_apdu, rsp->res_apdu_len);
    DBG("sap_transfer_apdu_rsp returned(%d)", ret);
    //recheck if we need to add padding data...ok?
    return 0;
}


/* Handle unsolicited SAP status notification */
static int handleSapStatusNoti(unsol_sap_status *noti)
{
    DBG("card_status = %d", noti->card_status);
    sim_card_conn_status = noti->card_status;
    sap_status_ind(sap_device_needed, noti->card_status);

    return 0;
}

/* Handle the SAP Status response */
static int handleSapStatusRes(ril_sap_res_sap_status *rsp)
{

    DBG("status = %d", rsp->sap_status);

    sap_status_ind(sap_device_needed, rsp->sap_status);

    return 0;
}

/* Handle the SAP SIM card power command response */
static int handleSapSimPowerRes(ril_sap_res_sim_power *rsp)
{

    DBG("msg_id = %d, result_code = %d",
		rsp->msg_id, rsp->result_code);

    switch (rsp->msg_id) {
        case OEM_SAP_POWER_SIM_OFF_RESP:
            sap_power_sim_off_rsp(sap_device_needed, rsp->result_code);
            break;
        case OEM_SAP_POWER_SIM_ON_RESP:
            sap_power_sim_on_rsp(sap_device_needed,rsp->result_code);
            break;
        case OEM_SAP_RESET_SIM_RESP:
            sap_reset_sim_rsp(sap_device_needed, rsp->result_code);
			       break;
        default:
            DBG("Oops, unknown msg_id = %d", rsp->msg_id);
            return 0;
    }

    return 0;
}

/* Handle the SAP Card Reader Status response */
static int handleSapReaderStatusRes(ril_sap_res_card_reader_status *rsp)
{
    DBG("result_code = %d, card_reader_status = %d",
		rsp->result_code, rsp->card_reader_status);

    sap_transfer_card_reader_status_rsp(sap_device_needed, rsp->result_code, rsp->card_reader_status);
    /*env->CallVoidMethod(pJavaObject,method_onhandleTransferCardReaderStatusRes,
		(jint)rsp->result_code,(jint)rsp->card_reader_status);*/

    return 0;
}

void com_samsung_ril_client_sap_disconnect()
{
    int err;

    DBG("ril_client = 0x%p",  ril_client);

    if (ril_client == NULL) {
        return;
    }

    if (err_handler != NULL) {
        err = RegisterErrorCallback(ril_client, NULL, NULL);
        if (err != RIL_CLIENT_ERR_SUCCESS) {
            DBG("Failed to un-RegisterErrorCallback(), err = %d", err);
        }
        err_handler = NULL;
    }

    if (unsol_handler != NULL) {
        err = RegisterUnsolicitedHandler(ril_client, RIL_UNSOL_SAP, NULL);
        if (err != RIL_CLIENT_ERR_SUCCESS) {
            DBG("Failed to un-RegisterUnsolicitedHandler(), err = %d", err);
        }
        unsol_handler = NULL;
    }

    if (req_handler != NULL) {
        err = RegisterRequestCompleteHandler(ril_client, RIL_REQUEST_OEM_HOOK_RAW, NULL);
        if (err != RIL_CLIENT_ERR_SUCCESS) {
            DBG("Failed to un-RegisterRequestCompleteHandler(), err = %d", err);
        }
        req_handler = NULL;
    }

    err = CloseClient_RILD(ril_client);
    if (err != RIL_CLIENT_ERR_SUCCESS) {
        DBG("Failed to CloseClient_RILD(), err = %d", err);
    }
    ril_client = NULL;
    ril_connected = FALSE;

    DBG("Exit");
}


static inline DBusMessage *invalid_args(DBusMessage *msg)
{
	return g_dbus_create_error(msg, "org.bluez.Error.InvalidArguments",
					"Invalid arguments in method call");
}

static DBusMessage *ongoing_call(DBusConnection *conn, DBusMessage *msg,
						void *data)
{
	dbus_bool_t ongoing;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_BOOLEAN, &ongoing,
						DBUS_TYPE_INVALID))
		return invalid_args(msg);

	if (ongoing_call_status && !ongoing) {
		/* An ongoing call has finished. Continue connection.*/
		sap_connect_rsp(sap_data, SAP_STATUS_OK,
						max_msg_size_supported);
		sap_status_ind(sap_data, SAP_STATUS_CHANGE_CARD_RESET);
		ongoing_call_status = ongoing;
	} else if (!ongoing_call_status && ongoing) {
		/* An ongoing call has started.*/
		ongoing_call_status = ongoing;
	}

	DBG("OngoingCall status set to %d", ongoing_call_status);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *max_msg_size(DBusConnection *conn, DBusMessage *msg,
						void *data)
{
	dbus_uint32_t size;

	if (sim_card_conn_status == SIM_CONNECTED)
		return g_dbus_create_error(msg, "org.bluez.Error.Failed",
				"Can't change msg size when connected.");

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &size,
							DBUS_TYPE_INVALID))
		return invalid_args(msg);

	max_msg_size_supported = size;

	DBG("MaxMessageSize set to %d", max_msg_size_supported);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *card_status(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	dbus_uint32_t status;

	DBG("status %d", sim_card_conn_status);

	if (sim_card_conn_status != SIM_CONNECTED)
		return g_dbus_create_error(msg, "org.bluez.Error.Failed",
				"Can't change msg size when not connected.");

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &status,
							DBUS_TYPE_INVALID))
		return invalid_args(msg);

	switch (status) {
	case 0: /* card removed */
		sim_card_conn_status = SIM_MISSING;
		DBG("SAP_STATUS_CHANGE_CARD_REMOVED");
		sap_status_ind(sap_device_needed, SAP_STATUS_CHANGE_CARD_REMOVED);
		break;

	case 1: /* card inserted */
		if (sim_card_conn_status == SIM_MISSING) {
			sim_card_conn_status = SIM_CONNECTED;
			sap_status_ind(sap_data,
					SAP_STATUS_CHANGE_CARD_INSERTED);
		}
		break;

	case 2: /* card not longer available*/
		sim_card_conn_status = SIM_POWERED_OFF;
		sap_status_ind(sap_data, SAP_STATUS_CHANGE_CARD_NOT_ACCESSIBLE);
		break;

	default:
		return g_dbus_create_error(msg, "org.bluez.Error.Failed",
				"Unknown card status. Use 0, 1 or 2.");
	}

	DBG("Card status changed to %d", status);

	return dbus_message_new_method_return(msg);
}

static GDBusMethodTable dummy_methods[] = {
	{ "OngoingCall", "b", "", ongoing_call},
	{ "MaxMessageSize", "u", "", max_msg_size},
	{ "CardStatus", "u", "", card_status},
	{ }
};

int sap_init(void)
{
	DBG("");
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

	if (g_dbus_register_interface(connection, SAP_SEC_PATH,
				SAP_SEC_IFACE, dummy_methods, NULL, NULL,
				NULL, NULL) == FALSE) {
		error("sap-dummy interface %s init failed on path %s",
					SAP_SEC_IFACE, SAP_SEC_PATH);
		return -1;
	}

	return 0;
}

void sap_exit(void)
{
	DBG("");
	dbus_connection_unref(connection);
	connection = NULL;
}
