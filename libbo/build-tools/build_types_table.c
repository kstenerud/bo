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


// Prints out a list of the first 128 characters and their associated flags.

#include <stdio.h>
#include <string.h>
#include <stdbool.h>



// Configuration

#define ENUM_NAME   "character_flag"
#define ENUM_PREFIX "CHARACTER_FLAG_"

static const char* const g_character_flag_names[] =
{
	"NONE",          // No flags for this character
	"CONTROL",       // Control character
	"WHITESPACE",    // This is a whitespace character
	"BASE_10",       // This character can be used to represent a number in base 10
	"BASE_16",       // This character can be used to represent a number in base 16
	"NUMBER",        // digits, sign, decimal point, etc
	"ALPHANUMERIC",  // This character is alphanumeric ([a-zA-Z0-9])
	"SYMBOL",        // This is a symbol character
	"PRINTABLE",     // This character represents a printable symbol
};

#define CHARACTER_FLAGS_TABLE_NAME   "g_character_flags"
#define CHARACTER_FLAGS_TABLE_TYPE   "uint8_t"
#define CHARACTER_FLAGS_TABLE_LENGTH 256



// Flag Information

typedef enum
{
	NONE         = 0x00,
	CONTROL      = 0x01,
	WHITESPACE   = 0x02,
	BASE_10      = 0x04,
	BASE_16      = 0x08,
	NUMBER       = 0x10,
	ALPHANUMERIC = 0x20,
	SYMBOL       = 0x40,
	PRINTABLE    = 0x80,
} character_flag;

static const character_flag g_character_flags[] =
{
	NONE,
	CONTROL,
	WHITESPACE,
	BASE_10,
	BASE_16,
	NUMBER,
	ALPHANUMERIC,
	SYMBOL,
	PRINTABLE,
};
static const int g_character_flags_count = sizeof(g_character_flags) / sizeof(g_character_flags[0]);



// Utility

#define CHARACTER_NAME_WIDTH 10

static void mark_flags(character_flag* flags, character_flag flag, int min_char, int max_char)
{
	for(int ch = min_char; ch <= max_char; ch++)
	{
		flags[ch] |= flag;
	}
}

static void mark_whitespace(character_flag* flags)
{
	flags[0x09] |= WHITESPACE; // Character Tabulation
	flags[0x0a] |= WHITESPACE; // Line Feed
	flags[0x0b] |= WHITESPACE; // Line Tabulation
	flags[0x0c] |= WHITESPACE; // Form Feed
	flags[0x0d] |= WHITESPACE; // Carriage Return
	flags[' '] |= WHITESPACE;
}

static void mark_numbers(character_flag* flags)
{
	mark_flags(flags, NUMBER, '0', '9');
	mark_flags(flags, NUMBER, 'a', 'f');
	mark_flags(flags, NUMBER, 'A', 'F');
	flags['+'] |= NUMBER;
	flags['-'] |= NUMBER;
	flags['.'] |= NUMBER;
}

static void mark_control(character_flag* flags)
{
	mark_flags(flags, CONTROL, 0x00, 0x1f);
	flags[0x7f] |= CONTROL;
}

static void mark_base_10(character_flag* flags)
{
	mark_flags(flags, BASE_10, '0', '9');
}

static void mark_base_16(character_flag* flags)
{
	mark_flags(flags, BASE_16, '0', '9');
	mark_flags(flags, BASE_16, 'a', 'f');
	mark_flags(flags, BASE_16, 'A', 'F');
}

static void mark_alphanumeric(character_flag* flags)
{
	mark_flags(flags, ALPHANUMERIC, '0', '9');
	mark_flags(flags, ALPHANUMERIC, 'a', 'z');
	mark_flags(flags, ALPHANUMERIC, 'A', 'Z');
}

static void mark_symbols(character_flag* flags)
{
	mark_flags(flags, SYMBOL, '!', '/');
	mark_flags(flags, SYMBOL, ':', '@');
	mark_flags(flags, SYMBOL, '[', '`');
	mark_flags(flags, SYMBOL, '{', '~');
}

static void mark_printable(character_flag* flags)
{
	mark_flags(flags, PRINTABLE, '!', '~');
}

