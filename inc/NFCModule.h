#pragma once

#include <Module.h>

class NFCModule: public Module
{
    public:
        NFCModule(Node* node, ConnectionManager* cm, const char* name, u16 storageSlot);
        void TimerEventHandler(u16 passedTime, u32 appTimer);

    private:
        ModuleConfiguration _configuration;

};
