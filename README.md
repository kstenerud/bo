BO (Binary Output)
==================

The Swiss army knife of data examination and manipulation.

This is the tool to use when you need to visualize or convert data in different formats.


How It Works
------------

Think of bo as a data processor that takes in streams of commands and data, and outputs accordingly:

    Command line arguments --\     +------+
                              \    |      |
    Files -----------------------> |  bo  | ---> stdout or file
                              /    |      |
    stdin -------------------/     +------+


Input consists of whitespace separated commands and data, which can be passed in via command line arguments, files, and stdin. Commands determine how input data is to be interpreted, and how output is to be formatted.



Brief Introduction
------------------

Here are the commands used in the examples that follow. i, o, P, and " are used to initiate commands. For anything not a command, bo will attempt to interpret it as a numeric value.

  * i{parameters}: Set input type.
  * o{parameters}: Set output type.
  * P{type}: Set the prefix and suffix from a preset ("s" for space separators, and "c" for C-style separators).
  * "a string": Add a string value.

Input and output type commands consist of multiple fields:

  * Type: What type to read data as, or what to output as (see below)
  * Data Width: The width of each datum in bytes (1, 2, 4, 8, 16)
  * Endianness: What endianness to use when reading or presenting data ("l" or "b" - only required for data width > 1)
  * Print Width: Minimum number of digits to use when printing numeric values (only for output. optional).

Types:

  * Integer (i): Integer in base 10
  * Hexadecimal (h): Integer in base 16
  * Octal (o): Integer in base 8
  * Boolean (b): Integer in base 2
  * Float (f): IEEE 754 binary floating point
  * Decimal (d): IEEE 754 binary decimal
  * String (s): String, with c-style encoding for escaped chars (tab, newline, hex, etc).
  * Binary (B): Data is interpreted or output using its binary representation rather than text.

For example, `ih2b` means set input type to hexadecimal, 2 bytes per value, big endian. `oo4l11` means set output type to octal, 4 bytes per value, little endian, 11 digits minimum.

All input sources are read as streams of whitespace separated commands and data. For example:

    oh1l2 Ps if4l 35.01 10.5 ih1 9f 5a

This stream does the following:

  * set output to hex, 1 byte per value, little endian (only required to separate the last field), min 2 digits per value.
  * Set output preset to "s" (space separated).
  * Set input to float, 4 bytes per value, little endian.
  * Read 35.01 and store internally according to input type (4 byte little endian float).
  * Read 10.5 and store internally according to input type (4 byte little endian float).
  * Set input to hex, 1 byte per value (endianness not needed).
  * Read 9f and store internally according to input type (1 byte hex).
  * Read 5a and store internally according to input type (1 byte hex).

And output will be `3d 0a 0c 42 00 00 28 41 9f 5a`



Examples of what you can do with bo
-----------------------------------

Note: the `-n` flag just appends a newline after processing.


#### See the per-byte layout (_oh1_, _Ps_) of a 4-byte integer (_decimal 12345678_) in big (_ih4b_) or little endian (_ih4l_) format.

    $ bo -n "oh1 Ps ih4b 12345678"
    12 34 56 78

Note: The arguments don't need to be separate cmdline arguments. `bo -n "oh1 Ps ih4b 12345678"` does the exact same thing because parsing separates by whitespace. Input files specified by the -i switch are also parsed the same way.

    $ bo -n "oh1 Ps ih4l 12345678"
    78 56 34 12


#### Convert integers between bases.

    $ bo -n "oh8b16 Pc ii8b 1000000"
    0x00000000000f4240

    $ bo -n "oi8b ih8b 7fffffffffffffff"
    9223372036854775807

    $ bo -n "ob8b ih8b ffe846134453a8c2"
    1111111111101000010001100001001101000100010100111010100011000010


#### Examine part of a memory dump (faked in this case) as an array of 32-bit big-endian floats at precision 3.

Fake a 12-byte memory dump as an example:

    $ bo "oB1 if4b 1.1 8.5 305.125" >memory_dump.bin

What it looks like in memory:

    $ bo -n -i memory_dump.bin "oh1b2 Pc iB1"
    0x3f, 0x8c, 0xcc, 0xcd, 0x41, 0x08, 0x00, 0x00, 0x43, 0x98, 0x90, 0x00

What it looks like interpreted as floats:

    $ bo -n -i memory_dump.bin "of4b3 Pc iB1"
    1.100, 8.500, 305.125


