#include <stdint.h>

#include "nrf.h"
#include "pn532.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "app_util_platform.h"
#include "pca10028.h"

#define UART_BUF_SIZE 10

uint8_t current_rx_byte;

typedef void (* app_uart_event_handler_t) (uart_event_t * p_app_uart_event);

static app_uart_event_handler_t m_event_handler;

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
    simple_uart_put('\x55');simple_uart_put('\x55');
    simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');
    simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');
    simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');
    simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\x00');
    simple_uart_put('\xFF');simple_uart_put('\x03');simple_uart_put('\xFD');simple_uart_put('\xD4');
    simple_uart_put('\x14');simple_uart_put('\x01');simple_uart_put('\x17');simple_uart_put('\x00');
    //get_ack();
}

void set_parameter() {
    send_preamble_and_start();
    simple_uart_put('\x03');
    simple_uart_put('\xFD');
    send_direction();
    simple_uart_put('\x12'); // set parameter command
    simple_uart_put('\x14');
    simple_uart_put('\x06');
    send_postamble();
    //get_ack();
    //gobble_number_of_bytes(9);
}

void write_80() {
    send_preamble_and_start();
    simple_uart_put('\x08');simple_uart_put('\xF8');
    send_direction();
    simple_uart_put('\x08'); // write register
    simple_uart_put('\x63');simple_uart_put('\x02');simple_uart_put('\x80');
    simple_uart_put('\x63');simple_uart_put('\x03');simple_uart_put('\x80');
    simple_uart_put('\x59');
    send_postamble();
    //get_ack();
    //gobble_number_of_bytes(9);
}

void config_rf() {
    send_preamble_and_start();
    simple_uart_put('\x04');
    simple_uart_put('\xFC');
    send_direction();
    simple_uart_put('\x32'); // rfconfiguration
    simple_uart_put('\x01');
    simple_uart_put('\x01');
    simple_uart_put('\xF8');
    send_postamble();
    //get_ack();
    //gobble_number_of_bytes(9);
}

void write_40_10() {
    send_preamble_and_start();
    simple_uart_put('\x08');
    simple_uart_put('\xF8');
    send_direction();
    simple_uart_put('\x08'); // write register
    simple_uart_put('\x63');simple_uart_put('\x05');simple_uart_put('\x40');
    simple_uart_put('\x63');simple_uart_put('\x3C');simple_uart_put('\x10');
    simple_uart_put('\xCD');
    send_postamble();
    //get_ack();
    //gobble_number_of_bytes(9);
}

void config_rf_max() {
    send_preamble_and_start();
    simple_uart_put('\x06');
    simple_uart_put('\xFA');
    send_direction();
    simple_uart_put('\x32'); // rf configuration
    simple_uart_put('\x05');
    simple_uart_put('\x00');
    simple_uart_put('\x01');
    simple_uart_put('\x02');
    simple_uart_put('\xF2');
    send_postamble();
    //get_ack();
    //gobble_number_of_bytes(9);
}


