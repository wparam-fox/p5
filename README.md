# p5
Windows explorer shell replacement

This is the 5th major revision of a windows shell replacement which was once known as "pixshell", short for "pixel shell".  In the early days, it was a single pixel in one corner of the screen that would bring up a launcher menu when clicked.  Over time it acquired many features, was eventually re-written to have plugins, but never had a respectable scripting language.  This version gave it a proper parsed language derived from scheme, and had "check for, catch, and handle *all* errors" baked in as a core principle.

In the end, not all parts of pixshell were converted over to p5, and it was always necessary to run both side-by-side to get access to all of the functionality.  Usage never caught on with anyone besides me, so the need to clean up and move over all of the functionality in the previous versions never materialized.

This program was in steady use from 2005 to 2014, when a few select bits were ported to linux, and then mac (specifically the key sequence launcher found in keyboard/pause.c).

It will not build in modern windows environments; it was written entirely in visual studio 6, and probably only builds there.  This version was never updated to be 64-bit ready; only the mac version runs in 64-bit mode.  The `__asm` parts inside of `Common/wp_scheme2.c` were especially interesting to port to the 64-bit calling convention.

Posting it here mostly for historical reasons.

