#ifndef HOTKEY_HANDLER_HPP
#define HOTKEY_HANDLER_HPP

/*
 * Hotkey handler class
 *
 * HotkeyHandler is ported from Goshin/VoodooWMI
 * https://github.com/goshin/VoodooWMI
 */

#include "common.h"
#include "TongfangEnhancer.hpp"

class HotkeyHandler {
    friend class TongfangEnhancerDriver;
    
private:
    bool debugMode = false;
    
    VoodooWMIController    *wmiController = nullptr;
    TongfangEnhancerDriver *driver        = nullptr;
    OSArray                *eventArray    = nullptr;
    
private:
    i8   doToggleTouchpad();
    void doAdjustBrightness(bool increase);
    void doEnterSleep();

public:
    HotkeyHandler(TongfangEnhancerDriver *driver);
    ~HotkeyHandler();
    
    void setEventArray(OSArray *array);
    void registerEventTable();
    void unregisterEventTable();
    void handleWMIEvent(WMIBlock *block, OSObject *eventData);
};

#endif
