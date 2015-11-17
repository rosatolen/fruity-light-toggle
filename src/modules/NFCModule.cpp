#include <NFCModule.h>

#include <Node.h>

extern "C" {
#include "app_util_platform.h"
#include "app_error.h"
#include "stdlib.h"
#include "pn532.h"
#include "simple_uart.h"
}

#define UART_TX_BUF_SIZE 1
#define UART_RX_BUF_SIZE 1

bool UART_CONFIGURED = false;
bool first_response_received = false;

NFCModule::NFCModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
    : Module(moduleID::NFC_MODULE_ID, node, cm, name, storageSlot) {
  _configuration.moduleId = moduleID::NFC_MODULE_ID;
  _configuration.moduleVersion = 1;
  _configuration.moduleActive = true;
  configurationPointer = &_configuration;
}

typedef enum {
    ACK_START,
    second_byte,
    third_byte,
    fourth_byte,
    fifth_byte,
    sixth_byte,
    ack_error,
    ack_processed
} ack_state;

ack_state current_ack_state = ACK_START;

void process_ack(uint8_t rx_byte) {
    Node *node = Node::getInstance();

    switch (current_ack_state)
    {
        case ACK_START:
            if (rx_byte == 0) current_ack_state = second_byte;
            else current_ack_state = ack_error;
        break;

        case second_byte:
            if (rx_byte == 0) current_ack_state = third_byte;
            else current_ack_state = ack_error;
        break;

        case third_byte:
            if (rx_byte == 255) current_ack_state = fourth_byte;
            else current_ack_state = ack_error;
        break;

        case fourth_byte:
            if (rx_byte == 0) current_ack_state = fifth_byte;
            else current_ack_state = ack_error;
        break;

        case fifth_byte:
            if (rx_byte == 255) current_ack_state = sixth_byte;
            else current_ack_state = ack_error;
        break;

        case sixth_byte:
            if (rx_byte == 0) current_ack_state = ack_processed;
            else current_ack_state = ack_error;
        break;
    }
}

typedef enum {
    RESPONSE_NEEDS_PROCESSING,
    PREAMBLE,
    START1,
    START2,
    LEN,
    NO_TAG_EXISTS,
    TAG_EXISTS,
    NOT_EXPECTED_RESPONSE
} response_state;

response_state current_response_state = RESPONSE_NEEDS_PROCESSING;

void process_response(uint8_t rx_byte) {
    Node *node = Node::getInstance();
    switch(current_response_state)
    {
        case RESPONSE_NEEDS_PROCESSING:
            if (rx_byte == 0) current_response_state = PREAMBLE;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;

        case PREAMBLE:
            if (rx_byte == 0) current_response_state = START1;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;

        case START1:
            if (rx_byte == 255) current_response_state = START2;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;

        case START2:
            if (rx_byte == 3) {
                current_response_state = NO_TAG_EXISTS;
                //node->LedRed->On();
                //node->LedBlue->On();
            }
            else if (rx_byte == 15) current_response_state = TAG_EXISTS;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;
    }
}

typedef enum {
    START_PROCESSING_REST_OF_RESPONSE,
    LCS,
    TO_HOST,
    COMMAND_RESPONSE,
    TAG_NUMBER,
    BAUDRATE,
    DATA2,
    DATA3,
    DATA4,
    DATA5,
    DATA6,
    DATA7,
    DATA8,
    DATA9,
    DATA10,
    DATA11,
    DATA12,
    DATA13,
    POSTAMBLE,
    GOING_TO_END,
    REST_OF_RESPONSE_PROCESSED,
    NOT_EXPECTED_REST_OF_RESPONSE
} rest_of_response_state_t;

rest_of_response_state_t rest_of_response_state = START_PROCESSING_REST_OF_RESPONSE;

int number_of_tags = 0;
int hard_coded_len_with_bytes = 15;
int bytes_left = hard_coded_len_with_bytes - 2 + 1 ; // minus to_host and TAG_NUMBER plus 1 for postamble