#### Convert the endianness of a data dump.

    $ bo "oB1 ih2b 0123 4567 89ab" >data.bin

    $ bo -n -i data.bin "oh2l Pc iB1"
    0x2301, 0x6745, 0xab89

Note: Once input type is set to binary, bo stops parsing, and just passes input DIRECTLY to the output system using the current output type.


#### Convert text files to C-friendly strings.

example.txt:

    This is a test.
    There are tabs  between these words.
    ¡8-Ⅎ⊥∩ sʇɹoddns osʃɐ ʇI

    $ bo -n -i example.txt "os iB1"
    This is a test.\nThere are tabs\tbetween\tthese\twords.\n¡8-Ⅎ⊥∩ sʇɹoddns osʃɐ ʇI


#### Reinterpret one type as another at the binary level (for whatever reason).

    $ bo -n "oi4b if4b 1.5"
    1069547520


#### Complex example: Build up a data packet with multiple data types.

    $ bo -n "oh1l2 Pc if4b 1.5 1.25 ii2b 1000 2000 3000 ih1 ff fe 7a ib1 10001011"

Does the following:

  * Set output to hex, 2 ditis per entry (`oh1l2`).
  * Set output prefix/suffix using the "c" preset (`Pc`).
  * Input 1.5 and 1.25 as 4-byte floats, big endian (`if4b 1.5 1.25`).
  * Input 1000, 2000, and 3000 as 2-byte decimal integers, big endian (`ii2b 1000 2000 3000`)
  * Input ff, fe, and 7a as 1-byte hex integers (`ih1 ff fe 7a`)
  * Input 10001011 as 1-byte binary integer (`ib1 10001011`)

The result:

    0x3f, 0xc0, 0x00, 0x00, 0x3f, 0xa0, 0x00, 0x00, 0x03, 0xe8, 0x07, 0xd0, 0x0b, 0xb8, 0xff, 0xfe, 0x7a, 0x8b



Usage
-----

    bo [options] command [command] ...


### Options

  * -i [filename]: Read commands/data from a file.
  * -o [filename]: Write output to a file.
  * -n Write a newline after processing is complete.
  * -h Print help and exit.
  * -v Print version and exit.


The `bo` command can take input from command line arguments, files (using the -i switch), and stdin (using `-i -`). You may specify as many `-i` switches as you like. Bo first reads all command line arguments, and then reads files in the order they were specified using the `-i` switch. For example:

    bo -i file1.txt -i - -i file2.txt "oh2b4 Pc" io4b "1 2 3"

Bo will:

  * Parse the string `oh2b4 Pc`
  * Parse the string `io4b`
  * Parse the string `1 2 3`
  * Parse from `file1.txt`
  * Parse from stdin (`-i -`)
  * Parse from `file2.txt`

By default, bo outputs to stdout. You can specify an output file using `-o`.



Commands
--------

Bo parses whitespace separated strings and interprets them as commands. The following commands are supported:

  * i{type}{data width}{endianness}: Specify how to interpret input data
  * o{type}{data width}{endianness}[print width]: Specify how to re-interpret data and how to print it
  * p{string}: Specify a prefix to prepend to each datum output
  * s{string}: Specify a suffix to append to each datum output (except for the last object)
  * P{type}: Specify a preset for prefix and suffix.
  * "...": Read a string value.
  * (numeric): Read a numeric value.

Data is interpreted according to the input format, stored in an intermediary buffer as binary data, and then later re-interpreted and printed according to the output format.

Strings are copied as bytes to the intermediary binary buffer, to be reinterpreted later by the output format.

Before processing any data, both input and output types must be specified via `input type` and `output type` commands. You may later change the input or output types again via commands, even after interpreting data. Any time the output type is changed, all buffers are flushed.



### Input Type Command

The input specifier command consists of 3 parts:

  * Type: How to interpret incoming data (integer, float, decimal, binary, etc)
  * Data Width: The width of each datum in bytes (1, 2, 4, 8, 16)
  * Endianness: What endianness to use when storing data (little or big endian)


### Output Type Command

The output specifier command consists of 4 parts:

  * Type: How to format the data for printing (integer, float, decimal, string, etc)
  * Data Width: The width of each datum in bytes (1, 2, 4, 8, 16)
  * Endianness: What endianness to use when presenting data (little or big endian)
  * Print Width: Minimum number of digits to use when printing numeric values (optional).

