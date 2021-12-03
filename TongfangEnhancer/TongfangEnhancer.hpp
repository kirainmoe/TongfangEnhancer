#ifndef TONGFANG_ENHANCER_HPP
#define TONGFANG_ENHANCER_HPP

#include <IOKit/IOService.h>
#include <IOKit/hid/IOHIDEventService.h>
#include <IOKit/IOUserClient.h>

#include "classes.hpp"
#include "common.h"

#include "VoodooWMIController.hpp"

using i8 = char;
using u8 = unsigned char;
using i32 = int;
using u32 = unsigned int;


class TongfangEnhancerDriver : public IOService {
    using super = IOService;
    OSDeclareDefaultStructors(TongfangEnhancerDriver)
    
    friend class HotkeyHandler;
    friend class FanControlHandler;
    friend class TongfangEnhancerUserClient;
    
private:
    HotkeyHandler      *hotkeyHandler     = nullptr;
    FanControlHandler  *fanControlHandler = nullptr;
    
private:
    bool                debugMode      = false;
    VoodooWMIController *wmiController = nullptr;
    
private:
    i8   dispatchCommand(uint8_t id, int arg1 = 0, int arg2 = 0);
    void sendMessageToDaemon(int type, int arg1, int arg2);
    bool sendKernelMessage(const char* vendorCode, u32 eventCode, int arg1, int arg2, int arg3);
    
public:
    virtual IOService* probe(IOService* provider, SInt32* score) APPLE_KEXT_OVERRIDE;
    virtual bool       start(IOService* provider)                APPLE_KEXT_OVERRIDE;
    virtual void       stop(IOService* provider)                 APPLE_KEXT_OVERRIDE;
    
    void onWMIEvent(WMIBlock* block, OSObject* eventData);
};


class TongfangEnhancerUserClient : public IOUserClient {
    OSDeclareDefaultStructors(TongfangEnhancerUserClient)
    
public:
   virtual IOReturn externalMethod(uint32_t selector,
                                   IOExternalMethodArguments* arguments,
                                   IOExternalMethodDispatch* dispatch = 0,
                                   OSObject* target = 0,
                                   void* reference = 0)
                                                        APPLE_KEXT_OVERRIDE;

   virtual IOReturn clientClose()                       APPLE_KEXT_OVERRIDE;
};

#endif
