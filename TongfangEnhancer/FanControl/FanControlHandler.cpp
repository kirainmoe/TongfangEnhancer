#include "FanControlHandler.hpp"

FanControlHandler::FanControlHandler(TongfangEnhancerDriver *driver)
{
    this->driver        = driver;
    this->wmiController = driver->wmiController;
    this->debugMode     = driver->debugMode;
}

FanControlHandler::~FanControlHandler() {}

/// Write value on ECRAM with a specific address
/// address and value should be composited by the following expression: (value << 16) + address
/// sizeof(data) is 4 bytes (32 bits)
///
/// @param address unsigned int
/// @param value unsigned int
void FanControlHandler::writeECRAM(u32 address, u32 value) {
    if (wmiController == nullptr) {
        IOLog("[FanControl] invalid WMI instance, ignoring writeECRAM request: %lu %lu\n", address, value);
        return;
    }
    
    // GetSetULong
    unsigned long data = (value << 16) + address;
    
    OSNumber *inputData = OSNumber::withNumber(data, 32);
    OSNumber *outputData = OSNumber::withNumber((unsigned long)0, 32);
    OSObject **result = new (OSObject*)();
    *result = outputData;
    
    IOLog("[FanControl] call WMI->evaluateMethod, GUID = %s, instance = %d, method = %lu, data = %lu\n",
          WMI_GUID,
          WMI_INSTANCE_ID,
          WMI_METHOD_ID,
          data);
    
    IOReturn wmiResult = wmiController->evaluateMethod(WMI_GUID, WMI_INSTANCE_ID, WMI_METHOD_ID, inputData, result);
    IOLog("[FanControl] evaluateMethod result: %d\n", wmiResult);
    
    // clean-up
    inputData->release();
    outputData->release();
    delete result;
}


void FanControlHandler::setFanMode(FanControlMode mode) {
    u32 fanControlByte = FanByteNormal;
    switch (mode) {
        case FanModeNormal:
        default:
            fanControlByte = FanByteNormal;
            break;
        case FanModeBoost:
            fanControlByte = FanByteBoost;
            break;
        case FanModeCustom:
            fanControlByte = FanByteSwitchToCustom;
            break;
    }
    writeECRAM(FAN_CONTROL_ADDRESS, fanControlByte);
}


void FanControlHandler::setFanSpeed(int speedLevel) {
    if (speedLevel < 0 || speedLevel > 5) {
        IOLog("[FanControl] invalid setFanSpeed() request, speedLevel should be in range [0, 5].\n");
        speedLevel = (speedLevel < 0) ? 0 : 5;
    }
    
    IOLog("[FanControl] set fan speed to level %d\n", speedLevel);
    
    setFanMode(FanModeCustom);
    writeECRAM(FAN_CONTROL_ADDRESS, (u32)FanByteLevel0 + (u32)speedLevel);
}
