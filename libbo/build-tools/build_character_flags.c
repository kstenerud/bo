//  Copyright (c) 2018 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


// Generates a header file containing a table of meta information for all
// basic characters.
//
// Just compile this file as a command line program and run. It will output the
// enum and character table as a header file to stdout.


// -------------
// Configuration
// -------------

#define ENUM_NAME                    "character_flag"
#define ENUM_PREFIX                  "CH_FLAG_"
#define CHARACTER_FLAGS_TABLE_NAME   "g_character_flags"
#define CHARACTER_FLAGS_TABLE_TYPE   unsigned char
#define CHARACTER_FLAGS_TABLE_LENGTH 256

// CHARACTER_FLAGS_TABLE_TYPE determines how many of these enum value get used.
// An 8-bit table type will only have the first 8 flags.
typedef enum
{
    NONE,

    // Included symbols for 8-bit flags
    CONTROL,            // Control character
    WHITESPACE,         // Whitespace character
    BASE_8,             // Can represent a number in base 8
    BASE_10,            // Can represent a number in base 10
    BASE_16,            // Can represent a number in base 16
    FP_NUMBER,          // Includes digits, sign, decimal point, etc
    ALPHANUMERIC,       // Letters and numbers ([a-zA-Z0-9])
    PRINTABLE,          // A single-byte printable symbol

    // Included symbols for 16-bit flags
    SYMBOL,             // A symbol character
    BASE_2,             // Can represent a number in base 2
    UTF_8,              // UTF-8 multibyte octet of any kind
    UTF_8_2_BYTES,      // UTF-8 2-byte initiator
    UTF_8_3_BYTES,      // UTF-8 3-byte initiator
    UTF_8_4_BYTES,      // UTF-8 4-byte initiator
    UTF_8_CONTINUATION, // UTF-8 continuation octet
} character_flag;

// The contents of g_character_flag_names can be defined in any order.
// The [XYZ] = "XYZ" syntax ensures proper order matching to the enum.
static const char* const g_character_flag_names[] =
{
    [NONE]               = "NONE",
    [CONTROL]            = "CONTROL",
    [SYMBOL]             = "SYMBOL",
    [WHITESPACE]         = "WHITESPACE",
    [BASE_2]             = "BASE_2",
    [BASE_8]             = "BASE_8",
    [BASE_10]            = "BASE_10",
    [BASE_16]            = "BASE_16",
    [FP_NUMBER]          = "FP_NUMBER",
    [ALPHANUMERIC]       = "ALPHANUMERIC",
    [UTF_8]              = "UTF_8",
    [UTF_8_2_BYTES]      = "UTF_8_2_BYTES",
    [UTF_8_3_BYTES]      = "UTF_8_3_BYTES",
    [UTF_8_4_BYTES]      = "UTF_8_4_BYTES",
    [UTF_8_CONTINUATION] = "UTF_8_CONTINUATION",
    [PRINTABLE]          = "PRINTABLE",
};



// -----
// Setup
// -----

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define STRINGIZE_PARAM_(arg) #arg
#define STRINGIZE_PARAM(arg) STRINGIZE_PARAM_(arg)
#define CHARACTER_FLAGS_TABLE_TYPE_NAME  STRINGIZE_PARAM(CHARACTER_FLAGS_TABLE_TYPE)

static int g_character_flag_names_count = sizeof(g_character_flag_names) / sizeof(*g_character_flag_names);



// ----
// Data
// ----

static uint64_t g_character_flags[256] = {0};
static char g_character_names[256][100] = {0};



// -------
// Utility
// -------

static int get_flag_values_max()
{
    return sizeof(CHARACTER_FLAGS_TABLE_TYPE) * 8;
}

static int get_flag_value(int flag_index)
{
    return flag_index == 0 ? 0 : 1 << (flag_index - 1);
}

static const char* get_flag_name(int flag_index)
{
    return g_character_flag_names[flag_index];
}

