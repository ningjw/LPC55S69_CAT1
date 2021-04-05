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
  * @brief   ������ϢidΪ13����Ϣ, ����ϢΪ��ȡmanage�ļ�����
  * @input
  * @return
***************************************************************************************/
static char* GetManageInfo(cJSON *pJson, cJSON * pSub)
{
    char   *fileStr;
    int sid = 0, num = 0, si = 0, len = 0;

    /*������Ϣ����,*/
    pSub = cJSON_GetObjectItem(pJson, "S");
    if(pSub != NULL) {
        si = pSub->valueint;
    } else {
        si = 0;
    }

    pSub = cJSON_GetObjectItem(pJson, "N");
    if(pSub != NULL) {
        num = pSub->valueint;
    } else {
        num = 1;
    }
    if(num > 10) num = 10;
    if(num < 1) num = 1;

    //�����ڴ����ڱ����ļ�����
    len = num * 13 + 1;
    fileStr = malloc(len);
    memset(fileStr, 0U, len);

    W25Q128_ReadAdcInfo(si, num, fileStr);

    /*����cjson��ʽ�Ļظ���Ϣ*/
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return NULL;
    }
    cJSON_AddNumberToObject(pJsonRoot, "Id", 13);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",sid);
    cJSON_AddStringToObject(pJsonRoot, "Name", fileStr);
    cJSON_AddNumberToObject(pJsonRoot, "T",adcInfoTotal.totalAdcInfo);
    cJSON_AddNumberToObject(pJsonRoot, "Kb",adcInfoTotal.freeOfKb);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    free(fileStr);
    fileStr = NULL;
    return p_reply;
}


/***************************************************************************************
  * @brief   ������ϢidΪ14����Ϣ, ����ϢΪͨ���ļ�����ȡ�ɼ�����
  * @input
  * @return
***************************************************************************************/
static char* GetSampleDataInFlash(cJSON *pJson, cJSON * pSub)
{
    extern AdcInfo adcInfo;
    char ret;
    char fileName[15] = {0};

    /*������Ϣ����,*/
    pSub = cJSON_GetObjectItem(pJson, "fileName");
    if(pSub != NULL && strlen(pSub->valuestring) == 12) {
        strcpy(fileName,pSub->valuestring);
    }

    /*��flash��ȡ�ļ�*/
    ret = W25Q128_ReadAdcData(fileName);
    if(ret == true) {
		sampTime.year = (fileName[0] - '0')*10 + (fileName[1]-'0') + 2000;
		sampTime.month =(fileName[2] - '0')*10 + (fileName[3]-'0');
		sampTime.day =  (fileName[4] - '0')*10 + (fileName[5]-'0');
		sampTime.hour = (fileName[6] - '0')*10 + (fileName[7]-'0');
		sampTime.minute=(fileName[8] - '0')*10 + (fileName[9]-'0');
		sampTime.second=(fileName[10] - '0')*10 + (fileName[11]-'0');

#if defined(BLE_VERSION) || defined(CAT1_VERSION)
        //����ͨ������(NFC)�������ź���Ҫ���ٸ���
        g_sys_para.shkPacksByBleNfc = (g_sample_para.shkCount / ADC_NUM_BLE_NFC) +  (g_sample_para.shkCount % ADC_NUM_BLE_NFC ? 1 : 0);
        
        //����ͨ������(NFC)����ת���ź���Ҫ���ٸ���
        g_sys_para.spdPacksByBleNfc = (g_sample_para.spdCount / ADC_NUM_BLE_NFC) +  (g_sample_para.spdCount % ADC_NUM_BLE_NFC ? 1 : 0);
        
        //���㽫��������ͨ��ͨ������(NFC)������Ҫ���ٸ���
        g_sys_para.sampPacksByBleNfc = g_sys_para.shkPacksByBleNfc + g_sys_para.spdPacksByBleNfc + 3;//wifi��Ҫ����3������������
        
        //ת���źŴ��ĸ�sid��ʼ����
        g_sys_para.spdStartSid = g_sys_para.shkPacksByBleNfc + 3;//��Ҫ����3������������
#elif defined(WIFI_VERSION)
        //����ͨ��WIFI�������ź���Ҫ���ٸ���
        g_sys_para.shkPacksByWifiCat1 = (g_sample_para.shkCount / ADC_NUM_WIFI_CAT1) +  (g_sample_para.shkCount % ADC_NUM_WIFI_CAT1 ? 1 : 0);
        
        //����ͨ��WIFI����ת���ź���Ҫ���ٸ���
        g_sys_para.spdPacksByWifiCat1 = (g_sample_para.spdCount / ADC_NUM_WIFI_CAT1) +  (g_sample_para.spdCount % ADC_NUM_WIFI_CAT1 ? 1 : 0);
        
        //���㽫��������ͨ��WIFI�ϴ���Ҫ���ٸ���
        g_sys_para.sampPacksByBleNfc = g_sys_para.shkPacksByWifiCat1 + g_sys_para.spdPacksByWifiCat1 + 1;//wifi��Ҫ����1������������
        
        //ת���źŴ��ĸ�sid��ʼ����
        g_sys_para.spdStartSid = g_sys_para.shkPacksByBleNfc + 1;//��Ҫ����1������������
#endif
	}else{
		g_sys_para.sampPacksByBleNfc = 0;
        g_sys_para.sampPacksByWifiCat1 = 0;
		g_sample_para.shkCount = 0;
        g_sample_para.spdCount = 0;
	}
    /*����cjson��ʽ�Ļظ���Ϣ*/
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) return NULL;

    cJSON_AddNumberToObject(pJsonRoot, "Id", 14);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",0);
