# Rowhammer demo attempt

This is an attempt to implement a rowhammer attack on a binary.  

## Compile

Use `make` to compile all

## How it works

* `rowhammer.cc`: Will check for vunlerable memory regions by hammering random pages 
* `hammer.cc`: Tries to allocate a space close to given binary pages and hammers everything within it
* `dumbp.cc`: Produces very large binary that could be used as a test target binary

