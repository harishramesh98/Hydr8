/*
 * hydr8.c
 *
 * Created: 5/3/2023 8:00:56 PM
 *  Author: haris
 */ 

#include "WifiHandlerThread/WifiHandler.h"
#include "hydr8.h"

extern uint8_t masterControl;

#include <errno.h>

uint8_t waterLevel_global = 0;

void vHydr8Task(void *pvParameters){
	WaterSensor_Init();
	uint8_t countIn = 0;
	uint8_t pumpON=0;
	uint8_t wlPercent=0;
	uint8_t wlDataLow[8];
	uint8_t wlDataHigh[12];
	
	
	//MQTT structs
	struct Hydr8WaterLevel hydr8WaterData;
	struct Hydr8TdsLevel hydr8TDSData;
	struct pumpLED pumpLedData;
	struct tdsLED tdsLedData;
	struct VolumeConsumed volData;
	pumpLedData.pumpLedState = 0;
	tdsLedData.tdsLedState = 0;
	
	//For calculating volume
	uint8_t waterLevelPrevious=0;
	uint16_t volume=0;
	volData.volumeData = 0;
	
	while(1){
		float tdsValue = getTDSdata();
		hydr8TDSData.tdsLevel = (uint16_t)tdsValue;
		WifiAddHydr8TDSDataToQueue(&hydr8TDSData);

		if(tdsValue < TDS_THRESHOLD){
			WifiAddHydr8TdsLedToQueue(&tdsLedData);
			countIn=0;
			while(countIn <= PUMP_LIM){
				memset(wlDataLow,0,8);
				memset(wlDataHigh,0,12);
				wlPercent = getWaterLevel(wlDataLow,wlDataHigh);
				if(wlPercent<=WL_LOW_THRESHOLD){
					pumpON=1;
				}
				else if(wlPercent>=WL_HIGH_THRESHOLD){
					pumpON=0;
				}
				
				if((pumpON==1)&&(masterControl==1)){
					port_pin_set_output_level(PIN_PB02, true);
					pumpLedData.pumpLedState = 1;
					WifiAddHydr8PumpLedToQueue(&pumpLedData);
				}
				else{
					port_pin_set_output_level(PIN_PB02, false);
						if(wlPercent<waterLevelPrevious){
							volume = volume + (waterLevelPrevious-wlPercent)*WATER_VOLUME_APPROX;
						}
					volData.volumeData = volume;
					WifiAddHydr8VolumeToQueue(&volData);
					waterLevelPrevious = wlPercent;
					pumpLedData.pumpLedState = 0;
					WifiAddHydr8PumpLedToQueue(&pumpLedData);
				}
				hydr8WaterData.waterLevel = wlPercent;
				WifiAddHydr8WLDataToQueue(&hydr8WaterData);
				vTaskDelay(500);
				countIn++;
			}
		}
		else{
			port_pin_set_output_level(PIN_PB02, false);
			hydr8WaterData.waterLevel = wlPercent;
			WifiAddHydr8WLDataToQueue(&hydr8WaterData);
			tdsLedData.tdsLedState=1;
			WifiAddHydr8TdsLedToQueue(&tdsLedData);
		}
	}
}