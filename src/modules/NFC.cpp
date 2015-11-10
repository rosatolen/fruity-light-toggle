#include <NFC.h>
#include <string>
#include <types.h>

extern "C" {
#include "stdlib.h"
#include "nrf_delay.h"
#include "pn532.h"
#include "simple_uart.h"
}

int error = -1;

bool id_exists_in_response(uint8_t *response, int response_length) {
    std::string response_string(response, response+response_length);

    if (response_string.find("id=") == std::string::npos) {
        return false;
    } else {
        return true;
    }
}

short get_id(uint8_t *packet, int packet_length) {
    std::string response_string;
    int checksum_and_postamble_size = 3;
    for (int i=0; i< packet_length - checksum_and_postamble_size; ++i) {
        if (isprint(packet[i])) response_string = response_string + static_cast<char>(packet[i]);
    }

    std::size_t found = response_string.find("=");
    std::string id = response_string.substr(found+1, 4);

    //CHECK NUMERIC LIMITS WITH std::numeric_limits<short>::lowest() std::numeric_limits<short>::max()

    return (short) atoi(id.c_str());
}

void get_number_of_bytes(uint8_t *storage, int number_of_bytes) {
    int i=0;
    while(i < number_of_bytes) {
        storage[i] = uart_get();
        i++;
    }
}

void NFC::dataExchange1() {
    in_data_exchange('\x00', '\xBB');
    gobble_number_of_bytes(26);
}

void NFC::dataExchange2() {
    in_data_exchange('\x04', '\xB7');
    gobble_number_of_bytes(26);
}

short NFC::dataExchange3() {
    in_data_exchange('\x08', '\xB3');
    short attendeeId = 0;
    uint8_t response[26] = {0};
    get_number_of_bytes(response, 26);
    if (id_exists_in_response(response, 26)) {
        return get_id(response, 26);
    } else {
        return -1;
    }
}

void NFC::dataExchange4() {
    in_data_exchange('\x0C', '\xAF');
    gobble_number_of_bytes(26);
}

bool NFC::inListPassiveTarget() {
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

    return tag_is_present();
}

short NFC::GetAttendeeId() {
    NFC::pn532_wakeup();
    set_parameter_command();

    short attendeeId = inListPassiveTarget();

    tag_is_present();

    powerdown();
    if (attendeeId == error) {
        return error;
    } else {
        return attendeeId;
    }

    return error;
}

bool is_ack(char *response, int response_length) {
    if (response_length != 6) return false;

    uint8_t ack[6] = {'\x00', '\x00', '\xFF', '\x00', '\xFF', '\x00'};
    int result = memcmp(ack, response, 6);
    if (result == 0) {
        return true;
    } else {
        return false;
    }
}

void NFC::CheckPn532Response() {
    static char input[6] = {0};

    if (simple_uart_get_with_timeout(0, (uint8_t*) input)) {
        ReadNFCResponse(input, 6, 1);

        if (is_ack(input, 6)) {
            //proceed with gobbling
            // does this return a bool for when it failed?
            Pn532InterruptListener(input);

        } else {
            // error state :( start over
        }
    }
}

void NFC::ReadNFCResponse(char *storage, uint8_t storageSize, uint8_t offset) {
    uint8_t byteBuffer;
    uint8_t counter = offset;

    while(true) {
        byteBuffer = simple_uart_get();

        if (counter == storageSize) {
            storage[counter] = '\0';
        } else {
            memcpy(storage + counter, &byteBuffer, sizeof(uint8_t));
        }
        counter ++;
    }
}

void NFC::Pn532InterruptListener(char *pn532response) {

}
