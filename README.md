# Implementation of Mara et al: Fast Global Illumination Approximations on Deep G-Buffers
See report.pdf for more information.

<img width="501" alt="rad" src="https://user-images.githubusercontent.com/12981474/31321401-7c47ff12-ac3a-11e7-82f9-b06121992857.png">

Above: Single-scatter radiosity using multiple layers.

Below: Benefit of using multiple layers on McGuire et. al.’s Scalable Ambient Obscurance (2012). Ambient Occlusion from both layers (top), only the first layer (middle) and only the second layer (bottom). The red box highlights an area where the second layer provided a very noticeable correction to an error in a single layer only approach.

![image](https://user-images.githubusercontent.com/12981474/31321412-a0239194-ac3a-11e7-85b0-0ee525f87c45.png)


To build project:
```bash
# UNIX Makefile
cd Glitter
cmake ..

# Mac OSX
cmake -G "Xcode" ..

# Microsoft Windows
cmake -G "Visual Studio 14" ..
cmake -G "Visual Studio 14 Win64" ..
...
```

Display Modes:
* Press 7 to view scene
* Press 8 to view SSAO buffer
* Press R to toggle single-scatter radiosity on/off (default: off)


Additional Controls:
Lights:
* Press 1 to select the first light
* Press 2 to select the second light
* Press 3 to select the third light
* Press J and L to move the selected light source along X axis
* Press I and K to move the selected light source along Z axis
* Press U and O to move the selected light source along Y axis
