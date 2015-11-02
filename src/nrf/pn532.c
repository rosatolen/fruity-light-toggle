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
    nrf_delay_us(1000);
}

void wait_for_number_of_response_bytes(int number) {
    int i = 0;
    while (i < number) {
        uart_get();
        i++;
    }
    nrf_delay_us(1000);
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

// void in_data_exchange(uint8_t len, uint8_t lcs, uint8_t cmd, ) {
//     send_preamble_and_start();
//     uart_put_char('\x05');
//     uart_put_char('\xFB');
//     send_direction();
//     uart_put_char('\x40'); // start data exchange command
//     uart_put_char('\x01');
//     uart_put_char('\x30'); // read 16 bytes
//     uart_put_char('\x00'); // from address 0
//     uart_put_char('\xBB');
//     send_postamble();

//     get_ack();
//     nrf_delay_us(1000);
//     wait_for_number_of_response_bytes(26);
//     //wait_for_number_of_response_bytes
//     // 00 00 FF - 13 ED - D5 41 00 04 A0 35 19 B2 BC 2B 80 A5 48 00 00 E1 10 12 00 EF 00
// }

void in_list_passive_target() {
    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    send_direction();
    uart_put_char('\x12'); // set parameter command
    uart_put_char('\x14');  
    uart_put_char('\x06');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(9);




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
    uart_put_char('\x63');uart_put_char('\x02');uart_put_char('\x80');
    uart_put_char('\x63');uart_put_char('\x03');uart_put_char('\x80');
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
    //nrf_delay_us(1000);




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
    //nrf_delay_us(1000);



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
    nrf_delay_us(1000);

    /* DEVIATION FROM POLLING */

    send_preamble_and_start();
    uart_put_char('\x06');
    uart_put_char('\xFA');
    send_direction();
    uart_put_char('\x32'); // rf config
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
    uart_put_char('\x63');uart_put_char('\x02');
    uart_put_char('\x63');uart_put_char('\x03');
    uart_put_char('\x63');uart_put_char('\x05');
    uart_put_char('\x63');uart_put_char('\x38');
    uart_put_char('\x63');uart_put_char('\x3C');
    uart_put_char('\x63');uart_put_char('\x3D');
    uart_put_char('\x19');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(15);

    send_preamble_and_start();
    uart_put_char('\x08');
    uart_put_char('\xF8');
    send_direction();
    uart_put_char('\x08'); // write register
    uart_put_char('\x63');uart_put_char('\x05');uart_put_char('\x40');
    uart_put_char('\x63');uart_put_char('\x3C');uart_put_char('\x10');
    uart_put_char('\xCD');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x06');
    uart_put_char('\xFA');
    send_direction();
    uart_put_char('\x32'); // rf configuration
    uart_put_char('\x05');
    uart_put_char('\x00');
    uart_put_char('\x01');
    uart_put_char('\x02');
    uart_put_char('\xF2');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x04');
    uart_put_char('\xFC');
    send_direction();
    uart_put_char('\x4A'); // inlist passive target
    uart_put_char('\x01');
    uart_put_char('\x00');
    uart_put_char('\xE1');
    send_postamble();
    get_ack();

    // waits for nfc tag for .2 seconds and if not there it responds with "4B 00 00"
    // get each piece of the response
    // 00 00 FF - 0F F1 - D5 4B - 01 01 00 44 00 07 04 A0 35 B3 BC 2B 80 A1 - 00
    wait_for_number_of_response_bytes(22);

    /* ** GET PAYLOADS ** */
    send_preamble_and_start();
    uart_put_char('\x05');
    uart_put_char('\xFB');
    send_direction();
    uart_put_char('\x40'); // start data exchange command
    uart_put_char('\x01');
    uart_put_char('\x30'); // read 16 bytes
    uart_put_char('\x00'); // from address 0
    uart_put_char('\xBB');
    send_postamble();
    get_ack();
    nrf_delay_us(1000);
    wait_for_number_of_response_bytes(26);
    //wait_for_number_of_response_bytes
    // 00 00 FF - 13 ED - D5 41 00 04 A0 35 19 B2 BC 2B 80 A5 48 00 00 E1 10 12 00 EF 00

    send_preamble_and_start();
    uart_put_char('\x05');
    uart_put_char('\xFB');
    send_direction();
    uart_put_char('\x40'); // data exchange command
    uart_put_char('\x01');
    uart_put_char('\x30'); // read 16 bytes
    uart_put_char('\x04'); // from address 4
    uart_put_char('\xB7');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(26);
    //wait_for_number_of_response_bytes
                                //  (1  3  160  16  D 3  24 209 20 U  4)  q c  o  n  
    // 00 00 FF - 13 ED - D5 41 00 01 03 A0 10 44 03 18 D1 01 14 55 04 71 63 6F 6E E7 00

    send_preamble_and_start();
    uart_put_char('\x05');
    uart_put_char('\xFB');
    send_direction();
    uart_put_char('\x40'); // data exchange command
    uart_put_char('\x01');
    uart_put_char('\x30'); // read 16 bytes
    uart_put_char('\x08'); // from address 8
    uart_put_char('\xB3');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(26);
    //wait_for_number_of_response_bytes
                                //  s  f  .  c  o  m  /  ?  i  d  =  3  5  6  6 (254)
    // 00 00 FF - 13 ED - D5 41 00 73 66 2E 63 6F 6D 2F 3F 69 64 3D 33 35 36 36 FE 5A 00

    send_preamble_and_start();
    uart_put_char('\x05');
    uart_put_char('\xFB');
    send_direction();
    uart_put_char('\x40'); // data exchange command
    uart_put_char('\x01');
    uart_put_char('\x30'); // read 16 bytes
    uart_put_char('\x0C'); // from address 12
    uart_put_char('\xAF');
    send_postamble();
    get_ack();   
    wait_for_number_of_response_bytes(26);
    //wait_for_number_of_response_byte
    // 00 00 FF - 13 ED - D5 41 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 EA 00


    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    send_direction();
    uart_put_char('\x52'); // in release
    uart_put_char('\x00');
    uart_put_char('\xDA');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(10);

    send_preamble_and_start();
    uart_put_char('\x04');
    uart_put_char('\xFC');
    send_direction();
    uart_put_char('\x32'); // rf config
    uart_put_char('\x01');
    uart_put_char('\x00');
    uart_put_char('\xF9');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    send_direction();
    uart_put_char('\x16'); // powerdown
    uart_put_char('\xF0');
    uart_put_char('\x26');
    send_postamble();
    get_ack();
    wait_for_number_of_response_bytes(10);
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
    uart_put_char('\x63');uart_put_char('\x02');uart_put_char('\x80');
    uart_put_char('\x63');uart_put_char('\x03');uart_put_char('\x80');
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
    // indicates mandatory target type to be polled at the 1st time;
    uart_put_char('\x20'); // \x20 is Passive 106 kbps ISO/IEC
    uart_put_char('\x10'); // Mifare Card
    uart_put_char('\x03'); // Passive ISO/IEC14443-4B
    uart_put_char('\x11'); // \x10 is FeliCa 212 kbps
    uart_put_char('\x12'); // \x100 is FeliCa 424 kbps
    uart_put_char('\x04'); // Innovision Jewel Tag
    /* ********* */

    uart_put_char('\x5C');

    send_postamble();

    get_ack();

    // wait for nfc tag target data
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
    // ndefmessage.print will print each of the records

    /* use target data in data exchange */
    send_preamble_and_start();
    // ??
    // ??
    send_direction();
    uart_put_char('\x40'); // inDataExchange command
    // tg info (logical number of target and more information bit 6 (only for TPE targets))
    // addr of mifare card
    // data from host to pn532 (max 16) either for writing or for authentication
    // data out for mifare:
  //  cmd addr data 0..15
//    cmd = '\xA0' // do i need auth? A0 cmd is 16 bytes read

    
    // grab the payload in 7th byte section
    // length of payload is in 3rd byte

    // check that mifareultralight tag exists then valid message
    // do we expect a "well known type" or a "uri type"? if uri type, check the URI Identifier code here: https://learn.adafruit.com/adafruit-pn532-rfid-nfc/ndef 
    // if "qconsf.com/?id=" is in the payload, then the message was read correctly, if not then you should show user feedback to try again

}

void powerdown () {
    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    uart_put_char('\xD4');
    uart_put_char('\x16');
    uart_put_char('\xF0');
    uart_put_char('\x26');
    send_postamble();
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