static void set_character_names_auto(char character_names[][CHARACTER_NAME_WIDTH], const int min_char, const int max_char)
{
	for(int ch = min_char; ch <= max_char; ch++)
	{
		character_names[ch][0] = ch;
		character_names[ch][1] = 0;
	}
}

static void set_character_name(char* dst, const char* const name)
{
	strcpy(dst, name);
}

static void set_character_names(char names[][CHARACTER_NAME_WIDTH])
{
	set_character_names_auto(names, '!', '~');
	set_character_name(names[0x00], "NUL");
	set_character_name(names[0x01], "SOH");
	set_character_name(names[0x02], "STX");
	set_character_name(names[0x03], "ETX");
	set_character_name(names[0x04], "EOT");
	set_character_name(names[0x05], "ENQ");
	set_character_name(names[0x06], "ACK");
	set_character_name(names[0x07], "BEL");
	set_character_name(names[0x08], "BS");
	set_character_name(names[0x09], "HT");
	set_character_name(names[0x0a], "LF");
	set_character_name(names[0x0b], "VT");
	set_character_name(names[0x0c], "FF");
	set_character_name(names[0x0d], "CR");
	set_character_name(names[0x0e], "SO");
	set_character_name(names[0x0f], "SI");
	set_character_name(names[0x10], "DLE");
	set_character_name(names[0x11], "DC1");
	set_character_name(names[0x12], "DC2");
	set_character_name(names[0x13], "DC3");
	set_character_name(names[0x14], "DC4");
	set_character_name(names[0x15], "NAK");
	set_character_name(names[0x16], "SYN");
	set_character_name(names[0x17], "ETB");
	set_character_name(names[0x18], "CAN");
	set_character_name(names[0x19], "EM");
	set_character_name(names[0x1a], "SUB");
	set_character_name(names[0x1b], "ESC");
	set_character_name(names[0x1c], "FS");
	set_character_name(names[0x1d], "GS");
	set_character_name(names[0x1e], "RS");
	set_character_name(names[0x1f], "US");
	set_character_name(names[' '], "Space");
	set_character_name(names[0x7f], "DEL");
}


// Printing

static void print_flags(const character_flag flag)
{
	if(flag == 0)
	{
		printf("%s%s", ENUM_PREFIX, g_character_flag_names[0]);
		return;
	}
	bool first_entry = true;
	for(int i = 1; i < g_character_flags_count; i++)
	{
		if(flag & g_character_flags[i])
		{
			if(!first_entry)
			{
				printf(" | ");
			}
			first_entry = false;
			printf("%s%s", ENUM_PREFIX, g_character_flag_names[i]);
		}
	}
}

static int get_flag_value(int flag_index)
{
	return flag_index == 0 ? 0 : 1 << (flag_index - 1);
}

static void print_enum()
{
	printf("typedef enum\n");
	printf("{\n");
	for(int i = 0; i < g_character_flags_count; i++)
	{
		printf("    %s%s = 0x%02x,\n", ENUM_PREFIX, g_character_flag_names[i], get_flag_value(i));
	}
	printf("} %s;\n", ENUM_NAME);
}

static void print_table(const character_flag* flags, const char names[][CHARACTER_NAME_WIDTH])
{
	printf("static const %s %s[%d] =\n", CHARACTER_FLAGS_TABLE_TYPE, CHARACTER_FLAGS_TABLE_NAME, CHARACTER_FLAGS_TABLE_LENGTH);
	printf("{\n");

	for(int i = 0; i < 128; i++)
	{
		printf("    /* %-5s */ (%s)(", names[i], CHARACTER_FLAGS_TABLE_TYPE);
		print_flags(flags[i]);
		printf("),\n");
	}

	printf("};\n");
}



// Program

int main(void)
{
	character_flag flags[128] = {0};
	char names[128][CHARACTER_NAME_WIDTH] = {0};

	mark_control(flags);
	mark_whitespace(flags);
	mark_base_10(flags);
	mark_base_16(flags);
	mark_numbers(flags);
	mark_alphanumeric(flags);
	mark_symbols(flags);
	mark_printable(flags);
	set_character_names(names);

	print_enum();
	printf("\n");
	print_table(flags, names);
}
