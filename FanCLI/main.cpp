#include <iostream>
#include <string>

#include <boost/program_options.hpp>


extern "C" {
#import <CoreServices/CoreServices.h>
#include "common.h"
#include "message.h"
#include "../MacKernelSDK/Headers/sys/kern_event.h"
}

using namespace boost::program_options;

const std::string emptyError = "";

void showAppInfo() {
    std::cout << std::endl;
    std::cout << "=== fancli - macOS CPU/GPU fans controller for TongFang laptops ===" << std::endl;
    std::cout << "=== https://github.com/kirainmoe/TongfangEnhancer ===" << std::endl;
    std::cout << std::endl;
}


/// Send message to TongfangFanControlDriver
///
/// @param message FanControlMessage
/// @return int
int sendMessageToDriver(WMIActionMessage message) {
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("TongfangEnhancerDriver"));
    
    if (service == IO_OBJECT_NULL) {
        std::cout << "[fancli] could not find any services matching pattern: TongfangFanControlDriver";
        return -1;
    }
    
    io_connect_t connection;
    int output = -1;
    
    if (IOServiceOpen(service, mach_task_self(), 0, &connection) == KERN_SUCCESS) {
        std::cout << "[fancli] successfully open IOService" << std::endl;
        
        size_t outputSize = sizeof(int);
        IOConnectCallStructMethod(connection, kClientSelectorDispatchCommand, &message, sizeof(WMIActionMessage), &output, &outputSize);
        IOServiceClose(connection);
        
        std::cout << "[fancli] successfully send kernel message" << std::endl;
    }
    
    IOObjectRelease(service);
    return output;
}


/// Send message to driver to disable auto adjust fan speed
///
/// @param enable bool
void toggleAutoAdjustFanSpeed(bool enable) {
    std::cout << "[fancli] toggle auto adjust fan speed: " << enable << std::endl;
    
    WMIActionMessage message;
    message.type = kActionSetAutoAdjustFanSpeed;
    message.arg1 = (int)enable;
    
    sendMessageToDriver(message);
}


/// Set fan mode (-m, --mode)
/// Accepted mode: normal / boost
///
/// @param mode FanControlMode
void setFanMode(FanControlMode mode) {
    toggleAutoAdjustFanSpeed(false);
    
    std::cout << "[fancli] setting fanMode: " << mode << std::endl;
    
    WMIActionMessage message;
    message.type = kActionSetMode;
    message.arg1 = mode;
    
    sendMessageToDriver(message);
}


/// Set fan speed level (-l, --level)
/// Accepted level: 0, 1, 2, 3, 4, 5
///
/// @param speedLevel int
void setFanSpeed(int speedLevel) {
    toggleAutoAdjustFanSpeed(false);
    
    std::cout << "[fancli] setting fan speedLevel: " << speedLevel << std::endl;
    
    WMIActionMessage message;
    message.type = kActionSetSpeed;
    message.arg1 = speedLevel;
    
    sendMessageToDriver(message);
}


void reloadFanConfig() {
    toggleAutoAdjustFanSpeed(true);
    
    std::cout << "[fancli] request reload fan config" << std::endl;
    
    WMIActionMessage message;
    message.type = kActionReloadFanConfig;
    
    sendMessageToDriver(message);
}


/// Show operation invalid message and exit with -1
void invalidOperation(options_description *desc, std::string err) {
    std::cout << "Error: Invalid operation";
    if (err.length() > 0) {
        std::cout << ": " << err;
    }
    std::cout << std::endl;
    
    showAppInfo();
    std::cout << (*desc) << std::endl;
    
    exit(-1);
}


int main(int argc, char* argv[]) {
    options_description desc("Allowed options");
    desc.add_options()
        ("mode,m", value<std::string>(), "Set laptop fan work mode [normal, boost]")
        ("auto,a", value<std::string>(), "Toggle auto adjust fan speed [true, false]")
        ("level,l", value<int>(), "Set fan speed level manually [0-5]")
        ("reload,r", "Send reload fan controlling config request to daemon process")
        ("help,h", "Display help message");
    
    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);
    
    if (vm.count("help")) {
        showAppInfo();
        std::cout << desc;
        return 0;
    }
    
    if (vm.count("mode")) {
        std::string fanMode = vm["mode"].as<std::string>();
        
        if (fanMode == "normal")
            setFanMode(FanModeNormal);
        else if (fanMode == "boost")
            setFanMode(FanModeBoost);
        else
            invalidOperation(&desc, "unrecognized mode: " + fanMode);
        
        return 0;
    }
    
    if (vm.count("level")) {
        int speedLevel = vm["level"].as<int>();
        
        if (speedLevel < 0 || speedLevel > 5) {
            invalidOperation(&desc, "speedLevel should be in range [0, 5]");
            return -1;
        }
        
        setFanSpeed(speedLevel);
        return 0;
    }
    
    if (vm.count("auto")) {
        std::string autoAdjust = vm["auto"].as<std::string>();
        toggleAutoAdjustFanSpeed(autoAdjust == "true");
        return 0;
    }
    
    if (vm.count("reload")) {
        reloadFanConfig();
        return 0;
    }
    
    invalidOperation(&desc, emptyError);
}
