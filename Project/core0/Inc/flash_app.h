#ifndef __FLASH_APP_H
#define __FLASH_APP_H

//有5个sector用于管理ADC采样数据, 每个采样数据占用20byte, 共可以保存20480/20=1024个
#define ADC_MAX_NUM          1023
#define ADC_INFO_LEN         sizeof(AdcInfo)

#define ADC_INFO_TOTAL_ADDR   0
#define ADC_INFO_START_ADDR  (ADC_INFO_TOTAL_ADDR + sizeof(AdcInfoTotal))
#define ADC_INFO_END_ADDR    20460
#define ADC_DATA_ADDR        ADC_INFO_END_ADDR + sizeof(AdcInfo)

typedef struct{
	uint32_t totalAdcData; //总的采样信息
	uint32_t addrOfNewInfo;//新采样参数需要保存的地址
	uint32_t addrOfNewData;//新采样数据需要保存的地址
	uint32_t freeOfKb;     //剩余空间
	uint32_t backup;       
}AdcInfoTotal;

typedef struct{
	uint32_t AdcDataAddr;//ADC数据地址
	uint32_t AdcDataLen; //ADC数据长度: 包括采样参数与采样数据
	char  AdcDataTime[12];//ADC数据采集时间,同时也做文件名
}AdcInfo;

extern uint8_t s_buffer[512];
void Flash_SavePara(void);
void Flash_ReadPara(void);
void LPC55S69_FlashSaveData(uint8_t* pBuffer,uint32_t WriteAddr,uint32_t NumByteToWrite);
void W25Q128_AddAdcData(void);
bool W25Q128_ReadLastShkData(char *adcDataTime);


#endif
