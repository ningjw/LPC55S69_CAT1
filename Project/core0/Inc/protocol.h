#ifndef __PROTOCOL_H
#define __PROTOCOL_H


uint16_t CRC16(uint8_t *data,uint32_t length);
uint32_t PacketUploadSampleData(uint8_t *txBuf, uint32_t sid);
uint32_t PacketSystemInfo(uint8_t *txBuf);
uint8_t ParseSamplePara(uint8_t *data);


#endif
