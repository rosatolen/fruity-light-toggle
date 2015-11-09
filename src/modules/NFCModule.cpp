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

void UART0_IRQHandler(void) {
    short attendeeId = NFC::GetAttendeeId();
}

void configure_uart(void)
{
//    const app_uart_comm_params_t uart_params {
//		20,
//		19,
//		RTS_PIN_NUMBER,
//		CTS_PIN_NUMBER,
//		false,
//		UART_BAUDRATE_BAUDRATE_Baud115200
//	};
//
//	uint8_t receiver[1] = {0};
//	uint8_t transmit[256] = {0};
//	const app_uart_buffers_t uart_buffers {
//		uart_buffers.rx_buf = receiver;
//		uart_buffers.rx_buf_size = 1;
//		uart_buffers.tx_buf = transmit;
//		uart_buffers.tx_buf_size = 256;
//	};
//
//
//    ret_code_t ret_code;
//    APP_UART_INIT(&uart_params, uart_buffers, UART0_IRQHandler, APP_IRQ_PRIORITY_LOW);
//    APP_ERROR_CHECK(ret_code);
    uart_115200_config(RTS_PIN_NUMBER, /*TX_PIN_NUM*/ 19, CTS_PIN_NUMBER, /*RX_PIN_NUM*/ 20);
    //NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos;
    NVIC_SetPriority(UART0_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_EnableIRQ(UART0_IRQn);
}

NFCModule::NFCModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
    : Module(moduleID::NFC_MODULE_ID, node, cm, name, storageSlot) {
  _configuration.moduleId = moduleID::NFC_MODULE_ID;
  _configuration.moduleVersion = 1;
  _configuration.moduleActive = true;
  configurationPointer = &_configuration;
}

void NFCModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
#ifdef ENABLE_NFC
    if (!UART_CONFIGURED) {
        uart_115200_config(RTS_PIN_NUMBER, /*TX_PIN_NUM*/ 19, CTS_PIN_NUMBER, /*RX_PIN_NUM*/ 20);
        UART_CONFIGURED = true;
    }

    // every second...
    // read current state of UART
    // schedule state change
    if (appTimer/1000 % 5 && appTimer % 1000 == 0) {

        // SHOULD CHECK IF MESH IS ALREADY SET UP BEFORE DOING THIS
        if(current_stage == 1) {
            NFC::pn532_wakeup();
            NFC::set_parameter_command();
            NFC::read_register();
            current_stage = 2;
//    /* BELOW HAS NOT PASSED BUILD */
        } else if (current_stage == 2) {
//            NFC::write_register();
//            NFC::rfconfiguration_1();
//            NFC::rfconfiguration_2();
//            current_stage = 3;
//        } else if (current_stage == 3) {
//            NFC::rfconfiguration_3();
//            NFC::rfconfiguration_4();
//            NFC::read_register2();
//            current_stage = 4;
//        } else if (current_stage == 4) {
//            NFC::write_register2();
//            NFC::rfconfiguration_5();
//            current_stage = 5;
//        } else if (current_stage == 5) {
//            if (NFC::inListPassiveTarget()) {
//                // tag is present! begin data exchange stage
//                current_stage = 6;
//            } else {
//                current_stage = final_stage;
//            }
//        } else if (current_stage == 6) {
//            // DATA EXCHANGE. should check to see if id_exists_in_response
//            NFC::dataExchange1();
//            // DATA EXCHANGE. should check to see if id_exists_in_response
//            NFC::dataExchange2();
//            // DATA EXCHANGE ID IS IN HERE FOR SAMPLE TAGS
//            if (NFC::dataExchange3() != -1) {
//                // found ID! go to next stage to release the tag.
//            } else {
//            //    // did not find ID go to next stage for now. refactor this to check each data exchange for ID
//            //    // release tag and reconfigure
//            //    // powerdown
//            }
//            current_stage = 7;
//        } else if (current_stage == 7) {
//            // DATA EXCHANGE. should check to see if id_exists_in_response
//            NFC::dataExchange4();
//            NFC::inRelease();
//            NFC::rfconfiguration_6();
            current_stage = final_stage;
        } else if (current_stage == final_stage) {
            powerdown();
            current_stage = 1;
        }
    }
#endif
}
