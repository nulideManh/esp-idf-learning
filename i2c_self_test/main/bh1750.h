#ifndef __BH1750_H__
#define __BH1750_H__

#include<stdint.h>


int I2C_Init(void);
void BH1750_Init(void);
int I2C_WriteData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen);
int I2C_ReadData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen);
float BH1750_ReadLightIntensity(void);
void callbackled();


#endif