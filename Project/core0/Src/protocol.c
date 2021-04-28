#include "main.h"

extern uint8_t s_nor_program_buffer[];
extern AdcInfoTotal adcInfoTotal;
extern AdcInfo adcInfo;

uint16_t ble_wait_time = 8;

/***************************************************************************************
  * @brief   
  * @input   
  * @return  
***************************************************************************************/
uint16_t CRC16(uint8_t *data,uint32_t length)
{
    uint16_t result = 0xFFFF;
    uint32_t i,j;

    if(length!=0)
    {
        for(i=0; i<length; i++)
        {
            result^=(uint16_t)(data[i]);
            for(j=0; j<8; j++)
            {
                if((result&0x0001) != 0)
                {
                    result>>=1;
                    result^=0xA001;	//a001
                }
                else result>>=1;
            }
        }
    }
    return result;
}


/***************************************************************************************
  * @brief   处理消息id为15的消息, 该消息为擦除flash中保存的所有采样数据
  * @input
  * @return
***************************************************************************************/
char *EraseAdcDataInFlash(void)
{
	SPI_Flash_Erase_Sector(0);

    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return NULL;
    }
    cJSON_AddNumberToObject(pJsonRoot, "Id", 15);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",0);
    cJSON_AddBoolToObject(pJsonRoot, "Status",true);
    uint32_t free = (SPI_FLASH_SIZE_BYTE - ADC_DATA_ADDR)/1024;
    cJSON_AddNumberToObject(pJsonRoot, "Kb", free);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    return p_reply;
}


