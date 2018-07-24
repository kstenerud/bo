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


#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bo/bo.h>
#include "bo_version.h"


#define STRINGIZE_PARAM_(arg) #arg
#define STRINGIZE_PARAM(arg) STRINGIZE_PARAM_(arg)

static const char* g_version = STRINGIZE_PARAM(BO_VERSION);

static const char g_usage[] =
	"Usage: bo [options] command [command] ...\n"
	"\n"
	"The default is to read commands from cmdline arguments and print output to stdout.\n"
	"\n"
	"Options:\n"
	"    -i [filename]: Read commands/data from a file (use \"-\" to read from stdin).\n"
	"    -o [filename]: Write output to a file (use \"-\" to write to stdout).\n"
	"    -n           : Write a newline after processing is complete.\n"
	"    -v           : Print version and exit.\n"
	"    -h           : Print help and exit.\n"
	"\n"
	"Commands:\n"
	"    i{type}{data width}{endianness}: Specify how to interpret input data\n"
	"    o{type}{data width}{endianness}{print width}: Specify how to re-interpret data and how to print it\n"
	"    p{string}: Specify a prefix to prepend to each datum output\n"
	"    s{string}: Specify a suffix to append to each datum object (except for the last object)\n"
	"    P{type}: Specify a preset for prefix and suffix.\n"
	"\n"
	"Types:\n"
	"    i: Integer in base 10\n"
	"    h: Integer in base 16\n"
	"    o: Integer in base 8\n"
	"    b: Integer in base 2\n"
	"    f: IEEE 754 binary floating point\n"
	"    d: IEEE 754 binary decimal\n"
	"    s: C-style string (including escaping). This type does not use widths or endianness.\n"
	"    B: Data is interpreted or output using its binary representation rather than text.\n"
	"\n"
	"Data Widths:\n"
	"    1 bytes (8-bit)\n"
	"    2 bytes (16-bit)\n"
	"    4 bytes (32-bit)\n"
	"    8 bytes (64-bit)\n"
	"    16 bytes (128-bit)\n"
	"\n"
	"Endianness:\n"
	"    l: Little Endian\n"
	"    b: Big Endian\n"
	"\n"
	"Print Width:\n"
	"    Any integer representing the minimum number of digits to print.\n"
	"    For floating point, the number of digits after the decimal point.\n"
	"\n"
	"Presets:\n"
	"    c: C-style preset: \", \" suffix, 0x prefix for hexadecimal, 0 prefix for octal.\n"
	"    s: Space preset: \" \" suffix.\n"
	"\n"
	"Notes:\n"
	"    Presets must be applied AFTER setting the output type.\n"
	"    Surrounding quoted quotes aren't necessary except for arguments containing string values.\n"
	"    A single command line argument may contain multiple commands.\n"
	"\n"
	"Example: Convert the string \"Testing\" to its hex representation using \"space\" preset:\n"
	"    bo \"oh1b2 Ps ih1b \\\"Testing\\\"\"\n"
	"\n"
	"Example: Convert the 32-bit float 1.5 to its little endian hex representation using \"C\" preset:\n"
	"    bo oh1l2 if4l Pc 1.5\n"
	;

static void print_version()
{
	printf("Bo version %s\n", g_version);
}

static void print_usage()
{
	printf("\n%s", g_usage);
}

static void on_error(__attribute__ ((unused)) void* user_data, const char* message)
{
	if(message == NULL || *message == 0)
	{
		return;
	}

	fprintf(stderr, "Error: %s\n", message);
}

static void perror_exit(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	perror("");
	exit(1);
}

static FILE* new_output_stream(const char* filename)
{
	if(filename == NULL)
	{
		return NULL;
	}
	if(strcmp(filename, "-") == 0)
	{
		return stdout;
	}

	FILE* stream = fopen(filename, "wb");
	if(stream == NULL)
	{
		perror_exit("Could not open %s for writing", filename);
	}
	return stream;
}

static FILE* new_input_stream(const char* filename)
{
	if(filename == NULL)
	{
		return NULL;
	}
	if(strcmp(filename, "-") == 0)
	{
		return stdin;
	}

	FILE* stream = fopen(filename, "rb");
	if(stream == NULL)
	{
		perror_exit("Could not open %s for reading", filename);
	}
	return stream;
}

