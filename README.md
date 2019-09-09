# DIY GeigerKit experimentation fork

A fork of the [DIY GeigerKit](https://sites.google.com/site/diygeigercounter/home) firmware for experimentation and extension of functionality.

Visit the [DIY GeigerKit online store](https://sites.google.com/site/diygeigercounter/buy-the-kit-1) to order kits, parts, and more!

All files and notes provided for personal use only, as-is with no warranty.

Built successfully with Arduino 1.8.9 on Windows. Note: If you have sporadic segfault problems with Arduino's GCC causing compilation to fail, [see this thread for a workaround](https://github.com/arduino/Arduino/issues/7949#issuecomment-492685178).

Requires the _EnableInterrupt_ and _LiquidCrystal_ libraries (install via the Library Manager).

For certain configurations, add the following libraries in `libraries_GK-B5` are in your Arduino libraries directory prior to building:

 * OPTIONAL - DogLCD is from [Gadgetoid](https://github.com/Gadgetoid/doglcd-arduino) - [The (apparent) datasheet](https://www.lcd-module.de/eng/pdf/grafik/dogm128e.pdf)
 * OPTIONAL - MeetAndroid is from [Amarino](http://www.amarino-toolkit.net/index.php/download.html) - specifically [`MeetAndroid_4.zip`](https://code.google.com/archive/p/amarino/downloads)
 * DEPRECATED - PinChangeInt is from [Google](https://code.google.com/archive/p/arduino-pinchangeint/) originally - then it moved to [GreyGnome's repo](https://github.com/GreyGnome/PinChangeInt) which is now deprecated in favor of [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt).