/***************************************************************************************
  * @brief   cat1上传采样数据是,通过该函数封装数据包
  * @input
  * @return
***************************************************************************************/
uint32_t PacketUploadSampleData(uint8_t *txBuf, uint32_t sid)
{
    uint32_t i = 0,index = 0;
    cJSON *pJsonRoot = NULL;
    char *p_reply = NULL;
    
    switch(sid)	{
    case 0:
		pJsonRoot = cJSON_CreateObject();
		if(NULL == pJsonRoot) {
			return 0;
		}
		cJSON_AddNumberToObject(pJsonRoot, "Id", 18);
		cJSON_AddNumberToObject(pJsonRoot, "Sid",sid);
		cJSON_AddStringToObject(pJsonRoot, "DP", g_sample_para.IDPath);//硬件版本号
        cJSON_AddStringToObject(pJsonRoot, "NP", g_sample_para.NamePath);//硬件版本号
        cJSON_AddStringToObject(pJsonRoot, "SU", g_sample_para.SpeedUnits);
        cJSON_AddStringToObject(pJsonRoot, "PU", g_sample_para.ProcessUnits);
        cJSON_AddNumberToObject(pJsonRoot, "DT", g_sample_para.DetectType);
        cJSON_AddNumberToObject(pJsonRoot, "ST", g_sample_para.Senstivity);
        cJSON_AddNumberToObject(pJsonRoot, "ZD", g_sample_para.Zerodrift);
        cJSON_AddNumberToObject(pJsonRoot, "EUType", g_sample_para.EUType);
        cJSON_AddStringToObject(pJsonRoot, "EU", g_sample_para.EU);
        cJSON_AddNumberToObject(pJsonRoot, "WindowsType", g_sample_para.WindowsType);
        cJSON_AddNumberToObject(pJsonRoot, "SF", g_sample_para.StartFrequency);
        cJSON_AddNumberToObject(pJsonRoot, "EF", g_sample_para.EndFrequency);
        cJSON_AddNumberToObject(pJsonRoot, "SR", g_sample_para.SampleRate);
        cJSON_AddNumberToObject(pJsonRoot, "L", g_sample_para.Lines);
        cJSON_AddNumberToObject(pJsonRoot, "B", g_sys_flash_para.bias);
        cJSON_AddNumberToObject(pJsonRoot, "RV", g_sys_flash_para.refV);
        cJSON_AddNumberToObject(pJsonRoot, "A", g_sample_para.Averages);
        cJSON_AddNumberToObject(pJsonRoot, "OL", g_sample_para.AverageOverlap);
        cJSON_AddNumberToObject(pJsonRoot, "AT", g_sample_para.AverageType);
        cJSON_AddNumberToObject(pJsonRoot, "EFL", g_sample_para.EnvFilterLow);
        cJSON_AddNumberToObject(pJsonRoot, "EFH", g_sample_para.EnvFilterHigh);
        cJSON_AddNumberToObject(pJsonRoot, "IM", g_sample_para.IncludeMeasurements);
        cJSON_AddNumberToObject(pJsonRoot, "SP", g_sample_para.Speed);
        cJSON_AddNumberToObject(pJsonRoot, "P", g_sample_para.Process);
        cJSON_AddNumberToObject(pJsonRoot, "PL", g_sample_para.ProcessMin);
        cJSON_AddNumberToObject(pJsonRoot, "PH", g_sample_para.ProcessMax);
        cJSON_AddNumberToObject(pJsonRoot,"PK",  g_sys_para.sampPacksByWifiCat1);
		cJSON_AddNumberToObject(pJsonRoot,"SHC", g_sample_para.shkCount);
        cJSON_AddNumberToObject(pJsonRoot, "Y", sampTime.year);
        cJSON_AddNumberToObject(pJsonRoot, "M", sampTime.month);
        cJSON_AddNumberToObject(pJsonRoot, "D", sampTime.day);
        cJSON_AddNumberToObject(pJsonRoot, "H", sampTime.hour);
        cJSON_AddNumberToObject(pJsonRoot, "Min", sampTime.minute);
        cJSON_AddNumberToObject(pJsonRoot, "S", sampTime.second);
		cJSON_AddStringToObject(pJsonRoot, "LBS", g_sys_para.LBS);
        p_reply = cJSON_PrintUnformatted(pJsonRoot);
        
        i = strlen(p_reply);
        memcpy(txBuf, p_reply, i);
        if(p_reply){
            free(p_reply);
            p_reply = NULL;
        }
        break;
    default:
        memset(g_commTxBuf, 0, FLEXCOMM_BUFF_LEN);
		i = 0;
		txBuf[i++] = 0xE7;
		txBuf[i++] = 0xE8;
		txBuf[i++] = sid & 0xff;
		txBuf[i++] = (sid >> 8) & 0xff;
        if(sid-1 < g_sys_para.shkPacksByWifiCat1)
        {
			index = ADC_NUM_WIFI_CAT1 * (sid - 1);
			for(uint16_t j =0; j<ADC_NUM_WIFI_CAT1; j++){//每个数据占用3个byte;每包可以上传335个数据. 335*3=1005
				txBuf[i++] = ((uint32_t)ShakeADC[index] >> 0) & 0xff;
				txBuf[i++] = ((uint32_t)ShakeADC[index] >> 8) & 0xff;
				txBuf[i++] = ((uint32_t)ShakeADC[index] >> 16)& 0xff;
				index++;
			}
        }
		txBuf[i++] = 0xEA;
		txBuf[i++] = 0xEB;
		txBuf[i++] = 0xEC;
		txBuf[i++] = 0xED;
        break;
    }
    return i;
}

uint32_t PacketSystemInfo(uint8_t *txBuf)
{
    uint32_t len = 0;
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return len;
    }
    LPC55S69_BatAdcUpdate();
    cJSON_AddNumberToObject(pJsonRoot, "Id", 23);
    cJSON_AddNumberToObject(pJsonRoot, "BatC", g_sys_para.batRemainPercent);
    cJSON_AddNumberToObject(pJsonRoot, "BatV", g_sys_para.batVoltage);
    cJSON_AddNumberToObject(pJsonRoot, "Y", sysTime.year);
    cJSON_AddNumberToObject(pJsonRoot, "Mon", sysTime.month);
    cJSON_AddNumberToObject(pJsonRoot, "D", sysTime.day);
    cJSON_AddNumberToObject(pJsonRoot, "H", sysTime.hour);
    cJSON_AddNumberToObject(pJsonRoot, "Min", sysTime.minute);
    cJSON_AddNumberToObject(pJsonRoot, "S", sysTime.second);
    cJSON_AddStringToObject(pJsonRoot, "CSQ", g_sys_para.CSQ);
    cJSON_AddStringToObject(pJsonRoot, "SV", SOFT_VERSION);//软件版本号
    cJSON_AddStringToObject(pJsonRoot, "HV", HARD_VERSION);//软件版本号
    cJSON_AddStringToObject(pJsonRoot, "SN", g_sys_flash_para.SN);
    cJSON_AddNumberToObject(pJsonRoot, "interval", g_sample_para.sampleInterval);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    strcpy((char *)txBuf,p_reply);
    cJSON_Delete(pJsonRoot);
    len = strlen(p_reply);
    if(p_reply){
        free(p_reply);
        p_reply = NULL;
    }
    return len;
}



