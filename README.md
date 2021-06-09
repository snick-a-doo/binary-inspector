# Inspect
A tool for finding interesting values in binary files.

The bytes in a given binary file could represent anything. Any four adjacent bytes could be a 32-bit integer, any eight bytes could be a 64-bit float, etc. But most of these values wouldn't be interesting if they weren't intended to encode nimbers. Inspect lets you pick the types and ranges you're interested in and then in shows the values and their byte offsets. Here's an example that finds 32-bit ints from -1000 to 1000, 64-bit floats from -1e6 to 1e6, and narrow strings 3 to 64 characters long. Note the compressed display of the run of zeroes from 0x48 to 0x50.

0000000 0                 f64 1.230000
                8         i32 432
                    c     s8  ÿîÿîÿîÿ
0000001     4   8         s8  moo
                    c     s8  wæeþing wïlløw
0000002            b      s8  first
0000003  1                s8  second
                8     e   s8  third
0000004    3              i32 -256
            4             i32 -1
               7          i32 255
                89abcdef  i32 0
0000005 0                 i32 0
           3              i32 256
            4             i32 1
