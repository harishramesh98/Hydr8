/*
 * SEN0244.c
 *
 * Created: 5/1/2023 6:36:11 PM
 *  Author: HR
 */
#include "SEN0244.h"
#include "stdint.h"
#include "SerialConsole.h"

float getTDSdata();
float getTDSdata(){
	uint16_t result1,result;
	uint16_t resultAgg=0;
	uint8_t loop = 10;
	for(uint16_t i=0;i<loop;i++){
		adc_start_conversion(&adc_instance);
		do {
		} while (adc_read(&adc_instance, &result1) == STATUS_BUSY);
		vTaskDelay(10);
		result = result1;
		resultAgg = resultAgg+result;
	}
	float resultAvg = resultAgg/loop;
	float voltage = (resultAvg*3.3)/4096; //Convert analog reading to Voltage
	float tdsValue = (133.42/voltage*voltage*voltage - 255.86*voltage*voltage + 857.39*voltage)*0.5;
	return tdsValue;
}
