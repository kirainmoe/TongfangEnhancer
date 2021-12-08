#ifndef COMMON_HPP
#define COMMON_HPP

// constants
const char WMI_GUID[] = "ABBC0F6F-8EA1-11D1-00A0-C90629100000";

const unsigned int DISPATCH_KEY_EVENT_METHOD_INDEX = 281;

// enums
enum WMIActionType {
    kActionSleep,
    kActionLockScreen,
    kActionSwitchScreen,
    kActionToggleAirplaneMode,
    kActionToggleTouchpad,
    kActionKeyboardBacklightDown,
    kActionKeyboardBacklightUp,
    kActionScreenBrightnessDown,
    kActionScreenBrightnessUp,
    kActionSetMode,
    kActionSetSpeed,
    kActionReloadFanConfig,
    kActionSetAutoAdjustFanSpeed,
};

enum FanControlMode {
    FanModeNormal,
    FanModeBoost,
    FanModeCustom,
};

enum FanControlByte {
    FanByteNormal = 0,                  // 0x00000000
    FanByteBoost = 64,                  // 0x00000040
    FanByteLevel0 = 128,                // 0x00000080
    FanByteLevel1 = 129,                // 0x00000081
    FanByteLevel2 = 130,                // 0x00000082
    FanByteLevel3 = 131,                // 0x00000083
    FanByteLevel4 = 132,                // 0x00000084
    FanByteLevel5 = 133,                // 0x00000085
    FanByteSwitchToCustom = 160,        // 0x000000A0
};

// kernel & events
const int  KERNEL_EVENT_CODE        = 0x8102;
const char KERNEL_EVENT_VENDOR_ID[] = "TongfangEnhancer";


// fan control WMI constant
const unsigned char WMI_INSTANCE_ID     = 0;
const unsigned int  WMI_METHOD_ID       = 4;
const unsigned int  FAN_CONTROL_ADDRESS = 1873;


#endif
