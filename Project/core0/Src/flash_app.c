#include "main.h"

#define SAVE_TOTAL_INFO() SPI_Flash_Write((uint8_t *)&adcInfoTotal.totalAdcData, ADC_INFO_TOTAL_ADDR, sizeof(AdcInfoTotal))
#define SAVE_DATA_INFO()  SPI_Flash_Write((uint8_t *)&adcInfo.AdcDataAddr, adcInfoTotal.addrOfNewInfo, sizeof(adcInfo))


extern rtc_datetime_t sampTime;
uint8_t s_buffer[512] = {0};

AdcInfoTotal adcInfoTotal;
AdcInfo adcInfo;
/*******************************************************************************
* 函数名  : LPC55S69_FlashSaveData
* 描述    : 从指定地址开始写入指定长度的数据
* 输入    : WriteAddr:起始地址(此地址必须为2的倍数!!)  pBuffer:数据指针  NumToWrite:
* 返回值  :
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
 	secpos = WriteAddr/PAGE_SIZE;//扇区地址  
	secoff = WriteAddr%PAGE_SIZE;//在扇区内的偏移
	secremain = PAGE_SIZE-secoff;//扇区剩余空间大小
	
 	if(NumByteToWrite<=secremain)
		secremain=NumByteToWrite;//不大于4096个字节
	__disable_irq();//关闭中断
    while(1) 
	{
		memory_read(secpos*PAGE_SIZE, (uint8_t *)FLEXSPI_BUF, PAGE_SIZE);
		memory_erase(secpos*PAGE_SIZE, PAGE_SIZE);
        for(i=0;i<secremain;i++)	                    //复制
        {
            FLEXSPI_BUF[i+secoff]=pBuffer[i];	  
        }
		memory_write(secpos*PAGE_SIZE, FLEXSPI_BUF, PAGE_SIZE);
		if(NumByteToWrite==secremain)
			break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

		   	pBuffer+=secremain;  //指针偏移
			WriteAddr+=secremain;//写地址偏移	   
		   	NumByteToWrite-=secremain;				//字节数递减
			if(NumByteToWrite>PAGE_SIZE)
				secremain=PAGE_SIZE;	           //下一页还是写不完
			else 
				secremain=NumByteToWrite;			//下一页可以写完了
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
    
	//前12字节保存的是 adcInfoTotal 结构体
	SPI_Flash_Read((uint8_t *)&adcInfoTotal.totalAdcData, ADC_INFO_TOTAL_ADDR, sizeof(adcInfoTotal));
	
	//判断为首次上电运行
	if(adcInfoTotal.totalAdcData == 0xFFFFFFFF || adcInfoTotal.totalAdcData == 0 ||
		adcInfoTotal.totalAdcData > ADC_MAX_NUM){
		DEBUG_PRINTF("%s: First run, init adcInfoTotal**************\r\n",__func__);
		//总数为初始化为0
		adcInfoTotal.totalAdcData = 0;
		//本次 AdcInfo 结构体保存地址
		adcInfoTotal.addrOfNewInfo = ADC_INFO_TOTAL_ADDR + sizeof(adcInfoTotal);
		//本次数据的开始地址
		adcInfoTotal.addrOfNewData = ADC_DATA_ADDR;
		
	}
	DEBUG_PRINTF("%s: total=%d, addrOfNewInfo=0x%x, addrOfNewData=0x%x\r\n",__func__,
	    adcInfoTotal.totalAdcData,adcInfoTotal.addrOfNewInfo,adcInfoTotal.addrOfNewData);
	
}



/***************************************************************************************
  * @brief   增加一条ADC数据
  * @input   
  * @return  
***************************************************************************************/
void W25Q128_AddAdcData(void)
{
	//初始化 adcInfo 结构体 数据时间
	snprintf(adcInfo.AdcDataTime, sizeof(adcInfo.AdcDataTime) , "%02d%02d%02d%02d%02d%02d", 
		                       sampTime.year%100, sampTime.month, sampTime.day, 
	                           sampTime.hour, sampTime.minute, sampTime.second);
	
	//数据长度
	adcInfo.AdcDataLen = sizeof(SysSamplePara) + g_sample_para.shkCount*4;
	//数据地址
	adcInfo.AdcDataAddr = adcInfoTotal.addrOfNewData;
	if((adcInfo.AdcDataAddr % 4) != 0){//判断地址是否四字节对齐
		adcInfo.AdcDataAddr = adcInfo.AdcDataAddr + (4 - (adcInfo.AdcDataAddr % 4));
	}
	//判断剩余空间是否能够保持本次采集
	if((adcInfo.AdcDataAddr + adcInfo.AdcDataLen) >= SPI_FLASH_SIZE_BYTE){
		adcInfo.AdcDataAddr = ADC_DATA_ADDR;
	}
	//保存 adcInfo 结构体
	SAVE_DATA_INFO();

	//保存采样参数与采样数据
	SPI_Flash_Write((uint8_t *)&g_sample_para.DetectType, adcInfo.AdcDataAddr, sizeof(SysSamplePara));
	SPI_Flash_Write((uint8_t *)&ShakeADC[0], adcInfo.AdcDataAddr+sizeof(SysSamplePara), g_sample_para.shkCount*4);
	
	/*---------------------------------------------------------------------*/
	//更新 adcInfoTotal 结构体中的总采样条数
	adcInfoTotal.totalAdcData++;//调用该函数,表示需要增加一条adc采样数据
	if(adcInfoTotal.totalAdcData > ADC_MAX_NUM){//判断出已达到最大值
		adcInfoTotal.totalAdcData = ADC_MAX_NUM;
	}
	
	//更新 adcInfoTotal 结构体中的下次采样信息保存地址
	adcInfoTotal.addrOfNewInfo += ADC_INFO_LEN;
	if(adcInfoTotal.addrOfNewInfo > ADC_INFO_END_ADDR){
		adcInfoTotal.addrOfNewInfo = ADC_INFO_START_ADDR;
	}
	
	//更新 adcInfoTotal 结构体中的下次采样数据保存地址
	adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData + adcInfo.AdcDataLen;
	if( adcInfoTotal.addrOfNewData % 4 != 0){//判断新地址不是4字节对齐的, 需要进行4字节对齐
		adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData + (4-adcInfoTotal.addrOfNewData%4);
	}
	
	//保存 adcInfoTotal 结构体
	SAVE_TOTAL_INFO();
}


