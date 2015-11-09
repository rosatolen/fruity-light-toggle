#ifndef PN532_CPP_H
#define PN532_CPP_H

#include <types.h>

class NFC {
    public:
        static short GetAttendeeId();
        static void CheckPn532Response();

        static void pn532_wakeup();
        static void set_parameter_command();
        static void read_register();
        static void write_register();
        static void rfconfiguration_1();
        static void rfconfiguration_2();
        static void rfconfiguration_3();
        static void rfconfiguration_4();
        static void read_register2();
        static void write_register2();
        static void rfconfiguration_5();
        static bool inListPassiveTarget();
        static void dataExchange1();
        static void dataExchange2();
        static short dataExchange3();
        static void dataExchange4();
        static void inRelease();
        static void rfconfiguration_6();
        // should have powerdown

    private:
        static void ReadNFCResponse(char *storage, uint8_t storageSize, uint8_t offset);
        static void Pn532InterruptListener(char *pn532response);
};

#endif
