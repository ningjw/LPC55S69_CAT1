#include "main.h"
#include "fsl_powerquad.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "math.h"

uint32_t ShakeADC[ADC_LEN];
float Temperature[64];

TaskHandle_t ADC_TaskHandle = NULL;  /* ADC???????? */

uint32_t timeCapt = 0;
char str[12];

uint32_t ADC_ShakeValue = 0;

float ADC_VoltageValue;

static float GetEnegryWindowCorrected(int windowType) {
    float NBF = 1;
    switch (windowType) {
        case 0://???δ?0
            NBF = 1;                                                 
            break;
        case 1:   //???Ǵ?1
            NBF = 1.33;
            break;
        case 2:   //??????2
            NBF = 1.6339;                                            
            break;
        case 3:    //??????3
            NBF = 1.59;                                             
            break;
        case 4:    //??????????4
            NBF = 1.65;//????
            break;
        case 5:    //Kaiser-Bessel(3.0)                      ??????
            NBF = 1.86;                                            
            break;
        case 6:    //top flat  ƽ????
            NBF = 2.26;                                           
            break;
        case 7:    //possion(3.0)
            NBF = 1.65;
            break;
        case 8:    //possion(4.0)
            NBF = 2.08;
            break;
        case 9:    //cauchy(4.0)
            NBF = 1.76;
            break;
        case 10:    //cauchy(5.0)
            NBF = 2.06;
            break;
        case 11:    //Gaussian(3.0)
            NBF = 1.64;
            break;
        case 12:    //Kaiser-Bessel(3.5)
            NBF = 1.93;
            break;

    }
    return NBF;
}


//???δ?0?????Ǵ?1????????2????????3????????????4????????5??????
static float GetRMS(float data[],int len, int windowType) 
{  
    float rms = 0.000;
    float sum = 0.000;
    float EnegryCorrected = GetEnegryWindowCorrected(windowType);
    for (int i = 0; i < len; i++) {
        sum = sum + data[i] * data[i];
    }
    float average = sum / len;
    rms = EnegryCorrected * sqrt(average);
    rms = (float) round(rms * 1000) / 1000.0f;   //????3λС??
    return rms;
}


