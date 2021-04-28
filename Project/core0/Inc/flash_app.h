#ifndef __FLASH_APP_H
#define __FLASH_APP_H

//��5��sector���ڹ���ADC��������, ÿ����������ռ��20byte, �����Ա���20480/20=1024��
#define ADC_MAX_NUM          1023
#define ADC_INFO_LEN         sizeof(AdcInfo)

#define ADC_INFO_TOTAL_ADDR   0
#define ADC_INFO_START_ADDR  (ADC_INFO_TOTAL_ADDR + sizeof(AdcInfoTotal))
#define ADC_INFO_END_ADDR    20460
#define ADC_DATA_ADDR        ADC_INFO_END_ADDR + sizeof(AdcInfo)

typedef struct{
	uint32_t totalAdcData; //�ܵĲ�����Ϣ
	uint32_t addrOfNewInfo;//�²���������Ҫ����ĵ�ַ
	uint32_t addrOfNewData;//�²���������Ҫ����ĵ�ַ
	uint32_t freeOfKb;     //ʣ��ռ�
	uint32_t backup;       
}AdcInfoTotal;

typedef struct{
	uint32_t AdcDataAddr;//ADC���ݵ�ַ
	uint32_t AdcDataLen; //ADC���ݳ���: ���������������������
	char  AdcDataTime[12];//ADC���ݲɼ�ʱ��,ͬʱҲ���ļ���
}AdcInfo;

extern uint8_t s_buffer[512];
void Flash_SavePara(void);
void Flash_ReadPara(void);
void LPC55S69_FlashSaveData(uint8_t* pBuffer,uint32_t WriteAddr,uint32_t NumByteToWrite);
void W25Q128_AddAdcData(void);
bool W25Q128_ReadLastShkData(char *adcDataTime);


#endif
