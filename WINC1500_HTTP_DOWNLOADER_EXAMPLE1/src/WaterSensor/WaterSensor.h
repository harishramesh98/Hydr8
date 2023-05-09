  /******************************************************************************
  * @file    WaterSensor.h
  * @author  HR and AK
  * 
  * @brief   WaterSensor driver file
  * @date    2023-04-20
  ******************************************************************************/
  #ifndef WaterSensor_H
  #define WaterSensor_H

  #ifdef __cplusplus
  extern "C" {
	  #endif
	  
/******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <math.h>

/******************************************************************************
 * Defines
 ******************************************************************************/
#define WaterSensor_ADDR_L 0x77
#define WaterSensor_ADDR_H 0x78
#define WaterSensor_L_size 8
#define WaterSensor_H_size 12
#define THRESHOLD 100


/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/

/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
/// Structure that describes an I2C data, determining address to use, data buffer to send, etc.
int WaterSensor_Init(void);
int WaterSensor_Read_Data_L(uint8_t *buffer, uint8_t count);
int WaterSensor_Read_Data_H(uint8_t *buffer, uint8_t count);
uint8_t getWaterLevel(uint8_t *bufferL, uint8_t *bufferH);


#ifdef __cplusplus
}
#endif

#endif /*WaterSensor_DRIVER_H */