/***************************************************************************************
  * @brief   start adc sample
  * @input
  * @return
***************************************************************************************/
void ADC_SampleStart(uint8_t reason)
{
	DEBUG_PRINTF("%s:sampNumber=%d,SampleRate=%d,\r\n",__func__,
				g_sample_para.sampNumber,g_sample_para.SampleRate);
	
    RTC_GetDatetime(RTC, &sampTime);
	
    //ϵͳָʾ??ָʾ???ڲ???,?Ƶ???
    g_sys_para.sysLedStatus = SYS_IN_SAMPLING;
    GPIO_PinWrite(GPIO, BOARD_LED_SYS_RED_PORT,  BOARD_LED_SYS_RED_PIN, OFF);
    GPIO_PinWrite(GPIO, BOARD_LED_SYS_GREEN_PORT, BOARD_LED_SYS_GREEN_PIN, OFF);
    
	g_sample_para.sampleReason = reason;
	g_sys_para.tempCount = 0;
    g_sample_para.shkCount = 0;
	memset(ShakeADC,0,ADC_LEN);
	memset(Temperature, 0, sizeof(Temperature));
	
	PWR_3V3A_ON;//????ADC???صĵ?Դ
	PWR_5V_ON;//????5V???˲?????Դ
	//?ж??Զ??ػ?????
    if(g_sys_flash_para.autoPwrOffCondition != 1) {
        g_sys_para.sysIdleCount = 0;
    }
	
	//?жϵ????Ƿ񳬳?????????
	if(g_sample_para.sampNumber > ADC_LEN){
		g_sample_para.sampNumber = ADC_LEN;
	}
	
	/* ????PWM ????LTC1063FA??ʱ??????,???Ʋ???????*/
	g_sample_para.Ltc1063Clk = 1000 * g_sample_para.SampleRate / 25;

#ifndef CAT1_VERSION
	SI5351a_SetPDN(SI_CLK0_CONTROL,true);
	si5351aSetFilterClk1(g_sample_para.Ltc1063Clk);
	SI5351a_SetPDN(SI_CLK1_CONTROL,true);//????ADCоƬʱ??
#endif
	if(g_sample_para.SampleRate > 45000){
		ADC_MODE_HIGH_SPEED;//ʹ?ø???ģʽ
		//ʹ??PWM??ΪADS1271??ʱ??, ?䷶ΧΪ37ns - 10000ns (10us)
#ifndef CAT1_VERSION
		si5351aSetAdcClk0(g_sample_para.SampleRate * 256);
#else	
		ADC_PwmClkStart(g_sample_para.SampleRate * 256, g_sample_para.Ltc1063Clk);
#endif
	}else{
		ADC_MODE_LOW_POWER;//ʹ?õ???ģʽ
		//ʹ??PWM??ΪADS1271??ʱ??, ?䷶ΧΪ37ns - 10000ns (10us)
#ifndef CAT1_VERSION
		si5351aSetAdcClk0(g_sample_para.SampleRate * 512);
#endif
		ADC_PwmClkStart(g_sample_para.SampleRate * 512, g_sample_para.Ltc1063Clk);
	}

	vTaskDelay(1);//?ȴ?500ms
	
	//??ʼ?ɼ?????ǰ??ȡһ???¶?
	Temperature[g_sys_para.tempCount++] = TMP101_ReadTemp();
	//????Ϊtrue??,????PIT?ж??вɼ??¶?????
	g_sys_para.WorkStatus = true;
	
	//????ǰ????????
	int ADC_InvalidCnt = 0;
	while (1) { //wait ads1271 ready
        while(ADC_READY == 1){};//?ȴ?ADC_READYΪ?͵?ƽ
		ADC_ShakeValue = ADS1271_ReadData();
		ADC_InvalidCnt++;
		if(ADC_InvalidCnt > 100) break;
    }
#ifndef CAT1_VERSION
	start_spd_caputer();
#endif
	__disable_irq();//?ر??ж?
	while(ADC_READY == 0){};//?ȴ?ADC_READYΪ?ߵ?ƽ
	while(1) { //wait ads1271 ready
        while(ADC_READY == 1){};//?ȴ?ADC_READYΪ?͵?ƽ
		ShakeADC[g_sample_para.shkCount++] = ADS1271_ReadData();
		if(g_sample_para.shkCount >= g_sample_para.sampNumber){
			g_sample_para.shkCount = g_sample_para.sampNumber;
			break;
		}
    }
	__enable_irq();//?????ж?
#ifndef CAT1_VERSION
	stop_spd_caputer();
#endif
    
	//?????ɼ?????ȡһ???¶?
	Temperature[g_sys_para.tempCount++] = TMP101_ReadTemp();
    /* ????ADC?????????¼?  */
    xTaskNotify(ADC_TaskHandle, EVT_SAMPLE_FINISH, eSetBits);
}


/***************************************************************************************
  * @brief   stop adc sample
  * @input
  * @return
***************************************************************************************/
void ADC_SampleStop(void)
{
	DEBUG_PRINTF("ADC_SampleStop \r\n");
	/* Stop get temperature*/
	g_sys_para.WorkStatus = false;
    
    //????????,?ָ?ָʾ????ɫ????.
	g_sys_para.sysLedStatus = SYS_IDLE;
    GPIO_PinWrite(GPIO, BOARD_LED_SYS_RED_PORT,  BOARD_LED_SYS_RED_PIN, OFF);
    GPIO_PinWrite(GPIO, BOARD_LED_SYS_GREEN_PORT, BOARD_LED_SYS_GREEN_PIN, ON);
    
	TMP101_Init();//?¶ȴ?????????????ģʽ
	
	//?ر?ʱ??????
	ADC_PwmClkStop();

	//?رյ?Դ
	PWR_3V3A_OFF;
	PWR_5V_OFF;//????5V???˲?????Դ

    /* ͳ??ƽ???¶?,??С?¶?,?????¶? */
    float sum = 0;
    int min_i = 0;
    int max_i = 0;
    for(int i=0;i<g_sys_para.tempCount;i++){
        sum += Temperature[g_sys_para.tempCount];
        min_i = Temperature[i] < Temperature[min_i] ? i : min_i;
        max_i = Temperature[i] > Temperature[max_i] ? i : max_i;
    }
    g_sample_para.Process = sum / g_sys_para.tempCount;
    g_sample_para.ProcessMax = Temperature[max_i];
    g_sample_para.ProcessMin = Temperature[min_i];
    
    LPC55S69_BatAdcUpdate();
    
    //????ͨ??WIFI?????????ź???Ҫ???ٸ???
    g_sys_para.shkPacksByWifiCat1 = (g_sample_para.shkCount / ADC_NUM_WIFI_CAT1) +  (g_sample_para.shkCount % ADC_NUM_WIFI_CAT1 ? 1 : 0);
    
    //???㽫????????ͨ??WIFI?ϴ???Ҫ???ٸ???
    g_sys_para.sampPacksByWifiCat1 = g_sys_para.shkPacksByWifiCat1 + 1;//wifi??Ҫ????1????????????
}



