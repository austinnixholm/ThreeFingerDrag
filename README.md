[![Release](https://img.shields.io/github/v/release/austinnixholm/ThreeFingerDrag?label=Download%20version)](https://github.com/austinnixholm/ThreeFingerDrag/releases/latest)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

# ![icon](https://i.ibb.co/JcnqD2W/png64x64-TFD.png)  ThreeFingerDrag

Emulates the MacOS three finger drag functionality on your Windows precision touchpad. Simply install and run, no configuration required.

## Features

* Mimics the same speed as the user's current touchpad settings
* Requires no additional configuration or fine-tuning of sensitivity
* Lightweight and minimalistic design
* Accessible menu through the system tray

## Usage

1. Download & install the [latest release](https://github.com/austinnixholm/ThreeFingerDrag/releases/latest) from the repository.
3. Launch the program, and you will be prompted to run ThreeFingerDrag on Windows startup. This is optional, and you can always choose to enable or disable this feature later.
4. ThreeFingerDrag will now run in the background of the system. You can configure the program through the system tray menu, which is accessible by clicking the ThreeFingerDrag icon in the notification area of the taskbar.

![system tray icon](https://i.ibb.co/1Rb1Gb1/image-2023-05-03-130955903.png) 

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

[1]: https://github.com/mohabouje/WinToast