static void close_stream(FILE* stream)
{
	if(stream == NULL || stream == stdin || stream == stdout)
	{
		return;
	}

	fclose(stream);
}

static void teardown(void* context, FILE* out_stream, const char** in_filenames, int in_filenames_count, bool should_print_newline)
{
	if(context != NULL)
	{
		bo_flush_and_destroy_context(context);
	}
	if(out_stream != NULL && should_print_newline)
	{
		fprintf(out_stream, "\n");
	}
	close_stream(out_stream);
	for(int i = 0; i < in_filenames_count; i++)
	{
		free((void*)in_filenames[i]);
	}
}

static bool on_output(void* void_user_data, char* data, int length)
{
	FILE* output_stream = (FILE*)void_user_data;
    int bytes_written = fwrite(data, 1, length, output_stream);
    if(bytes_written != length)
    {
    	perror("Error writing to putput stream");
        return false;
    }
    return true;
}

static bool process_stream(void* context, FILE* stream)
{
	char buffer[10000];
	const int bytes_to_read = sizeof(buffer) - 1;
	int bytes_read;
	bool is_successful = true;

	while((bytes_read = fread(buffer, 1, bytes_to_read, stream)) > 0)
	{
		if(ferror(stream))
		{
			perror("Error reading from input stream");
			is_successful = false;
		}
		buffer[bytes_read] = 0;
		bo_data_segment_type segment_type = feof(stream) ? DATA_SEGMENT_LAST : DATA_SEGMENT_STREAM;
		bo_process(context, buffer, bytes_read, segment_type);
	}
	if(is_successful && ferror(stream))
	{
		perror("Error reading from input stream");
		is_successful = false;
	}

	return is_successful;
}


int main(int argc, char* argv[])
{
	void* context = NULL;
	const int max_file_count = 1000;
	const char* in_filenames[max_file_count];
	int in_file_count = 0;
	FILE* out_stream = stdout;
	bool should_print_newline = false;
	bool has_args = false;
	bool is_flush_successful = false;

	int opt = 0;
    while((opt = getopt (argc, argv, "i:o:hnv")) != -1)
    {
    	switch(opt)
        {
		    case 'i':
		    	if(in_file_count >= max_file_count)
		    	{
					fprintf(stderr, "Too many input files. Maximum allowed is %d.\n", max_file_count);
					goto failed;
		    	}
		    	in_filenames[in_file_count++] = strdup(optarg);
        		break;
		    case 'o':
		    	close_stream(out_stream); // Just in case the user does something stupid
		    	out_stream = new_output_stream(optarg);
        		break;
			case 'n':
        		should_print_newline = true;
        		break;
        	case 'h':
        		print_version();
				print_usage();
				goto success;
			case 'v':
        		print_version();
        		goto success;
      		default:
        		print_version();
				print_usage();
				goto failed;
      	}
	}
	has_args = optind < argc;

	if(!has_args && in_file_count == 0)
	{
		fprintf(stderr, "Must specify input string and/or input stream\n");
		goto failed;
	}

	context = bo_new_context(out_stream, on_output, on_error);

	for(int i = optind; i < argc; i++)
	{
		if(bo_process(context, argv[i], strlen(argv[i]), DATA_SEGMENT_LAST) == NULL)
		{
			goto failed;
		}
	}

	for(int i = 0; i < in_file_count; i++)
	{
		FILE* in_stream = new_input_stream(in_filenames[i]);
		bool result = process_stream(context, in_stream);
		close_stream(in_stream);
		if(!result)
		{
			goto failed;
		}
	}

	is_flush_successful = bo_flush_and_destroy_context(context);
	context = NULL;

	if(!is_flush_successful)
	{
		goto failed;
	}

success:
	teardown(context, out_stream, in_filenames, in_file_count, should_print_newline);
	return 0;

failed:
	printf("Use bo -h for help.\n");
	teardown(context, out_stream, in_filenames, in_file_count, false);
	return 1;
}