static const char* get_character_name(int ch)
{
    return g_character_names[ch];
}

static void set_character_name(int ch, const char* const name)
{
    strcpy(g_character_names[ch], name);
}

static bool is_character_flag_set(int ch, int flag_index)
{
    return (g_character_flags[ch] & get_flag_value(flag_index)) != 0;
}

static void add_flag_to_character(character_flag flag, int ch)
{
    g_character_flags[ch] |= get_flag_value(flag);
}

static void add_flag_to_characters_in_range(character_flag flag, int min_char, int max_char)
{
    for(int ch = min_char; ch <= max_char; ch++)
    {
        add_flag_to_character(flag, ch);
    }
}

static void add_flag_to_characters(character_flag flag, const char* str)
{
    for(; *str != 0; str++)
    {
        add_flag_to_character(flag, *str);
    }
}

static void set_character_names_literal(const int min_char, const int max_char)
{
    char name[2] = {0};
    for(int ch = min_char; ch <= max_char; ch++)
    {
        name[0] = ch;
        set_character_name(ch, name);
    }
}



// --------
// Printing
// --------

static void print_flags(const int ch)
{
    bool first_entry = true;
    bool did_find_flag = false;
    for(int i = 1; i <= get_flag_values_max() && i < g_character_flag_names_count; i++)
    {
        if(is_character_flag_set(ch, i))
        {
            if(!first_entry)
            {
                printf(" | ");
            }
            first_entry = false;
            printf("%s%s", ENUM_PREFIX, get_flag_name(i));
            did_find_flag = true;
        }
    }
    if(!did_find_flag)
    {
        printf("%s%s", ENUM_PREFIX, get_flag_name(NONE));
    }
}

static void print_enum()
{
    const int number_width = (int)sizeof(CHARACTER_FLAGS_TABLE_TYPE) * 2;
    int text_width = 0;
    for(int i = 0; i <= get_flag_values_max() && i < g_character_flag_names_count; i++)
    {
        int length = strlen(get_flag_name(i));
        if(length > text_width)
        {
            text_width = length;
        }
    }

    printf("typedef enum\n{\n");
    for(int i = 0; i <= get_flag_values_max() && i < g_character_flag_names_count; i++)
    {
        printf("    %s%*s = 0x%0*x,\n",
            ENUM_PREFIX,
            -text_width,
            get_flag_name(i),
            number_width,
            get_flag_value(i));
    }
    printf("} %s;\n", ENUM_NAME);
}

static void print_table()
{
    const int table_length = sizeof(g_character_flags) / sizeof(*g_character_flags);

    printf("static const %s %s[%d] =\n{\n",
        CHARACTER_FLAGS_TABLE_TYPE_NAME,
        CHARACTER_FLAGS_TABLE_NAME,
        CHARACTER_FLAGS_TABLE_LENGTH);

    for(int i = 0; i < table_length; i++)
    {
        printf("    /* %02x: %-5s */ (%s)(", i, get_character_name(i), CHARACTER_FLAGS_TABLE_TYPE_NAME);
        print_flags(i);
        printf("),\n");
    }

    printf("};\n");
}



// -----------
// Definitions
// -----------

static void add_whitespace_flags()
{
    add_flag_to_characters(WHITESPACE, " \t\n\v\f\r");
}

static void add_number_flags()
{
    add_flag_to_characters_in_range(FP_NUMBER, '0', '9');
    add_flag_to_characters(FP_NUMBER, "+-.eE");
}

static void add_control_flags()
{
    add_flag_to_characters_in_range(CONTROL, 0x00, 0x1f);
    add_flag_to_character(CONTROL, 0x7f);
}

static void add_base_2_flags()
{
    add_flag_to_characters_in_range(BASE_2, '0', '1');
}

static void add_base_8_flags()
{
    add_flag_to_characters_in_range(BASE_8, '0', '7');
}

