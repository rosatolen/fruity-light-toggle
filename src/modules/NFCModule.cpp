#include <NFCModule.h>
#include <NFC.h>

#include <Node.h>

extern "C" {
#include "app_util_platform.h"
#include "app_uart.h"
#include "pn532.h"
}

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 1

int current_stage = 1;
int final_stage = 10;
int stages[10] = {1,2,3,4,5,6,7,8,9,10};
bool UART_CONFIGURED = false;
bool PN532_IS_SETUP = false;

void UART0_IRQHandler(void) {
    short attendeeId = NFC::GetAttendeeId();
}

void configure_uart(void)
{
}

NFCModule::NFCModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
    : Module(moduleID::NFC_MODULE_ID, node, cm, name, storageSlot) {
  _configuration.moduleId = moduleID::NFC_MODULE_ID;
  _configuration.moduleVersion = 1;
  _configuration.moduleActive = true;
  configurationPointer = &_configuration;
}

//*app_uart_event_handler_t nfcEventHandler() {
//    static char input[6] = {0};
//
//    if (simple_uart_get_with_timeout(0, (uint8_t*) input)) {
//        ReadNFCResponse(input, 6, 1);
//
//        if (is_ack(input, 6)) {
//            //proceed with gobbling
//            // does this return a bool for when it failed?
//            Pn532InterruptListener(input);
//
//        } else {
//            // error state :( start over
//        }
//    }
//}

void module_wakeup() {
    uint8_t wakeup[26] = {
        '\x55','\x55',
        '\x00','\x00','\x00','\x00',
        '\x00','\x00','\x00','\x00',
        '\x00','\x00','\x00','\x00',
        '\x00','\x00','\x00','\x00',
        '\xFF','\x03','\xFD','\xD4',
        '\x14','\x01','\x17','\x00'};

    for (int i=0; i < 26; i++) {
        uart_put_char(wakeup[i]);
    }
    get_ack();
}


void NFCModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
#ifdef ENABLE_NFC
    if (!UART_CONFIGURED) {
        uart_115200_config(RTS_PIN_NUMBER, /*TX_PIN_NUM*/ 19, CTS_PIN_NUMBER, /*RX_PIN_NUM*/ 20);
        //////// Setup with APP_UART_INIT
        //    Declare an app_uart_event_handler_t and place it in initialization below
        //const app_uart_comm_params_t uart_params {
        //    20,
        //    19,
        //    RTS_PIN_NUMBER,
        //    CTS_PIN_NUMBER,
        //    false,
        //    UART_BAUDRATE_BAUDRATE_Baud115200
        //};

        //uint8_t receiver[1] = {0};
        //uint8_t transmit[256] = {0};
        //const app_uart_buffers_t uart_buffers {
        //    uart_buffers.rx_buf = receiver;
        //    uart_buffers.rx_buf_size = 1;
        //    uart_buffers.tx_buf = transmit;
        //    uart_buffers.tx_buf_size = 256;
        //};

        // Do I need this?
        //NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos;
        //NVIC_SetPriority(UART0_IRQn, APP_IRQ_PRIORITY_LOW);
        //NVIC_EnableIRQ(UART0_IRQn);

        //ret_code_t ret_code;

        //APP_UART_INIT(&uart_params, nfcEventHandler, APP_IRQ_PRIORITY_LOW, ret_code);
        //APP_ERROR_CHECK(ret_code);

        //app_uart_put(wakeup sequence)
        //    app_uart_get - will trigger event handler to handle the ACK and send the next message
        UART_CONFIGURED = true;
    }

    // schedule state change
    if (appTimer/1000 % 5 && appTimer % 1000 == 0) {
        if (!PN532_IS_SETUP) {
            NFC::pn532_wakeup();
            NFC::set_parameter_command();
            NFC::write_register();
            NFC::rfconfiguration_2();
            NFC::write_register2();
            NFC::rfconfiguration_5();
            PN532_IS_SETUP = true;
        }

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
