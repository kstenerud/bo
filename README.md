Binary Output
=============

Reinterpret and output binary data.



Usage
-----

### String Mode:

In string mode, bo reads commands as a string argument from the command line.

    bo [options] command-string

Options:

  * -o some-file: Send output to some-file instead of to stdout


### Stream Mode:

In stream mode, bo reads commands from a file, or from stdin if no input file is specified.

    bo [options]

Options:

  * -f some-file: Read commands from some-file instead of stdin
  * -o some-file: Send output to some-file instead of stdout



Commands
--------

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

  * ih2l: Input type hexadecimal encoded integer, 2 bytes per value, little endian
  * io4b: Input type octal encoded integer, 4 bytes per value, big endian
  * if4l: Input type floating point, 4 bytes per value, little endian


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

  * oi4l1: Interpret data as 4-byte integers and output in base 10 with minimum 1 digit (i.e. no zero padding)
  * of8l10: Interpret data as 8-byte floats and output with minimum 10 digits after the decimal point.




License
-------

Copyright 2018 Karl Stenerud

Released under MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
