#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

#include "hardware/pwm.h"  //PS-20241120
#include "hardware/vreg.h"  //PS-20241120

#include "Ntag/Ntag.h"

#define PLL_SYS_KHZ (339 * 1000)//(339 * 1000)//(468 * 1000)//(133 * 1000)

///////////////////////////////////////
#define IOO_INDICATOR			6
#define INDICATOR_TOG			sio_hw->gpio_togl = (1<<IOO_INDICATOR);
#define INDICATOR_ON			sio_hw->gpio_set = (1<<IOO_INDICATOR);
#define INDICATOR_OFF 			sio_hw->gpio_clr = (1<<IOO_INDICATOR);
void X_InitIO()
{
    gpio_init(IOO_INDICATOR);
    gpio_set_dir(IOO_INDICATOR, GPIO_OUT);
    INDICATOR_ON; 
}
///////////////////////////////////////


int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 12204000//115200
// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5
uint16_t uartrecvcounter;
uint8_t recv_data[1024];

void __not_in_flash_func(X_RecvHandle)()
{
    uint16_t i;
    uint8_t sum;
    
    //data not enough
    if (uartrecvcounter < 5) {
        return; 
    }
    //check length
    if ((uartrecvcounter) != (recv_data[0]*256 + recv_data[1] + 4)) {
        return; 
    }
    //check sum
    sum = 0;
    for(i=0;i<(uartrecvcounter-1);i++){
        sum += recv_data[i];
    }
    if (sum != recv_data[uartrecvcounter-1]){
        return;         
    }

    X_Ntag_Emul();
    return;
}
void __not_in_flash_func(uart1ISR)()
{
    uint16_t ot;
    //uint16_t i;
    ot = 0;
    while(1)
    {
        if (uart_is_readable(UART_ID)){
            recv_data[uartrecvcounter] = uart_getc(UART_ID);
            uartrecvcounter ++;
            ot = 0;
        }
        else ot ++;

        if (ot > 45) break;  //about 0.82us*2
    }
    X_RecvHandle();
    /*
    printf("recv %d\n", uartrecvcounter);
    for(i=0;i<uartrecvcounter;i++) printf("%02x", recv_data[i]);
    printf("\n");
    */
    uartrecvcounter = 0;
}
void X_InitUart1()
{
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart
    uart_set_hw_flow(UART_ID, false, false);  //turn off flow control
    uart_set_fifo_enabled(UART_ID, true);
    hw_write_masked(&uart_get_hw(UART_ID)->ifls, \
                    0b100 << UART_UARTIFLS_RXIFLSEL_LSB, \
                    UART_UARTIFLS_RXIFLSEL_BITS);  // trigger condition: fifo not empty
    irq_set_exclusive_handler(UART1_IRQ, uart1ISR);
    irq_set_enabled(UART1_IRQ, true);  
    uart_set_irq_enables(UART_ID, true, false);
    irq_set_priority(UART1_IRQ, 1);

    uart_is_enabled(UART_ID);
}


int main()
{
    vreg_set_voltage(VREG_VOLTAGE_1_30);  //PS-20241120
    sleep_ms(5);  //delay for change pll to overclock
    
    // Set the system frequency to 133 MHz. vco_calc.py from the SDK tells us
    // this is exactly attainable at the PLL from a 12 MHz crystal: FBDIV =
    // 133 (so VCO of 1596 MHz), PD1 = 6, PD2 = 2. This function will set the
    // system PLL to 133 MHz and set the clk_sys divisor to 1.
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // The previous line automatically detached clk_peri from clk_sys, and
    // attached it to pll_usb, so that clk_peri won't be disturbed by future
    // changes to system clock or system PLL. If we need higher clk_peri
    // frequencies, we can attach clk_peri directly back to system PLL (no
    // divider available) and then use the clk_sys divider to scale clk_sys
    // independently of clk_peri.
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
    
    stdio_init_all();

    // Timer example code - This example fires off the callback after 2000ms
    //add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    // Watchdog example code
    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
        // Whatever action you may take if a watchdog caused a reboot
    }
    
    // Enable the watchdog, requiring the watchdog to be updated every 100ms or the chip will reboot
    // second arg is pause on debug which means the watchdog will pause when stepping through code
    //watchdog_enable(100, 1);
    //watchdog_enable(500, 1);
    
    // You need to call this function at least more often than the 100ms in the enable call to prevent a reboot
    //watchdog_update();

    printf("System Clock Frequency is %d Hz\n", clock_get_hz(clk_sys));
    printf("USB Clock Frequency is %d Hz\n", clock_get_hz(clk_usb));
    // For more examples of clocks use see https://github.com/raspberrypi/pico-examples/tree/master/clocks
    X_InitIO();
    X_InitUart1();

    while (true) {
        //printf("Hello, world!\n");

        // Send out a string, with CR/LF conversions
        //uart_puts(UART_ID, " Hello, UART!\n");  //uart_write_blocking

        //sleep_ms(200);
        //watchdog_update();
    }
}
