# Sloppy
Sloppy is a fixed sized cache memory allocator.
Based on the Slab memory allocation, designed by Bonvick.

references : His paper [here](https://pdfs.semanticscholar.org/1acc/3a14da69dd240f2fbc11d00e09610263bdbd.pdf?_ga=2.249523655.1104392717.1591767251-111770065.1590953620) and Wikipedia [Article](https://en.wikipedia.org/wiki/Slab_allocation)

setup instructions
 * git clone https://github.com/RohanVDvivedi/Sloppy.git
 * cd Sloppy
 * sudo make clean install
 * add "-lsloppy" linker flag, while compiling your application