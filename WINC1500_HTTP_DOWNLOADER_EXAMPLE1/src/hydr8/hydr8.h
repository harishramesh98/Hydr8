/*
 * hydr8.h
 *
 * Created: 5/3/2023 8:01:11 PM
 *  Author: haris
 */ 


#ifndef HYDR8_H_
#define HYDR8_H_

#pragma once
#include "main.h"
#include "asf.h"
#include "SerialConsole.h"
#include "FreeRTOS_CLI.h"
#include "WaterSensor/WaterSensor.h"
#include "SEN0244/SEN0244.h"

#define HYDR8_TASK_SIZE	500
#define HYDR8_PRIORITY (configMAX_PRIORITIES - 1)

#define TDS_THRESHOLD 200.0
#define PUMP_LIM 10
#define WL_LOW_THRESHOLD 20
#define WL_HIGH_THRESHOLD 80
#define WATER_VOLUME_APPROX 30
#define MAX_VOLUME 300


void vHydr8Task(void *pvParameters);
#endif /* HYDR8_H_ */