/***********************************************************************
  * @ ??????  ?? ADC?ɼ?????
  * @ ????˵????
  * @ ????    ?? ??
  * @ ????ֵ  ?? ??
  **********************************************************************/
void ADC_AppTask(void)
{
    uint32_t r_event;
    BaseType_t xReturn = pdTRUE;
#if 0
    /*???´????򿪺???,??????ADC??Դ,һֱ?ɼ?ADS1271????*/
	ADC_MODE_LOW_POWER;
	PWR_5V_ON;
	PWR_3V3A_ON;
#ifndef CAT1_VERSION
	si5351aSetAdcClk0(1000000);//??ADS1271?ṩʱ??
	si5351aSetFilterClk1(1000000);//?????˲???ʱ??
#else
	ADC_PwmClkStart(100000, 100000);
#endif

	while (1) { //wait ads1271 ready
        while(ADC_READY == 1){};//?ȴ?ADC_READYΪ?͵?ƽ
		ADC_ShakeValue = ADS1271_ReadData();
		ADC_VoltageValue = ADC_ShakeValue * 2.048 / 0x800000;
    }
#ifndef CAT1_VERSION
	SI5351a_SetPDN(SI_CLK0_CONTROL, false);
	SI5351a_SetPDN(SI_CLK1_CONTROL, false);
#endif
	PWR_3V3A_OFF;//?ر?ADC?ɼ????صĵ?Դ
	PWR_5V_OFF;
#endif
	
    DEBUG_PRINTF("ADC_AppTask Running\r\n");
    
    LPC55S69_AdcInit();
	
	TMP101_Init();
#if 1
    xTaskNotify(ADC_TaskHandle, EVT_SAMPLE_START, eSetBits);
#else
    vTaskDelay(5000);
	SystemSleep();
#endif
    while(1)
    {
        /*?ȴ?ADC???ɲ????¼?*/
        xReturn = xTaskNotifyWait(pdFALSE, ULONG_MAX, &r_event, portMAX_DELAY);
		
		/* ??ʼ?????¼?*/
		if(r_event & EVT_SAMPLE_START)
		{
			ADC_SampleStart(AUTO_SAMPLE);
		}
		/* ????????*/
		else if(r_event & EVT_ENTER_SLEEP)
		{
			SystemSleep();
		}
		/* ???ɲ????¼?*/
		else if(r_event & EVT_SAMPLE_FINISH) 
		{
            ADC_SampleStop();
			/* ---------------???????ź?ת??-----------------------*/
#if 1
			float tempValue = 0;
			for(uint32_t i = 0; i < g_sample_para.shkCount; i++) {
				if((uint32_t)ShakeADC[i] < 0x800000){
					tempValue = ShakeADC[i] * g_sys_flash_para.bias * 1.0f / 0x800000;
				}else{
					tempValue = ((ShakeADC[i] - 0x800000) * g_sys_flash_para.bias * 1.0f / 0x800000) - g_sys_flash_para.bias;
				}
				DEBUG_PRINTF("%01.5f,",tempValue);
			}
			
//			g_sys_para.shkRMS = GetRMS(ShakeADC, g_sample_para.shkCount, g_sample_para.WindowsType);
#endif          

#if defined(BLE_VERSION) || defined(WIFI_VERSION)
			/*֪ͨ?̲߳???????, ???Ի?ȡ??????????*/
			xTaskNotifyGive( BLE_WIFI_TaskHandle);
#elif defined(CAT1_VERSION)
			/*??Ҫ֪ͨCAT1?߳?,???????Զ??ϴ???????*/
			xTaskNotify(CAT1_TaskHandle, EVT_UPLOAD_SAMPLE, eSetBits);
#endif
		}
	}
}



