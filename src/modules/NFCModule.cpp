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
bool PN532_IS_SETUP = false;

typedef enum {
    SETUP,
    CHECK_TAG,
    READ_TAG,
    ERROR
} nfc_state_t;

typedef enum {
    DOWN,
    NO_PARAM,
    NOT_WRITTEN_80,
    RF_NOT_CONFIG,
    NOT_WRITTEN_40_10,
    RF_MAX_NOT_CONFIG,
    SETUP_DONE
} setup_state_t;

nfc_state_t current_state = SETUP;
setup_state_t current_setup_state = DOWN;

void setup() {
    switch(current_setup_state)
    {
        case DOWN:
        wakeup();
        current_setup_state = NO_PARAM;
        break;

        case NO_PARAM:
        set_parameter();
        current_setup_state = NOT_WRITTEN_80;
        break;

        case NOT_WRITTEN_80:
        write_80();
        current_setup_state = RF_NOT_CONFIG;
        break;

        case RF_NOT_CONFIG:
        config_rf();
        current_setup_state = NOT_WRITTEN_40_10;
        break;

        case NOT_WRITTEN_40_10:
        write_40_10();
        current_setup_state = RF_MAX_NOT_CONFIG;
        break;

        case RF_MAX_NOT_CONFIG:
        config_rf_max();
        current_setup_state = SETUP_DONE;
        break;
    }
}

NFCModule::NFCModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
    : Module(moduleID::NFC_MODULE_ID, node, cm, name, storageSlot) {
  _configuration.moduleId = moduleID::NFC_MODULE_ID;
  _configuration.moduleVersion = 1;
  _configuration.moduleActive = true;
  configurationPointer = &_configuration;
}

void nfcEventHandler(uart_event *event) {
    Node *node = Node::getInstance();
    node->LedRed->On();
    node->LedGreen->On();
    node->LedBlue->On();

    //app_uart_put(wakeup sequence)
    //    app_uart_get - will trigger event handler to handle the ACK and send the next message
    //static char input[6] = {0};
    //if (simple_uart_get_with_timeout(0, (uint8_t*) input)) {
    //    ReadNFCResponse(input, 6, 1);
    //    if (is_ack(input, 6)) {
    //        //proceed with gobbling
    //        // does this return a bool for when it failed?
    //        Pn532InterruptListener(input);
    //    } else {
    //        // error state :( start over
    //    }
    //}

    return;
}



void NFCModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
#ifdef ENABLE_NFC
    if (!UART_CONFIGURED) {
        uart_115200_config(RTS_PIN_NUMBER, 19, CTS_PIN_NUMBER, 20, nfcEventHandler);
        UART_CONFIGURED = true;
    }

    if (appTimer % 100 == 0) {
        if (current_setup_state != SETUP_DONE) setup();
    }

    // schedule state change
    if (appTimer/1000 % 5 && appTimer % 1000 == 0) {
        //powerdown();
    /* BELOW HAS NOT PASSED BUILD */
    //    if (NFC::inListPassiveTarget()) {
    //        // DATA EXCHANGE. should check to see if id_exists_in_response
    //       NFC::dataExchange1();
    //       // DATA EXCHANGE. should check to see if id_exists_in_response
    //       NFC::dataExchange2();
    //       // DATA EXCHANGE ID IS IN HERE FOR SAMPLE TAGS
    //       NFC::dataExchange3();
    //       // DATA EXCHANGE. should check to see if id_exists_in_response
    //       NFC::dataExchange4();
    //       // NFC::inRelease(); - using release would allow people to register themselves more than once
    //    }
    }
#endif
}