void process_rest_of_tag_exists_response(uint8_t rx_byte) {
    Node *node = Node::getInstance();

    switch(rest_of_response_state)
    {
        case START_PROCESSING_REST_OF_RESPONSE:
            if (rx_byte == 241) rest_of_response_state = LCS;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

            // could be any number
        case LCS:
            if (rx_byte == 213) rest_of_response_state = TO_HOST;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case TO_HOST:
            if (rx_byte == 75) {
                rest_of_response_state = TAG_NUMBER;
            }
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case TAG_NUMBER:
            if (rx_byte == 1){
                number_of_tags = 1;
                rest_of_response_state = BAUDRATE;
            } else {
                // two tags at once is not supported
                rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            }
            break;

        case BAUDRATE:
            if (rx_byte == 1) {
                rest_of_response_state = DATA2;
            }
            else {
                rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            }
            break;

        // data bytes i don't care about
        case DATA2:
            if (rx_byte < 255){
                rest_of_response_state = DATA3;
            }
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA3:
            //if (rx_byte == 68) {
            if (rx_byte < 255) {
                rest_of_response_state = DATA4;
            }
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

            // data byte i don't care about
        case DATA4:
            //if (rx_byte == 0) rest_of_response_state = DATA5;
            if (rx_byte < 255) {
                rest_of_response_state = DATA5;

            }
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA5:
            //if (rx_byte == 7) rest_of_response_state = DATA6;
            if (rx_byte < 255) rest_of_response_state = DATA6;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA6:
            if (rx_byte < 255) rest_of_response_state = DATA7;
            //if (rx_byte == 4) rest_of_response_state = DATA7;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA7:
            if (rx_byte < 255) rest_of_response_state = DATA8;
            //if (rx_byte == 160) rest_of_response_state = DATA8;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA8:
            if (rx_byte < 255) rest_of_response_state = DATA9;
            //if (rx_byte == 53) rest_of_response_state = DATA9;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA9:
            if (rx_byte < 255) rest_of_response_state = DATA10;
            //if (rx_byte == 178) rest_of_response_state = DATA10;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA10:
            if (rx_byte < 255) rest_of_response_state = DATA11;
            //if (rx_byte == 188) rest_of_response_state = DATA11;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA11:
            if (rx_byte < 255) rest_of_response_state = DATA12;
            //if (rx_byte == 43) rest_of_response_state = DATA12;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA12:
            if (rx_byte < 255) rest_of_response_state = DATA13;
            //if (rx_byte == 128) rest_of_response_state = DATA13;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case DATA13:
            if (rx_byte < 255) rest_of_response_state = POSTAMBLE;
            //if (rx_byte == 161) rest_of_response_state = POSTAMBLE;
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;

        case POSTAMBLE:
            //if (rx_byte == 0) {
            if (rx_byte < 255) {
                rest_of_response_state = REST_OF_RESPONSE_PROCESSED;
            }
            else rest_of_response_state = NOT_EXPECTED_REST_OF_RESPONSE;
            break;
            // 00 00 FF 0F F1 D5 4B 01 01 00 44 00 07 04 A0 35 B2 BC 2B 80 A1 00 - tag exists
    }
}

typedef enum {
    start_grabbing,
    end_grabbing
} data_grab_state_t;

data_grab_state_t grab_state = start_grabbing;

int max_remaining_byte_size_in_a_frame = 15;
const int four_frame_data_dump_size = 60;
static uint8_t data_dump1[four_frame_data_dump_size] = {0};
int count = 0;
int dump_index = 0;

void grab_frame_bytes_into_mass_dump(uint8_t rx_byte) {
    Node *node = Node::getInstance();
    switch(grab_state)
    {
        case start_grabbing:
            if (count < max_remaining_byte_size_in_a_frame) {
                data_dump1[dump_index] = rx_byte;
                count++;
                dump_index++;
                grab_state = start_grabbing;
            } else {
                grab_state = end_grabbing;
                count = 0;
            }
        break;
    }
}