void gobble_number_of_bytes(int number) {
    int i = 0;
    while (i < number) {
        uart_get();
        i++;
    }
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

uint8_t hex_to_char(uint8_t hex) {
    uint8_t hex_map_to_i[10] = { '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39' };
    for (int i=0; i < 10; i++) {
        if (hex_map_to_i[i] == hex) return i;
    }

    // Pretty on the inside RETURN
    return -1;
}
int hex_to_dec(uint8_t hex) {
    uint8_t hex_map_to_dec[51] = {'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0A', '\x0B', '\x0C', '\x0D', '\x0E', '\x0F', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1A', '\x1B', '\x1C', '\x1D', '\x1E', '\x1F', '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2A', '\x2B', '\x2C', '\x2D', '\x2E', '\x2F', '\x30', '\x31', '\x32'};
    for (int i=0; i < 51; i++) {
        if(hex_map_to_dec[i] == hex) return i;
    }
    return -1;
}

unsigned short get_attendee_id(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    uint8_t attendeeId[4] = {0};
    attendeeId[0] = hex_to_char(a);
    attendeeId[1] = hex_to_char(b);
    attendeeId[2] = hex_to_char(c);
    attendeeId[3] = hex_to_char(d);

    return 1000 * attendeeId[0] + 100 * attendeeId[1] + 10 * attendeeId[2] + attendeeId[3];
}

int get_packet_length_from_preamble() {
    uart_get(); // nom \x00
    uart_get(); // nom \x00
    uart_get(); // nom \xFF
    return  hex_to_dec(uart_get());
}

unsigned short find_attendee_id() {
    short attendeeId = 0;
    int packet_length = 0;
    uint8_t success_response[11] = {'s', 'f', '.', 'c', 'o', 'm', '/', '?', 'i', 'd', '='};
    packet_length = get_packet_length_from_preamble();

    uint8_t packet[packet_length];

    for (int i = 0; i < packet_length; i++ ){
        packet[i] = uart_get();
    }

    int found_index = 0;
    for(int i = 0; i < packet_length; i++) {
        if(success_response[0] == packet[i]) found_index = i;
    }

    for(int i = 0; i < 10; i++) {
        if(success_response[i] != packet[found_index]) {
           return 0;
        }
        found_index++;
    }

    get_attendee_id(packet[found_index+1], packet[found_index+2], packet[found_index+3], packet[found_index+4]);

    // nom postamble
    gobble_number_of_bytes(3);
    return attendeeId;
}

void get_ack() {
    gobble_number_of_bytes(6);
}

void send_preamble_and_start() {
    simple_uart_put('\x00');simple_uart_put('\x00');simple_uart_put('\xFF');
}

void send_direction() {
    simple_uart_put('\xD4');
}

void send_postamble() {
    simple_uart_put('\x00');
}

const uint8_t LENGTH = '\x05';
const uint8_t LENGTH_CHECK_SUM = '\xFB';
const uint8_t COMMAND = '\x40';
const uint8_t LOGICAL_NUMBER = '\x01';
const uint8_t MIFARE_COMMAND = '\x30';

void in_data_exchange(uint8_t start_address, uint8_t dcs) {
    send_preamble_and_start();
    simple_uart_put(LENGTH);
    simple_uart_put(LENGTH_CHECK_SUM);
    send_direction();
    simple_uart_put(COMMAND);
    simple_uart_put(LOGICAL_NUMBER);
    simple_uart_put(MIFARE_COMMAND);
    simple_uart_put(start_address);
    simple_uart_put(dcs);
    send_postamble();

    get_ack();
    nrf_delay_us(1000);
}

unsigned short in_list_passive_target() {
    send_preamble_and_start();
    simple_uart_put('\x04');
    simple_uart_put('\xFC');
    send_direction();
    simple_uart_put('\x4A'); // inlistpassive target
    simple_uart_put('\x01');
    simple_uart_put('\x00');
    simple_uart_put('\xE1');
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

    return attendeeId;
}

void powerdown () {
    send_preamble_and_start();
    simple_uart_put('\x03');
    simple_uart_put('\xFD');
    simple_uart_put('\xD4');
    simple_uart_put('\x16');
    simple_uart_put('\xF0');
    simple_uart_put('\x26');
    send_postamble();
}

void UART0_IRQHandler(void)
{
    // Handle reception
    if ((NRF_UART0->EVENTS_RXDRDY != 0) && (NRF_UART0->INTENSET & UART_INTENSET_RXDRDY_Msk))
    {
        //tell the handler to change state?
        uart_event event;

        // Clear UART RX event flag
        NRF_UART0->EVENTS_RXDRDY  = 0;
        current_rx_byte                 = (uint8_t)NRF_UART0->RXD;

        m_event_handler(&event);
    }

    // Handle transmission.
    if ((NRF_UART0->EVENTS_TXDRDY != 0) && (NRF_UART0->INTENSET & UART_INTENSET_TXDRDY_Msk))
    {
        // Clear UART TX event flag.
        NRF_UART0->EVENTS_TXDRDY = 0;
        //change state? if so , call m_event_handler
    }

    // Handle errors.
    if ((NRF_UART0->EVENTS_ERROR != 0) && (NRF_UART0->INTENSET & UART_INTENSET_ERROR_Msk))
    {
        uint32_t       error_source;
        uart_event event;

        // Clear UART ERROR event flag.
        NRF_UART0->EVENTS_ERROR = 0;

        // Clear error source.
        error_source        = NRF_UART0->ERRORSRC;
        NRF_UART0->ERRORSRC = error_source;

        event.event_t = UART_ERROR;
        m_event_handler(&event);
    }
}

void uart_115200_config(uint8_t rts_pin_number,
                        uint8_t txd_pin_number,
                        uint8_t cts_pin_number,
                        uint8_t rxd_pin_number,
                        uart_event_handler event_handler) {
    m_event_handler = event_handler;

    //?? needed? nrf_gpio_pin_set(txd_pin_number);
    nrf_gpio_cfg_output(txd_pin_number);
    nrf_gpio_cfg_input(rxd_pin_number, NRF_GPIO_PIN_PULLUP);
    // old - nrf_gpio_cfg_input(rxd_pin_number, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD = txd_pin_number;
    NRF_UART0->PSELRXD = rxd_pin_number;
    NRF_UART0->BAUDRATE      = (UART_BAUDRATE_BAUDRATE_Baud115200);
    //NRF_UART0->ENABLE        = (UART_ENABLE_ENABLE_Enabled);

    // parity excluded
    //?? needed?
    NRF_UART0->CONFIG = (UART_CONFIG_PARITY_Excluded << UART_CONFIG_PARITY_Pos);

    // flow control disabled
    NRF_UART0->ENABLE        = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_TXDRDY = 0;
    //?? needed?
    NRF_UART0->CONFIG &= ~(UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
    NRF_UART0->TASKS_STARTTX = 1;
    NRF_UART0->TASKS_STARTRX = 1;

    // enable uart interrupt
    NRF_UART0->INTENCLR = 0xffffffffUL;
    NRF_UART0->INTENSET = (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
    //    WHY DOES THIS CAUSE HANGING?????
    //    | (UART_INTENSET_TXDRDY_Set << UART_INTENSET_TXDRDY_Pos) |
    //    (UART_INTENSET_ERROR_Set << UART_INTENSET_ERROR_Pos);

    NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_SetPriority(UART0_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_EnableIRQ(UART0_IRQn);
}
