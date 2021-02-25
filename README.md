# The MB Rewind Project

These are the source files required to build the Rewind plugin along with a few other plugins used by the games using rewind.

## Build

Build using CMake.  
Visual Studio 2019 required for Windows.  
Xcode 9.4.1 for Mac.

To build the plugin.

```
cmake -DMBPBUILD:BOOL=OFF CMakeLists.txt
cmake  --build . --target Rewind FrameRateUnlock
```

To build the plugin for MBP-Rewind

```
cmake -DMBPBUILD:BOOL=ON CMakeLists.txt
cmake  --build . --target Rewind FrameRateUnlock DiscordRPC
```
