/*
*	WeChat mpbledemo2 
*
*	author by anqiren  2014/12/02  V1.0bat
*
**/
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#include "nrf_gpio.h"
#include "epb_MmBp.h"
#include "ble_wechat_util.h"
#include "ble_wechat_service.h"
#include "app_error.h"
#include "md5.h"
#include "crc32.h"
#include "AES.h"
#include "mpbledemo2.h"
#include "comsource.h"
#include "LED.h"
#include "communication.h"

mpbledemo2_info m_info = {CMD_NULL, {NULL, 0}};

#if defined EAM_md5AndNoEnrypt || EAM_md5AndAesEnrypt
	uint8_t md5_type_and_id[16];
#endif

uint8_t challeange[CHALLENAGE_LENGTH] = {0x11,0x22,0x33,0x44}; //为了方便这里填了一组确定的数组，使用过程中请自行生成随机数

mpbledemo2_state mpbledemo2Sta = {false, false, false, false, false, false,0,0,0};
const uint8_t key[16] = DEVICE_KEY
uint8_t session_key[16] = {0};
data_handler *m_mpbledemo2_handler = NULL;


/**@brief   Function for the light initialization.
 *
 * @details Initializes all lights used by this application.
 */

int32_t mpbledemo2_get_md5(void)
{
	int32_t error_code = 0;
	#if defined EAM_md5AndNoEnrypt || EAM_md5AndAesEnrypt
	char device_type[] = DEVICE_TYPE;
	char device_id[] = DEVICE_ID;
	char argv[sizeof(DEVICE_TYPE) + sizeof(DEVICE_ID) - 1];
	memcpy(argv,device_type,sizeof(DEVICE_TYPE));
/*when add the DEVICE_ID to DEVICE_TYPE, the offset shuld -1 to overwrite '\0'  at the end of DEVICE_TYPE */
	memcpy(argv + sizeof(DEVICE_TYPE)-1,device_id,sizeof(DEVICE_ID));
#ifdef CATCH_LOG	
	printf ( "\r\nDEVICE_TYPE and DEVICE_ID:%s\r\n",argv);
#endif
	error_code = md5(argv, md5_type_and_id);
#ifdef CATCH_LOG
	printf ( "\r\nMD5:");
	for ( uint8_t i = 0; i < 16; i++ )
	printf ( " %02x", md5_type_and_id[i] );
	putchar ( '\n' );
#endif
#endif
	return error_code;
}

void mpbledemo2_reset()
{
	mpbledemo2Sta.auth_send = false;
	mpbledemo2Sta.auth_state = false;
	mpbledemo2Sta.indication_state = false;
	mpbledemo2Sta.init_send = false;
	mpbledemo2Sta.init_state = false;
	mpbledemo2Sta.send_data_seq = 0;
	mpbledemo2Sta.push_data_seq = 0;
	mpbledemo2Sta.seq = 0;
}
int32_t mpbledemo2_init(void)
{
	mpbledemo2_reset();
	return (mpbledemo2_get_md5());
}

/**@brief Function for handling button pull events.
 *
 * @param[in]   pin_no   The pin number of the button pressed.
 */
int32_t mpbledemo2_button_handler(void *p_wcs, uint8_t pin_no)
{
	ble_wechat_t *p_wcs_t = (ble_wechat_t*)p_wcs;
		if (m_mpbledemo2_handler == NULL) {
			m_mpbledemo2_handler = get_handler_by_type(PRODUCT_TYPE_MPBLEDEMO2);
		}
		uint8_t *data = NULL;
		uint32_t len = 0;
		switch (pin_no)
    {
        case MPBLEDEMO2_BUTTON_1:
				{
				#ifdef CATCH_LOG
					printf("\r\n button 1");
				#endif
					ARGS_ITEM_SET(mpbledemo2_info, m_mpbledemo2_handler->m_data_produce_args, cmd, CMD_SENDDAT);
					//填指令，这里是发送数据就填 CMD_SENDDAT
					ARGS_ITEM_SET(mpbledemo2_info, m_mpbledemo2_handler->m_data_produce_args, send_msg.len, sizeof(SEND_HELLO_WECHAT));
					//填发送数据长度，这里是发送数据就填 sizeof(SEND_HELLO_WECHAT)
					ARGS_ITEM_SET(mpbledemo2_info, m_mpbledemo2_handler->m_data_produce_args, send_msg.str, SEND_HELLO_WECHAT);
					//填发送的数据，这里是发送数据就填 SEND_HELLO_WECHAT
				}
            break;
        default:
            break;
    }
		m_mpbledemo2_handler->m_data_produce_func(m_mpbledemo2_handler->m_data_produce_args, &data, &len);
		//这里回调数据生产函数，来产生数据
		if(data == NULL)
		{
			return errorCodeProduce;
		}
		ble_wechat_indicate_data(p_wcs_t,m_mpbledemo2_handler, data, len);
		//发送数据
		m_mpbledemo2_handler->m_data_free_func(data,len);
		//发送数据之后回调相应的 free 函数，避免内存泄漏。 
		return 0;
}