uint8_t ParseSamplePara(uint8_t *data)
{
	cJSON *pJson = cJSON_Parse((char *)data);
    if(NULL == pJson) {
        return false;
    }
	
	cJSON * pSub = NULL;
	pSub = cJSON_GetObjectItem(pJson, "IP");
    if (NULL != pSub) {
        memset(g_sample_para.IDPath, 0, sizeof(g_sample_para.IDPath));
        strcpy(g_sample_para.IDPath, pSub->valuestring);
    }
    pSub = cJSON_GetObjectItem(pJson, "NP");
    if (NULL != pSub) {
        memset(g_sample_para.NamePath, 0, sizeof(g_sample_para.NamePath));
        strcpy(g_sample_para.NamePath, pSub->valuestring);
    }
    pSub = cJSON_GetObjectItem(pJson, "WT");
    if (NULL != pSub) {
        ble_wait_time = pSub->valueint;
    }

    pSub = cJSON_GetObjectItem(pJson, "SU");
    if (NULL != pSub) {
        memset(g_sample_para.SpeedUnits, 0, sizeof(g_sample_para.SpeedUnits));
        strcpy(g_sample_para.SpeedUnits, pSub->valuestring);
    }
    pSub = cJSON_GetObjectItem(pJson, "PU");
    if (NULL != pSub) {
        memset(g_sample_para.ProcessUnits, 0, sizeof(g_sample_para.ProcessUnits));
        strcpy(g_sample_para.ProcessUnits, pSub->valuestring);
    }
    pSub = cJSON_GetObjectItem(pJson, "DT");
    if (NULL != pSub) {
        g_sample_para.DetectType = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "ST");
    if (NULL != pSub) {
        g_sample_para.Senstivity = pSub->valuedouble;
    }
    pSub = cJSON_GetObjectItem(pJson, "ZD");
    if (NULL != pSub) {
        g_sample_para.Zerodrift = pSub->valuedouble;
    }
    pSub = cJSON_GetObjectItem(pJson, "ET");
    if (NULL != pSub) {
        g_sample_para.EUType = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "EU");
    if (NULL != pSub) {
        memset(g_sample_para.EU, 0, sizeof(g_sample_para.EU));
        strcpy(g_sample_para.EU, pSub->valuestring);
    }
    pSub = cJSON_GetObjectItem(pJson, "W");
    if (NULL != pSub) {
        g_sample_para.WindowsType = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "SF");
    if (NULL != pSub) {
        g_sample_para.StartFrequency = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "EF");
    if (NULL != pSub) {
        g_sample_para.EndFrequency = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "SR");
    if (NULL != pSub) {
        g_sample_para.SampleRate = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "L");
    if (NULL != pSub) {
        g_sample_para.Lines = pSub->valueint;
    }
	pSub = cJSON_GetObjectItem(pJson, "A");
    if (NULL != pSub) {
        g_sample_para.Averages = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "OL");
    if (NULL != pSub) {
        g_sample_para.AverageOverlap = pSub->valuedouble;
    }
    pSub = cJSON_GetObjectItem(pJson, "AT");
    if (NULL != pSub) {
        g_sample_para.AverageType = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "EFL");
    if (NULL != pSub) {
        g_sample_para.EnvFilterLow = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "EFH");
    if (NULL != pSub) {
        g_sample_para.EnvFilterHigh = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "IM");
    if (NULL != pSub) {
        g_sample_para.IncludeMeasurements = pSub->valueint;
    }
    pSub = cJSON_GetObjectItem(pJson, "interval");
    if (NULL != pSub) {
        g_sample_para.sampleInterval = pSub->valueint;
    }
    //计算采样点数
    g_sample_para.sampNumber = 2.56 * g_sample_para.Lines * g_sample_para.Averages * (1 - g_sample_para.AverageOverlap)
                            + 2.56 * g_sample_para.Lines * g_sample_para.AverageOverlap;
	
	cJSON_Delete(pJson);
	SPI_Flash_Erase_Sector(0);
    Flash_SavePara();
	return true;
}




