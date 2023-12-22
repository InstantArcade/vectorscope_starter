# Hackaday Supercon Vectorscope badge starter for C++
## by Bob Hickman (Instant Arcade)

If you want to bypass the MicroPython and Vector OS, here's a project that interfaces with the hardware of the badge directly in C++.
It is set up as a PlatformIO project and supports both the Hackaday Supercon Vectorscope badge and the [Waveshare RP2040 Touch LCD](https://www.waveshare.com/rp2040-touch-lcd-1.28.htm) if you don't have or don't want to use your badge.

It demonstrates some scrolling text that conforms to the circular form factor of the screen, and you can specify the radius at which to draw and the starting angle.

**Note** that the angles are in degrees, and 0 degrees is to the right, increasing as you move clockwise.

There are examples for writing text that is in *full* color (RGB565 packed - 2 bytes per pixel) and also 1-bit mono. Fonts are also supplied along iwth the source asset PNG files.

In the tools folder there's a handy Python script (img2header.py) that can convert to a few different formats including the native RGB565 packed and byte-swapped format the display uses natively.

Example use: `img2header.py vectorscope_starter/assets/amigafont_wood.png amiga_wood.h amiga_wood --rgb565swapped`

Have fun and do what you want with the code. I'm looking forward to seeing what you make with it.

Cheers,

Bob (Instant Arcade)

P.S. Thanks to Ben for his great [Vector Video](https://github.com/unwiredben/vector-video) that was the starting point for all of this.