int32_t device_auth(ble_wechat_t *p_wcs)
{	
	if (m_mpbledemo2_handler == NULL) {
			m_mpbledemo2_handler = get_handler_by_type(PRODUCT_TYPE_MPBLEDEMO2);
		}
		uint8_t *data = NULL;
		uint32_t len = 0;
		
		ARGS_ITEM_SET(mpbledemo2_info, m_mpbledemo2_handler->m_data_produce_args, cmd, CMD_AUTH);	
		m_mpbledemo2_handler->m_data_produce_func(m_mpbledemo2_handler->m_data_produce_args, &data, &len);
		if(data == NULL)
		{
			return errorCodeProduce;
		}
		//sent data
		
	#ifdef CATCH_LOG
		printf("\r\n auth send! ");
	#endif
		#ifdef CATCH_LOG
				printf("\r\n##send data: ");
				uint8_t *d = data;
				for(uint8_t i=0;i<len;++i){
				printf(" %x",d[i]);}
				BpFixHead *fix_head = (BpFixHead *)data;
				printf("\r\n CMDID: %d", ntohs(fix_head->nCmdId));
				printf("\r\n len: %d", ntohs(fix_head->nLength ));
				printf("\r\n Seq: %d", ntohs(fix_head->nSeq));
			#endif
				ble_wechat_indicate_data(p_wcs, m_mpbledemo2_handler, data, len);
				m_mpbledemo2_handler->m_data_free_func(data,len);
		return 0;
	}
	

int32_t	device_init(ble_wechat_t *p_wcs)
	{
		uint8_t *data = NULL;
		uint32_t len = 0;
		ARGS_ITEM_SET(mpbledemo2_info, m_mpbledemo2_handler->m_data_produce_args, cmd, CMD_INIT);
		m_mpbledemo2_handler->m_data_produce_func(m_mpbledemo2_handler->m_data_produce_args, &data, &len);
		if(data == NULL)
		{
			return errorCodeProduce;
		}
		//sent data
		
		ble_wechat_indicate_data(p_wcs, m_mpbledemo2_handler, data, len);
		m_mpbledemo2_handler->m_data_free_func(data,len);
	#ifdef CATCH_LOG
		printf("\r\n init send! ");
	#endif
		#ifdef CATCH_LOG
				printf("\r\n##send data: ");
				uint8_t *d = data;
				for(uint8_t i=0;i<len;++i){
				printf(" %x",d[i]);}
				BpFixHead *fix_head = (BpFixHead *)data;
				printf("\r\n CMDID: %d", ntohs(fix_head->nCmdId));
				printf("\r\n len: %d", ntohs(fix_head->nLength ));
				printf("\r\n Seq: %d", ntohs(fix_head->nSeq));
			#endif
		return 0;
}

/*设备处理 write 事件函数
		判断 indicate  是否被订阅，如果订阅了则设置状态位为 true，
		否则为 false。订阅成功之后才能发送数据。*/
static void mpbledemo2_on_write(ble_wechat_t *p_wcs, ble_evt_t * p_ble_evt)
{
	ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	if (p_evt_write->handle == p_wcs->indicate_handles.cccd_handle&& p_evt_write->len == 2) 
		{
			if (ble_srv_is_indication_enabled(p_evt_write->data))
				{
					mpbledemo2Sta.indication_state = true;
				#ifdef CATCH_LOG
					printf("\r\n indication_enabled!");
				#endif
				} 
			else
				{
					mpbledemo2Sta.indication_state = false;
				}
		}
}

