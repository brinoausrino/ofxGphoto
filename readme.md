# ofxGphoto

ofxGphoto wraps [libgphoto](https://github.com/gphoto/libgphoto2) to [openFrameworks](https://openframeworks.cc/). It is oriented close to [ofxEdsdk](https://github.com/kylemcdonald/ofxEdsdk) both in forms of usage and internal workflow.

libgphoto is a C library that enables the communication with a [lot of digital cameras](http://www.gphoto.org/proj/libgphoto2/support.php).

## Installation/Usage on Linux

### Install libgphoto.

Arch linux
```
sudo pacman -S libgphoto2 
``` 

Debian based
```
sudo apt-get install libgphoto2
```

### Create a project

Create a new project using the project generator. 

Add linker flags `-lgphoto2`,`-lgphoto2_port`

using qt
```
in projectName.qbs set


of.linkerFlags: ['-lgphoto2','-lgphoto2_port']      // flags passed to the linker
```

ofxGphoto is tested with libgphoto 2.5.26, on Arch Linux release 2021.02.10 with openFrameworks 0.11 and up. Any afford to make it work on other operating Systems is highly welcome.
