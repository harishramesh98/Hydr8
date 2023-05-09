/**************************************************************************//**
* @file      BootMain.c
* @brief     Main file for the ESE516 bootloader. Handles updating the main application
* @details   Main file for the ESE516 bootloader. Handles updating the main application
* @author    Eduardo Garcia
* @date      2020-02-15
* @version   2.0
* @copyright Copyright University of Pennsylvania
******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "sd_mmc_spi.h"

#include "SD Card/SdCard.h"
#include "Systick/Systick.h"
#include "SerialConsole/SerialConsole.h"
#include "ASF/sam0/drivers/dsu/crc32/crc32.h"
#include <stdio.h>




/******************************************************************************
* Defines
******************************************************************************/
#define APP_START_ADDRESS  ((uint32_t)0x12000) ///<Start of main application. Must be address of start of main application
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04) ///< Main application reset vector address
//#define MEM_EXAMPLE 1 //COMMENT ME TO REMOVE THE MEMORY WRITE EXAMPLE BELOW

/******************************************************************************
* Structures and Enumerations
******************************************************************************/

struct usart_module cdc_uart_module; ///< Structure for UART module connected to EDBG (used for unit test output)

/******************************************************************************
* Local Function Declaration
******************************************************************************/
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);


/******************************************************************************
* Global Variables
******************************************************************************/
//INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";	///<Test TEXT File name
char test_bin_file[] = "0:sd_binary.bin";	///<Test BINARY File name
char Test_file_name_A[] = "0:FlagA.txt";
char Test_file_name_B[] = "0:FlagB.txt";
char Test_bin_file_A[] = "0:ESE516_MAIN_FW.bin";
char Test_bin_file_B[] = "0:TestB.bin";
char File_name_A[] = "0:A.txt";
char File_name_B[] = "0:B.txt";
Ctrl_status status; ///<Holds the status of a system initialization
FRESULT res, res1, res2, res3, res4; //Holds the result of the FATFS functions done on the SD CARD TEST
FATFS fs; //Holds the File System of the SD CARD
FIL file_object; //FILE OBJECT used on main for the SD Card Test
int state_AB;

/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn		int main(void)
* @brief	Main function for ESE516 Bootloader Application

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/