/* 设备连接断开时，复位设备，以便在重新连接是重新进行 auth 和 init 等相关操作。 */
static void mpbledemo2_on_disconnect(ble_wechat_t *p_wcs, ble_evt_t * p_ble_evt)
{
	mpbledemo2_reset();
}
void ble_mpbledemo2_on_ble_evt(void *p_wcs, void *p_ble_evt)
{
	ble_wechat_t *p_wcs_t = (ble_wechat_t*)p_wcs;
	ble_evt_t* p_ble_evt_t = (ble_evt_t*)p_ble_evt;
	switch (p_ble_evt_t->header.evt_id)
    {
			case BLE_GAP_EVT_CONNECTED:
			break;
      case BLE_GAP_EVT_DISCONNECTED:
				mpbledemo2_on_disconnect(p_wcs_t, p_ble_evt);
			break;
			case BLE_GATTS_EVT_WRITE:
				mpbledemo2_on_write(p_wcs_t, p_ble_evt);
			break;
			default:
				break;
    }
}

/*设备主流程函数 
		根据设备状态，判断是否进行 auth 或者 init 操作*/
void mpbledemo2_main_process(void *p_wcs)
{
	ble_wechat_t *p_wcs_t = (ble_wechat_t*)p_wcs;
	int error_code;
	//			indicate 订阅成功  					&& 				还未 auth 成功			&&		还未发送 auth 数据包		就进行 auth 操作。
	if((mpbledemo2Sta.indication_state) && (!mpbledemo2Sta.auth_state) && (!mpbledemo2Sta.auth_send) )
	{
		error_code = device_auth(p_wcs_t);
		APP_ERROR_CHECK(error_code);
		mpbledemo2Sta.auth_send = true;
	//auth 之后把 auth_send 状态设置为 true，以避免重复发 auth 包。		
	}
	//			auth 成功了						&&					还未 init 成功包		&&				还未发送 init 包				就进行 init 操作。
	if((mpbledemo2Sta.auth_state) && (!mpbledemo2Sta.init_state) && (!mpbledemo2Sta.init_send))
	{
		error_code = device_init(p_wcs_t);
		APP_ERROR_CHECK(error_code);
		mpbledemo2Sta.init_send = true;
		//init 之后把 init_send 状态设置为 true，以避免重复发 init 包。
	}
	return ;
}

//设备释放内存函数
void mpbledemo2_data_free_func(uint8_t *data, uint32_t len)
{
	if(data)
		free(data);
		data = NULL;
}