#if defined(BLE_VERSION) || defined(CAT1_VERSION)
	cJSON_AddNumberToObject(pJsonRoot,"PK",  g_sys_para.sampPacksByBleNfc);
#else
    cJSON_AddNumberToObject(pJsonRoot,"PK",  g_sys_para.sampPacksByWifiCat1);
#endif
    cJSON_AddNumberToObject(pJsonRoot, "V", g_sample_para.shkCount);
    cJSON_AddNumberToObject(pJsonRoot, "S", g_sample_para.spdCount);
    cJSON_AddNumberToObject(pJsonRoot, "SS", g_sys_para.spdStartSid);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);

    return p_reply;
}


/***************************************************************************************
  * @brief   ������ϢidΪ15����Ϣ, ����ϢΪ����flash�б�������в�������
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


#ifndef CAT1_VERSION
/***************************************************************************************
  * @brief   ������ϢidΪ16����Ϣ, ����ϢΪ���õ�ص���
  * @input
  * @return
***************************************************************************************/
static char *SetBatCapacity(cJSON *pJson, cJSON * pSub)
{
    uint8_t batC = 0;
    uint8_t batM = 0;
    pSub = cJSON_GetObjectItem(pJson, "BatC");
    if (NULL != pSub) {
        batC = pSub->valueint;
        if( batC != 0) {
            if(batC > 100) batC = 100;
            LTC2942_SetAC(batC / 100 * 0xFFFF);
        }
    }

    pSub = cJSON_GetObjectItem(pJson, "BatM");
    if (NULL != pSub) {
        batM = pSub->valueint;
        if(batM != 0) {
            if(batM == 1) {
                LTC2942_SetPrescaler(LTC2942_PSCM_1);
            } else if(batM == 2) {
                LTC2942_SetPrescaler(LTC2942_PSCM_2);
            } else if(batM == 4) {
                LTC2942_SetPrescaler(LTC2942_PSCM_4);
            } else if(batM == 8) {
                LTC2942_SetPrescaler(LTC2942_PSCM_8);
            } else if(batM == 16) {
                LTC2942_SetPrescaler(LTC2942_PSCM_16);
            } else if(batM == 32) {
                LTC2942_SetPrescaler(LTC2942_PSCM_32);
            } else if(batM == 64) {
                LTC2942_SetPrescaler(LTC2942_PSCM_64);
            } else if(batM == 128) {
                LTC2942_SetPrescaler(LTC2942_PSCM_128);
            }
        }
    }

    /*����cjson��ʽ�Ļظ���Ϣ*/
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) return NULL;
    cJSON_AddNumberToObject(pJsonRoot, "Id", 16);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",0);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    return p_reply;
}

#endif

/***************************************************************************************
  * @brief   ������ϢidΪ17����Ϣ, ����ϢΪͨ��wifi���òɼ�����
  * @input
  * @return
***************************************************************************************/
static char * SetSampleParaByWifi(cJSON *pJson, cJSON * pSub)
{
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
#if 0
    pSub = cJSON_GetObjectItem(pJson, "B");
    if (NULL != pSub) {
        g_sys_flash_para.bias = pSub->valuedouble;
    }
    pSub = cJSON_GetObjectItem(pJson, "RV");
    if (NULL != pSub) {
        g_sys_flash_para.refV = pSub->valuedouble;
    }
#endif
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
    //�����������
    g_sample_para.sampNumber = 2.56 * g_sample_para.Lines * g_sample_para.Averages * (1 - g_sample_para.AverageOverlap)
                            + 2.56 * g_sample_para.Lines * g_sample_para.AverageOverlap;

    /*����cjson��ʽ�Ļظ���Ϣ*/
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return NULL;
    }
    cJSON_AddNumberToObject(pJsonRoot, "Id", 17);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",0);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    return p_reply;
}