static void add_base_10_flags()
{
    add_flag_to_characters_in_range(BASE_10, '0', '9');
}

static void add_base_16_flags()
{
    add_flag_to_characters_in_range(BASE_16, '0', '9');
    add_flag_to_characters_in_range(BASE_16, 'a', 'f');
    add_flag_to_characters_in_range(BASE_16, 'A', 'F');
}

static void add_alphanumeric_flags()
{
    add_flag_to_characters_in_range(ALPHANUMERIC, '0', '9');
    add_flag_to_characters_in_range(ALPHANUMERIC, 'a', 'z');
    add_flag_to_characters_in_range(ALPHANUMERIC, 'A', 'Z');
}

static void add_symbol_flags()
{
    add_flag_to_characters_in_range(SYMBOL, '!', '/');
    add_flag_to_characters_in_range(SYMBOL, ':', '@');
    add_flag_to_characters_in_range(SYMBOL, '[', '`');
    add_flag_to_characters_in_range(SYMBOL, '{', '~');
}

static void add_printable_flags()
{
    add_flag_to_characters_in_range(PRINTABLE, '!', '~');
}

static void add_utf_8_flags()
{
    add_flag_to_characters_in_range(UTF_8, 0x80, 0xf7);
    add_flag_to_characters_in_range(UTF_8_2_BYTES, 0xc0, 0xdf);
    add_flag_to_characters_in_range(UTF_8_3_BYTES, 0xe0, 0xef);
    add_flag_to_characters_in_range(UTF_8_4_BYTES, 0xf0, 0xf7);
    add_flag_to_characters_in_range(UTF_8_CONTINUATION, 0x80, 0xbf);
}

static void set_character_names()
{
    set_character_names_literal('!', '~');
    set_character_name(0x00, "NUL");
    set_character_name(0x01, "SOH");
    set_character_name(0x02, "STX");
    set_character_name(0x03, "ETX");
    set_character_name(0x04, "EOT");
    set_character_name(0x05, "ENQ");
    set_character_name(0x06, "ACK");
    set_character_name(0x07, "BEL");
    set_character_name(0x08, "BS");
    set_character_name(0x09, "HT");
    set_character_name(0x0a, "LF");
    set_character_name(0x0b, "VT");
    set_character_name(0x0c, "FF");
    set_character_name(0x0d, "CR");
    set_character_name(0x0e, "SO");
    set_character_name(0x0f, "SI");
    set_character_name(0x10, "DLE");
    set_character_name(0x11, "DC1");
    set_character_name(0x12, "DC2");
    set_character_name(0x13, "DC3");
    set_character_name(0x14, "DC4");
    set_character_name(0x15, "NAK");
    set_character_name(0x16, "SYN");
    set_character_name(0x17, "ETB");
    set_character_name(0x18, "CAN");
    set_character_name(0x19, "EM");
    set_character_name(0x1a, "SUB");
    set_character_name(0x1b, "ESC");
    set_character_name(0x1c, "FS");
    set_character_name(0x1d, "GS");
    set_character_name(0x1e, "RS");
    set_character_name(0x1f, "US");
    set_character_name(' ', "Space");
    set_character_name(0x7f, "DEL");
}



// -------
// Program
// -------

int main(void)
{
    set_character_names();
    add_control_flags();
    add_whitespace_flags();
    add_base_2_flags();
    add_base_8_flags();
    add_base_10_flags();
    add_base_16_flags();
    add_number_flags();
    add_alphanumeric_flags();
    add_symbol_flags();
    add_printable_flags();
    add_utf_8_flags();

    printf(
        "/* This file generated by " __FILE__ " */\n"
        "\n"
        "#ifndef ks_character_flags_H\n"
        "#define ks_character_flags_H\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
        "\n"
        );

    print_enum();
    printf("\n");
    print_table();

    printf(
        "\n"
        "\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
        "#endif // ks_character_flags_H\n"
        );
}