//设备生产数据函数
/*
		根据 void *args 的指向的结构体（mpbledemo2_info）参数来打包相应的数据包放入
		uint8_t **r_data 指向的地址，并把长度放入 int *r_len
*/
void mpbledemo2_data_produce_func(void *args, uint8_t **r_data, uint32_t *r_len)
{
		
		static uint16_t bleDemoHeadLen = sizeof(BlueDemoHead);
		//获取 bluedemohead 的长度这是 mpbledemo2 与服务器通信定义的结构头。 
		mpbledemo2_info *info = (mpbledemo2_info *)args;
		//把 args 指向 mpbledemo2_info 类型，以获取其内容。
		BaseRequest basReq = {NULL};
		//这里把 BaseRequest  类型 basReq 设置为 null。 
		static uint8_t fix_head_len = sizeof(BpFixHead);
		//获取 BpFixHead 的长度这是微信服务与微信 APP 通信定义的结构头。
		BpFixHead fix_head = {0xFE, 1, 0, htons(ECI_req_auth), 0};
		//初始化 fix_head 	
		mpbledemo2Sta.seq++;
		///req 包数据不能为 0 首先要对 seq++操作。
		switch (info->cmd)
		{
		case CMD_AUTH:
			{
			#if defined EAM_md5AndAesEnrypt
				uint8_t deviceid[] = DEVICE_ID;
				static uint32_t seq = 0x00000001;//
				uint32_t ran = 0x11223344;//为了方便起见这里放了一个固定值做为随机数，在使用时请自行生成随机数。
				ran = t_htonl(ran);
				seq = t_htonl(seq);
				uint8_t id_len = strlen(DEVICE_ID);
				uint8_t* data = malloc(id_len+8);
				if(!data){printf("\r\nNot enough memory!");return;}
				memcpy(data,deviceid,id_len);
				memcpy(data+id_len,(uint8_t*)&ran,4);
				memcpy(data+id_len+4,(uint8_t*)&seq,4);
				uint32_t crc = crc32(0, data, id_len+8);
				crc = t_htonl(crc);
				memset(data,0x00,id_len+8);
				memcpy(data,(uint8_t*)&ran,4);
				memcpy(data+4,(uint8_t*)&seq,4);
				memcpy(data+8,(uint8_t*)&crc,4);	
				uint8_t CipherText[16];
				AES_Init(key);
				AES_Encrypt_PKCS7 (data, CipherText, 12, key);
				if(data){free(data);data = NULL;}
				AuthRequest authReq = {&basReq, true,{md5_type_and_id, MD5_TYPE_AND_ID_LENGTH}, PROTO_VERSION, AUTH_PROTO, (EmAuthMethod)AUTH_METHOD, true ,{CipherText, CIPHER_TEXT_LENGTH}, false, {NULL, 0}, false, {NULL, 0}, false, {NULL, 0},true,{DEVICE_ID,sizeof(DEVICE_ID)}};
				seq++;
			#endif
				
			#if defined EAM_macNoEncrypt
				static uint8_t mac_address[MAC_ADDRESS_LENGTH];
				get_mac_addr(mac_address);
				AuthRequest authReq = {&basReq, false,{NULL, 0}, PROTO_VERSION, AUTH_PROTO, (EmAuthMethod)AUTH_METHOD, false,{NULL, 0}, true, {mac_address, MAC_ADDRESS_LENGTH}, false, {NULL, 0}, false, {NULL, 0},true,{DEVICE_ID,sizeof(DEVICE_ID)}};
			#endif
				
			#if defined EAM_md5AndNoEnrypt
				AuthRequest authReq = {&basReq, true,{md5_type_and_id, MD5_TYPE_AND_ID_LENGTH}, PROTO_VERSION, (EmAuthMethod)AUTH_PROTO, (EmAuthMethod)AUTH_METHOD, false ,{NULL, 0}, false, {NULL, 0}, false, {NULL, 0}, false, {NULL, 0},true,{DEVICE_ID,sizeof(DEVICE_ID)}};
			#endif
				*r_len = epb_auth_request_pack_size(&authReq) + fix_head_len;
				*r_data = (uint8_t *)malloc(*r_len);
				if(!(*r_data)){printf("\r\nNot enough memory!");return;}
				if(epb_pack_auth_request(&authReq, *r_data+fix_head_len, *r_len-fix_head_len)<0)
				{
					*r_data = NULL;
					return;
				}
				fix_head.nCmdId = htons(ECI_req_auth);
				fix_head.nLength = htons(*r_len);
				fix_head.nSeq = htons(mpbledemo2Sta.seq);
				memcpy(*r_data, &fix_head, fix_head_len);
				return ;
		}
		case CMD_INIT:
			{
				//has challeange
				InitRequest initReq = {&basReq,false, {NULL, 0},true, {challeange, CHALLENAGE_LENGTH}};
				*r_len = epb_init_request_pack_size(&initReq) + fix_head_len;
			#if defined EAM_md5AndAesEnrypt
				uint8_t length = *r_len;				
				uint8_t *p = malloc(AES_get_length( *r_len-fix_head_len));
				if(!p){printf("\r\nNot enough memory!");return;}
				*r_len = AES_get_length( *r_len-fix_head_len)+fix_head_len;
			#endif
			//pack data
				*r_data = (uint8_t *)malloc(*r_len);
				if(!(*r_data)){printf("\r\nNot enough memory!");return;}
				if(epb_pack_init_request(&initReq, *r_data+fix_head_len, *r_len-fix_head_len)<0)
				{*r_data = NULL;return;}
				//encrypt body
			#if defined EAM_md5AndAesEnrypt
				AES_Init(session_key);
				AES_Encrypt_PKCS7(*r_data+fix_head_len,p,length-fix_head_len,session_key);//原始数据长度
				memcpy(*r_data + fix_head_len, p, *r_len-fix_head_len);
				if(p)free(p);
			#endif
				fix_head.nCmdId = htons(ECI_req_init);
				fix_head.nLength = htons(*r_len);
				fix_head.nSeq = htons(mpbledemo2Sta.seq);
				memcpy(*r_data, &fix_head, fix_head_len);
				return  ;
		}
		case CMD_SENDDAT:
			{
			#ifdef CATCH_LOG
				printf("\r\n msg to send : %s",(uint8_t*)info->send_msg.str);
			#endif
				BlueDemoHead  *bleDemoHead = (BlueDemoHead*)malloc(bleDemoHeadLen+info->send_msg.len);
				if(!bleDemoHead){printf("\r\nNot enough memory!");return;}
				bleDemoHead->m_magicCode[0] = MPBLEDEMO2_MAGICCODE_H;
				bleDemoHead->m_magicCode[1] = MPBLEDEMO2_MAGICCODE_L;
				bleDemoHead->m_version = htons( MPBLEDEMO2_VERSION);
				bleDemoHead->m_totalLength = htons(bleDemoHeadLen + info->send_msg.len);
				bleDemoHead->m_cmdid = htons(sendTextReq);
				bleDemoHead->m_seq = htons(mpbledemo2Sta.seq);
				bleDemoHead->m_errorCode = 0;	
				/*connect body and head.*/
				/*turn to uint8_t* befort offset.*/
				memcpy((uint8_t*)bleDemoHead+bleDemoHeadLen, info->send_msg.str, info->send_msg.len);			
				SendDataRequest sendDatReq = {&basReq, {(uint8_t*) bleDemoHead, (bleDemoHeadLen + info->send_msg.len)}, false, (EmDeviceDataType)NULL};
				*r_len = epb_send_data_request_pack_size(&sendDatReq) + fix_head_len;
			#if defined EAM_md5AndAesEnrypt
				uint16_t length = *r_len;
				uint8_t *p = malloc(AES_get_length( *r_len-fix_head_len));
				if(!p){printf("\r\nNot enough memory!");return;}
				*r_len = AES_get_length( *r_len-fix_head_len)+fix_head_len;
			#endif
				*r_data = (uint8_t *)malloc(*r_len);
				if(!(*r_data)){printf("\r\nNot enough memory!");return;}
				if(epb_pack_send_data_request(&sendDatReq, *r_data+fix_head_len, *r_len-fix_head_len)<0)
				{
					*r_data = NULL;
				#if defined EAM_md5AndAesEnrypt
					if(p){free(p);
					p = NULL;}
					#endif
					printf("\r\nepb_pack_send_data_request error!");
					return;
				}
			#if defined EAM_md5AndAesEnrypt
				//encrypt body
				AES_Init(session_key);
				AES_Encrypt_PKCS7(*r_data+fix_head_len,p,length-fix_head_len,session_key);//原始数据长度
				memcpy(*r_data + fix_head_len, p, *r_len-fix_head_len);
				if(p){free(p); p = NULL;}
			#endif
				fix_head.nCmdId = htons(ECI_req_sendData);
				fix_head.nLength = htons(*r_len);
				fix_head.nSeq = htons(mpbledemo2Sta.seq);
				memcpy(*r_data, &fix_head, fix_head_len);
				if(bleDemoHead){free(bleDemoHead);bleDemoHead = NULL;}
			#ifdef CATCH_LOG
				printf("\r\n##send data: ");
				uint8_t *d = *r_data;
				for(uint8_t i=0;i<*r_len;++i){
				printf(" %x",d[i]);}
				BpFixHead *fix_head = (BpFixHead *)*r_data;
				printf("\r\n CMDID: %d",ntohs(fix_head->nCmdId));
				printf("\r\n len: %d", ntohs(fix_head->nLength ));
				printf("\r\n Seq: %d", ntohs(fix_head->nSeq));
			#endif
			mpbledemo2Sta.send_data_seq++;
				
				return ;
		}
	}	
}
//设备处理数据函数
int mpbledemo2_data_consume_func(uint8_t *data, uint32_t len)
{	
		BpFixHead *fix_head = (BpFixHead *)data;
		//把数据指向 BpFixHead 固定头 
		uint8_t fix_head_len = sizeof(BpFixHead);
	#ifdef CATCH_LOG
		printf("\r\n##Received data: ");
		uint8_t *d = data;
		for(uint8_t i=0;i<len;++i){
		printf(" %x",d[i]);}
		printf("\r\n CMDID: %d", ntohs(fix_head->nCmdId));
		printf("\r\n len: %d", ntohs(fix_head->nLength));
		printf("\r\n Seq: %d",ntohs(fix_head->nSeq));
	#endif			
		switch(ntohs(fix_head->nCmdId))		//通过包头的 nCmdId 来判断数据包的类型 
		{
			case ECI_none:
				{
				}
				break;
			case ECI_resp_auth:			//authResp 包，则调用相应的解包函数来解包包体。
				{
					AuthResponse* authResp;
					authResp = epb_unpack_auth_response(data+fix_head_len,len-fix_head_len);
				#ifdef CATCH_LOG
					printf("\r\n@@Received 'authResp'\r\n");
				#endif
					if(!authResp){return errorCodeUnpackAuthResp;}
				#ifdef CATCH_LOG
					printf("\r\n unpack 'authResp' success!\r\n");
				#endif
					if(authResp->base_response)
					//判断是 base_response 中的错误码，如果 auth 成功，则设置 auth 状态为 true。//否则打印相关错误信息。 
					{
						if(authResp->base_response->err_code == 0)
						{
							mpbledemo2Sta.auth_state = true;

						}
						else
						{
						#ifdef CATCH_LOG
							printf("\r\n error code:%d",authResp->base_response->err_code);
						#endif
							#ifdef CATCH_LOG
							if(authResp->base_response->has_err_msg)
							{
								printf("\r\n base_response error msg:%s",authResp->base_response->err_msg.str);	
							}
							#endif
							epb_unpack_auth_response_free(authResp);
							return authResp->base_response->err_code;
						}
					}
				#if defined EAM_md5AndAesEnrypt// get sessionkey
					if(authResp->aes_session_key.len)
					{
					#ifdef CATCH_LOG
						printf("\r\nsession_key:");
					#endif
						AES_Init(key);			//执行扩展密钥操作
						AES_Decrypt(session_key,authResp->aes_session_key.data,authResp->aes_session_key.len,key);
						//解密数据			明文							密文													长度									初始化向量
						#ifdef CATCH_LOG
						for(uint8_t i = 0;i<16;i++)
						{
							printf(" 0x%02x",session_key[i]);	
						}
						#endif
					}
				#endif
					epb_unpack_auth_response_free(authResp);
				}
				break;
			case ECI_resp_sendData:
				{
				
			#ifdef CATCH_LOG
				printf("\r\n@@Received 'sendDataResp'\r\n");
			#endif
			#if defined EAM_md5AndAesEnrypt		
					uint32_t length = len- fix_head_len;//加密后数据长度
					uint8_t *p = malloc (length);
					if(!p){printf("\r\nNot enough memory!"); if(data)free(data);data = NULL; return 0;}
					AES_Init(session_key);//执行扩展密钥操作				
					AES_Decrypt(p,data+fix_head_len,len- fix_head_len,session_key);
					//解密数据
					uint8_t temp;
					temp = p[length - 1];//算出填充长度
					len = len - temp;//取加密前数据总长度
					memcpy(data + fix_head_len, p ,length -temp);//把明文放回
					if(p){free(p);p = NULL;}
			#endif	
				SendDataResponse *sendDataResp;
				sendDataResp = epb_unpack_send_data_response(data+fix_head_len,len-fix_head_len);
				if(!sendDataResp)
					{
						return errorCodeUnpackSendDataResp;
					}
			#ifdef CATCH_LOG
				BlueDemoHead *bledemohead = (BlueDemoHead*)sendDataResp->data.data;
				if(ntohs(bledemohead->m_cmdid) == sendTextResp)
					{
						printf("\r\n received msg: %s\r\n",sendDataResp->data.data+sizeof(BlueDemoHead));
					}
			#endif
					if(sendDataResp->base_response->err_code)
					{
						epb_unpack_send_data_response_free(sendDataResp);
						return sendDataResp->base_response->err_code;
					}
					epb_unpack_send_data_response_free(sendDataResp);
			}
				break;
			case ECI_resp_init:
				{
			#ifdef CATCH_LOG
				printf("\r\n@@Received 'initResp'\r\n");
			#endif
			#if defined EAM_md5AndAesEnrypt		
					uint32_t length = len- fix_head_len;//加密后数据长度
					uint8_t *p = malloc (length);
					if(!p){printf("\r\nNot enough memory!");if(data)free(data);data = NULL; return 0;}
					AES_Init(session_key);
					AES_Decrypt(p,data+fix_head_len,len- fix_head_len,session_key);
					//解密数据
					uint8_t temp;
					temp = p[length - 1];//算出填充长度
					len = len - temp;//取加密前数据总长度
					memcpy(data + fix_head_len, p ,length -temp);//把明文放回
					if(p){free(p);p = NULL;}
			#endif		
				InitResponse *initResp = epb_unpack_init_response(data+fix_head_len, len-fix_head_len);
				if(!initResp)
				{
					return errorCodeUnpackInitResp;
				}
				#ifdef CATCH_LOG
					printf("\r\n unpack 'initResp' success!");
				#endif	
					if(initResp->base_response)
					{
						if(initResp->base_response->err_code == 0)
						{
							if(initResp->has_challeange_answer)
							{
								if(crc32(0,challeange,CHALLENAGE_LENGTH) == initResp->challeange_answer)
								{
									mpbledemo2Sta.init_state = true;
								}
							}
							else mpbledemo2Sta.init_state = true;
							mpbledemo2Sta.wechats_switch_state = true;
						}
						else
						{
						#ifdef CATCH_LOG
							printf("\r\n error code:%d",initResp->base_response->err_code);
						#endif	
							if(initResp->base_response->has_err_msg)
							{
							#ifdef CATCH_LOG
								printf("\r\n base_response error msg:%s",initResp->base_response->err_msg.str);
							#endif	
							}
							epb_unpack_init_response_free(initResp);
							return initResp->base_response->err_code;
						}
					}
				epb_unpack_init_response_free(initResp);
			}
				break;
			case ECI_push_recvData:		//收到数据
				{	
						
				#if defined EAM_md5AndAesEnrypt
					uint32_t length = len- fix_head_len;//加密后数据长度
					uint8_t *p = malloc (length);
					if(!p){printf("\r\nNot enough memory!");if(data)free(data); data =NULL; return 0;}
					AES_Init(session_key);	
					AES_Decrypt(p,data+fix_head_len,len- fix_head_len,session_key);
					//解密数据
					uint8_t temp;
					temp = p[length - 1];//算出填充长度
					len = len - temp;//取加密前数据总长度
					memcpy(data + fix_head_len, p ,length -temp);//把明文放回
					if(p){free(p);p = NULL;}
				#endif
			RecvDataPush *recvDatPush;
			recvDatPush = epb_unpack_recv_data_push(data+fix_head_len, len-fix_head_len);
		#ifdef CATCH_LOG
			printf("\r\n@@Received 'recvDataPush'\r\n");
		#endif
			if(!recvDatPush)
			{
				return errorCodeUnpackRecvDataPush;
			}
		#ifdef CATCH_LOG
			printf("\r\n unpack the 'recvDataPush' successfully! \r\n");
			if(recvDatPush->base_push == NULL)
			{
				printf("\r\n recvDatPush->base_push is NULL! \r\n");
			}
			else 
			{
				printf("\r\n recvDatPush->base_push is not NULL! \r\n");
			}
			printf("\r\n recvDatPush->data.len: %x \r\n",recvDatPush->data.len);
			printf("\r\n recvDatPush->data.data:  \r\n");
			const uint8_t *d = recvDatPush->data.data;
			for(uint8_t i=0;i<recvDatPush->data.len;++i){
			printf(" %x",d[i]);}
			if(recvDatPush->has_type)
			{
				printf("\r\n recvDatPush has type! \r\n");
				printf("\r\n type: %d\r\n",recvDatPush->type);
			}
		#endif	
			BlueDemoHead *bledemohead = (BlueDemoHead*)recvDatPush->data.data;
			//把收到的数据包包体中的数据段指向  BlueDemoHead  类型 
		#ifdef CATCH_LOG
			printf("\r\n magicCode: %x",bledemohead->m_magicCode[0]);
			printf(" %x",bledemohead->m_magicCode[1]);
			printf("\r\n version: %x",ntohs(bledemohead->m_version));
			printf("\r\n totalLength: %x",ntohs(bledemohead->m_totalLength));
			printf("\r\n cmdid: %x",ntohs(bledemohead->m_cmdid ));
			printf("\r\n errorCode: %x",ntohs(bledemohead->m_errorCode));
		#endif	
			if(ntohs(bledemohead->m_cmdid ) == openLightPush)
			{
				#ifdef CATCH_LOG
						printf("\r\n light on!! ");
				#endif
						nrf_gpio_pin_set(BLUE_LED);				
			}
			else if(ntohs(bledemohead->m_cmdid )  == closeLightPush)
			{
		#ifdef CATCH_LOG
					printf("\r\n light off!! ");
		#endif
			
					
			}
			epb_unpack_recv_data_push_free(recvDatPush);
			mpbledemo2Sta.push_data_seq++;		
	/*
	包头里的 Seq 表示一个命令序号
		Req 命令：seq 不能为零，设备每发一个 req 请求，必须 seq+1。每个请求里的序号和对应的回包里的序号相同。 
		Resp 命令：回包的序号和请求里的序号相同。 
		push：序号为零。 
	*/
		}
				break;
			case ECI_push_switchView:		//切换界面 push 通知 
				{
			
					mpbledemo2Sta.wechats_switch_state = !mpbledemo2Sta.wechats_switch_state;
			#ifdef CATCH_LOG
				printf("\r\n@@Received 'switchViewPush'\r\n");
			#endif
			#if defined EAM_md5AndAesEnrypt		
					uint32_t length = len- fix_head_len;//加密后数据长度
					uint8_t *p = malloc (length);
					if(!p){printf("\r\nNot enough memory!");if(data)free(data);data = NULL; return 0;}
					AES_Init(session_key);
					//解密数据
					AES_Decrypt(p,data+fix_head_len,len- fix_head_len,session_key);
					
					uint8_t temp;
					temp = p[length - 1];//算出填充长度
					len = len - temp;//取加密前数据总长度
					memcpy(data + fix_head_len, p ,length -temp);//把明文放回
					if(p){free(p);p = NULL;}
			#endif		
				SwitchViewPush *swichViewPush;
				swichViewPush = epb_unpack_switch_view_push(data+fix_head_len,len-fix_head_len);
				if(!swichViewPush)
				{
					return errorCodeUnpackSwitchViewPush;
				}
				epb_unpack_switch_view_push_free(swichViewPush);
			}
				break;
			case ECI_push_switchBackgroud:		//IOS 平台微信进入后台通知。
				{
			#ifdef CATCH_LOG
				printf("\r\n@@Received 'switchBackgroudPush'\r\n");
			#endif
			#if defined EAM_md5AndAesEnrypt
					uint32_t length = len- fix_head_len;//加密后数据长度
					uint8_t *p = malloc (length);
					if(!p){printf("\r\nNot enough memory!");if(data)free(data);data = NULL;  return 0;}
					AES_Init(session_key);
					//解密数据
					AES_Decrypt(p,data+fix_head_len,len- fix_head_len,session_key);
					uint8_t temp;
					temp = p[length - 1];//算出填充长度
					len = len - temp;//取加密前数据总长度
					memcpy(data + fix_head_len, p ,length -temp);//把明文放回
					if(data){free(p);p = NULL;}
			#endif
				SwitchBackgroudPush *switchBackgroundPush = epb_unpack_switch_backgroud_push(data+fix_head_len,len-fix_head_len);
				if(! switchBackgroundPush)
				{
					return errorCodeUnpackSwitchBackgroundPush;
				}	
				epb_unpack_switch_backgroud_push_free(switchBackgroundPush);
			}
				break;
			case ECI_err_decode:
				break;
			default:
				{
		#ifdef CATCH_LOG
			printf("\r\n !!ERROR CMDID:%d",ntohs(fix_head->nCmdId));
		#endif
			}
				break;
		}
		return 0;
}
void mpbledemo2_data_error_func(int error_code)
{
	if(error_code)
	{
		#ifdef CATCH_LOG
			printf("\r\n! error: mpbledemo2 reseted");
		#endif
		NVIC_SystemReset();
	}
}
data_handler mpbledemo2_data_handler = {
		.m_product_type = PRODUCT_TYPE_MPBLEDEMO2,		 							//设备类型信息
		.m_data_produce_func = &mpbledemo2_data_produce_func,				//设备生产数据函数接口
		.m_data_free_func = &mpbledemo2_data_free_func,							//设备释放数据接口
		.m_data_consume_func = &mpbledemo2_data_consume_func,				//设备处理数据函数接口
		.m_data_error_func = &mpbledemo2_data_error_func,						//设备错误处理函数接口
	
		.m_data_on_ble_evt_func = &ble_mpbledemo2_on_ble_evt,				//设备处理 ble 事件函数接口
		.m_data_init_func = &mpbledemo2_init,												//设备初始化函数接口
		.m_data_main_process_func = &mpbledemo2_main_process,				//设备主流程函数接口
		.m_data_button_handler_func = &mpbledemo2_button_handler,		//设备按键处理函数接口 
		.m_data_produce_args = &m_info,															//设备数据 
		.next = NULL 																								//设备存储链表指针 
};
