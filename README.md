# Linux Frame Buffer (fbdev) Projects!

--- 

## Overview 
- The linux framebuffer is a linux subsystem used to show graphical content on the system console. 
- The frame buffer device provides an abstraction for the graphics hardware of a computer. It represents the frame buffer of some video hardware and allows application software to access the graphics hardware through a well-defined interface, so the software needs not to know anything about the register shit. 
- THe device is accessible trough specific nodes, usually found in the /dev directory
> example would be /dev/fb* 

### User's view of the dev/fb* 
- From the user's POV, the frame buffer would look like any other device in /dev, however its a character device using a majroity 29; the minor specifies the frame buffer number. By conventio nthe following device nodes are used: 
```bash
0 = /dev/fb0    First Frame Buffer
1 = /dev/fb1    Second Frame Buffer
    ...
31 = /dev/fb31  32nd Frame Buffer
```
For backwards compatibility, you may want to create the following symbolic links:
```bash
/dev/fb0current -> fb0
/dev/fb1current -> fb1
```
and so on... 

The frame buffer devices are also *normal* memory devices, this means, you can read and write their content. You can, for example, make a screen snapshot by: 
```bash
cp /dev/fbX myfile
```
There also can be more than one frame buffer at a time
> If you have a graphics card in addition to the build-in hardware. The corresponding frame buffer devices (/dev/fb0 and /dev/fb1, etc) work independently of the rest. 
Application software that uses the frame buffer device (e.g the X server) will use /dev/fb0 by default (Older software uses /dev/fb0current). You can specificy an alterantive frame buffer device by setting the environmental variable $FRAMEBUFFER to the path name of a frame buffer device, (e.g for sh/bash users):
```bash
export FRAMEBUFFER=/dev/fb1
```
or (for csh users):
```bash
setenv FRAMEBUFFER /dev/fb1 
```
After this the X server will use the second frame buffer (Index starts from 0). 

### Programmer's Views of /dev/fb*
As said, the frame buffer device is a memory device, much like /dev/mem, and it has the saem features. You can read it, you can write it, you can seek to some locaton in it and mmap() it (the main usage). The difference is that the memory that appears in the special file is not the whole memory, but the frame buffer or some video hardware. 

/dev/fb* also allows several ioctls on it, which lots of information about the hardware can be queried and set. The color map handling works via ioctls, too. Look into <linux/fb.h> for more information on what ioctls exist and which data structures they work. Here is a brief overview: 
    * You can request unchangeable information about the hardware, like name, organization of the screen memory (planes, packed pixels, ...) and address and length of the screen memory.
    * You can request and change variable information about the hardware, like visible and virtual geometry, depth, color map format, timing, and so on. If you try to change that information, the driver maybe will round up some values to meet the hardware’s capabilities (or return EINVAL if that isn’t possible).
    * You can get and set parts of the color map. Communication is done with 16 bits per color part (red, green, blue, transparency) to support all existing hardware. The driver does all the computations needed to apply it to the hardware (round it down to less bits, maybe throw away transparency).

All of this hardware abstraction makes the implementation of application programs easier and mroe portable. E.G the X server works completely on /dev/fb* and thus does not need t oknow, for example, how the color registers of the concrete hardware are organizaed. XF68_FBDev is a general X server for bitmapped. unaccelerated, video hardware. The only thing that has to be built into application programs is the screen organization (bitplanes or chunky pixels etc.), because it works on the frame buffer image data directly.

For the fututure, it is planned that frame buffer drvers for graphic cards and the like can be implemented as kernel modules that are loaded at runtime. Such a driver would just have to call register_framebuffer() and supply some functions. Writing and distributing such drivers independently from the kernel will save much trouble... 

