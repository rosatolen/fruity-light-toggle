#include <stdint.h>

#include "nrf.h"
#include "pn532.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

void uart_put_char(uint8_t cr)
{

    NRF_UART0->TXD = (uint8_t)cr;


    while (NRF_UART0->EVENTS_TXDRDY != 1)
    {
        // Wait for TXD data to be sent
        // should setup error feedback sending char fails :(
    }

    NRF_UART0->EVENTS_TXDRDY = 0;
}

uint8_t uart_get(void)
{
    while (NRF_UART0->EVENTS_RXDRDY != 1)
    {
        // Wait for RXD data to be received
    }

    NRF_UART0->EVENTS_RXDRDY = 0;
    return (uint8_t)NRF_UART0->RXD;
}


void wakeup() {
        uart_put_char('\x55');uart_put_char('\x55');
        
        uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');
        uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');
        uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');
        uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\x00');

        uart_put_char('\xFF');uart_put_char('\x03');uart_put_char('\xFD');uart_put_char('\xD4');
        uart_put_char('\x14');uart_put_char('\x01');uart_put_char('\x17');uart_put_char('\x00');

        get_ack();
}

void wait_for_number_of_response_bytes(int number) {
    int i = 0;
    while (i < number) {
        uart_get();
        i++;
    }
}

void get_ack() {
    wait_for_number_of_response_bytes(6);
}

void send_preamble_and_start() {
    uart_put_char('\x00');uart_put_char('\x00');uart_put_char('\xFF');
}

void send_direction() {
    uart_put_char('\xD4');
}

void send_postamble() {
    uart_put_char('\x00');
}

void poll() {
    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    send_direction();
    uart_put_char('\x12');
    uart_put_char('\x14');  // set parameter command
    uart_put_char('\x06');
    send_postamble();

    get_ack();

    uart_get(); 
    uart_get(); 
    uart_get();
    uart_get();
    uart_get();
    uart_get();
    uart_get();
    uart_get();
    uart_get();

    send_preamble_and_start();
    uart_put_char('\x0C');uart_put_char('\xF4');
    send_direction();
    uart_put_char('\x06'); // read register
    
    uart_put_char('\x63');uart_put_char('\x02');
    uart_put_char('\x63');uart_put_char('\x03');
    uart_put_char('\x63');uart_put_char('\x0D');
    uart_put_char('\x63');uart_put_char('\x38');
    uart_put_char('\x63');uart_put_char('\x3D');
    uart_put_char('\xB0');
    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(14);

    send_preamble_and_start();
    uart_put_char('\x08');uart_put_char('\xF8');
    send_direction();
    uart_put_char('\x08'); // write register
    
    uart_put_char('\x63');
    uart_put_char('\x02');
    uart_put_char('\x80');
    uart_put_char('\x63');
    uart_put_char('\x03');
    uart_put_char('\x80');
    uart_put_char('\x59');
    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x04');
    uart_put_char('\xFC');
    send_direction();
    uart_put_char('\x32'); // rfconfiguration
    uart_put_char('\x01');
    uart_put_char('\x00');
    uart_put_char('\xF9');
    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x04');
    uart_put_char('\xFC');
    send_direction();
    uart_put_char('\x32'); // rfconfiguration
    uart_put_char('\x01');
    uart_put_char('\x01');
    uart_put_char('\xF8');

    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x06');
    uart_put_char('\xFA');
    send_direction();
    uart_put_char('\x32'); // rfconfiguration
    uart_put_char('\x05');
    uart_put_char('\xFF');
    uart_put_char('\xFF');
    uart_put_char('\xFF');
    uart_put_char('\xF8');

    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x0E');
    uart_put_char('\xF2');
    send_direction();
    uart_put_char('\x06'); // read register
    uart_put_char('\x63');
    uart_put_char('\x02');
    uart_put_char('\x63');
    uart_put_char('\x03');
    uart_put_char('\x63');
    uart_put_char('\x05');
    uart_put_char('\x63');
    uart_put_char('\x38');
    uart_put_char('\x63');
    uart_put_char('\x3C');
    uart_put_char('\x63');
    uart_put_char('\x3D');
    uart_put_char('\x19');

    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(15);

    send_preamble_and_start();
    uart_put_char('\x08');
    uart_put_char('\xF8');
    send_direction();
    uart_put_char('\x08'); // write register
    uart_put_char('\x63');
    uart_put_char('\x05');
    uart_put_char('\x40');
    uart_put_char('\x63');
    uart_put_char('\x3C');
    uart_put_char('\x10');
    uart_put_char('\xCD');

    send_postamble();

    get_ack();

    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x0A');
    uart_put_char('\xF6');
    send_direction();
    uart_put_char('\x60'); // "inAutoPoll" command
    uart_put_char('\x14'); // poll # ; \xFF is endless polling
    uart_put_char('\x02'); // polling period in units of 150ms

    /*  types of nfc to read */
    uart_put_char('\x20'); // indicates mandatory target type to be polled at the 1st time; \x20 is Passive 106 kbps ISO/IEC
    uart_put_char('\x10');
    uart_put_char('\x03');
    uart_put_char('\x11');
    uart_put_char('\x12');
    uart_put_char('\x04');
    /* *** */

    uart_put_char('\x5C');

    send_postamble();

    get_ack();

    // wait for nfc tag
    wait_for_number_of_response_bytes(24);
    // libnfc will...
    // InDataExchange command \x40 (21 times... but why that many?!)
    // InRelease command \x52
    // RFconfiguration command \x32
    // PowerdDown command \x16

    // elechouse library will guessTagType
    // CHECK WE ALWAYS USE MIFARE ULTRALIGHT
    // then create a mifareultralight object
    // then call ultralight.read and return a nfctag object (made of the uidlength, tag type, and ndefmessage)
    // nfctag.print will call ndefmessage.print
    // ndefmessage.print will

    // if "qconsf.com/?id=" is in the payload, then the message was read correctly, if not then you should show user feedback to try again


}

void powerdown() {
    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    uart_put_char('\xD4');
    uart_put_char('\x16');
    uart_put_char('\xF0');
    uart_put_char('\x26');
    uart_put_char('\x00');
}

void uart_115200_config(uint8_t rts_pin_number,
                        uint8_t txd_pin_number,
                        uint8_t cts_pin_number,
                        uint8_t rxd_pin_number) {
    nrf_gpio_cfg_output(txd_pin_number);
    nrf_gpio_cfg_input(rxd_pin_number, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD = txd_pin_number;
    NRF_UART0->PSELRXD = rxd_pin_number;
    NRF_UART0->BAUDRATE      = (UART_BAUDRATE_BAUDRATE_Baud115200);
    NRF_UART0->ENABLE        = (UART_ENABLE_ENABLE_Enabled);
    NRF_UART0->TASKS_STARTTX = 1;
    NRF_UART0->TASKS_STARTRX = 1;
    NRF_UART0->EVENTS_RXDRDY = 0;
}
