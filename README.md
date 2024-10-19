# milkdrop2020
Portable version of milkdrop2 (a music visualizer)

Source code originally released by Ryan Geiss <guava@geisswerks.com>

More information on milkdrop2 can be found here: [http://www.geisswerks.com/about_milkdrop.html](http://www.geisswerks.com/about_milkdrop.html)

Click [here](http://m1lkdr0p.com) for a live running demo running via emscripten + webgl.

This work is in no way connected with the author or creators of Winamp.


# Latest Setup Guide

Get the EMSDK up and running following these instructions: https://emscripten.org/docs/getting_started/downloads.html

After installation make sure to install the last known working version that compiles properly: 

```
emsdk install 3.0.1
emsdk activate 3.0.1
```
**NOTE:** Make sure that your environment has the proper emsdk activated before running a build. You can check the version of the compiler you're using with `emcc --version`

Run a build:

```
./cmake-build-emscripten.sh
```

If you see any build issues complaining about missing ZLIB library, try running the makefile within app/emscripten:

```
make -f app/emscripten/Makefile
```

It's fine if it fails..it will download the appropriate zlib library and can now be used by the cmake build script.


TODO:

- Refactor Visualizer Class to a bare minimal visualizer that can compile to latest emsdk version