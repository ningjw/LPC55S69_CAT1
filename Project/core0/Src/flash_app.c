#include "main.h"

#define SAVE_TOTAL_INFO() SPI_Flash_Write((uint8_t *)&adcInfoTotal.totalAdcData, ADC_INFO_TOTAL_ADDR, sizeof(AdcInfoTotal))
#define SAVE_DATA_INFO()  SPI_Flash_Write((uint8_t *)&adcInfo.AdcDataAddr, adcInfoTotal.addrOfNewInfo, sizeof(adcInfo))


extern rtc_datetime_t sampTime;
uint8_t s_buffer[512] = {0};

AdcInfoTotal adcInfoTotal;
AdcInfo adcInfo;
/*******************************************************************************
* ������  : LPC55S69_FlashSaveData
* ����    : ��ָ����ַ��ʼд��ָ�����ȵ�����
* ����    : WriteAddr:��ʼ��ַ(�˵�ַ����Ϊ2�ı���!!)  pBuffer:����ָ��  NumToWrite:
* ����ֵ  :
*******************************************************************************/
void LPC55S69_FlashSaveData(uint8_t* pBuffer,uint32_t WriteAddr,uint32_t NumByteToWrite)
{
	uint32_t secpos=0;
	uint16_t secoff=0;
	uint16_t secremain=0;	   
 	uint16_t i=0;    
	uint8_t *FLEXSPI_BUF;
    if(NumByteToWrite == 0) return;
   	FLEXSPI_BUF = s_buffer;	     
 	secpos = WriteAddr/PAGE_SIZE;//������ַ  
	secoff = WriteAddr%PAGE_SIZE;//�������ڵ�ƫ��
	secremain = PAGE_SIZE-secoff;//����ʣ��ռ��С
	
 	if(NumByteToWrite<=secremain)
		secremain=NumByteToWrite;//������4096���ֽ�
	__disable_irq();//�ر��ж�
    while(1) 
	{
		memory_read(secpos*PAGE_SIZE, (uint8_t *)FLEXSPI_BUF, PAGE_SIZE);
		memory_erase(secpos*PAGE_SIZE, PAGE_SIZE);
        for(i=0;i<secremain;i++)	                    //����
        {
            FLEXSPI_BUF[i+secoff]=pBuffer[i];	  
        }
		memory_write(secpos*PAGE_SIZE, FLEXSPI_BUF, PAGE_SIZE);
		if(NumByteToWrite==secremain)
			break;//д�������
		else//д��δ����
		{
			secpos++;//������ַ��1
			secoff=0;//ƫ��λ��Ϊ0 	 

		   	pBuffer+=secremain;  //ָ��ƫ��
			WriteAddr+=secremain;//д��ַƫ��	   
		   	NumByteToWrite-=secremain;				//�ֽ����ݼ�
			if(NumByteToWrite>PAGE_SIZE)
				secremain=PAGE_SIZE;	           //��һҳ����д����
			else 
				secremain=NumByteToWrite;			//��һҳ����д����
		}	 
	};
	__enable_irq();
}


void Flash_SavePara(void)
{
	memory_erase(SYS_PARA_ADDR,PAGE_SIZE);
	memory_write(SYS_PARA_ADDR,(uint8_t *)&g_sys_flash_para, sizeof(SysFlashPara));
	g_sys_flash_para.batRemainPercentBak = g_sys_para.batRemainPercent;
    
    memory_erase(SAMPLE_PARA_ADDR, sizeof(g_sample_para));
	LPC55S69_FlashSaveData((uint8_t *)&g_sample_para, SAMPLE_PARA_ADDR, sizeof(g_sample_para));
}

void Flash_ReadPara(void)
{
	uint16_t i = 0;
	memory_read(SYS_PARA_ADDR, (uint8_t *)&g_sys_flash_para, sizeof(SysFlashPara));
	
    memory_read(SAMPLE_PARA_ADDR, (uint8_t *)&g_sample_para, sizeof(g_sample_para));
    
	//ǰ12�ֽڱ������ adcInfoTotal �ṹ��
	SPI_Flash_Read((uint8_t *)&adcInfoTotal.totalAdcData, ADC_INFO_TOTAL_ADDR, sizeof(adcInfoTotal));
	
	//�ж�Ϊ�״��ϵ�����
	if(adcInfoTotal.totalAdcData == 0xFFFFFFFF || adcInfoTotal.totalAdcData == 0 ||
		adcInfoTotal.totalAdcData > ADC_MAX_NUM){
		DEBUG_PRINTF("%s: First run, init adcInfoTotal**************\r\n",__func__);
		//����Ϊ��ʼ��Ϊ0
		adcInfoTotal.totalAdcData = 0;
		//���� AdcInfo �ṹ�屣���ַ
		adcInfoTotal.addrOfNewInfo = ADC_INFO_TOTAL_ADDR + sizeof(adcInfoTotal);
		//�������ݵĿ�ʼ��ַ
		adcInfoTotal.addrOfNewData = ADC_DATA_ADDR;
		
	}
	DEBUG_PRINTF("%s: total=%d, addrOfNewInfo=0x%x, addrOfNewData=0x%x\r\n",__func__,
	    adcInfoTotal.totalAdcData,adcInfoTotal.addrOfNewInfo,adcInfoTotal.addrOfNewData);
	
}



