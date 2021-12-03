#include <IOKit/pwr_mgt/RootDomain.h>

#include "HotkeyHandler.hpp"

enum {
    kKeyboardSetTouchStatus = iokit_vendor_specific_msg(100),  // set disable/enable touchpad (data is bool*)
    kKeyboardGetTouchStatus = iokit_vendor_specific_msg(101),  // get disable/enable touchpad (data is bool*)
    kKeyboardKeyPressTime = iokit_vendor_specific_msg(110),    // notify of timestamp a non-modifier key was pressed (data is uint64_t*)
};

HotkeyHandler::HotkeyHandler(TongfangEnhancerDriver *driver)
{
    this->driver        = driver;
    this->wmiController = driver->wmiController;
    this->debugMode     = driver->debugMode;
}

HotkeyHandler::~HotkeyHandler() {}

void HotkeyHandler::setEventArray(OSArray *array)
{
    this->eventArray = array;
}

void HotkeyHandler::registerEventTable()
{
    for (int i = 0; i < eventArray->getCount(); i++) {
        OSDictionary* dict = OSDynamicCast(OSDictionary, eventArray->getObject(i));
        if (!dict) {
            IOLog("%s::failed to parse hotkey event %d", driver->getName(), i);
            continue;
        }
        
        OSString* guid = OSDynamicCast(OSString, dict->getObject("GUID"));
        OSNumber* notifyId = OSDynamicCast(OSNumber, dict->getObject("NotifyID"));
        OSNumber* eventId = OSDynamicCast(OSNumber, dict->getObject("EventData"));
        OSNumber* actionId = OSDynamicCast(OSNumber, dict->getObject("ActionID"));
        
        if (!guid || notifyId == nullptr || eventId == nullptr || actionId == nullptr) {
            IOLog("%s::failed to parse hotkey event %d", driver->getName(), i);
            continue;
        }
        
        wmiController->registerWMIEvent(guid->getCStringNoCopy(),
                                        driver,
                                        OSMemberFunctionCast(WMIEventAction, driver, &TongfangEnhancerDriver::onWMIEvent));
    }
}

void HotkeyHandler::unregisterEventTable()
{
    if (this->eventArray == nullptr) {
        return;
    }
    
    for (int i = 0; i < eventArray->getCount(); i++) {
        OSDictionary* dict = OSDynamicCast(OSDictionary, eventArray->getObject(i));
        OSString* guid = OSDynamicCast(OSString, dict->getObject("GUID"));
        wmiController->unregisterWMIEvent(guid->getCStringNoCopy());
    }
}

void HotkeyHandler::handleWMIEvent(WMIBlock *block, OSObject *eventData)
{
    int obtainedEventData = 0;
    if (OSNumber* numberObj = OSDynamicCast(OSNumber, eventData)) {
        obtainedEventData = numberObj->unsigned32BitValue();
    }
    
    IOLog("%s::onWMIEvent (0X%02X, 0X%02X)\n", driver->getName(), block->notifyId, obtainedEventData);
    
    for (int i = 0; i < eventArray->getCount(); i++) {
        OSDictionary* dict = OSDynamicCast(OSDictionary, eventArray->getObject(i));
        UInt8 notifyId = OSDynamicCast(OSNumber, dict->getObject("NotifyID"))->unsigned8BitValue();
        int eventData = OSDynamicCast(OSNumber, dict->getObject("EventData"))->unsigned32BitValue();
        UInt8 actionId = OSDynamicCast(OSNumber, dict->getObject("ActionID"))->unsigned8BitValue();
        if (block->notifyId == notifyId && obtainedEventData == eventData) {
            driver->dispatchCommand(actionId);
        }
    }
}


i8 HotkeyHandler::doToggleTouchpad()
{
    const OSSymbol* key          = OSSymbol::withCString("RM,deliverNotifications");
    i8              isEnabled    = -1;
    OSDictionary*   serviceMatch = driver->propertyMatching(key, kOSBooleanTrue);
    
    if (OSIterator* iterator = driver->getMatchingServices(serviceMatch)) {
        IOLog("[%s] get driver iterator\n", driver->getName());
        
        while (IOService* candidateService = OSDynamicCast(IOService, iterator->getNextObject())) {
            candidateService->message(kKeyboardGetTouchStatus, driver, &isEnabled);
            if (isEnabled != -1) {
                IOLog("%s::get touchpad service: %s\n", driver->getName(), candidateService->getMetaClass()->getClassName());
                isEnabled = !isEnabled;
                candidateService->message(kKeyboardSetTouchStatus, driver, &isEnabled);
                break;
            }
        }
        iterator->release();
    } else {
        IOLog("[%s] failed to get touchpad service\n", driver->getName());
    }
    
    key->release();
    serviceMatch->release();
    return isEnabled;
}


void HotkeyHandler::doAdjustBrightness(bool increase)
{
    OSDictionary* serviceMatch = driver->serviceMatching("IOHIDEventService");
    if (IOService* hidEventService = driver->waitForMatchingService(serviceMatch, 1e9)) {
        IOLog("%s::get HID event service\n", driver->getName());

        void** vtable = *(void***)hidEventService;
        typedef void(*dispatchKeyboardEventMethod)(void* self, AbsoluteTime timeStamp, UInt32 usagePage, UInt32 usage, UInt32 value, IOOptionBits options);
        dispatchKeyboardEventMethod method = (dispatchKeyboardEventMethod)vtable[DISPATCH_KEY_EVENT_METHOD_INDEX];

        AbsoluteTime timestamp;
        clock_get_uptime(&timestamp);
        UInt32 keyCode = increase ? kHIDUsage_KeyboardF15 : kHIDUsage_KeyboardF14;
        method(hidEventService, timestamp, kHIDPage_KeyboardOrKeypad, keyCode, true, 0);
        method(hidEventService, timestamp, kHIDPage_KeyboardOrKeypad, keyCode, false, 0);

        hidEventService->release();
    } else {
        IOLog("%s failed to get HID event service\n", driver->getName());
    }
    serviceMatch->release();
}


void HotkeyHandler::doEnterSleep()
{
    if (IOPMrootDomain* rootDomain = (driver->getPMRootDomain())) {
        rootDomain->receivePowerNotification(kIOPMSleepNow);
    }
}