/***************************************************************************************
  * @brief   cat1�ϴ�����������,ͨ���ú�����װ���ݰ�
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
        cJSON_AddNumberToObject(pJsonRoot, "WT", ble_wait_time);
        cJSON_AddStringToObject(pJsonRoot, "DP", g_sample_para.IDPath);//Ӳ���汾��
        cJSON_AddStringToObject(pJsonRoot, "NP", g_sample_para.NamePath);//Ӳ���汾��
        cJSON_AddStringToObject(pJsonRoot, "SU", g_sample_para.SpeedUnits);
        cJSON_AddStringToObject(pJsonRoot, "PU", g_sample_para.ProcessUnits);
        cJSON_AddNumberToObject(pJsonRoot, "DT", g_sample_para.DetectType);
        cJSON_AddNumberToObject(pJsonRoot, "ST", g_sample_para.Senstivity);
        cJSON_AddNumberToObject(pJsonRoot, "ZD", g_sample_para.Zerodrift);
        cJSON_AddNumberToObject(pJsonRoot, "ET", g_sample_para.EUType);
        cJSON_AddStringToObject(pJsonRoot, "EU", g_sample_para.EU);
        cJSON_AddNumberToObject(pJsonRoot, "W", g_sample_para.WindowsType);
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
        cJSON_AddNumberToObject(pJsonRoot, "Y", sampTime.year);
        cJSON_AddNumberToObject(pJsonRoot, "M", sampTime.month);
        cJSON_AddNumberToObject(pJsonRoot, "D", sampTime.day);
        cJSON_AddNumberToObject(pJsonRoot, "H", sampTime.hour);
        cJSON_AddNumberToObject(pJsonRoot, "Min", sampTime.minute);
        cJSON_AddNumberToObject(pJsonRoot, "S", sampTime.second);
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
			for(uint16_t j =0; j<ADC_NUM_WIFI_CAT1; j++){//ÿ������ռ��3��byte;ÿ�������ϴ�335������. 335*3=1005
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

char *GetIdentityInfoByNfc(cJSON *pJson, cJSON * pSub)
{
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return NULL;
    }
    cJSON_AddNumberToObject(pJsonRoot, "Id", 20);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",0);
    cJSON_AddNumberToObject(pJsonRoot, "IMEI", sysTime.year);
    cJSON_AddNumberToObject(pJsonRoot, "ICCID", sysTime.month);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    return p_reply;
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
    cJSON_AddStringToObject(pJsonRoot, "SV", SOFT_VERSION);//����汾��
    cJSON_AddStringToObject(pJsonRoot, "HV", HARD_VERSION);//����汾��
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

uint32_t PacketVersionInfo(uint8_t *txBuf)
{
    uint32_t len = 0;
    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return len;
    }
    cJSON_AddNumberToObject(pJsonRoot, "Id", 26);
    cJSON_AddStringToObject(pJsonRoot, "HV", HARD_VERSION);//Ӳ���汾��
    cJSON_AddStringToObject(pJsonRoot, "SV", SOFT_VERSION);//����汾��
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    len = strlen(p_reply);
    if(p_reply){
        free(p_reply);
        p_reply = NULL;
    }
    return len;
}


/***************************************************************************************
  * @brief   ����json���ݰ�
  * @input
  * @return
***************************************************************************************/
uint8_t* ParseJson(char *pMsg)
{
    char *p_reply = NULL;

    cJSON *pJson = cJSON_Parse((char *)pMsg);
    if(NULL == pJson) {
        return NULL;
    }

    // get string from json
    cJSON * pSub = cJSON_GetObjectItem(pJson, "Id");
    if(NULL == pSub) {
        return NULL;
    }

    switch(pSub->valueint)
    {
    case 13:
        p_reply = GetManageInfo(pJson, pSub);
        break;
    case 14:
        p_reply = GetSampleDataInFlash(pJson, pSub);//��ȡflash�еĲ�������
        break;
    case 15:
        p_reply = EraseAdcDataInFlash();//����flash�еĲ�������
        break;
#ifndef	CAT1_VERSION
    case 16:
        p_reply = SetBatCapacity(pJson, pSub);//У���������
        break;

	case 17:
		p_reply = SetSampleParaByWifi(pJson, pSub);//ͨ��wifi���ò�������
		break;
	case 18:
		p_reply = GetSampleDataByWifi(pJson, pSub);//
		break;
	case 19:
		break;

    case 20:
        p_reply = GetIdentityInfoByNfc(pJson, pSub);
		break;
#endif
    default:break;
    }

    cJSON_Delete(pJson);

    return (uint8_t *)p_reply;
}



