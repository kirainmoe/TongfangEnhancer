#ifndef FAN_CONTROL_HANDLER_HPP
#define FAN_CONTROL_HANDLER_HPP

#include "common.h"
#include "TongfangEnhancer.hpp"

class FanControlHandler {
    friend class TongfangEnhancerDriver;
    
private:
    bool debugMode = false;
    
    VoodooWMIController    *wmiController = nullptr;
    TongfangEnhancerDriver *driver        = nullptr;
    
private:
    void writeECRAM(u32 address, u32 value);
    
public:
    FanControlHandler(TongfangEnhancerDriver *driver);
    ~FanControlHandler();
    
    void setFanMode(FanControlMode mode);
    void setFanSpeed(int speedLevel);
};

#endif