/***************************************************************************************
  * @brief   ����һ��ADC����
  * @input   
  * @return  
***************************************************************************************/
void W25Q128_AddAdcData(void)
{
	//��ʼ�� adcInfo �ṹ�� ����ʱ��
	snprintf(adcInfo.AdcDataTime, sizeof(adcInfo.AdcDataTime) , "%02d%02d%02d%02d%02d%02d", 
		                       sampTime.year%100, sampTime.month, sampTime.day, 
	                           sampTime.hour, sampTime.minute, sampTime.second);
	
	//���ݳ���
	adcInfo.AdcDataLen = sizeof(SysSamplePara) + g_sample_para.shkCount*4;
	//���ݵ�ַ
	adcInfo.AdcDataAddr = adcInfoTotal.addrOfNewData;
	if((adcInfo.AdcDataAddr % 4) != 0){//�жϵ�ַ�Ƿ����ֽڶ���
		adcInfo.AdcDataAddr = adcInfo.AdcDataAddr + (4 - (adcInfo.AdcDataAddr % 4));
	}
	//�ж�ʣ��ռ��Ƿ��ܹ����ֱ��βɼ�
	if((adcInfo.AdcDataAddr + adcInfo.AdcDataLen) >= SPI_FLASH_SIZE_BYTE){
		adcInfo.AdcDataAddr = ADC_DATA_ADDR;
	}
	//���� adcInfo �ṹ��
	SAVE_DATA_INFO();

	//��������������������
	SPI_Flash_Write((uint8_t *)&g_sample_para.DetectType, adcInfo.AdcDataAddr, sizeof(SysSamplePara));
	SPI_Flash_Write((uint8_t *)&ShakeADC[0], adcInfo.AdcDataAddr+sizeof(SysSamplePara), g_sample_para.shkCount*4);
	
	/*---------------------------------------------------------------------*/
	//���� adcInfoTotal �ṹ���е��ܲ�������
	adcInfoTotal.totalAdcData++;//���øú���,��ʾ��Ҫ����һ��adc��������
	if(adcInfoTotal.totalAdcData > ADC_MAX_NUM){//�жϳ��Ѵﵽ���ֵ
		adcInfoTotal.totalAdcData = ADC_MAX_NUM;
	}
	
	//���� adcInfoTotal �ṹ���е��´β�����Ϣ�����ַ
	adcInfoTotal.addrOfNewInfo += ADC_INFO_LEN;
	if(adcInfoTotal.addrOfNewInfo > ADC_INFO_END_ADDR){
		adcInfoTotal.addrOfNewInfo = ADC_INFO_START_ADDR;
	}
	
	//���� adcInfoTotal �ṹ���е��´β������ݱ����ַ
	adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData + adcInfo.AdcDataLen;
	if( adcInfoTotal.addrOfNewData % 4 != 0){//�ж��µ�ַ����4�ֽڶ����, ��Ҫ����4�ֽڶ���
		adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData + (4-adcInfoTotal.addrOfNewData%4);
	}
	
	//���� adcInfoTotal �ṹ��
	SAVE_TOTAL_INFO();
}


/***************************************************************************************
  * @brief   ɾ�����µ�һ��ADC����
  * @input   
  * @return  
***************************************************************************************/
void W25Q128_DelLastShkData(void)
{
	//flash�л�δ����������,ֱ�ӷ���
	if(adcInfoTotal.totalAdcData == 0){
		return;
	}
	
	//�ҵ���һ��
	uint32_t tempAddr = adcInfoTotal.addrOfNewInfo - ADC_INFO_LEN;
	if(tempAddr < ADC_INFO_START_ADDR){
		tempAddr = ADC_INFO_END_ADDR;
	}
	
	//��ȡ�� adcInfo �ṹ��
	SPI_Flash_Read((uint8_t *)adcInfo.AdcDataAddr, tempAddr, sizeof(AdcInfo));
	
	/*-------------------------------------------------------*/
	//���� adcInfoTotal �ṹ���е��ܲ�������
	if(adcInfoTotal.totalAdcData){
		adcInfoTotal.totalAdcData--;
	}
	
	//���� adcInfoTotal �ṹ���е��´β�����Ϣ�����ַ
	adcInfoTotal.addrOfNewInfo -= ADC_INFO_LEN;
	if(adcInfoTotal.addrOfNewInfo < ADC_INFO_START_ADDR){
		adcInfoTotal.addrOfNewInfo = ADC_INFO_END_ADDR;
	}
	
	//���� adcInfoTotal �ṹ���е��´β������ݱ����ַ
	adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData - adcInfo.AdcDataLen;
	if( adcInfoTotal.addrOfNewData % 4 != 0){//�ж��µ�ַ����4�ֽڶ����, ��Ҫ����4�ֽڶ���
		adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData + (4-adcInfoTotal.addrOfNewData%4);
	}
	
	//���� adcInfoTotal �ṹ��
	SAVE_TOTAL_INFO();
	
	return;
}
/***************************************************************************************
  * @brief   ��ȡ������flash�����µ�һ��ADC����
  * @input   
  * @return  �������ݵ�ַ
***************************************************************************************/
bool W25Q128_ReadLastShkData(char *adcDataTime)
{
	//flash�л�δ����������,ֱ�ӷ���
	if(adcInfoTotal.totalAdcData == 0){
		return false;
	}
	
	//�ҵ���һ��
	uint32_t tempAddr = adcInfoTotal.addrOfNewInfo - ADC_INFO_LEN;
	if(tempAddr < ADC_INFO_START_ADDR){
		tempAddr = ADC_INFO_END_ADDR;
	}
	
	//��ȡ�� adcInfo �ṹ��
	SPI_Flash_Read((uint8_t *)adcInfo.AdcDataAddr, tempAddr, sizeof(AdcInfo));
	
	//��������������������
	SPI_Flash_Read((uint8_t *)&g_sample_para.DetectType, adcInfo.AdcDataAddr, sizeof(SysSamplePara));
	SPI_Flash_Read((uint8_t *)&ShakeADC[0], adcInfo.AdcDataAddr+sizeof(SysSamplePara), g_sample_para.shkCount*4);
	
	return true;
}