### Frame Buffer Resolution Maintencance: 
Frame buffer resolutions are maintained using the utility fbset. It can change the video mode properties of a frame buffer device. Its main usage is to change the current video mode, e.g during boot up in one of your /etc/rc* or /etc/init.d/* files

fbset uses a video mode database stored in a config file, so you can easily add your own modes and refer to them with a simple identifier. 

### The X server. 

THe X server (XF86_FBDev) is the most notable application programe for the frame buffer device. Starting with XFree86 release 3.2, the X server is part of XFree86 and has 2 modes:
* If the *display* subsection for the fbdev driver is in the /etc/XF86Config file contains a:
    ```bash
    Modes "default"
    ```
    line, the X server will use the scheme discussed above, i.e. it will start up in the resolution determined by /dev/fb0 (or $FRAMEBUFFER, if set). You still have to specify the color depth (using the Depth keyword) and virtual resolution (using the Virtual keyboard) though. THis is the default for the configuration file supplied with XFree86. It's the most simple configuration, but it has some limitations. 
* Therefor it's also possible to specify resolutions in the /etc/XF86Conf file. This allows for on-the-fly resolution switching while retaining the same virtual desktop size. The frame buffer device that's uses is still /dev/fb0current (or $FRAMEBUFFER), but the available resolutions are defined by /etc/XF86Config now. The disadvantage is that you will have to specify the timings in a different format (But using fbset -x may help). 
To tune a video mode, you can use fbset or xvidtune. Note that xvidtune doesn't work 100% with XF68_FBDev: the reported clock values are always incorrect. 

### Video Mode Timings: 

A monitor draws an image on the screen by using an electron beam (3 electron beams for color models, 1 electron beam for monochrome monitors). The front of the screen is covered by a pattern of colored phosphors (pixels). If a phosphor is hit by a nelectron, it emits a photon, and thus becoems visible to the human eye. 

The eectron beam draws horizontal lines (scanlines) from left to right, and from the top to the bottom of the screen. By modifying the intensity of the electron beam, pixels with various colors and intensities can be shown. 

After each scanline the electron beam has to move back to the left side of the screen and to the next line, this is called the horizontral retrace. After the whole screen (frame) was painted, the beam moves back to the upper left corner, this is called the vertical retrace. During both the horizontal, and vertical retrace, the electron beam is turned off (blanked). 

The speed at which the electron beam paints the pixels is determined by the dotclock in the graphics board. For a dotclock of e.g 28.37516 MHz (millions of cycles per second), each pixel is 35242 ps (picoseconds) long: 
```math 
1 / (28.37516E6 Hz) = 35.242E-9 seconds 
```
If the screen resolution is 640x480 it will take: 
```math
640*35.242E-9 S = 22.555E-6 s 
```
to paint the 640 (xres) pixels on one scanline. But the horizontal retrace also takes time (e.g 272 pixels) so a full scanline takes 
```math
(640+272) * 35.242E-9 s = 32.141E-6 s. 
```
We'll say that the hoizontal scanrate is about 31 kHz: 
```math 
1 / (32.141E-6 s) = 31.113E3 hz 
```
A full screen counts 480 (yres) lines, but we have t oconsider the vertical retrace too (e.g 49 lines) so a full screen will take:
```math 
(480+49) * 32.141E-6 s = 17.002E-3 s
```
The vertical scanrate is about 59 Hz: 
```math
1/(17.002E-3 s) = 58.815 Hz
```
This means that the screen is refreshed about 59 times per second. To have a stable picture, without visible flcikering, vesa recommends a vertical scanrate of at least 72 Hz, but the perceived flicker is very human dependent: some peopple can use 50 Hz without any trouble, while some notice if its less than 80 Hz. 
Sicne the monitor does not know when a new scanline starts, the graphic board will supply a synchronized pulse (horizontal synce or hsync) for each scanline. Similarly it supplies a synchronization pulse (veritcal sync or vscyn) for each new frame. THe position of the image on the screen is influenced by the moments at which the synchronization pulse occur. 
The following picture summarizes all timings. The horizonta lretrace time is the sum of the left margin ,the right margin, and the hsync length, while the vertical trace is the sum of the upper margin, the lower margin, and the vsyn length: 
```ASCII
+----------+---------------------------------------------+----------+-------+
|          |                ↑                            |          |       |
|          |                |upper_margin                |          |       |
|          |                ↓                            |          |       |
+----------###############################################----------+-------+
|          #                ↑                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|   left   #                |                            #  right   | hsync |
|  margin  #                |       xres                 #  margin  |  len  |
|<-------->#<---------------+--------------------------->#<-------->|<----->|
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |yres                        #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                |                            #          |       |
|          #                ↓                            #          |       |
+----------###############################################----------+-------+
|          |                ↑                            |          |       |
|          |                |lower_margin                |          |       |
|          |                ↓                            |          |       |
+----------+---------------------------------------------+----------+-------+
|          |                ↑                            |          |       |
|          |                |vsync_len                   |          |       |
|          |                ↓                            |          |       |
+----------+---------------------------------------------+----------+-------+
```
The frame buffer device expects all horizontal timings in number of dotclocks (in picoseconds, 1E-12 s), and vertical timing in number of scanlines. 

### Converting XFree86 Timing values in frame buffer device timings 
An XFREE86 Mode line consists of the following fields: 
```
    "800x600"     50      800  856  976 1040    600  637  643  666
    < name >     DCF       HR  SH1  SH2  HFL     VR  SV1  SV2  VFL
```
The frame buffer device uses the following fields:
* pixclock: pixel clock in ps (pico seconds)
* left_margin: the time from sync to picture
* right_margin: time from picture to sync
* Upper_margin: time from sync to picture
* Lower_margin: time from picture to sync
* hsync_len: length of horizontal sync
* vsync_len: length of the vertical sync 

1. Pixelclock: 
   xfree: in MHz
   fb: in picoseconds (ps)
   pixclock = 1000000 / DCF
2. Horizontal Timings: 
   left_margin = HFL - SH2
   right_margin = SH1 - HR 
   hsync_len = SH2 - SH1
3. Vertical Timings: 
   upper_margin = VFL - SV2
   lower_margin = SV1 - VR 
   vsync_len = SV2 - SV1

Good examples for VESA timings can be found in the XFree86 source tree under: “xc/programs/Xserver/hw/xfree86/doc/modeDB.txt”.


