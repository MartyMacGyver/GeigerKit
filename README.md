# DIY GeigerKit experimentation fork

A fork of the [DIY GeigerKit](https://sites.google.com/site/diygeigercounter/home) firmware for experimentation and extension of functionality.

Visit the [DIY GeigerKit online store](https://sites.google.com/site/diygeigercounter/buy-the-kit-1) to order kits, parts, and more!

All files and notes provided for personal use only, as-is with no warranty.

I've built this successfully with Arduino 1.8.9 on Windows.

Ensure the libraries in `libraries_GK-B5` are in your Arduino libraries directory prior to building:

 * DogLCD is from [Gadgetoid](https://github.com/Gadgetoid/doglcd-arduino)
 * MeetAndroid is from [Amarino](http://www.amarino-toolkit.net/index.php/download.html) - specifically [`MeetAndroid_4.zip`](https://code.google.com/archive/p/amarino/downloads)
 * PinChangeInt is from [Google](https://code.google.com/archive/p/arduino-pinchangeint/) originally - then it moved to [GreyGnome's repo](https://github.com/GreyGnome/PinChangeInt) which is now deprecated in favor of [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt).

If you have problems with Arduino's GCC throwing a seg fault and failing compilation, [see this thread for a workaround](https://github.com/arduino/Arduino/issues/7949#issuecomment-492685178).
