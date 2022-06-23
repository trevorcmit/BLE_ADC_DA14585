/**
****************************************************************************************
* @file user_periph_setup.h
* @brief Peripherals setup header file.
****************************************************************************************
*/
#ifndef _USER_PERIPH_SETUP_H_
#define _USER_PERIPH_SETUP_H_

#include "gpio.h"
#include "uart.h"
#include "spi.h"
#include "spi_flash.h"
#include "i2c.h"
#include "i2c_eeprom.h"


// ****** DEFINES ****** 

// UART Configuration
// Define UART2 Tx Pad
#if defined (__DA14531__)
    #define UART2_TX_PORT           GPIO_PORT_0
    #define UART2_TX_PIN            GPIO_PIN_6
#else
    // #define UART2_TX_PORT           GPIO_PORT_0
    // #define UART2_TX_PIN            GPIO_PIN_4
#endif

// Define UART2 Settings
#define UART2_BAUDRATE              UART_BAUDRATE_115200
#define UART2_DATABITS              UART_DATABITS_8
#define UART2_PARITY                UART_PARITY_NONE
#define UART2_STOPBITS              UART_STOPBITS_1
#define UART2_AFCE                  UART_AFCE_DIS
#define UART2_FIFO                  UART_FIFO_EN
#define UART2_TX_FIFO_LEVEL         UART_TX_FIFO_LEVEL_0
#define UART2_RX_FIFO_LEVEL         UART_RX_FIFO_LEVEL_0


/****************************************************************************************/
/* LED configuration                                                                    */
/****************************************************************************************/
#if defined (__DA14531__)
    #define GPIO_LED_PORT           GPIO_PORT_0
    #define GPIO_LED_PIN            GPIO_PIN_9
#else
    #define GPIO_LED_PORT           GPIO_PORT_1
    #define GPIO_LED_PIN            GPIO_PIN_1
#endif

/****************************************************************************************/
/* SPI configuration                                                                    */
/****************************************************************************************/
// Define SPI Pads
#if defined (__DA14531__)
    #define SPI_EN_PORT             GPIO_PORT_0
    #define SPI_EN_PIN              GPIO_PIN_1
    #define SPI_CLK_PORT            GPIO_PORT_0
    #define SPI_CLK_PIN             GPIO_PIN_4
    #define SPI_DO_PORT             GPIO_PORT_0
    #define SPI_DO_PIN              GPIO_PIN_0
    #define SPI_DI_PORT             GPIO_PORT_0
    #define SPI_DI_PIN              GPIO_PIN_3

#elif !defined (__DA14586__) // DA14585 Section
    
    // Existing SPI Assignments
    #define SPI_EN_PORT             GPIO_PORT_0
    #define SPI_EN_PIN              GPIO_PIN_3
    #define SPI_CLK_PORT            GPIO_PORT_0
    #define SPI_CLK_PIN             GPIO_PIN_0
    #define SPI_DO_PORT             GPIO_PORT_0
    #define SPI_DO_PIN              GPIO_PIN_6
    #define SPI_DI_PORT             GPIO_PORT_0
    #define SPI_DI_PIN              GPIO_PIN_5

    // NEW SPI Assignments for Initial Project Configuration
    // #define SPI_EN_PORT             GPIO_PORT_0
    // #define SPI_EN_PIN              GPIO_PIN_1
    // #define SPI_CLK_PORT            GPIO_PORT_0
    // #define SPI_CLK_PIN             GPIO_PIN_4
    // #define SPI_DO_PORT             GPIO_PORT_0
    // #define SPI_DO_PIN              GPIO_PIN_6     // Default IPC uses Pin 0 here
    // #define SPI_DI_PORT             GPIO_PORT_0
    // #define SPI_DI_PIN              GPIO_PIN_3
#endif

#if !defined (__DA14586__)
    #define SPI_FLASH_DEV_SIZE          (256 * 1024)
#endif


/***************************************************************************************/
/* Production debug output configuration                                               */
/***************************************************************************************/
#if PRODUCTION_DEBUG_OUTPUT
#if defined (__DA14531__)
    #define PRODUCTION_DEBUG_PORT   GPIO_PORT_0
    #define PRODUCTION_DEBUG_PIN    GPIO_PIN_11
#else
    #define PRODUCTION_DEBUG_PORT   GPIO_PORT_2
    #define PRODUCTION_DEBUG_PIN    GPIO_PIN_5
#endif
#endif


// ADC Configuration
// #define ADC_INPUT_PORT                  GPIO_PORT_0
// #define ADC_INPUT_PIN                   GPIO_PIN_2  // 2 is not in used by default SPI configuration




// FUNCTION DECLARATIONS

#if DEVELOPMENT_DEBUG
/**
****************************************************************************************
* @brief   Reserves application's specific GPIOs
* @details Used only in Development mode (#if DEVELOPMENT_DEBUG)
*          i.e. to reserve P0_1 as Generic Purpose I/O:
*          RESERVE_GPIO(DESCRIPTIVE_NAME, GPIO_PORT_0, GPIO_PIN_1, PID_GPIO);
****************************************************************************************
*/
void GPIO_reservations(void);
#endif

/**
****************************************************************************************
* @brief   Sets the functionality of application pads
* @details i.e. to set P0_1 as Generic purpose Output:
*          GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_1, OUTPUT, PID_GPIO, false);
****************************************************************************************
*/
void set_pad_functions(void);

/**
****************************************************************************************
* @brief   Initializes application's peripherals and pins
****************************************************************************************
*/
void periph_init(void);


#endif // _USER_PERIPH_SETUP_H_
