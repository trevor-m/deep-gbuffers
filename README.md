Trevor Morris
CS280, W17

Final Project: 
Mara et al: Fast Global Illumination Approximations on Deep G-Buffers
See report.pdf for more information

To build project:
```bash
# UNIX Makefile
cmake ..

# Mac OSX
cmake -G "Xcode" ..

# Microsoft Windows
cmake -G "Visual Studio 14" ..
cmake -G "Visual Studio 14 Win64" ..
...
```

Display Modes:
Press 7 to view scene
Press 8 to view SSAO buffer
Press R to toggle single-scatter radiosity on/off (default: off)


Additional Controls:
Lights:
Press 1 to select the first light
Press 2 to select the second light
Press 3 to select the third light
Press J and L to move the selected light source along X axis
Press I and K to move the selected light source along Z axis
Press U and O to move the selected light source along Y axis
