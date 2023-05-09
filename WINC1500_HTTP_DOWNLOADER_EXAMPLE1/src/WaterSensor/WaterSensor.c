/******************************************************************************
  * @file    WaterSensor.h
  * @author  HR and AK
  * @brief   WaterSensor driver file
  * @date    2023-04-20
  ******************************************************************************/
  
/******************************************************************************
 * Includes
 ******************************************************************************/

#include "WaterSensor.h"
#include "i2c_master.h"
#include "i2c_master_interrupt.h"
#include "I2cDriver\I2cDriver.h"
#include "stdint.h"
#include "SerialConsole.h"

int WaterSensor_Read_Data_L(uint8_t *buffer, uint8_t count);
int WaterSensor_Init(void);

I2C_Data WaterSensorData;
int WaterSensor_Init(){
	
     uint8_t cmd[] = {0x77, 0x78};

     WaterSensorData.address = WaterSensor_ADDR_L;
     WaterSensorData.msgOut = (const uint8_t *) &cmd[0];
     WaterSensorData.lenOut = sizeof(cmd);
     WaterSensorData.lenIn = 0;
     int32_t error = I2cWriteDataWait(&WaterSensorData, 0xff);
     return error;
}

int WaterSensor_Read_Data_L(uint8_t *buffer, uint8_t count){
	
	uint8_t cmd[] = {0x77, 0x78};
	WaterSensorData.address = WaterSensor_ADDR_L;
	WaterSensorData.msgOut = (const uint8_t*) &cmd[0];
	WaterSensorData.lenOut = 1;
	WaterSensorData.msgIn = buffer;
	WaterSensorData.lenIn = 8;

	int error = I2cReadDataWait(&WaterSensorData, 0xff, 0xff);

	if (ERROR_NONE != error) {
		SerialConsoleWriteString("Error reading WaterSensor low!/r/n");
	}
	return error;
}

int WaterSensor_Read_Data_H(uint8_t *buffer, uint8_t count){
	uint8_t cmd[] = {0x77, 0x78};
	WaterSensorData.address = WaterSensor_ADDR_H;
	WaterSensorData.msgOut = (const uint8_t*) &cmd[1];
	WaterSensorData.lenOut = 1;
	WaterSensorData.msgIn = buffer;
	WaterSensorData.lenIn = 12;

	int error = I2cReadDataWait(&WaterSensorData, 0xff, 0xff);

	if (ERROR_NONE != error) {
		SerialConsoleWriteString("Error reading WaterSensor low!/r/n");
	}
	return error;
}

uint8_t getWaterLevel(uint8_t *wlDataLow, uint8_t *wlDataHigh){
	int state = WaterSensor_Read_Data_L(wlDataLow,sizeof(wlDataLow));
	vTaskDelay(50);
	state = WaterSensor_Read_Data_H(wlDataHigh,sizeof(wlDataHigh));
	uint32_t touch_val = 0;
	uint8_t trig_section = 0;
	for (int i = 0 ; i < 8; i++) {
		if (wlDataLow[i] > THRESHOLD) {
			touch_val |= 1 << i;

		}
	}
	for (int i = 0 ; i < 12; i++) {
		if (wlDataHigh[i] > THRESHOLD) {
			touch_val |= (uint32_t)1 << (8 + i);
		}
	}

	while (touch_val & 0x01)
	{
		trig_section++;
		touch_val >>= 1;
	}
	uint8_t waterLevelCent = trig_section*5;
	return waterLevelCent;
}