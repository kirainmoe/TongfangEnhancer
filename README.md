# TongfangEnhancer

The final piece of the puzzle: A macOS kernel extension enables hotkey and fan control support for Tongfang laptops.

## Components

This driver currently consists of `VoodooWMI.kext`, `TongfangEnhancer.kext` and `TongfangEnhancerDaemon` and `FanCLI`.

### VoodooWMI

A Windows Management Instrumentation platform driver for macOS.

### TongfangEnhancer

This kernel extension provides interfaces for interacting with WMI through VoodooWMI, and implements handlers like hotkey and fan control.

### TongfangEnhancerDaemon

This application handles hotkey or fan control requests sent by users. It also takes a part in OSD and adjusting the fan control speed curve.

### FanCLI

A command line interface to control the fan speed manually. 

## Credits

- [VoodooWMI](https://github.com/Goshin/VoodooWMI/)

## License

TongfangEnhancer is licensed under GNU Public Licence V2.