int main(void)
{

/*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
system_init();
delay_init();
InitializeSerialConsole();
system_interrupt_enable_global();
/* Initialize SD MMC stack */
sd_mmc_init();

//Initialize the NVM driver
configure_nvm();

irq_initialize_vectors();
cpu_irq_enable();

//Configure CRC32
dsu_crc32_init();

SerialConsoleWriteString("ESE516 - ENTER BOOTLOADER");	//Order to add string to TX Buffer

/*END SYSTEM PERIPHERALS INITIALIZATION*/


/*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/

//EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
//See function inside to see how to open a file
SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

if(StartFilesystemAndTest() == false)
{
SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
delay_cycles_ms(5000);
system_reset();
}
else
{
SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
}

/*END SIMPLE SD CARD MOUNTING AND TEST!*/


/*3.) STARTS BOOTLOADER HERE!*/

//PERFORM BOOTLOADER HERE!

//	#define MEM_EXAMPLE
//#ifdef MEM_EXAMPLE
//Example
//Check out the NVM API at http://asf.atmel.com/docs/latest/samd21/html/group__asfdoc__sam0__nvm__group.html#asfdoc_sam0_nvm_examples . It provides important information too!

//We will ask the NVM driver for information on the MCU (SAMD21)
struct nvm_parameters parameters;
char helpStr[64]; //Used to help print values
nvm_get_parameters (&parameters); //Get NVM parameters
snprintf(helpStr, 63,"NVM Info: Number of Pages %d. Size of a page: %d bytes. \r\n", parameters.nvm_number_of_pages, parameters.page_size);
SerialConsoleWriteString(helpStr);
//The SAMD21 NVM (and most if not all NVMs) can only erase and write by certain chunks of data size.
//The SAMD21 can WRITE on pages. However erase are done by ROW. One row is conformed by four (4) pages.
//An erase is mandatory before writing to a page.


//We will do the following in this example:
//Erase the first row of the application data
//Read a page of data from the SD Card. The SD CARD initialization writes a file with this data as its test for initialization
//Write it to the first row.


//configure pin BUTTON_0_PIN for SWO
struct port_config config_port_pin;//Define structure needed to configure a pin
port_get_config_defaults(&config_port_pin); //Initialize structure with default configurations.
/* Set buttons as input */
config_port_pin.direction  = PORT_PIN_DIR_INPUT;
config_port_pin.input_pull = PORT_PIN_PULL_UP;
port_pin_set_config(BUTTON_0_PIN, &config_port_pin);

uint8_t readBuffer[256]; //Buffer the size of one row
uint32_t numBytesRead = 0;

// change state of flag based on SWO button press and Flags (FlagA.txt or FlagB.txt) created
state_AB = 2;
delay_ms(20);

Test_bin_file_A[0] = LUN_ID_SD_MMC_0_MEM + '0';
res3 = f_open(&file_object, (char const *)Test_bin_file_A, FA_READ|FA_OPEN_EXISTING);
Test_bin_file_B[0] = LUN_ID_SD_MMC_0_MEM + '0';
res4 = f_open(&file_object, (char const *)Test_bin_file_B, FA_READ);

if(res3 == FR_OK){
state_AB = 1;
}
else if(res4 == FR_OK){
state_AB = 0;
}
else{
state_AB = 2;
}

Test_file_name_B[0] = LUN_ID_SD_MMC_0_MEM + '0';
res1 = f_open(&file_object, (char const *)Test_file_name_B, FA_READ|FA_OPEN_EXISTING);
Test_file_name_A[0] = LUN_ID_SD_MMC_0_MEM + '0';
res2 = f_open(&file_object, (char const *)Test_file_name_A, FA_READ);

if(port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE && res1 == FR_OK){
//if(state_AB == 1){
// Test_file_name_B[0] = LUN_ID_SD_MMC_0_MEM + '0';
// res = f_open(&file_object, (char const *)Test_file_name_B, FA_READ|FA_OPEN_EXISTING);
// if (res == FR_OK)
// {
state_AB = 0; //change state of flag
delay_ms(20);
f_unlink((char const*)Test_file_name_B); //unlink FlagB.txt
//}
}
else if (port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE && res2 == FR_OK){
// Test_file_name_A[0] = LUN_ID_SD_MMC_0_MEM + '0';
// res = f_open(&file_object, (char const *)Test_file_name_A, FA_READ);
// if (res == FR_OK)
// {
state_AB = 1; //changing state of flag
delay_ms(20);
f_unlink((char const*)Test_file_name_A); //unlink FlagA.txt
//}
}

//}
//check if Status is 1 or 0. If Status is 1 then run file TestA.bin else run TestB.bin
if(state_AB == 1){
for(int w = 0; w < 165052; w=w + 256){
enum status_code nvmError = nvm_erase_row(APP_START_ADDRESS + w);
if(nvmError != STATUS_OK)
{
//SerialConsoleWriteString("Erase error");
}

//Make sure it got erased - we read the page. Erasure in NVM is an 0xFF
for(int iter = 0; iter < 256; iter++)
{
char *a = (char *)(APP_START_ADDRESS + iter); //Pointer pointing to address APP_START_ADDRESS
if(*a != 0xFF)
{
//SerialConsoleWriteString("Error - test page is not erased!");
break;
}
}


Test_bin_file_A[0] = LUN_ID_SD_MMC_0_MEM + '0';
res = f_open(&file_object, (char const *)Test_bin_file_A, FA_READ);

if (res != FR_OK)
{
//SerialConsoleWriteString("Could not open test file!\r\n");
}

FRESULT f_unlink (const TCHAR* File_name_A);

int numBytesLeft = 256;
numBytesRead = 0;
int numberBytesTotal = 0;
res = f_lseek(&file_object, w);
while(numBytesLeft  != 0)
{

res = f_read(&file_object, &readBuffer[numberBytesTotal], numBytesLeft, &numBytesRead); //Question to students: What is numBytesRead? What are we doing here?
numBytesLeft -= numBytesLeft;
numberBytesTotal += numBytesRead;
}
SerialConsoleWriteString("A read successfully\r\n");
//Write data to first row. Writes are per page, so we need four writes to write a complete row
res = nvm_write_buffer (APP_START_ADDRESS + w, &readBuffer[0], 64);
res = nvm_write_buffer (APP_START_ADDRESS + 64 + w, &readBuffer[64], 64);
res = nvm_write_buffer (APP_START_ADDRESS + 128 + w, &readBuffer[128], 64);
res = nvm_write_buffer (APP_START_ADDRESS + 192 + w, &readBuffer[192], 64);

}
if (res != FR_OK)
{
SerialConsoleWriteString("Test write to NVM failed!\r\n");
}
else
{
SerialConsoleWriteString("Test write to NVM succeeded!\r\n");
}
}

else if (state_AB == 0){
for(int w = 0; w < 32768; w=w + 256){
enum status_code nvmError = nvm_erase_row(APP_START_ADDRESS + w);
if(nvmError != STATUS_OK)
{
SerialConsoleWriteString("Erase error");
}

//Make sure it got erased - we read the page. Erasure in NVM is an 0xFF
for(int iter = 0; iter < 256; iter++)
{
char *a = (char *)(APP_START_ADDRESS + iter); //Pointer pointing to address APP_START_ADDRESS
if(*a != 0xFF)
{
//SerialConsoleWriteString("Error - test page is not erased!");
break;
}
}


Test_bin_file_B[0] = LUN_ID_SD_MMC_0_MEM + '0';
res = f_open(&file_object, (char const *)Test_bin_file_B, FA_READ);

if (res != FR_OK)
{
//SerialConsoleWriteString("Could not open test file!\r\n");
}

FRESULT f_unlink (const TCHAR* File_name_B);

int numBytesLeft = 256;
numBytesRead = 0;
int numberBytesTotal = 0;
res = f_lseek(&file_object, w);
while(numBytesLeft  != 0)
{

res = f_read(&file_object, &readBuffer[numberBytesTotal], numBytesLeft, &numBytesRead); //Question to students: What is numBytesRead? What are we doing here?
numBytesLeft -= numBytesLeft;
numberBytesTotal += numBytesRead;
}
SerialConsoleWriteString("A read successfully\r\n");
//Write data to first row. Writes are per page, so we need four writes to write a complete row
res = nvm_write_buffer (APP_START_ADDRESS + w, &readBuffer[0], 64);
res = nvm_write_buffer (APP_START_ADDRESS + 64 + w, &readBuffer[64], 64);
res = nvm_write_buffer (APP_START_ADDRESS + 128 + w, &readBuffer[128], 64);
res = nvm_write_buffer (APP_START_ADDRESS + 192 + w, &readBuffer[192], 64);

}
if (res != FR_OK)
{
SerialConsoleWriteString("Test write to NVM failed!\r\n");
}
else
{
SerialConsoleWriteString("Test write to NVM succeeded!\r\n");
}
}

/*END BOOTLOADER HERE!*/

//4.) DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!
SerialConsoleWriteString("ESE516 - EXIT BOOTLOADER");	//Order to add string to TX Buffer
delay_cycles_ms(100); //Delay to allow print

//Deinitialize HW - deinitialize started HW here!
DeinitializeSerialConsole(); //Deinitializes UART
sd_mmc_deinit(); //Deinitialize SD CARD


//Jump to application
jumpToApplication();

//Should not reach here! The device should have jumped to the main FW.

}







