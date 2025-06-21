!!! warning "Not Implemented"
    This feature is not currently implemented.

While working with Syriinge, you might ocassionally run into situations where you want to patch a large volume of memory with some data. You could write a function in your plugin that just loads the file from the SD and `memcpy`'s the data to the address you want, but there is a better way.

## Introducing Patches

Patch Files are binary files tagged with metadata such as a target address or addresses and when to apply the patch.

Just like with hooks, Patches can be constructed such that they are only applied when a specific module is loaded or reloaded. 

Likewise, they can also be configured to just apply the patch once.

## Using Patch Files
Patch Files are loaded from the `patches` directory in your build's `pf` folder.

## Creating Patch Files
**TODO**