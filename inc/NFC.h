#ifndef PN532_CPP_H
#define PN532_CPP_H

#include <types.h>

class NFC {
    public:
        static short GetAttendeeId();
        static void CheckPn532Response();
        static void pn532_wakeup();
    private:
        static void ReadNFCResponse(char *storage, uint8_t storageSize, uint8_t offset);
        static void Pn532InterruptListener(char *pn532response);
};

#endif