bool id_exists_in_response(uint8_t *response, int response_length) {
    std::string response_string(response, response+response_length);

    if (response_string.find("id=") == std::string::npos) {
        return false;
    } else {
        return true;
    }
}

short get_id() {
    std::string response_string;
    for (int i=0; i< four_frame_data_dump_size; ++i) {
        if (isprint(data_dump1[i])) response_string = response_string + static_cast<char>(data_dump1[i]);
    }

    std::size_t found = response_string.find("=");
    std::string id = response_string.substr(found+1, 4);

    //CHECK NUMERIC LIMITS WITH std::numeric_limits<short>::lowest() std::numeric_limits<short>::max()

    return (short) atoi(id.c_str());
}

typedef enum {
    SETUP,
    NEEDS_RETRY,
    ERROR,
    TAG_PRESENCE_UNKNOWN, // check if tag exists
    IN_LIST_ACK_PROCESSING,
    PROCESSING_REST_OF_RESPONSE,
    READ_TAG_ACK,
    SECOND_READ_ACK,
    THIRD_READ_ACK,
    GET_PAID_1,
    GET_PAID_2,
    GET_PAID_3,
    GET_PAID_4,
    FOURTH_READ_ACK,
    FIFTH_READ_ACK,
    EAT_FIRST_DATA_COMMAND,
    EAT_SECOND_DATA_COMMAND,
    EAT_THIRD_DATA_COMMAND,
    EAT_FOURTH_DATA_COMMAND,
    ID_TAKEN
} nfc_state_t;

nfc_state_t current_nfc_state = SETUP;

short attendeeId = 0;
bool found_id = false;

