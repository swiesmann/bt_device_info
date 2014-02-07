# bt_device_info

This is a small tool for listing all your bluetooth devices and their version/features/configuration/state as avaialble by the [Bluez bluetooth api](http://www.bluez.org). Actually a lot of the code was merged from various Bluez tools and library files (especially [tools/hcitool.c](http://git.kernel.org/cgit/bluetooth/bluez.git/tree/tools/hcitool.c), [tools/hcieventmask.c](http://git.kernel.org/cgit/bluetooth/bluez.git/tree/tools/hcieventmask.c) and [lib/hci.c](http://git.kernel.org/cgit/bluetooth/bluez.git/tree/lib/hci.c) were very helpful). I created this little tool since I wanted to know about the properties and features of bluetooth devices I have and I couldn't find a tool that gave me EVERYTHING there is to probe for programmatically. Also, I wanted to get to know the Bluez API because I intend to write code for bluetooth low energy. 

## NOTE
Accessing the C API is not the recommended way to interact with bluetooth devices on Linux. Bluez has a DBUS server that exposes the API. It seems all of the required BLE functionality is already in the C API - but I don't know if the DBUS part is fully implemented.

## Build
**Prerequisites**: You need the `gcc` and the bluetooth headers installed. I.e. on Ubuntu do
```bash
$ sudo apt-get install build-essential libbluetooth-dev
```

Then to build:
```bash
$ gcc bt_device_info.c -o bt_device_info -lbluetooth
```

## Run
You can run `./bt_device_info --help` to see all the options:
```bash
$ ./bt_device_info --help

Usage: ./bt_device_info [OPTION]
Prints driver-level information about the Bluetooth devices connected to this machine. Uses code from the Bluez project (http://www.bluez.org/).

Available options:

  -v, --verbose              print more details about the adapter
  -u. --unsupported          also show bluetooth featured this adapter does not support
                             or are simply not activated/active at the moment
                             (will be marked as not supported; implies --verbose)
  -c, --color                colorized output for improved readability
  -h, --help                 this text
```

### Show a short summary
Just run `bt_device_info` without any options (`--color` is optional).
```bash
$ ./bt_device_info --color
```

**Output:**

![Output of "./bt_device_info --color"](https://github.com/swiesmann/bt_device_info/blob/master/readme_images/bt_device_info_normal.png?raw=true "Output of "./bt_device_info --color")




### Show more details
**--verbose** will give you details like spported bluetooth version and features about the adapter.
```bash
./bt_device_info ./bt_device_info --color --verbose
```

**Output:**

![Output of "./bt_device_info --color --verbose"](https://github.com/swiesmann/bt_device_info/blob/master/readme_images/bt_device_info_verbose.png?raw=true "Output of "./bt_device_info --color --verbose")

**--unsupported** also lists features that the adapter does not support. This is just to get an overview of all the data you can access through the Bluez API. You can also request most of this information from a remote device (not using this tool, though). Unspported/unavailable features will be marked with a `0` while the working ones are deonoted with a `1`.
```bash
./bt_device_info --color --verbose --unsupported
```

**Output:**

![Output of "./bt_device_info --color --verbose --unsupported"](https://github.com/swiesmann/bt_device_info/blob/master/readme_images/bt_device_info_unsupported.png?raw=true "Output of "./bt_device_info --color --verbose --unsupported")
