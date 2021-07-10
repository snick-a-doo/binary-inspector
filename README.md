# Inspect
A tool for finding interesting values in binary files.

The bytes in a given binary file could represent anything. Any four adjacent bytes could be a 32-bit integer, any eight bytes could be a 64-bit float, etc. But most of these values aren't meaningful. Inspect lets you pick the types and ranges you're interested in and then in shows the values and their byte offsets. Here's an example that finds 32-bit ints from -1000 to 1000, 64-bit floats from -1e6 to 1e6, and narrow strings 3 to 64 characters long.

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

The offset is shown on the left. The least siginificant hex digit is displaced to allow quick visaul comparion and to allow compact display of repeated values. Note the compressed display of the run of zeroes from 0x48 to 0x50.

The rest of the line shows the data type and the value.

# Usage
Types and ranges to display are specified on the command line.

    inspect [options] file
        -d --f64=[range] show double-precision floats.
        -f --f32=[range] show single-precision floats.
        -l --i64=[range] show 64-bit integers.
        -i --i32=[range] show 32-bit integers.
        -s --i16=[range] show 16-bit integers.
        -Z --s16=[range] show 2-byte Latin-1 strings.
        -z --s8=[range]  show 1-byte Latin-1 strings.
        -A --s16=[range] show 2-byte ASCII strings.
        -a --s8=[range]  show 1-byte ASCII strings.

Range is given as <low>:<high>[:<min>]. For strings, <low> and <high> are lengths. If <min> is given, values between -<min> and <min> that aren't exactly zero are filtered out. This is useful for f64 and f32 to avoid numbers with large negative exponents. The file test/test_data gives 4 lines out output with --f64=-1e6:1e6:1e-6, but 41 lines with --f64=-1e6:1e-6:0. Most of the extra lines have 3-digit negative exponents.

With no options, the behavior is the same as
--f64=-1e6:1e6:1e-6 --i32=-1000:1000 --s8=3:64 
