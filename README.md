[![Release](https://img.shields.io/github/v/release/austinnixholm/ThreeFingerDrag?label=Download%20version)](https://github.com/austinnixholm/ThreeFingerDrag/releases/latest)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![GitHub all releases](https://img.shields.io/github/downloads/austinnixholm/threefingerdrag/total)

[![TFD-GH-DARK-512.png](https://i.postimg.cc/RZXdx8qh/TFD-GH-DARK-512.png)](https://postimg.cc/hXQT92LR#gh-dark-mode-only)
[![TFD-GH-LIGHT-512.png](https://i.postimg.cc/VsnFk553/TFD-GH-LIGHT-512.png)](https://postimg.cc/14RFWmGH#gh-light-mode-only)

Emulates the macOS three-finger-drag feature on your Windows precision touchpad. Simply install and run, minimal setup required.

## Features

* Easily configurable 
* Lightweight and minimalistic design
* Accessible menu through the system tray

## Usage

1. Download & install the [latest release](https://github.com/austinnixholm/ThreeFingerDrag/releases/latest) from the repository.
3. Launch the program, and you will be prompted to run ThreeFingerDrag on Windows startup. This is optional, and you can always choose to enable or disable this feature later.
4. ThreeFingerDrag will now run in the background of the system. You can configure the program through the system tray menu, which is accessible by clicking the ThreeFingerDrag icon in the notification area of the taskbar.

<p align="center">
  <img src="https://i.postimg.cc/mgRCDSK9/Three-Finger-Drag-XW25-QZPTg-P.png"/> 
</p>

**NOTE:** It's recommended to remove any existing three-finger swipe gestures within `Touchpad Settings` in order to prevent possible interference.

## Build with Visual Studio

You can build the project using Visual Studio 2017 or newer.

1. Open the ThreeFingerDrag.sln file in Visual Studio.
2. Select the "Release" configuration and click "Build Solution".
3. The built executable will be located in the `/build/` directory in the base project folder.

## Create Installer via [Inno Setup](https://jrsoftware.org/isinfo.php)

1. After building the executable, locate the `inno_script.iss` file in the base project folder.
2. Right click on the inno_script.iss file and select "Compile" or open the file in the InnoSetup editor and compile it from there.
3. The created installer executable will be located in the `/installers/` directory in the base project folder.

## Credits

* [mohabouje/WinToast][1] - A library for displaying toast notifications on Windows 8 and later.
* [pulzed/mINI][2] - A library for manipulating INI files.

[1]: https://github.com/mohabouje/WinToast
[2]: https://github.com/pulzed/mINI/