/***************************************************************************************
  * @brief   删除最新的一条ADC数据
  * @input   
  * @return  
***************************************************************************************/
void W25Q128_DelLastShkData(void)
{
	//flash中还未保存有数据,直接返回
	if(adcInfoTotal.totalAdcData == 0){
		return;
	}
	
	//找到上一条
	uint32_t tempAddr = adcInfoTotal.addrOfNewInfo - ADC_INFO_LEN;
	if(tempAddr < ADC_INFO_START_ADDR){
		tempAddr = ADC_INFO_END_ADDR;
	}
	
	//读取到 adcInfo 结构体
	SPI_Flash_Read((uint8_t *)adcInfo.AdcDataAddr, tempAddr, sizeof(AdcInfo));
	
	/*-------------------------------------------------------*/
	//更新 adcInfoTotal 结构体中的总采样条数
	if(adcInfoTotal.totalAdcData){
		adcInfoTotal.totalAdcData--;
	}
	
	//更新 adcInfoTotal 结构体中的下次采样信息保存地址
	adcInfoTotal.addrOfNewInfo -= ADC_INFO_LEN;
	if(adcInfoTotal.addrOfNewInfo < ADC_INFO_START_ADDR){
		adcInfoTotal.addrOfNewInfo = ADC_INFO_END_ADDR;
	}
	
	//更新 adcInfoTotal 结构体中的下次采样数据保存地址
	adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData - adcInfo.AdcDataLen;
	if( adcInfoTotal.addrOfNewData % 4 != 0){//判断新地址不是4字节对齐的, 需要进行4字节对齐
		adcInfoTotal.addrOfNewData = adcInfoTotal.addrOfNewData + (4-adcInfoTotal.addrOfNewData%4);
	}
	
	//保存 adcInfoTotal 结构体
	SAVE_TOTAL_INFO();
	
	return;
}
/***************************************************************************************
  * @brief   读取保存在flash里最新的一条ADC数据
  * @input   
  * @return  返回数据地址
***************************************************************************************/
bool W25Q128_ReadLastShkData(char *adcDataTime)
{
	//flash中还未保存有数据,直接返回
	if(adcInfoTotal.totalAdcData == 0){
		return false;
	}
	
	//找到上一条
	uint32_t tempAddr = adcInfoTotal.addrOfNewInfo - ADC_INFO_LEN;
	if(tempAddr < ADC_INFO_START_ADDR){
		tempAddr = ADC_INFO_END_ADDR;
	}
	
	//读取到 adcInfo 结构体
	SPI_Flash_Read((uint8_t *)adcInfo.AdcDataAddr, tempAddr, sizeof(AdcInfo));
	
	//保存采样参数与采样数据
	SPI_Flash_Read((uint8_t *)&g_sample_para.DetectType, adcInfo.AdcDataAddr, sizeof(SysSamplePara));
	SPI_Flash_Read((uint8_t *)&ShakeADC[0], adcInfo.AdcDataAddr+sizeof(SysSamplePara), g_sample_para.shkCount*4);
	
	return true;
}

