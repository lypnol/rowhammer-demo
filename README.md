# Rowhammer demo attempt

This is an attempt to implement a rowhammer attack on a binary.  

## Compile

Use `make` to compile all

## How it works

* `rowhammer.cc`: Will check for vunlerable memory regions by hammering random pages 
* `hammer.cc`: Tries to allocate a space close to given binary pages and hammers everything within it
* `dumbp.cc`: Produces very large binary that could be used as a test target binary

`hammer 1 /path/to/dumbp` will try a rowhammer attack on `dumbp` binary by allocating 1 GB of memory (hopefully next to it).  
It uses `mmmap` to allocate this space with the binary first page as an address hint see [http://man7.org/linux/man-pages/man2/mmap.2.html](http://man7.org/linux/man-pages/man2/mmap.2.html).