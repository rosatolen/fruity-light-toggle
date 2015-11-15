#include <string>
#include <sstream>

#include <assert.h>
#include <iostream>

extern "C" {
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
}

typedef unsigned char uint8_t;

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
    for (int i=0; i< packet_length; ++i) {
        if (isprint(packet[i])) response_string = response_string + static_cast<char>(packet[i]);
    }

    std::size_t found = response_string.find("=");
    std::string id = response_string.substr(found+1, 4);

    std::cout << id;
    std::cout << '\n';
    //CHECK NUMERIC LIMITS WITH std::numeric_limits<short>::lowest() std::numeric_limits<short>::max()
    short number = atoi(id.c_str());

    std::cout << atoi(id.c_str());
    std::cout << '\n';
    return (short) atoi(id.c_str());
}

bool is_ack(uint8_t *response, int response_length) {
    if (response_length != 6) return false;

    uint8_t ack[6] = {'\x00', '\x00', '\xFF', '\x00', '\xFF', '\x00'};
    int result = memcmp(ack, response, 6);
    if (result == 0) {
        return true;
    } else {
        return false;
    }
}

int main() {
    uint8_t response[28] =
    {'\x00', '\x00', '\xFF', '\x13',
    '\xED', '\xD5', '\xD5', '\x41',
    '\x00', '\x73', '\x66', '\x2E',
    '\x63', '\x6F', '\x6D', '\x2F',
    '\x3F', '\x69', '\x64', '\x3D',
    '\x33', '\x35', '\x36', '\x36',
    '\xFE'};

    uint8_t packet_with_id_300[17] =
    {'\x00', '\x73', '\x66', '\x2E',
    '\x63', '\x6F', '\x6D', '\x2F',
    '\x3F', '\x69', '\x64', '\x3D',
    '\x33', '\x30', '\x30', '\xFE',
    '\xFE'};

    uint8_t packet_with_id_1000[17] =
    {'\x00', '\x73', '\x66', '\x2E',
    '\x63', '\x6F', '\x6D', '\x2F',
    '\x3F', '\x69', '\x64', '\x3D',
    '\x31', '\x30', '\x30', '\x30',
    '\xFE'};

    uint8_t packet_with_id_1500[17] =
    {'\x00', '\x73', '\x66', '\x2E',
    '\x63', '\x6F', '\x6D', '\x2F',
    '\x3F', '\x69', '\x64', '\x3D',
    '\x31', '\x35', '\x30', '\x30',
    '\xFE'};

    uint8_t packet_with_id_9000[17] =
    {'\x00', '\x73', '\x66', '\x2E',
    '\x63', '\x6F', '\x6D', '\x2F',
    '\x3F', '\x69', '\x64', '\x3D',
    '\x39', '\x30', '\x30', '\x30',
    '\xFE'};

    uint8_t nfc_tag_data_dump[51] =
    {
        '\x00', '\x04', '\x5C', '\x35',
        '\xE5', '\xAA', '\xBC', '\x2B',
        '\x80', '\xBD', '\x48', '\x00',
        '\x00', '\xE1', '\x10', '\x12',
        '\x00',

        '\x00', '\x01', '\x03', '\xA0',
        '\x10', '\x44', '\x03', '\x18',
        '\xD1', '\x02', '\x14', '\x55',
        '\x04', '\x71', '\x63', '\x6F',
        '\x6E',

        '\x00', '\x73', '\x66', '\x2E',
        '\x63', '\x6F', '\x6D', '\x2F',
        '\x3F', '\x69', '\x64', '\x3D',
        '\x33', '\x35', '\x36', '\x35',
        '\xFE'
    };

    uint8_t ack[6] = {'\x00', '\x00', '\xFF', '\x00', '\xFF', '\x00'};
    uint8_t notack[6] = {'\xFF', '\x00', '\xFF', '\x00', '\xFF', '\x00'};
    uint8_t long_response[7] = {'\xFF', '\x00', '\xFF', '\x00', '\xFF', '\x00', '\xFF'};
    uint8_t short_response[2] = {'\x00', '\x00'};

    assert(id_exists_in_response(response, 28));
    assert((short) 3566 == get_id(response, 28));
    assert(!is_ack(short_response, 2));
    assert(is_ack(ack, 6));
    assert(!is_ack(notack, 6));
    assert(!is_ack(long_response, 7));
    assert(id_exists_in_response(nfc_tag_data_dump, 51));
    assert((short) 3565 == get_id(nfc_tag_data_dump, 51));
    assert((short) 3565 == get_id(nfc_tag_data_dump, 51));

    /* NUMBER LIMITS */
    assert((short) 300 == get_id(packet_with_id_300, 17));
    assert((short) 1000 == get_id(packet_with_id_1000, 17));
    assert((short) 1500 == get_id(packet_with_id_1500, 17));
    assert((short) 9000 == get_id(packet_with_id_9000, 17));

    printf("Tests succeeded!\n");
}
