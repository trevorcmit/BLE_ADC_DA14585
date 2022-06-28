/**
 ****************************************************************************************
 * @file user_periph_setup.c
 * @brief Peripherals setup and initialization.
 ****************************************************************************************
 */
#include "user_periph_setup.h"
#include "datasheet.h"
#include "system_library.h"
#include "rwip_config.h"
#include "gpio.h"
#include "uart.h"
#include "syscntl.h"

/**
****************************************************************************************
* @brief Each application reserves its own GPIOs here.
****************************************************************************************
*/

#if DEVELOPMENT_DEBUG

void GPIO_reservations(void) {
    /*
        i.e. to reserve P0_1 as Generic Purpose I/O:
        RESERVE_GPIO(DESCRIPTIVE_NAME, GPIO_PORT_0, GPIO_PIN_1, PID_GPIO);
    */

    // #if defined (CFG_PRINTF_UART2)
    //     RESERVE_GPIO(UART2_TX, UART2_TX_PORT, UART2_TX_PIN, PID_UART2_TX);
    // #endif

    RESERVE_GPIO(LED, GPIO_LED_PORT, GPIO_LED_PIN, PID_GPIO);

    // #if !defined (__DA14586__)
        // RESERVE_GPIO(SPI_EN, SPI_EN_PORT, SPI_EN_PIN, PID_SPI_EN);
    // #endif

    // New Reservations for Initial Project Configuration, Does Not Cause Errors Alone
    RESERVE_GPIO(SPI_FLASH_CS, SPI_EN_PORT, SPI_EN_PIN, PID_SPI_EN);
    RESERVE_GPIO(SPI_FLASH_CLK, SPI_CLK_PORT, SPI_CLK_PIN, PID_SPI_CLK);
    RESERVE_GPIO(SPI_FLASH_DO, SPI_DO_PORT, SPI_DO_PIN, PID_SPI_DO);
    RESERVE_GPIO(SPI_FLASH_DI, SPI_DI_PORT, SPI_DI_PIN, PID_SPI_DI);

    // Reservation for ADC Pin from DA14531 General Purpose ADC Tutorial
    RESERVE_GPIO(ADC_INPUT, ADC_INPUT_PORT, ADC_INPUT_PIN, PID_ADC);
}

#endif

void set_pad_functions(void) {
    /*
        i.e. to set P0_1 as Generic purpose Output:
        GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_1, OUTPUT, PID_GPIO, false);
    */

    #if defined (__DA14586__)
        // Disallow spontaneous DA14586 SPI Flash wake-up
        GPIO_ConfigurePin(GPIO_PORT_2, GPIO_PIN_3, OUTPUT, PID_GPIO, true);
    // #else
        // Disallow spontaneous SPI Flash wake-up
        // GPIO_ConfigurePin(SPI_EN_PORT, SPI_EN_PIN, OUTPUT, PID_SPI_EN, true);
    #endif

    // #if defined (CFG_PRINTF_UART2)
    //     // Configure UART2 TX Pad
    //     GPIO_ConfigurePin(UART2_TX_PORT, UART2_TX_PIN, OUTPUT, PID_UART2_TX, false);
    // #endif

    GPIO_ConfigurePin(GPIO_LED_PORT, GPIO_LED_PIN, OUTPUT, PID_GPIO, false);

    // Added for Initial Project Configuration
    GPIO_ConfigurePin(SPI_EN_PORT, SPI_EN_PIN, OUTPUT, PID_SPI_EN, true); // already Configured earlier
    GPIO_ConfigurePin(SPI_CLK_PORT, SPI_CLK_PIN, OUTPUT, PID_SPI_CLK, false);
    GPIO_ConfigurePin(SPI_DO_PORT, SPI_DO_PIN, OUTPUT, PID_SPI_DO, false);
    GPIO_ConfigurePin(SPI_DI_PORT, SPI_DI_PIN, INPUT, PID_SPI_DI, false);

    // Added for General Purpose ADC
    GPIO_ConfigurePin(ADC_INPUT_PORT, ADC_INPUT_PIN, INPUT, PID_ADC, false);
}

// #if defined (CFG_PRINTF_UART2)
// // Configuration struct for UART2
// static const uart_cfg_t uart_cfg = {
//     .baud_rate = UART2_BAUDRATE,
//     .data_bits = UART2_DATABITS,
//     .parity = UART2_PARITY,
//     .stop_bits = UART2_STOPBITS,
//     .auto_flow_control = UART2_AFCE,
//     .use_fifo = UART2_FIFO,
//     .tx_fifo_tr_lvl = UART2_TX_FIFO_LEVEL,
//     .rx_fifo_tr_lvl = UART2_RX_FIFO_LEVEL,
//     .intr_priority = 2,
// };
// #endif

void periph_init(void) {
    #if defined (__DA14531__)
        // In Boost mode enable the DCDC converter to supply VBAT_HIGH for the used GPIOs
        syscntl_dcdc_turn_on_in_boost(SYSCNTL_DCDC_LEVEL_3V0);
    #else
        // Power up peripherals' power domain
        SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
        while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));
        SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);
    #endif

        // ROM patch
        patch_func();

        // Initialize peripherals
    // #if defined (CFG_PRINTF_UART2)
    //     // Initialize UART2
    //     uart_initialize(UART2, &uart_cfg);
    // #endif

        // Set pad functionality
        set_pad_functions();

        // Enable the pads
        GPIO_set_pad_latch_en(true);
}
