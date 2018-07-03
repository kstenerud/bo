Binary Output
=============

Reinterpret and output binary data.


How it Works
------------

Bo reads data and interprets it according to its current input mode, converting it to a binary format. It then re-interprets the binary data according to the output mode and writes it to output. This allows you to do things such as:

  * Read four 8-bit values and reinterpret them as a 32-bit value.
  * Input a 64-bit integer and reinterpret as eight 8-bit values.
  * Build up a binary packet by hand and then print it out to see what it looks like encoded.
  * Read binary data and reinterpret it as any type with any width, using any endianness.
  * Output data in "C-friendly" encoding (0xff, 0xff, ...)



Usage
-----

    bo [options] command ...

### Options

  * -i [filename]: Read commands/data from a file.
  * -o [filename]: Write output to a file.

Specifying "-" as the filename will cause bo to use stdin (when used with -i) or stdout (when used with -o).
By default, bo won't read from any files, and will write output to stdout.

Commands may be passed in as command line arguments and as files via the -i switch. You may specify as many -i switches as you like. Bo will first parse all command line arguments, and then all files specified via -i switches.

#### Example

    bo -i file1 -i file2 -i file3 oh2b4 ih2l 1234 "5678 9abc"

This will do the following:

  * set output type (in this case, hex, 2 bytes per value, big endian, minimum 4 characters per value)
  * set input type (in this case, hex, 2 bytes per value, little endian)
  * process "1234" as a command (in this case, 16-bit hex because of the input type)
  * process "5678 9abc" as commands (in this case, 16-bit hex because of the input type)
  * process file1 as commands
  * process file2 as commands

See more examples in the **Examples** section.






### Examples

    bo -i file.bin oh1l2 Pc iB

  * Set output type to hex, 1 byte per value, little endian (doesn't matter here), 2 digits per value.
  * Use the "c" preset, which sets prefix to "0x" and suffix to ", ".
  * Set input to binary. No more commands will be accepted. All input after parsing this argument will be treated as binary.
  * Read binary data from file1.bin and output as 0x01, 0xfe, 0x5a, etc.


    bo -o file.bin oB if4b "1.1 -400000 89.45005"

  * Set output type to binary.
  * Set input type to floating point, 4 bytes per value, big endian.
  * Set the output file to "file.bin"
  * Write three 32-bit ieee754 floating point values in binary to file.bin in big endian order.


    bo if8l oh1l2 "s\" \"" 1.5

  * See what 1.5 as a 64-bit ieee754 floating point value looks like when stored in little endian order.

Commands
--------

Bo parses whitespace separated strings and interprets them as commands. The following commands are supported:

  * i: Set input type.
  * o: Set output type.
  * p: Set the prefix to prepend to every output value.
  * s: Set the suffix to append to every value except the last.
  * P: Set the prefix and suffix from a preset.
  * "...": Read a string value.
  * (numeric): Read a numeric value.

Bo reads a series of whitespace separated commands and data, and uses them to generate binary data for output. The commands are as follows:

  * i{type}{data width}{endianness}: Specify how to interpret input data
  * o{type}{data width}{endianness}{print width}: Specify how to re-interpret data and how to print it
  * p{string}: Specify a prefix to prepend to each datum output
  * s{string}: Specify a suffix to append to each datum object (except for the last object)

Anything not a command will be interpreted as data.

Data is interpreted according to the input format, and then later re-interpreted and printed according to the output format.

String type...

For example, data input as hex encoded little endian 16-bit integers (ih2l), and then re-interpreted as hex encoded little endian 32-bit integers (oh4l8) with a space (" ") suffix would look like:

    input:  1234 5678 9abc 5432
    output: 78563412 3254bc9


### Input Specifier

The input specifier command consists of 3 parts:

  * Numeric Type: How to interpret the text represenation of a number (integer, float, decimal, etc)
  * Data Width: What width of the numeric type to use in bytes (1, 2, 4, 8, 16)
  * Endianness: What endianness to use when storing data (little or big endian)

#### Numeric Type

Determines how the text representation of a number is to be interpreted:

  * Integer (i): Integer in base 10
  * Hexadecimal (h): Integer in base 16
  * Octal (o): Integer in base 8
  * Boolean (b): Integer in base 2
  * Float (f): IEEE 754 binary floating point
  * Decimal (d): IEEE 754 binary decimal (TODO)

#### Data Width

Determines how wide of a data field to store the number in:

  * 1 bytes (8-bit)
  * 2 bytes (16-bit)
  * 4 bytes (32-bit)
  * 8 bytes (64-bit)
  * 16 bytes (128-bit)

#### Endianness

Determines in which order bits and bytes are encoded:

  * Little Endian: Lowest bit and byte comes first
  * Big Endian: Highest bit and byte comes first

#### Examples

  * `ih2l`: Input type hexadecimal encoded integer, 2 bytes per value, little endian
  * `io4b`: Input type octal encoded integer, 4 bytes per value, big endian
  * `if4l`: Input type floating point, 4 bytes per value, little endian


### Output Specifier

The output specifier command consists of 4 parts:

  * Numeric Type: How to interpret the binary represenation of a number (integer, float, decimal, etc)
  * Data Width: What width of the numeric type to read in bytes (1, 2, 4, 8, 16)
  * Endianness: What endianness to use when reading data (little or big endian)
  * Print Width: Minimum number of digits to use when printing numeric values.

#### Numeric Type

Same as for input, with the following additions:

  * Binary (B): Output the binary data directly. No width or endianness values may be specified.

#### Data Width

Same as for input.

#### Endianness

Same as for input.

#### Print Width

Specifies the minimum number of digits to print when outputting numeric values.

For integer types, zeroes are prepended until the printed value has the specified number of digits.
For floating point types, zeroes are appended until the fractional portion has the specified number of digits. The whole number portion is not used and has no effect in this calculation.

#### Examples

  * `oi4l1`: Interpret data as 4-byte integers and output in base 10 with minimum 1 digit (i.e. no zero padding)
  * `of8l10`: Interpret data as 8-byte floats and output with minimum 10 digits after the decimal point.




License
-------

Copyright 2018 Karl Stenerud

Released under MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
