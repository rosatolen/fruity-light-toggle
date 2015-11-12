#include <NFCModule.h>
#include <NFC.h>

#include <Node.h>

extern "C" {
#include "app_util_platform.h"
#include "app_error.h"
#include "pn532.h"
}

#define UART_TX_BUF_SIZE 1
#define UART_RX_BUF_SIZE 1

bool UART_CONFIGURED = false;

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
    LCS,
    TO_HOST,
    COMMAND_RESPONSE,
    NO_TAG_EXISTS,
    ONE_TAG_EXISTS,
    TWO_TAGS_EXIST,
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
            if (rx_byte == 3) current_response_state = NO_TAG_EXISTS;
            if (rx_byte == 15) current_response_state = LEN;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;

        case LEN:
            if (rx_byte == 241) current_response_state = LCS;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;

        case LCS:
            if (rx_byte == 213) current_response_state = TO_HOST;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;

        case TO_HOST:
            else if (rx_byte == 1) current_response_state = ONE_TAG_EXISTS;
            else if (rx_byte == 2) current_response_state = TWO_TAGS_EXIST;
            else current_response_state = NOT_EXPECTED_RESPONSE;
            break;
    }
}

typedef enum {
    SETUP,
    TAG_PRESENCE_UNKNOWN, // check if tag exists
    TAG_EXISTS, // read tag
    NEEDS_RETRY,
    ERROR
} nfc_state_t;

nfc_state_t current_nfc_state = SETUP;

void nfcEventHandler(uint8_t rx_byte) {
    Node *node = Node::getInstance();

    switch(current_nfc_state)
    {
        case ERROR:
            // do nothing until we need to search for tag again
            //node->LedRed->On();
        break;

        case SETUP:
        break;

        case TAG_PRESENCE_UNKNOWN:
            // Handle Ack
            if (current_ack_state != ack_processed) process_ack(rx_byte);
            if (current_ack_state == ack_error) break;

            // Handle Response
            process_response(rx_byte);

            if (current_response_state == NOT_EXPECTED_RESPONSE) break;
            if (current_response_state == NO_TAG_EXISTS) current_nfc_state = NEEDS_RETRY;
            if (current_response_state == ONE_TAG_EXISTS) current_nfc_state = TAG_EXISTS;
            if (current_response_state == TWO_TAGS_EXIST) current_nfc_state = NEEDS_RETRY;
                // two tags at once is not supported at this time
        break;

        case TAG_EXISTS:
            // GET PAID //
            //        // DATA EXCHANGE. should check to see if id_exists_in_response
            //       NFC::dataExchange1();
            //in_data_exchange('\x00', '\xBB');
            // process ack
            //
            //       // DATA EXCHANGE. should check to see if id_exists_in_response
            //       NFC::dataExchange2();
            //
            //       // DATA EXCHANGE ID IS IN HERE FOR SAMPLE TAGS
            //       NFC::dataExchange3();
            //
            //       // DATA EXCHANGE. should check to see if id_exists_in_response
            //       NFC::dataExchange4();
            //
            //       // NFC::inRelease(); - using release would allow people to register themselves rapidly more than once but i don't want that right now...
            //    }
        break;
    }
}

void setup_state_machine() {
    current_response_state = RESPONSE_NEEDS_PROCESSING;
    current_nfc_state = TAG_PRESENCE_UNKNOWN;
    current_ack_state = ACK_START;
}

void NFCModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
#ifdef ENABLE_NFC
    if (!UART_CONFIGURED) {
        uart_115200_config(RTS_PIN_NUMBER, 19, CTS_PIN_NUMBER, 20, nfcEventHandler);
        UART_CONFIGURED = true;
    }

    if (appTimer % 100 == 0) {
        if (get_setup_state() != SETUP_DONE) setup();
        if (get_setup_state() == SETUP_DONE) current_nfc_state = TAG_PRESENCE_UNKNOWN;
    }

    if (appTimer/1000 % 5 && appTimer % 1000 == 0) {
        setup_state_machine();
        in_list_passive_target();
        if (current_nfc_state == ERROR) current_nfc_state = TAG_PRESENCE_UNKNOWN;
    }
#endif
}