/***************************************************************************************
  * @brief   �����̼���
  * @input
  * @return
***************************************************************************************/
uint8_t*  ParseFirmPacket(uint8_t *pMsg)
{
    uint16_t crc = 0;
    uint8_t  err_code = 0;
	uint32_t app_data_addr = 0;
	
	if(g_sys_flash_para.firmCoreIndex == 1){
		app_data_addr = CORE1_DATA_ADDR;
	}else{
		app_data_addr = CORE0_DATA_ADDR;
	}
#if defined(BLE_VERSION) || defined(CAT1_VERSION)
    crc = CRC16(pMsg+4, FIRM_DATA_LEN_BLE_NFC);//�Լ��������CRC16
    if(pMsg[FIRM_TOTAL_LEN_BLE_NFC-2] != (uint8_t)crc || pMsg[FIRM_TOTAL_LEN_BLE_NFC-1] != (crc>>8)) {
        err_code = 1;
    } else {
        /* ��id */
        g_sys_flash_para.firmPacksCount = pMsg[2] | (pMsg[3]<<8);

        g_sys_flash_para.firmCurrentAddr = app_data_addr+g_sys_flash_para.firmPacksCount * FIRM_DATA_LEN_BLE_NFC;//
        DEBUG_PRINTF("\nADDR = 0x%x\n",g_sys_flash_para.firmCurrentAddr);
        LPC55S69_FlashSaveData(pMsg+4, g_sys_flash_para.firmCurrentAddr, FIRM_DATA_LEN_BLE_NFC);
    }
#elif defined(WIFI_VERSION)
    crc = CRC16(pMsg+4, FIRM_DATA_LEN_WIFI_CAT1);//�Լ��������CRC16
    if(pMsg[FIRM_TOTAL_LEN_WIFI_CAT1-2] != (uint8_t)crc || pMsg[FIRM_TOTAL_LEN_WIFI_CAT1-1] != (crc>>8)) {
        err_code = 1;
    } else {
        /* ��id */
        g_sys_flash_para.firmPacksCount = pMsg[2] | (pMsg[3]<<8);

        g_sys_flash_para.firmCurrentAddr = app_data_addr+g_sys_flash_para.firmPacksCount * FIRM_DATA_LEN_WIFI_CAT1;//
        DEBUG_PRINTF("\nADDR = 0x%x\n",g_sys_para.firmCurrentAddr);
        LPC55S69_FlashSaveData(pMsg+4, g_sys_flash_para.firmCurrentAddr, FIRM_DATA_LEN_WIFI_CAT1);
    }
#endif
    /* ��ǰΪ���һ��,���������̼���crc16�� */
    if(g_sys_flash_para.firmPacksCount == g_sys_flash_para.firmPacksTotal - 1) {
		
        g_sys_para.BleWifiLedStatus = BLE_WIFI_CONNECT;
#if defined( WIFI_VERSION) || defined(BLE_VERSION)
        g_flexcomm3StartRx = false;
#endif
//		printf("�����ļ�:\r\n");
//
//		for(uint32_t i = 0;i<g_sys_para.firmCore0Size; i++){
//			if(i%16 == 0) printf("\n");
//			printf("%02X ",*(uint8_t *)(FlexSPI_AMBA_BASE + APP_START_SECTOR * SECTOR_SIZE+i));
//		}

        crc = CRC16((uint8_t *)app_data_addr, g_sys_flash_para.firmCore0Size);
        DEBUG_PRINTF("\nCRC=%d",crc);
        if(crc != g_sys_flash_para.firmCrc16) {
            g_sys_flash_para.firmCore0Update = NO_VERSION;
            err_code = 2;
        } else {
            DEBUG_PRINTF("\nCRC Verify OK\n");
            g_sys_flash_para.firmCore0Update = BOOT_NEW_VERSION;
        }
    }

    cJSON *pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot) {
        return NULL;
    }
    cJSON_AddNumberToObject(pJsonRoot, "Id", 10);
    cJSON_AddNumberToObject(pJsonRoot, "Sid",1);
    cJSON_AddNumberToObject(pJsonRoot, "P", g_sys_flash_para.firmPacksCount);
    cJSON_AddNumberToObject(pJsonRoot, "E", err_code);
    char *p_reply = cJSON_PrintUnformatted(pJsonRoot);
    cJSON_Delete(pJsonRoot);
    g_sys_flash_para.firmPacksCount++;
    return (uint8_t*)p_reply;
}


/***************************************************************************************
  * @brief   ����Э����ں���
  * @input
  * @return
***************************************************************************************/
uint8_t* ParseProtocol(uint8_t *pMsg)
{
    if(NULL == pMsg) {
        return NULL;
    }
	
    if(pMsg[0] == 0xE7 && pMsg[1] == 0xE7 ) { //Ϊ�̼�������
        return ParseFirmPacket(pMsg);
    } else { //Ϊjson���ݰ�
		DEBUG_PRINTF("%s: rev wifi data:\r\n%s\r\n",__func__,pMsg);
        return ParseJson((char *)pMsg);
    }
}

