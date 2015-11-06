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

bool uart_get_with_timeout()
{
    int32_t timeout = 1;
    bool succeeded = true;
    while (NRF_UART0->EVENTS_RXDRDY != 1)
    {
        // Wait for RXD data to be received
        if (timeout-- >= 0) {
            nrf_delay_us(100);
        } else {
            succeeded = false;
            break;
        }
    }

    NRF_UART0->EVENTS_RXDRDY = 0;
    uint8_t *rx_data = (uint8_t)NRF_UART0->RXD;

    return succeeded;
}

bool gobble_number_of_bytes_with_timeout(int number) {
    int i = 0;
    while (i < number) {
        if (uart_get_with_timeout() == false) return false;
        i++;
    }
    return true;
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

void gobble_number_of_bytes(int number) {
    int i = 0;
    while (i < number) {
        uart_get();
        i++;
    }
    nrf_delay_us(1000);
}

bool tag_is_present() {
    uart_get(); // get 00
    uart_get(); // get 00
    uart_get(); // get FF
    uart_get(); // get LEN
    uart_get(); // get LCS
    uart_get(); // get DIR
    int i = 0;
    while (i < 8) {
        if(uart_get() == '\x4B') {
            if(uart_get() =='\x00') {
                return false; 
            } else {
                if (gobble_number_of_bytes_with_timeout(14) == false) return false;
                return true;
            }
            i++;
        }
        i++;
    }

    return false;
} 

uint8_t hex_to_decimal(uint8_t hex) {
    uint8_t hex_map_to_i[9] = { '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39' };
    for (int i=0; i < 9; i++) {
        if (hex_map_to_i[i] == hex) return i;
    }

    // Pretty on the inside RETURN
    return -1;
}

unsigned short get_attendee_id() {
    uint8_t attendeeId[4] = {0};
    int i = 0;
    while (i < 4) {
        attendeeId[i] = hex_to_decimal(uart_get());
        i++;
    }

    return 1000 * attendeeId[0] + 100 * attendeeId[1] + 10 * attendeeId[2] + attendeeId[3];
}

unsigned short find_attendee_id() {
    int i = 0;
    short attendeeId = 0;
    uint8_t success_response[16] = {'\x13', '\xED', '\xD5', '\x41', '\x00', 's', 'f', '.', 'c', 'o', 'm', '/', '?', 'i', 'd', '='};
    uart_get(); // nom \x00
    uart_get(); // nom \x00
    uart_get(); // nom \xFF
    if(uart_get() == success_response[i]) {
        i++;
        while(i < 16) {
            if(uart_get() != success_response[i]) {
                break;
             }
             i++;
        }
        if(i > 15) {
            attendeeId = get_attendee_id();
        }
    }
    uart_get(); // nom \xFE
    uart_get(); // nom \x5b
    uart_get(); // nom \00
    return attendeeId;
}

void get_ack() {
    gobble_number_of_bytes(6);
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

const uint8_t LENGTH = '\x05';
const uint8_t LENGTH_CHECK_SUM = '\xFB';
const uint8_t COMMAND = '\x40';
const uint8_t LOGICAL_NUMBER = '\x01';
const uint8_t MIFARE_COMMAND = '\x30';

void in_data_exchange(uint8_t start_address, uint8_t dcs) {
    send_preamble_and_start();
    uart_put_char(LENGTH);
    uart_put_char(LENGTH_CHECK_SUM);
    send_direction();
    uart_put_char(COMMAND);
    uart_put_char(LOGICAL_NUMBER);
    uart_put_char(MIFARE_COMMAND);
    uart_put_char(start_address);
    uart_put_char(dcs);
    send_postamble();

    get_ack();
    nrf_delay_us(1000);
}

unsigned short in_list_passive_target() {
    send_preamble_and_start();
    uart_put_char('\x03');
    uart_put_char('\xFD');
    send_direction();
    uart_put_char('\x12'); // set parameter command
    uart_put_char('\x14');  
    uart_put_char('\x06');
    send_postamble();
    get_ack();
    gobble_number_of_bytes(9);


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
    gobble_number_of_bytes(14);

    send_preamble_and_start();
    uart_put_char('\x08');uart_put_char('\xF8');
    send_direction();
    uart_put_char('\x08'); // write register
    uart_put_char('\x63');uart_put_char('\x02');uart_put_char('\x80');
    uart_put_char('\x63');uart_put_char('\x03');uart_put_char('\x80');
    uart_put_char('\x59');
    send_postamble();
    get_ack();
    gobble_number_of_bytes(9);


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
    gobble_number_of_bytes(9);

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
    gobble_number_of_bytes(9);

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
    gobble_number_of_bytes(9);
    nrf_delay_us(1000);

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
    gobble_number_of_bytes(9);


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
    gobble_number_of_bytes(15);

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
    gobble_number_of_bytes(9);

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
    gobble_number_of_bytes(9);

    send_preamble_and_start();
    uart_put_char('\x04');
    uart_put_char('\xFC');
    send_direction();
    uart_put_char('\x4A'); // inlistpassive target
    uart_put_char('\x01');
    uart_put_char('\x00');
    uart_put_char('\xE1');
    send_postamble();
    get_ack();

    if(!tag_is_present()) {
        powerdown();
        return 0;
    }

    /* ** GET PAID ** */
    in_data_exchange('\x00', '\xBB');
    gobble_number_of_bytes(26);
    //gobble_number_of_bytes
    // 00 00 FF - 13 ED - D5 41 00 04 A0 35 19 B2 BC 2B 80 A5 48 00 00 E1 10 12 00 EF 00
    in_data_exchange('\x04', '\xB7');
    gobble_number_of_bytes(26);
    //gobble_number_of_bytes
                                //  (1  3  160  16  D 3  24 209 20 U  4)  q c  o  n  
    // 00 00 FF - 13 ED - D5 41 00 01 03 A0 10 44 03 18 D1 01 14 55 04 71 63 6F 6E E7 00
    in_data_exchange('\x08', '\xB3');

    short attendeeId = 0;
    attendeeId = find_attendee_id();
    if (attendeeId == 0) return 0;
    //gobble_number_of_bytes
                                //  s  f  .  c  o  m  /  ?  i  d  =  3  5  6  6 (254)
    // 00 00 FF - 13 ED - D5 41 00 73 66 2E 63 6F 6D 2F 3F 69 64 3D 33 35 36 36 FE 5A 00
    in_data_exchange('\x0C', '\xAF');
    gobble_number_of_bytes(26);
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
    gobble_number_of_bytes(10);

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
    gobble_number_of_bytes(9);

    powerdown();

    return attendeeId;
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
