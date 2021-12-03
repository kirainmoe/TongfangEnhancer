//
//  main.m
//  mainly from AsusSMCDaemon (MIT License)
//
//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <CoreWLAN/CoreWLAN.h>
#import <CoreServices/CoreServices.h>
#import <sys/ioctl.h>
#import <sys/socket.h>
#import <dlfcn.h>
#import <sys/kern_event.h>

#import "BezelServices.h"
#import "OSD.h"
#import "common.h"
#import "message.h"
#include "FanControl.h"

extern void RunApplicationEventLoop(void);

// requires IOBluetooth.framework
extern void IOBluetoothPreferenceSetControllerPowerState(int);
extern int IOBluetoothPreferenceGetControllerPowerState(void);

void dispatchMessage(struct WMIActionMessage *message);

static void *(*_BSDoGraphicWithMeterAndTimeout)(CGDirectDisplayID arg0, BSGraphic arg1, int arg2, float v, int timeout) = NULL;


bool _loadBezelServices() {
    // Load BezelServices framework
    void *handle = dlopen("/System/Library/PrivateFrameworks/BezelServices.framework/Versions/A/BezelServices", RTLD_GLOBAL);
    if (!handle) {
        NSLog(@"Error opening framework");
        return NO;
    } else {
        _BSDoGraphicWithMeterAndTimeout = dlsym(handle, "BSDoGraphicWithMeterAndTimeout");
        return _BSDoGraphicWithMeterAndTimeout != NULL;
    }
}

bool _loadOSDFramework() {
    return [[NSBundle bundleWithPath:@"/System/Library/PrivateFrameworks/OSD.framework"] load];
}

