#include "Daemon.hpp"
#include "FanControl.h"

bool shouldAdjustFanSpeed = false;

pthread_mutex_t configLock;

/// Send message to TongfangFanControlDriver
///
/// @param message FanControlMessage
/// @return int
extern "C" int sendMessageToDriver(WMIActionMessage message) {
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("TongfangEnhancerDriver"));
    
    if (service == IO_OBJECT_NULL) {
        std::cout << "[TongfangEnhancerDaemon] could not find any services matching pattern: TongfangFanControlDriver" << std:: endl;
        return -1;
    }
    
    io_connect_t connection;
    int output = -1;
    
    if (IOServiceOpen(service, mach_task_self(), 0, &connection) == KERN_SUCCESS) {
        std::cout << "[TongfangEnhancerDaemon] successfully open IOService" << std::endl;
        
        size_t outputSize = sizeof(int);
        IOConnectCallStructMethod(connection, kClientSelectorDispatchCommand, &message, sizeof(WMIActionMessage), &output, &outputSize);
        IOServiceClose(connection);
        
        std::cout << "[TongfangEnhancerDaemon] successfully send kernel message" << std::endl;
    }
    
    IOObjectRelease(service);
    return output;
}

/// Set fan mode (-m, --mode)
/// Accepted mode: normal / boost
///
/// @param mode FanControlMode
extern "C" void setFanMode(FanControlMode mode) {
    std::cout << "[fandaemon] setting fanMode: " << mode << std::endl;
    
    WMIActionMessage message;
    message.type = kActionSetMode;
    message.arg1 = mode;
    
    sendMessageToDriver(message);
}


/// Set fan speed level (-l, --level)
/// Accepted level: 0, 1, 2, 3, 4, 5
///
/// @param speedLevel int
extern "C" void setFanSpeed(int speedLevel) {
    std::cout << "[fandaemon] setting fan speedLevel: " << speedLevel << std::endl;
    
    WMIActionMessage message;
    message.type = kActionSetSpeed;
    message.arg1 = speedLevel;
    
    sendMessageToDriver(message);
}


/// initialize default fan controlling curve config
extern "C" void initFanControlConfig() {
    pthread_mutex_lock(&configLock);
    
    fanControlConfig.clear();
    fanControlConfig.push_back(FanControlItem(0.0, 0, FanModeNormal));
    fanControlConfig.push_back(FanControlItem(45.0, 0, FanModeCustom));
    fanControlConfig.push_back(FanControlItem(55.0, 1, FanModeCustom));
    fanControlConfig.push_back(FanControlItem(65.0, 2, FanModeCustom));
    fanControlConfig.push_back(FanControlItem(75.0, 3, FanModeCustom));
    fanControlConfig.push_back(FanControlItem(80.0, 4, FanModeCustom));
    fanControlConfig.push_back(FanControlItem(85.0, 5, FanModeCustom));
    fanControlConfig.push_back(FanControlItem(95.0, 0, FanModeBoost));
    
    pthread_mutex_unlock(&configLock);
}


/// load user defined fan controlling curve config from const path
extern "C" bool loadFanControlConfig() {
    if (access(FAN_CONTROL_CONFIG_PATH.c_str(), R_OK) != 0) {
        std::cout << "[fandaemon] Custom fan config not exists. Using default config." << std::endl;
        return false;
    }
    
    std::ifstream ifs;
    ifs.open(FAN_CONTROL_CONFIG_PATH, std::ios::in);
    
    if (!ifs.is_open()) {
        std::cout << "[fandaemon] Can not open fan config." << std::endl;
        return false;
    }
    
    pthread_mutex_lock(&configLock);
    
    std::string configRaw;
    fanControlConfig.clear();
    while (getline(ifs, configRaw)) {
        FanControlItem item;
        if (3 == (sscanf(configRaw.c_str(), "%lf%d%d", &item.tempLimit, &item.speedLevel, &item.mode)))
            fanControlConfig.push_back(item);
    }
    
    std::sort(fanControlConfig.begin(), fanControlConfig.end());
    std::cout << "[fandaemon] Using fan config from: " << FAN_CONTROL_CONFIG_PATH << std::endl;
    
    pthread_mutex_unlock(&configLock);
    
    return fanControlConfig.size() > 0;
}


/// reload fan controlling curve config when receive an IPC message (refresh request)
extern "C" void reloadFanControlConfig() {
    std::cout << "[fandaemon] Loading fan controlling curve config..." << std::endl;
    if (!loadFanControlConfig())
        initFanControlConfig();
}

/// adjust fan speed by current CPU temperature
extern "C" void adjustFanSpeed(double temp) {
    if (!shouldAdjustFanSpeed) {
        return;
    }
    
    pthread_mutex_lock(&configLock);
    
    if (fanControlConfig.size() <= 0) {
        pthread_mutex_unlock(&configLock);
        return;
    }
    FanControlItem target = fanControlConfig[0];
    
    for (auto item : fanControlConfig) {
        if (item.tempLimit > temp) {
            break;
        }
        target = item;
    }
     
    std::cout << "[fandaemon] Adjust fan speed: mode = " << target.mode << ", speedLevel = " << target.speedLevel << std::endl;
    
    if (target.mode != FanModeCustom) {
        setFanMode((FanControlMode) target.mode);
    } else if (target.speedLevel >= 0 && target.speedLevel <= 5) {
        setFanSpeed(target.speedLevel);
    }
    pthread_mutex_unlock(&configLock);
}


extern "C" void* monitorCPUTemperature(unsigned int interval) {
    while (1) {
        double temp = temperature();
        std::cout << "[fandaemon] Current CPU Temperature = " << temp << std::endl;
        adjustFanSpeed(temp);
        sleep(interval);
    }
}


extern "C" void setShouldAdjustFanSpeed(bool enable) {
    std::cout << "[fandaemon] set auto adjust fan speed = " << enable << std::endl;
    shouldAdjustFanSpeed = enable;
}
