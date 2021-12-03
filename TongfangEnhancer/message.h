#ifndef MESSAGE_H
#define MESSAGE_H

struct WMIActionMessage {
    int type;
    int arg1;
    int arg2;
};

enum IOUserClientSelectorCode {
    kClientSelectorDispatchCommand,
};

#endif
