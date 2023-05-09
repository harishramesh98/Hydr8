/**************************************************************************/ /**
 * @file      CliThread.c
 * @brief     File for the CLI Thread handler. Uses FREERTOS + CLI
 * @author    Eduardo Garcia
 * @date      2020-02-15

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "CliThread.h"
#include "I2cDriver/I2cDriver.h"
// #include "DistanceDriver/DistanceSensor.h"
// #include "IMU\lsm6dso_reg.h"
// #include "SeesawDriver/Seesaw.h"
#include "WifiHandlerThread/WifiHandler.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
static const char pcWelcomeMessage[]  = "FreeRTOS CLI.\r\nType Help to view a list of registered commands.\r\n";

/*static const CLI_Command_Definition_t xImuGetCommand = {"imu", "imu: Returns a value from the IMU\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_GetImuData, 0};*/

static const CLI_Command_Definition_t xOTAUCommand = {"fw", "fw: Download a file and perform an FW update\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_OTAU, 0};

static const CLI_Command_Definition_t xResetCommand = {"reset", "reset: Resets the device\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_ResetDevice, 0};

// static const CLI_Command_Definition_t xNeotrellisTurnLEDCommand = {"led",
//                                                                    "led [keynum][R][G][B]: Sets the given LED to the given R,G,B values.\r\n",
//                                                                    (const pdCOMMAND_LINE_CALLBACK)CLI_NeotrellisSetLed,
//                                                                    4};

// static const CLI_Command_Definition_t xNeotrellisProcessButtonCommand = {"getbutton",
//                                                                          "getbutton: Processes and prints the FIFO button buffer from the seesaw.\r\n",
//                                                                          (const pdCOMMAND_LINE_CALLBACK)CLI_NeotrellProcessButtonBuffer,
//                                                                          0};
// 
// static const CLI_Command_Definition_t xDistanceSensorGetDistance = {"getdistance",
//                                                                     "getdistance: Returns the distance from the US-100 Sensor.\r\n",
//                                                                     (const pdCOMMAND_LINE_CALLBACK)CLI_DistanceSensorGetDistance,
//                                                                     0};
// 
// static const CLI_Command_Definition_t xSendDummyGameData = {"game", "game: Sends dummy game data\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_SendDummyGameData, 0};
static const CLI_Command_Definition_t xI2cScan = {"i2c", "i2c: Scans I2C bus\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_i2cScan, 0};	
static const CLI_Command_Definition_t xtdsScan = {"tds", "tds: Scans ADC bus\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_adcScan, 0};	
static const CLI_Command_Definition_t xpumpon = {"pumpon", "pumpon: switches ON pump\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_gpioON, 0};	
static const CLI_Command_Definition_t xpumpoff = {"pumpoff", "pumpoff: switches OFF pump\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_gpioOFF, 0};
// static const CLI_Command_Definition_t xWaterLevel = {"waterlevel", "waterlevel: Scans I2c bus\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_WaterLevel, 0};
// static const CLI_Command_Definition_t xtest = {"test", "test: checks water level and controls pump\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_test, 0};
// static const CLI_Command_Definition_t xtestall = {"testall", "testall: checks water level and controls pump\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_testall, 0};
	
	
	
	
/*int WaterSensor_Read_Data(uint8_t *buffer, uint8_t count);*/
// Clear screen command
const CLI_Command_Definition_t xClearScreen = {CLI_COMMAND_CLEAR_SCREEN, CLI_HELP_CLEAR_SCREEN, CLI_CALLBACK_CLEAR_SCREEN, CLI_PARAMS_CLEAR_SCREEN};

SemaphoreHandle_t cliCharReadySemaphore;  ///< Semaphore to indicate that a character has been received

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
 static void FreeRTOS_read(char *character);
/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * CLI Thread
 ******************************************************************************/

void vCommandConsoleTask(void *pvParameters)
{
    // REGISTER COMMANDS HERE
    FreeRTOS_CLIRegisterCommand(&xOTAUCommand);
	
/*    FreeRTOS_CLIRegisterCommand(&xImuGetCommand);*/
    FreeRTOS_CLIRegisterCommand(&xClearScreen);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);
	
//     FreeRTOS_CLIRegisterCommand(&xNeotrellisTurnLEDCommand);
//     FreeRTOS_CLIRegisterCommand(&xNeotrellisProcessButtonCommand);
//     FreeRTOS_CLIRegisterCommand(&xDistanceSensorGetDistance);
//     FreeRTOS_CLIRegisterCommand(&xSendDummyGameData);

	FreeRTOS_CLIRegisterCommand(&xI2cScan);
	FreeRTOS_CLIRegisterCommand(&xtdsScan);
	FreeRTOS_CLIRegisterCommand(&xpumpon);
	FreeRTOS_CLIRegisterCommand(&xpumpoff);
// 	FreeRTOS_CLIRegisterCommand(&xWaterLevel);
// 	FreeRTOS_CLIRegisterCommand(&xtest);
// 	FreeRTOS_CLIRegisterCommand(&xtestall);

    char cRxedChar[2];
    unsigned char cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    /* The input and output buffers are declared static to keep them off the stack. */
    static char pcOutputString[MAX_OUTPUT_LENGTH_CLI], pcInputString[MAX_INPUT_LENGTH_CLI];
    static char pcLastCommand[MAX_INPUT_LENGTH_CLI];
    static bool isEscapeCode = false;
    static char pcEscapeCodes[4];
    static uint8_t pcEscapeCodePos = 0;

    /* This code assumes the peripheral being used as the console has already
    been opened and configured, and is passed into the task as the task
    parameter.  Cast the task parameter to the correct type. */

    /* Send a welcome message to the user knows they are connected. */
    SerialConsoleWriteString((char *)pcWelcomeMessage);

    // Any semaphores/mutexes/etc you needed to be initialized, you can do them here
    cliCharReadySemaphore = xSemaphoreCreateBinary();
    if (cliCharReadySemaphore == NULL) {
        LogMessage(LOG_ERROR_LVL, "Could not allocate semaphore\r\n");
        vTaskSuspend(NULL);
    }

    for (;;) {
        FreeRTOS_read(&cRxedChar[0]);

        if (cRxedChar[0] == '\n' || cRxedChar[0] == '\r') {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
            SerialConsoleWriteString((char *)"\r\n");
            // Copy for last command
            isEscapeCode = false;
            pcEscapeCodePos = 0;
            strncpy(pcLastCommand, pcInputString, MAX_INPUT_LENGTH_CLI - 1);
            pcLastCommand[MAX_INPUT_LENGTH_CLI - 1] = 0;  // Ensure null termination

            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            explanation of why this is. */
            do {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(pcInputString,        /* The command string.*/
                                                               pcOutputString,       /* The output buffer. */
                                                               MAX_OUTPUT_LENGTH_CLI /* The size of the output buffer. */
                );

                /* Write the output generated by the command interpreter to the
                console. */
                // Ensure it is null terminated
                pcOutputString[MAX_OUTPUT_LENGTH_CLI - 1] = 0;
                SerialConsoleWriteString(pcOutputString);

            } while (xMoreDataToFollow != pdFALSE);

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
            memset(pcOutputString, 0, MAX_OUTPUT_LENGTH_CLI);
        } else {
            /* The if() clause performs the processing after a newline character
is received.  This else clause performs the processing if any other
character is received. */

            if (true == isEscapeCode) {
                if (pcEscapeCodePos < CLI_PC_ESCAPE_CODE_SIZE) {
                    pcEscapeCodes[pcEscapeCodePos++] = cRxedChar[0];
                } else {
                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }

                if (pcEscapeCodePos >= CLI_PC_MIN_ESCAPE_CODE_SIZE) {
                    // UP ARROW SHOW LAST COMMAND
                    if (strcasecmp(pcEscapeCodes, "oa")) {
                        /// Delete current line and add prompt (">")
                        sprintf(pcInputString, "%c[2K\r>", 27);
                        SerialConsoleWriteString((char *)pcInputString);
                        /// Clear input buffer
                        cInputIndex = 0;
                        memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
                        /// Send last command
                        strncpy(pcInputString, pcLastCommand, MAX_INPUT_LENGTH_CLI - 1);
                        cInputIndex = (strlen(pcInputString) < MAX_INPUT_LENGTH_CLI - 1) ? strlen(pcLastCommand) : MAX_INPUT_LENGTH_CLI - 1;
                        SerialConsoleWriteString(pcInputString);
                    }

                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }
            }
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            else if (cRxedChar[0] == '\r') {
                /* Ignore carriage returns. */
            } else if (cRxedChar[0] == ASCII_BACKSPACE || cRxedChar[0] == ASCII_DELETE) {
                char erase[4] = {0x08, 0x20, 0x08, 0x00};
                SerialConsoleWriteString(erase);
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if (cInputIndex > 0) {
                    cInputIndex--;
                    pcInputString[cInputIndex] = 0;
                }
            }
            // ESC
            else if (cRxedChar[0] == ASCII_ESC) {
                isEscapeCode = true;  // Next characters will be code arguments
                pcEscapeCodePos = 0;
            } else {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a n is entered the complete
                string will be passed to the command interpreter. */
                if (cInputIndex < MAX_INPUT_LENGTH_CLI) {
                    pcInputString[cInputIndex] = cRxedChar[0];
                    cInputIndex++;
                }

                // Order Echo
                cRxedChar[1] = 0;
                SerialConsoleWriteString(&cRxedChar[0]);
            }
        }
    }
}

/**
 * @fn			void FreeRTOS_read(char* character)
 * @brief		This function block the thread unless we received a character
 * @details		This function blocks until UartSemaphoreHandle is released to continue reading characters in CLI
 * @note
 */
static void FreeRTOS_read(char *character)
{
    // We check if there are more characters in the buffer that arrived since the last time
    // This function returns -1 if the buffer is empty, other value otherwise
    int ret = SerialConsoleReadCharacter((uint8_t *)character);

    while (ret == -1) {
        // there are no more characters - block the thread until we receive a semaphore indicating reception of at least 1 character
        xSemaphoreTake(cliCharReadySemaphore, portMAX_DELAY);

        // If we are here it means there are characters in the buffer - we re-read from the buffer to get the newly acquired character
        ret = SerialConsoleReadCharacter((uint8_t *)character);
    }
}

/**
 * @fn			void CliCharReadySemaphoreGiveFromISR(void)
 * @brief		Give cliCharReadySemaphore binary semaphore from an ISR
 * @details
 * @note
 */
void CliCharReadySemaphoreGiveFromISR(void)
{
    static BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(cliCharReadySemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/******************************************************************************
 * CLI Functions - Define here
 ******************************************************************************/

// Example CLI Command. Reads from the IMU and returns data.
// BaseType_t CLI_GetImuData(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
// {
//     static int16_t data_raw_acceleration[3];
//     static float acceleration_mg[3];
//     uint8_t reg;
//     stmdev_ctx_t *dev_ctx = GetImuStruct();
// 	struct ImuDataPacket imuPacket;
// 
//     /* Read output only if new xl value is available */
//     lsm6dso_xl_flag_data_ready_get(dev_ctx, &reg);
// 
//     if (reg) {
//         memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
//         lsm6dso_acceleration_raw_get(dev_ctx, data_raw_acceleration);
//         acceleration_mg[0] = lsm6dso_from_fs2_to_mg(data_raw_acceleration[0]);
//         acceleration_mg[1] = lsm6dso_from_fs2_to_mg(data_raw_acceleration[1]);
//         acceleration_mg[2] = lsm6dso_from_fs2_to_mg(data_raw_acceleration[2]);
// 
//         snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Acceleration [mg]:X %d\tY %d\tZ %d\r\n", (int)acceleration_mg[0], (int)acceleration_mg[1], (int)acceleration_mg[2]);
// 		imuPacket.xmg = (int)acceleration_mg[0];
// 		imuPacket.ymg = (int)acceleration_mg[1];
// 		imuPacket.zmg = (int)acceleration_mg[2];
// 		WifiAddImuDataToQueue(&imuPacket);
//     } else {
//         snprintf((char *)pcWriteBuffer, xWriteBufferLen, "No data ready! Sending dummy data \r\n");
// 		imuPacket.xmg = -1;
// 		imuPacket.ymg = -2;
// 		imuPacket.zmg = -3;
// 		WifiAddImuDataToQueue(&imuPacket);
//     }
//     return pdFALSE;
// }

// THIS COMMAND USES vt100 TERMINAL COMMANDS TO CLEAR THE SCREEN ON A TERMINAL PROGRAM LIKE TERA TERM
// SEE http://www.csie.ntu.edu.tw/~r92094/c++/VT100.html for more info
// CLI SPECIFIC COMMANDS
static char bufCli[CLI_MSG_LEN];
BaseType_t xCliClearTerminalScreen(char *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    char clearScreen = ASCII_ESC;
    snprintf(bufCli, CLI_MSG_LEN - 1, "%c[2J", clearScreen);
    snprintf(pcWriteBuffer, xWriteBufferLen, bufCli);
    return pdFALSE;
}

// Example CLI Command. Reads from the IMU and returns data.
BaseType_t CLI_OTAU(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    WifiHandlerSetState(WIFI_DOWNLOAD_INIT);
	vTaskDelay(20000);
	system_reset();
    return pdFALSE;
}

// Example CLI Command. Resets system.
BaseType_t CLI_ResetDevice(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    system_reset();
    return pdFALSE;
}

/**
 BaseType_t CLI_NeotrellisSetLed( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString )
 * @brief	CLI command to turn on a given LED to a given R,G,B, value
 * @param[out] *pcWriteBuffer. Buffer we can use to write the CLI command response to! See other CLI examples on how we use this to write back!
 * @param[in] xWriteBufferLen. How much we can write into the buffer
 * @param[in] *pcCommandString. Buffer that contains the complete input. You will find the additional arguments, if needed. Please see
 https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Implementing_A_Command.html#Example_Of_Using_FreeRTOS_CLIGetParameter
 Example 3

 * @return		Returns pdFALSE if the CLI command finished.
 * @note         Please see https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Accessing_Command_Line_Parameters.html
                                 for more information on how to use the FreeRTOS CLI.

 */
// BaseType_t CLI_NeotrellisSetLed(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
// {
//     snprintf((char *) pcWriteBuffer, xWriteBufferLen, "Students to fill out!");
//     // Check code SeesawSetLed and SeesawSetLed
//     // How do you get parameters? Checl link in comments!
//     // Check that the input is sanitized: Key between 0-15, RGB between 0-255. Print if there is an error!
//     // return pdFalse to tell the FreeRTOS CLI your call is done and does not need to call again.
//     // This function expects 4 arguments inside pcCommandString: key, R, G, B.
//     return pdFALSE;
// }
// 
/**
 BaseType_t CLI_NeotrellProcessButtonBuffer( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString )
 * @brief	CLI command to process the Neotrellis FIFO button buffer. The Seesaw driver will store all button events until we read them.
 This function will read all the events in the buffer and print the action of each one. For example this is a print:
                 Key 10 pressed
                 Key 11 pressed
                 Key 11 released
                 Key 10 released
                 The function will print "Buffer Empty" if there is nothing on the button buffer.
 * @param[out] *pcWriteBuffer. Buffer we can use to write the CLI command response to! See other CLI examples on how we use this to write back!
 * @param[in] xWriteBufferLen. How much we can write into the buffer
 * @param[in] *pcCommandString. Buffer that contains the complete input. You will find the additional arguments, if needed. Please see
 https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Implementing_A_Command.html#Example_Of_Using_FreeRTOS_CLIGetParameter
 Example 3

 * @return		Returns pdFALSE if the CLI command finished.
 * @note         Please see https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Accessing_Command_Line_Parameters.html
                                 for more information on how to use the FreeRTOS CLI.

 */
// BaseType_t CLI_NeotrellProcessButtonBuffer(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
// {
//     // See functions SeesawGetKeypadCount and SeesawReadKeypad
//     // Returns the number of key events currently on the Seesaw Keypad
// 
//     // Returns the number of requested events in the Seesaw FIFO buffer into the buffer variable
// 
//     //	NEO_TRELLIS_SEESAW_KEY(number) ;
//     // snprintf(pcWriteBuffer,xWriteBufferLen, "count: %d\num_req_eve:%d\t\n",count,num_req_eve);
//     // Print to pcWriteBuffer in order.
//     // If the string is too long to print, print what you can.
//     // The function you write will be useful in the future.
//     uint8_t buffer[64];
//     uint8_t count = SeesawGetKeypadCount();
//     if (count >= 1) {
//         int32_t res = SeesawReadKeypad(buffer, 1);
//         if (res == 0) {
//             uint8_t pos, press;
//             press = buffer[0] & 0x3;
//             pos = buffer[0] >> 2;
//             int num = NEO_TRELLIS_SEESAW_KEY(pos);
//             if (press == 0x2) {
//                 snprintf((char *) pcWriteBuffer, xWriteBufferLen, "Button #%d is released\r\n", NEO_TRELLIS_SEESAW_KEY(num));
//             } else if (press == 0x3) {
//                 snprintf((char *) pcWriteBuffer, xWriteBufferLen, "Button #%d is pressed\r\n", NEO_TRELLIS_SEESAW_KEY(num));
//             }
//         }
//         return pdTRUE;
//     } else {
//         pcWriteBuffer = 0;
//         return pdFALSE;
//     }
// }
// 
// /**
//  BaseType_t CLI_DistanceSensorGetDistance( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString )
//  * @brief	Returns distance in mm
//  * @param[out] *pcWriteBuffer. Buffer we can use to write the CLI command response to! See other CLI examples on how we use this to write back!
//  * @param[in] xWriteBufferLen. How much we can write into the buffer
//  * @param[in] *pcCommandString. Buffer that contains the complete input. You will find the additional arguments, if needed. Please see
//  https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Implementing_A_Command.html#Example_Of_Using_FreeRTOS_CLIGetParameter
//  Example 3
// 
//  * @return		Returns pdFALSE if the CLI command finished.
//  * @note         Please see https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Accessing_Command_Line_Parameters.html
//                                  for more information on how to use the FreeRTOS CLI.
// 
//  */
// BaseType_t CLI_DistanceSensorGetDistance(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
// {
//     uint16_t distance = 0;
//     int error = DistanceSensorGetDistance(&distance, 100);
//     if (0 != error) {
//         snprintf((char *) pcWriteBuffer, xWriteBufferLen, "Sensor Error %d!\r\n", error);
//     } else {
//         snprintf((char *) pcWriteBuffer, xWriteBufferLen, "Distance: %d mm\r\n", distance);
//     }
// 
//     error = WifiAddDistanceDataToQueue(&distance);
//     if (error == pdTRUE) {
//         strcat((char *) pcWriteBuffer, "Distance Data MQTT Post\r\n");
//     }
//     return pdFALSE;
// }
// 
// /**
//  BaseType_t CLI_SendDummyGameData( int8_t *pcWriteBuffer,size_t xWriteBufferLen,const int8_t *pcCommandString )
//  * @brief	Returns dummy game data
//  * @param[out] *pcWriteBuffer. Buffer we can use to write the CLI command response to! See other CLI examples on how we use this to write back!
//  * @param[in] xWriteBufferLen. How much we can write into the buffer
//  * @param[in] *pcCommandString. Buffer that contains the complete input. You will find the additional arguments, if needed. Please see
//  https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Implementing_A_Command.html#Example_Of_Using_FreeRTOS_CLIGetParameter
//  Example 3
// 
//  * @return		Returns pdFALSE if the CLI command finished.
//  * @note         Please see https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI_Accessing_Command_Line_Parameters.html
//                                  for more information on how to use the FreeRTOS CLI.
// 
//  */
// BaseType_t CLI_SendDummyGameData(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
// {
//     struct GameDataPacket gamevar;
// 
//     gamevar.game[0] = 0;
//     gamevar.game[1] = 1;
//     gamevar.game[2] = 2;
//     gamevar.game[3] = 3;
//     gamevar.game[4] = 4;
//     gamevar.game[5] = 5;
//     gamevar.game[6] = 6;
//     gamevar.game[7] = 7;
//     gamevar.game[8] = 8;
//     gamevar.game[9] = 9;
//     gamevar.game[10] = 0xFF;
// 
//     int error = WifiAddGameDataToQueue(&gamevar);
//     if (error == pdTRUE) {
//         snprintf((char *) pcWriteBuffer, xWriteBufferLen, "Dummy Game Data MQTT Post\r\n");
//     }
//     return pdFALSE;
// }


/**************************************************************************/ /**
 * @brief    Scan i2c
 * @param    p_cli 
 * @param    argc 
 * @param    argv 
 ******************************************************************************/
BaseType_t CLI_i2cScan(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{

		I2C_Data i2cOled; 
        //uint8_t address;
		//Send 0 command byte
		uint8_t dataOut[2] = {0,0};
		uint8_t dataIn[2];
		dataOut[0] = 0;
		dataOut[1] = 0;
		i2cOled.address = 0;
		i2cOled.msgIn = (uint8_t*) &dataIn[0];
		i2cOled.lenOut = 1;
		i2cOled.msgOut = (const uint8_t*) &dataOut[0];
		i2cOled.lenIn = 1;

            SerialConsoleWriteString("0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
            for (int i = 0; i < 128; i += 16)
            {
				snprintf(bufCli, CLI_MSG_LEN - 1, "%02x: ", i);
                SerialConsoleWriteString(bufCli);

                for (int j = 0; j < 16; j++)
                {

                    i2cOled.address = (i + j);

                    	
                    int32_t ret = I2cPingAddressWait(&i2cOled, 100, 100);
                    if (ret == 0)
                    {
						snprintf(bufCli, CLI_MSG_LEN - 1, "%02x ", i2cOled.address);
                        SerialConsoleWriteString(bufCli);
                    }
                    else
                    {
                        snprintf(bufCli, CLI_MSG_LEN - 1, "X  ");
						SerialConsoleWriteString(bufCli);
                    }
                }
                SerialConsoleWriteString( "\r\n");
            }
            SerialConsoleWriteString( "\r\n");
			return pdFALSE;

}


/**************************************************************************/ /**
 * @brief    Scan ADC once  
 * @param    p_cli 
 * @param    argc 
 * @param    argv 
 ******************************************************************************/
BaseType_t CLI_adcScan(){
	
		float tdsValue = getTDSdata();
 		char buff1[30];
 		snprintf(buff1, 30, "TDS reading: %d ppm\r\n", (uint16_t)tdsValue);
		SerialConsoleWriteString(buff1);
		
		return pdFALSE;
}

/**************************************************************************/ /**
 * @brief    runs gpio  
 * @param    gpio
 * @param    argc 
 * @param    argv 
 ******************************************************************************/
BaseType_t CLI_gpioON(){
	SerialConsoleWriteString("Starting motor \r\n");
	port_pin_set_output_level(PIN_PB02, true);
	return pdFALSE;
}

/**************************************************************************/ /**
 * @brief    runs gpio  
 * @param    gpio
 * @param    argc 
 * @param    argv 
 ******************************************************************************/
BaseType_t CLI_gpioOFF(){
	port_pin_set_output_level(PIN_PB02, false);
	SerialConsoleWriteString("Done motor \r\n");
	return pdFALSE;
}

// /**************************************************************************/ /**
//  * @brief    runs i2c and gets gets water level
//  * @param    gpio
//  * @param    argc 
//  * @param    argv 
//  ******************************************************************************/
// BaseType_t CLI_WaterLevel()
// {   char buffer[20];
// 	uint8_t wlDataLow[8];
// 	uint8_t wlDataHigh[12];
// 
// 	//Post processing
// 	uint8_t wlPercent = getWaterLevel(wlDataLow,wlDataHigh);
// 	snprintf(buffer, 20, "Water level:%d \r\n", wlPercent);
// 	SerialConsoleWriteString(buffer);
// 	return pdFALSE;
// }
// 
// /**************************************************************************/ /**
//  * @brief    runs i2c and gets water level and switches on pumpon
//  * @param    gpio
//  * @param    argc 
//  * @param    argv 
//  ******************************************************************************/
// BaseType_t CLI_test(){
// 	int counter = 0;
// 	char buffer[20];
// 	int state= WaterSensor_Init();
// 	while(1){
// 		uint8_t wlDataLow[8];
// 		uint8_t wlDataHigh[12];
// 		memset(wlDataLow,0,8);
// 		memset(wlDataHigh,0,12);
// 		uint8_t pumpON;
// 		
// 		uint8_t wl = getWaterLevel(wlDataLow,wlDataHigh);
// 		
// 		snprintf(buffer, 20, "Water level:%d \r\n", wl);
// 		SerialConsoleWriteString(buffer);
// 		
// 		if(wlPercent<=20){
// 			pumpON=1;
// 		}
// 		else if(wlPercent>=70){
// 			pumpON=0;
// 		}
// 		
// 		if(pumpON==1){
// 			SerialConsoleWriteString("Starting motor \r\n");
// 			port_pin_set_output_level(PIN_PB02, true);
// 		}
// 		else{
// 			SerialConsoleWriteString("Stopping motor \r\n");
// 			port_pin_set_output_level(PIN_PB02, false);
// 		}
// 		counter++;
// 		if(counter>=100){
// 			SerialConsoleWriteString("Stopping motor, count reached \r\n");
// 			port_pin_set_output_level(PIN_PB02, false);
// 			break;
// 		}
// 		vTaskDelay(1000);
// 	}
// 
// 	return pdFALSE;
// }
// 
// /**************************************************************************/ /**
//  * @brief    Tests all sensors in a loop of sorts
//  * @param    gpio,i2c and adc
//  * @param     
//  * @param     
//  ******************************************************************************/
// BaseType_t CLI_testall(){
// 	uint8_t countOut=0;
// 	uint8_t tdsLim = 100;
// 	uint8_t pumpLim = 10;
// 	uint8_t wlLow = 20;
// 	uint8_t wlHigh = 60;  
// 	
// 	uint8_t buffer[20];
// 	float tdsThreshold = 200.0;
// 	
// 	while(countOut <= tdsLim){
// 		float tdsValue = getTDSdata();
// 		snprintf((char *) buffer, sizeof(buffer), "TDS value:  %d \r\n", (uint16_t)tdsValue);
// 		SerialConsoleWriteString(buffer);
// 		if(tdsValue < tdsThreshold){
// 			uint8_t countIn=0;
// 			while(countIn <= pumpLim){
// 				uint8_t wlDataLow[8];
// 				uint8_t wlDataHigh[12];
// 				memset(wlDataLow,0,8);
// 				memset(wlDataHigh,0,12);
// 				uint8_t pumpON;
// 				
// 				uint8_t wlPercent = getWaterLevel(wlDataLow,wlDataHigh);
// 				
// 				snprintf((char *) buffer, sizeof(buffer), "Water level:  %d \r\n", wlPercent);
// 				SerialConsoleWriteString(buffer);
// 				
// 				if(wlPercent<=wlLow){
// 					pumpON=1;
// 				}
// 				else if(wlPercent>=wlHigh){
// 					pumpON=0;
// 				}
// 				
// 				if(pumpON==1){
// 					SerialConsoleWriteString("Motor on\r\n");
// 					port_pin_set_output_level(PIN_PB02, true);
// 				}
// 				else{
// 					SerialConsoleWriteString("Motor off\r\n");
// 					port_pin_set_output_level(PIN_PB02, false);
// 				}
// 				vTaskDelay(1000);
// 				countIn++;
// 			}
// 			countOut++;
// 		}
// 		else{
// 			SerialConsoleWriteString("TDS value too high. Exiting\r\n");
// 			SerialConsoleWriteString("Motor off\r\n");
// 			port_pin_set_output_level(PIN_PB02, false);
// 			return pdFALSE;
// 		}
// 	}
// 	SerialConsoleWriteString("Both counters done. Normal exit\r\n");
// 	return pdFALSE;
// }