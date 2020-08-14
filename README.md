# Sloppy
Sloppy is a fixed sized cache memory allocator.
Based on the Slab memory allocation, designed by Bonvick.

references : His paper [here](https://pdfs.semanticscholar.org/1acc/3a14da69dd240f2fbc11d00e09610263bdbd.pdf?_ga=2.249523655.1104392717.1591767251-111770065.1590953620) and Wikipedia [Article](https://en.wikipedia.org/wiki/Slab_allocation)

## Setup instructions

**Download source code :**
 * `git clone https://github.com/RohanVDvivedi/Sloppy.git`

**Build from source :**
 * `cd Sloppy`
 * `make clean all`

**Install from the build :**
 * `sudo make install`
 * ***Once you have installed from source, you may discard the build by*** `make clean`

## Using The library
 * add `-lsloppy -lpthread -lcutlery` linker flag, while compiling your application
 * do not forget to include appropriate public api headers as and when needed. this includes
   * `#include<cache.h>`

## Instructions for uninstalling library

**Uninstall :**
 * `cd Sloppy`
 * `sudo make uninstall`