/******************************************************************************
* Static Functions
******************************************************************************/



/**************************************************************************//**
* function      static void StartFilesystemAndTest()
* @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       Returns true is SD card and file system test passed. False otherwise.
******************************************************************************/
static bool StartFilesystemAndTest(void)
{
bool sdCardPass = true;
uint8_t binbuff[256];

//Before we begin - fill buffer for binary write test
//Fill binbuff with values 0x00 - 0xFF
for(int i = 0; i < 256; i++)
{
binbuff[i] = i;
}

//MOUNT SD CARD
Ctrl_status sdStatus= SdCard_Initiate();
if(sdStatus == CTRL_GOOD) //If the SD card is good we continue mounting the system!
{
SerialConsoleWriteString("SD Card initiated correctly!\n\r");

//Attempt to mount a FAT file system on the SD Card using FATFS
SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
memset(&fs, 0, sizeof(FATFS));
res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs); //Order FATFS Mount
if (FR_INVALID_DRIVE == res)
{
LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
sdCardPass = false;
goto main_end_of_test;
}
SerialConsoleWriteString("[OK]\r\n");

//Create and open a file
SerialConsoleWriteString("Create a file (f_open)...\r\n");

test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
res = f_open(&file_object,
(char const *)test_file_name,
FA_CREATE_ALWAYS | FA_WRITE);

if (res != FR_OK)
{
LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
sdCardPass = false;
goto main_end_of_test;
}

SerialConsoleWriteString("[OK]\r\n");

//Write to a file
SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

if (0 == f_puts("Test SD/MMC stack\n", &file_object))
{
f_close(&file_object);
LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
sdCardPass = false;
goto main_end_of_test;
}

SerialConsoleWriteString("[OK]\r\n");
f_close(&file_object); //Close file
SerialConsoleWriteString("Test is successful.\n\r");


//Write binary file
//Read SD Card File
test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
res = f_open(&file_object, (char const *)test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);

if (res != FR_OK)
{
SerialConsoleWriteString("Could not open binary file!\r\n");
LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
sdCardPass = false;
goto main_end_of_test;
}

//Write to a binaryfile
SerialConsoleWriteString("Write to test file (f_write)...\r\n");
uint32_t varWrite = 0;
if (0 != f_write(&file_object, binbuff,256, (UINT*) &varWrite))
{
f_close(&file_object);
LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
sdCardPass = false;
goto main_end_of_test;
}

SerialConsoleWriteString("[OK]\r\n");
f_close(&file_object); //Close file
SerialConsoleWriteString("Test is successful.\n\r");

main_end_of_test:
SerialConsoleWriteString("End of Test.\n\r");

}
else
{
SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
sdCardPass = false;
}

return sdCardPass;
}



/**************************************************************************//**
* function      static void jumpToApplication(void)
* @brief        Jumps to main application
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return
******************************************************************************/
static void jumpToApplication(void)
{
// Function pointer to application section
void (*applicationCodeEntry)(void);

// Rebase stack pointer
__set_MSP(*(uint32_t *) APP_START_ADDRESS);

// Rebase vector table
SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

// Set pointer to application section
applicationCodeEntry =
(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

// Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry,
//which should be the start of the main FW.
applicationCodeEntry();
}



/**************************************************************************//**
* function      static void configure_nvm(void)
* @brief        Configures the NVM driver
* @details
* @return
******************************************************************************/
static void configure_nvm(void)
{
struct nvm_config config_nvm;
nvm_get_config_defaults(&config_nvm);
config_nvm.manual_page_write = false;
nvm_set_config(&config_nvm);
}



