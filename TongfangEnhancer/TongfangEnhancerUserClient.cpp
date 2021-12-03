#include "TongfangEnhancer.hpp"
#include "Hotkey/HotkeyHandler.hpp"

extern "C" {
#include <sys/kern_event.h>
#include "message.h"
}

IOReturn TongfangEnhancerUserClient::externalMethod(uint32_t selector,
                                                    IOExternalMethodArguments* arguments,
                                                    IOExternalMethodDispatch* dispatch,
                                                    OSObject* target,
                                                    void* reference) {
    TongfangEnhancerDriver* driver = OSDynamicCast(TongfangEnhancerDriver, getProvider());
    
    if (!driver) {
        return kIOReturnError;
    }

    if (selector == kClientSelectorDispatchCommand) {
        const WMIActionMessage* input = static_cast<const WMIActionMessage*>(arguments->structureInput);
        *static_cast<int*>(arguments->structureOutput) = driver->dispatchCommand(input->type, input->arg1, input->arg2);
        return kIOReturnSuccess;
    }
    
    return kIOReturnNotFound;
}

IOReturn TongfangEnhancerUserClient::clientClose() {
    if (!isInactive()) {
        terminate();
    }
    return kIOReturnSuccess;
}