#### Type

Determines the type to be used for interpreting incoming data, or presenting outgoing data.

  * Integer (i): Integer in base 10
  * Hexadecimal (h): Integer in base 16
  * Octal (o): Integer in base 8
  * Boolean (b): Integer in base 2
  * Float (f): IEEE 754 binary floating point
  * Decimal (d): IEEE 754 binary decimal (TODO)
  * String (s): String, with c-style encoding for escaped chars (tab, newline, hex, etc).
  * Binary (B): Data is interpreted or output using its binary representation rather than text.

##### Notes on the boolean type

As the boolean type operates on sub-byte values, its behavior may seem surprising at first. In little endian systems, not only does byte 0 occur first in a word, but so does bit 0. Bo honors this bit ordering when printing. For example:

    $ bo ob2b ih2b 6
    0000000000000110
    $ bo ob2l ih2l 6
    0110000000000000

Note, however, that when inputting textual representations of boolean values, they are ALWAYS read as big endian (same as with all other types), and then stored according to the input endianness:

    $ bo ob2b ib2b 1011
    0000000000001011
    $ bo ob2l ib2l 1011
    1101000000000000
    $ bo ob2b ib2l 1011
    0000101100000000
    $ bo ob2l ib2b 1011
    0000000011010000

#### Data Width

Determines how wide of a data field to store the data in:

  * 1 bytes (8-bit)
  * 2 bytes (16-bit)
  * 4 bytes (32-bit)
  * 8 bytes (64-bit)
  * 16 bytes (128-bit)

#### Endianness

Determines in which order bits and bytes are encoded:

  * Little Endian (l): Lowest bit and byte comes first
  * Big Endian (b): Highest bit and byte comes first

The endianness field is optional when data width is 1, EXCEPT for output type boolean.

#### Print Width

Specifies the minimum number of digits to print when outputting numeric values. This is an optional field, and defaults to 1.

For integer types, zeroes are prepended until the printed value has the specified number of digits.
For floating point types, zeroes are appended until the fractional portion has the specified number of digits. The whole number portion is not used and has no effect in this calculation.

#### Examples

  * `ih2l`: Input type hexadecimal encoded integer, 2 bytes per value, little endian
  * `io4b`: Input type octal encoded integer, 4 bytes per value, big endian
  * `if4l`: Input type floating point, 4 bytes per value, little endian
  * `oi4l`: Output as 4-byte integers in base 10 with default minimum 1 digit (i.e. no zero padding)
  * `of8l10`: Interpret data as 8-byte floats and output with 10 digits after the decimal point.


### Prefix Specifier Command

Defines what to prefix each output entry with.

Note: Examples have escaped quotes since that's usually what you'll be doing with command line arguments.

  * `p\"0x\"`: Prefix all values with "0x".
  * `p\"The next value is: \"`: Prefix all values with "The next value is: "


### Suffix Specifier Command

Defines what to suffix each output entry with (except for the last entry).

Note: Examples have escaped quotes since that's usually what you'll be doing with command line arguments.

  * `s\", \"`: Suffix all values with ", ".
  * `s\" | \"`: Suffix all values with " | "


### Preset Command

Presets define preset prefixes and suffixes for common tasks. Currently, the following are supported:

  * c: C-style prefix/suffix: ", " suffix, and prefix based on output type: 0x for hex, 0 for octal, and nothing for everything else.
  * s: Space separator between entries.



Building
--------

Requirements:

  * CMake
  * C/C++ compiler

Commands:

    mkdir build
    cd build
    cmake ..
    make
    ./bo_app/bo -h



Tests
-----

    ./libbo/test/libbo_test



Libbo
-----

All of bo's functionality is in the library libbo. The API is small (4 calls, 2 callbacks) and pretty straightforward since all commands and configurations are done through the parsed data. The basic process is:

  * Build a context object according to your needs.
  * Call one or more process functions.
  * Flush and destroy the context.
  * Gather output and errors via the callbacks.

`test_helpers.cpp` shows how to parse strings, and `main.c` from bo_app shows how to use file streams.



Issues
------

  * IEEE754 decimal types are not yet implemented.
  * 128 bit values are not yet implemented.
  * 16-bit ieee754 floating point is not yet implemented.



License
-------

Copyright 2018 Karl Stenerud

Released under MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