void showBezelServices(BSGraphic image, float filled) {
    CGDirectDisplayID currentDisplayId = [NSScreen.mainScreen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
    _BSDoGraphicWithMeterAndTimeout(currentDisplayId, image, 0x0, filled, 1);
}

void showOSD(OSDGraphic image, int filled, int total) {
    CGDirectDisplayID currentDisplayId = [NSScreen.mainScreen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
    [[NSClassFromString(@"OSDManager") sharedManager] showImage:image onDisplayID:currentDisplayId priority:OSDPriorityDefault msecUntilFade:1000];
}

void showKBoardBLightStatus(int level, int max) {
    if (_BSDoGraphicWithMeterAndTimeout != NULL) {
        // El Capitan and probably older systems
        if (level)
            showBezelServices(BSGraphicKeyboardBacklightMeter, (float)level/max);
        else
            showBezelServices(BSGraphicKeyboardBacklightDisabledMeter, 0);
    } else {
        // Sierra+
        if (level)
            showOSD(OSDGraphicKeyboardBacklightMeter, level, max);
        else
            showOSD(OSDGraphicKeyboardBacklightDisabledMeter, level, max);
    }
}

BOOL airplaneModeEnabled = NO, lastWifiState;
int lastBluetoothState;
void toggleAirplaneMode() {
    airplaneModeEnabled = !airplaneModeEnabled;

    CWInterface *currentInterface = [CWWiFiClient.sharedWiFiClient interface];
    NSError *err = nil;

    if (airplaneModeEnabled) {
        showOSD(OSDGraphicNoWiFi, 0, 0);
        lastWifiState = currentInterface.powerOn;
        lastBluetoothState = IOBluetoothPreferenceGetControllerPowerState();
        [currentInterface setPower:NO error:&err];
        IOBluetoothPreferenceSetControllerPowerState(0);
    } else {
        showOSD(OSDGraphicHotspot, 0, 0);
        [currentInterface setPower:lastWifiState error:&err];
        IOBluetoothPreferenceSetControllerPowerState(lastBluetoothState);
    }
}

void switchDisplayMode() {
    // open last Privacy subpane viewed:
    NSURL * url = [NSURL fileURLWithPath:@"/System/Library/PreferencePanes/Displays.prefPane"];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

void lockScreen() {
    void *lib = dlopen("/System/Library/PrivateFrameworks/login.framework/Versions/Current/login", RTLD_LAZY);
    void (*SACLockScreenImmediate)(void) = dlsym(lib, "SACLockScreenImmediate");

    SACLockScreenImmediate();
}

void toggleTouchpad() {
    struct WMIActionMessage message = {.type = kActionToggleTouchpad};
    int result = sendMessageToDriver(message);
    if (result != -1) {
        if (result) {
            showOSD(OSDGraphicKeyboardBacklightDisabledMeter, 0, 0);
        } else {
            showOSD(OSDGraphicKeyboardBacklightDisabledNotConnected, 0, 0);
        }
    }
}

OSStatus onHotKeyEvent(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData) {
    EventHotKeyID eventId;
    GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(eventId), NULL, &eventId);

    printf("[TongfangEnhancerDaemon] onHotKeyEvent: ActionID %d\n", eventId.id);
    struct WMIActionMessage message = {.type = eventId.id};
    dispatchMessage(&message);

    return noErr;
}

void registerHotKeys() {
    EventHotKeyRef eventHotKeyRef;
    EventHotKeyID eventHotKeyID;
    EventTypeSpec eventType;
    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;

    InstallApplicationEventHandler(&onHotKeyEvent, 1, &eventType, NULL, NULL);

    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("TongfangEnhancerDriver"));
    if (service == IO_OBJECT_NULL) {
        printf("[TongfangEnhancerDaemon] could not find any services matching\n");
        return;
    }
    CFTypeRef platform_dict_ref = IORegistryEntryCreateCFProperty(service, CFSTR("Platform"), kCFAllocatorDefault, 0);
    if (platform_dict_ref) {
        CFTypeRef events_array_ref;
        if (CFDictionaryGetValueIfPresent(platform_dict_ref, CFSTR("PlainHotkeys"), &events_array_ref)) {
            for (int i = 0; i < CFArrayGetCount(events_array_ref); i++) {
                CFTypeRef event_dict_ref = CFArrayGetValueAtIndex(events_array_ref, i);
                CFTypeRef keycode_num_ref = CFDictionaryGetValue(event_dict_ref, CFSTR("KeyCode"));
                CFTypeRef modifiers_num_ref = CFDictionaryGetValue(event_dict_ref, CFSTR("Modifiers"));
                CFTypeRef action_num_ref = CFDictionaryGetValue(event_dict_ref, CFSTR("ActionID"));
                CFTypeRef note_string_ref = CFDictionaryGetValue(event_dict_ref, CFSTR("Note"));
                uint32_t keycode, modifiers, action_id;
                CFNumberGetValue(keycode_num_ref, kCFNumberIntType, &keycode);
                CFNumberGetValue(modifiers_num_ref, kCFNumberIntType, &modifiers);
                CFNumberGetValue(action_num_ref, kCFNumberIntType, &action_id);
                printf("[TongfangEnhancerDaemon] Registering Hotkey: (key: 0x%x, mod: 0x%x, action: 0x%x) %s\n",
                       keycode, modifiers, action_id, CFStringGetCStringPtr(note_string_ref, 0));
                eventHotKeyID.id = action_id;
                RegisterEventHotKey(keycode, modifiers, eventHotKeyID, GetApplicationEventTarget(), 0, &eventHotKeyRef);
            }
        }
        CFRelease(platform_dict_ref);
    }
    IOObjectRelease(service);
}

void dispatchMessage(struct WMIActionMessage *message) {
    printf("[TongfangEnhancerDaemon] type:%d x:%d y:%d\n", message->type, message->arg1, message->arg2);

    switch (message->type) {
        case kActionSleep:
        case kActionScreenBrightnessDown:
        case kActionScreenBrightnessUp:
            sendMessageToDriver(*message);
            break;
        case kActionLockScreen:
            lockScreen();
            break;
        case kActionToggleAirplaneMode:
            toggleAirplaneMode();
            break;
        case kActionSwitchScreen:
            switchDisplayMode();
            break;
        case kActionToggleTouchpad:
            toggleTouchpad();
            break;
        case kActionKeyboardBacklightDown:
            showOSD(OSDGraphicKeyboardBacklightMeter, 0, 0);
            break;
        case kActionKeyboardBacklightUp:
            showOSD(OSDGraphicKeyboardBacklightMeter, 0, 0);
            break;
        case kActionSetMode:
            setFanMode((enum FanControlMode) message->arg1);
            break;
        case kActionSetSpeed:
            setFanSpeed(message->arg1);
            break;
        case kActionReloadFanConfig:
            reloadFanControlConfig();
            break;
        default:
            printf("[TongfangEnhancerDaemon] unknown type %d\n", message->type);
    }
}

void kernelMessageLoop() {
    int systemSocket = -1;

    // create system socket to receive kernel event data
    systemSocket = socket(PF_SYSTEM, SOCK_RAW, SYSPROTO_EVENT);

    // struct for vendor code
    struct kev_vendor_code vendorCode = {0};
    strncpy(vendorCode.vendor_string, KERNEL_EVENT_VENDOR_ID, KEV_VENDOR_CODE_MAX_STR_LEN);

    // get vendor name -> vendor code mapping
    // ->vendor id, saved in 'vendorCode' variable
    ioctl(systemSocket, SIOCGKEVVENDOR, &vendorCode);

    // struct for kernel request
    struct kev_request kevRequest = {0};
    kevRequest.kev_class = KEV_ANY_CLASS;
    kevRequest.kev_subclass = KEV_ANY_SUBCLASS;

    // tell kernel what we want to filter on
    ioctl(systemSocket, SIOCSKEVFILT, &kevRequest);

    // bytes received from system socket
    ssize_t bytesReceived = -1;

    // message from kext
    // ->size is cumulation of header, struct, and max length of a proc path
    char kextMsg[KEV_MSG_HEADER_SIZE + sizeof(struct WMIActionMessage)] = {0};

    struct WMIActionMessage *message = NULL;

    while (YES) {
        bytesReceived = recv(systemSocket, kextMsg, sizeof(kextMsg), 0);

        if (bytesReceived != sizeof(kextMsg)) {
            continue;
        }

        // struct for broadcast data from the kext
        struct kern_event_msg *kernEventMsg = {0};
        kernEventMsg = (struct kern_event_msg*)kextMsg;

        if (KERNEL_EVENT_CODE != kernEventMsg->event_code) {
            continue;
        }

        message = (struct WMIActionMessage*)&kernEventMsg->event_data[0];

        dispatchMessage(message);
    }
}

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        printf("[TongfangEnhancerDaemon] daemon started...\n");

        if (!_loadBezelServices()) {
            _loadOSDFramework();
        }

        registerHotKeys();

        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
            kernelMessageLoop();
        });
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
            reloadFanControlConfig();
            monitorCPUTemperature(30);
        });

        RunApplicationEventLoop();
    }

    return 0;
}
