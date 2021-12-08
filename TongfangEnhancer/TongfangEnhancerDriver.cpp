#include "TongfangEnhancer.hpp"

#include "Hotkey/HotkeyHandler.hpp"
#include "FanControl/FanControlHandler.hpp"

extern "C" {
#include <sys/kern_event.h>
}

OSDefineMetaClassAndStructors(TongfangEnhancerDriver, IOService)
OSDefineMetaClassAndStructors(TongfangEnhancerUserClient, IOUserClient)


IOService* TongfangEnhancerDriver::probe(IOService *provider, SInt32 *score)
{
    IOService* result = super::probe(provider, score);
    if (result == nullptr) {
        return nullptr;
    }
    
    wmiController = OSDynamicCast(VoodooWMIController, provider);
    debugMode     = OSDynamicCast(OSBoolean, getProperty("DebugMode"))->getValue();
    
    // instantiate handler
    hotkeyHandler = new HotkeyHandler(this);
    fanControlHandler = new FanControlHandler(this);
    
    
    // read hotkey platform data from Info.plist
    // omit info.plist integrity check for the module itself.
    OSDictionary*        platforms = OSDynamicCast(OSDictionary, getProperty("Platforms"));
    OSCollectionIterator* iterator = OSCollectionIterator::withCollection(platforms);
    
    while (OSSymbol* key = OSDynamicCast(OSSymbol, iterator->getNextObject())) {
        OSDictionary* platform  = OSDynamicCast(OSDictionary, platforms->getObject(key));
        const char*   guidMatch = OSDynamicCast(OSString, platform->getObject("GUIDMatch"))->getCStringNoCopy();
        
        if (wmiController->hasGuid(guidMatch)) {
            IOLog("%s::find matched hotkey scheme: %s\n", getName(), key->getCStringNoCopy());
            hotkeyHandler->setEventArray(OSDynamicCast(OSArray, platform->getObject("WMIEvents")));
            
            OSDictionary* matchedPlatform = OSDictionary::withDictionary(platform);
            matchedPlatform->setObject("Name", key);
            setProperty("Platform", matchedPlatform);
            removeProperty("Platforms");
            
            matchedPlatform->release();
            iterator->release();
            return this;
        }
    }
    iterator->release();
    
    if (wmiController->hasGuid(WMI_GUID)) {
        return this;
    }
    
    return nullptr;
}


bool TongfangEnhancerDriver::start(IOService *provider)
{
    if (!super::start(provider)) {
        return false;
    }
    
    hotkeyHandler->registerEventTable();
    
    registerService();
    return true;
}


void TongfangEnhancerDriver::stop(IOService *provider)
{
    hotkeyHandler->unregisterEventTable();
    super::stop(provider);
}


void TongfangEnhancerDriver::onWMIEvent(WMIBlock *block, OSObject *eventData)
{
    hotkeyHandler->handleWMIEvent(block, eventData);
}


void TongfangEnhancerDriver::sendMessageToDaemon(int type, int arg1 = 0, int arg2 = 0) {
    struct kev_msg kernelEventMsg = {0};

    uint32_t vendorID = 0;
    if (KERN_SUCCESS != kev_vendor_code_find(KERNEL_EVENT_VENDOR_ID, &vendorID)) {
        return;
    }
    kernelEventMsg.vendor_code = vendorID;
    kernelEventMsg.event_code = KERNEL_EVENT_CODE;
    kernelEventMsg.kev_class = KEV_ANY_CLASS;
    kernelEventMsg.kev_subclass = KEV_ANY_SUBCLASS;

    kernelEventMsg.dv[0].data_length = sizeof(int);
    kernelEventMsg.dv[0].data_ptr = &type;
    kernelEventMsg.dv[1].data_length = sizeof(int);
    kernelEventMsg.dv[1].data_ptr = &arg1;
    kernelEventMsg.dv[2].data_length = sizeof(int);
    kernelEventMsg.dv[2].data_ptr = &arg2;

    kev_msg_post(&kernelEventMsg);
}


i8 TongfangEnhancerDriver::dispatchCommand(u8 id, int arg1, int arg2) {
    IOLog("[%s] received command ID = %u\n", getName(), id);
    
    switch (id) {
        case kActionLockScreen:
        case kActionSwitchScreen:
        case kActionToggleAirplaneMode:
        case kActionKeyboardBacklightUp:
        case kActionKeyboardBacklightDown:
        case kActionReloadFanConfig:
            sendMessageToDaemon(id);
            break;
        case kActionSleep:
            hotkeyHandler->doEnterSleep();
            break;
        case kActionToggleTouchpad:
            return hotkeyHandler->doToggleTouchpad();
            break;
        case kActionScreenBrightnessDown:
            hotkeyHandler->doAdjustBrightness(false);
            break;
        case kActionScreenBrightnessUp:
            hotkeyHandler->doAdjustBrightness(true);
            break;
        case kActionSetMode:
            fanControlHandler->setFanMode((FanControlMode) arg1);
            break;
        case kActionSetSpeed:
            fanControlHandler->setFanSpeed(arg1);
            break;
        case kActionSetAutoAdjustFanSpeed:
            sendMessageToDaemon(id, arg1, arg2);
            break;
        default:
            return -1;
    }
    return 0;
}