void nfcEventHandler(uint8_t rx_byte) {
    Node *node = Node::getInstance();


    switch(current_nfc_state)
    {
        case ID_TAKEN:
            // too bad ass to keep going
        break;

        case NEEDS_RETRY:
            // do nothing until we need to search for tag again
        break;

        case ERROR:
            // do nothing until we need to search for tag again
            //node->LedRed->On();
        break;

        case SETUP:
            if (rx_byte == 255) first_response_received = true;
        break;

        case TAG_PRESENCE_UNKNOWN:
            // Handle Ack
            if (current_ack_state != ack_processed) process_ack(rx_byte);
            if (current_ack_state == ack_error) current_nfc_state = NEEDS_RETRY;
            if (current_ack_state == ack_processed) current_nfc_state = IN_LIST_ACK_PROCESSING;
            break;

        case IN_LIST_ACK_PROCESSING:
            if (current_response_state != NO_TAG_EXISTS || current_response_state != TAG_EXISTS) process_response(rx_byte);

            if (current_response_state == NOT_EXPECTED_RESPONSE) current_nfc_state = NEEDS_RETRY;
            if (current_response_state == NO_TAG_EXISTS) current_nfc_state = NEEDS_RETRY;
            // two tags at once is not supported at this time

            if (current_response_state == TAG_EXISTS) current_nfc_state = PROCESSING_REST_OF_RESPONSE;
        break;

        case PROCESSING_REST_OF_RESPONSE:
            if (rest_of_response_state != REST_OF_RESPONSE_PROCESSED) process_rest_of_tag_exists_response(rx_byte);
            if (rest_of_response_state == NOT_EXPECTED_REST_OF_RESPONSE) current_nfc_state = NEEDS_RETRY;

            if (rest_of_response_state == REST_OF_RESPONSE_PROCESSED) {
                in_data_exchange('\x00', '\xBB');
                current_nfc_state = SECOND_READ_ACK;
            }
        break;

        case SECOND_READ_ACK:
            if (rx_byte == 65) {
                // \x41
                current_nfc_state = EAT_FIRST_DATA_COMMAND;
            }
        break;

        case EAT_FIRST_DATA_COMMAND:
            if (rx_byte == 0) current_nfc_state = GET_PAID_1;
        break;

        case GET_PAID_1:
            grab_state = start_grabbing;
            if (grab_state != end_grabbing) grab_frame_bytes_into_mass_dump(rx_byte);

            if (grab_state == end_grabbing) {
                in_data_exchange('\x04', '\xB7');
                current_nfc_state = THIRD_READ_ACK;
            }
        break;

        case THIRD_READ_ACK:
            if (rx_byte == 65) {
                // \x41
                current_nfc_state = EAT_SECOND_DATA_COMMAND;
            }
        break;

        case EAT_SECOND_DATA_COMMAND:
            if (rx_byte == 0) current_nfc_state = GET_PAID_2;
        break;

        case GET_PAID_2:
            grab_state = start_grabbing;
            if (grab_state != end_grabbing) grab_frame_bytes_into_mass_dump(rx_byte);

            if (grab_state == end_grabbing) {
                in_data_exchange('\x08', '\xB3');
                current_nfc_state = FOURTH_READ_ACK;
            }
        break;

        case FOURTH_READ_ACK:
            // \x41
            if (rx_byte == 65) {
                current_nfc_state = EAT_THIRD_DATA_COMMAND;
            }
        break;

        case EAT_THIRD_DATA_COMMAND:
            if (rx_byte == 0) current_nfc_state = GET_PAID_3;
        break;

        case GET_PAID_3:
            grab_state = start_grabbing;
            if (grab_state != end_grabbing) grab_frame_bytes_into_mass_dump(rx_byte);

            if (grab_state == end_grabbing) {
                in_data_exchange('\x0C', '\xAF');
                current_nfc_state = FIFTH_READ_ACK;
            }
        break;

        case FIFTH_READ_ACK:
            // \x41
            if (rx_byte == 65)
                current_nfc_state = GET_PAID_4;
            break;

        case EAT_FOURTH_DATA_COMMAND:
            if (rx_byte == 0) current_nfc_state = GET_PAID_4;
        break;

        case GET_PAID_4:
            grab_state = start_grabbing;
            if (grab_state != end_grabbing) grab_frame_bytes_into_mass_dump(rx_byte);

            if (grab_state == end_grabbing) {
                if (id_exists_in_response(data_dump1, four_frame_data_dump_size)) {
                    node->PutInRetryStorage(get_id());

                    node->FlashWhite(6);
                    current_nfc_state = ID_TAKEN;
                } else {
                    current_nfc_state = NEEDS_RETRY;
                }
            }
            break;
    }
}

void setup_state_machine() {
    current_response_state = RESPONSE_NEEDS_PROCESSING;
    rest_of_response_state = START_PROCESSING_REST_OF_RESPONSE;
    current_nfc_state = TAG_PRESENCE_UNKNOWN;
    current_ack_state = ACK_START;
    number_of_tags = 0;
    attendeeId = 0;
    found_id = false;
    dump_index = 0;
    for (int i=0; i<four_frame_data_dump_size; i++) {
        data_dump1[i] = 0;
    }
}

void NFCModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
#ifdef ENABLE_NFC
    if (!UART_CONFIGURED) {
        uart_115200_config(RTS_PIN_NUMBER, 19, CTS_PIN_NUMBER, 20, nfcEventHandler);
        UART_CONFIGURED = true;
    }

    if (appTimer % 10 == 0) {
        if (get_setup_state() != SETUP_DONE) setup();
        if (get_setup_state() == SETUP_DONE) {
            if (first_response_received)
                current_nfc_state = TAG_PRESENCE_UNKNOWN;
            else {
                node->LedRed->On();
            }
        }
    }

    if (get_setup_state() == SETUP_DONE && (appTimer/1000 % 5 && appTimer % 1000 == 0)) {
        setup_state_machine();
        in_list_passive_target();
    }
#endif
}
