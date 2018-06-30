#include <bo/bo.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>

static void print_usage()
{
	printf("\n");
	printf("Version: %s\n", bo_version());
	printf("Usage: bo <input>\n");
}

static void on_error(const char* fmt, ...)
{
	if(fmt == NULL || *fmt == 0)
	{
		return;
	}

	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);	
}

#define PERROR_EXIT(FMT, ...) do { fprintf(stderr, FMT, __VA_ARGS__); perror(""); return 1; } while(false)

static int process_stream(const char* input_filename, const char* output_filename)
{
	FILE* in_stream = input_filename == NULL ? stdin : fopen(input_filename, "rb");
	if(in_stream == NULL)
	{
		PERROR_EXIT("Could not open %s for reading", input_filename);
	}
	FILE* out_stream = output_filename == NULL ? stdout : fopen(output_filename, "wb");
	if(out_stream == NULL)
	{
		PERROR_EXIT("Could not open %s for writing", output_filename);
	}

	void* context = bo_new_stream_context(out_stream, on_error);
	return bo_process_stream(in_stream, context) ? 0 : 1;
}

static int process_string(const char* input, const char* output_filename)
{
	FILE* out_stream = output_filename == NULL ? stdout : fopen(output_filename, "wb");
	if(out_stream == NULL)
	{
		PERROR_EXIT("Could not open %s for writing", output_filename);
	}

	int returnval = 0;
	const int buffer_size = 100000;
	char* buffer = malloc(buffer_size);
	void* context = bo_new_buffer_context((uint8_t*)buffer, buffer_size, on_error);
	int bytes_processed = bo_process_string(input, context);
	if(bytes_processed > 0)
	{
		int bytes_written = fwrite(buffer, 1, bytes_processed, out_stream);
		if(bytes_written != bytes_processed)
		{
			if(output_filename == NULL)
			{
				output_filename = "STDOUT";
			}
			PERROR_EXIT("Error writing to %s", output_filename);
		}
		fflush(out_stream);
	}
	else
	{
		returnval = 1;
	}
	free(buffer);
	return returnval;
}

int main(int argc, char* argv[])
{
	const char* input_filename = NULL;
	const char* output_filename = NULL;
	const char* output_config = NULL;
	int opt;
    while((opt = getopt (argc, argv, "f:o:b:")) != -1)
    {
    	switch(opt)
        {
		    case 'f':
        		input_filename = optarg;
        		break;
		    case 'o':
        		output_filename = optarg;
        		break;
		    case 'b':
        		output_config = optarg;
        		break;
      		default:
				print_usage();
				return 1;
      	}
	}

	if(optind < argc)
	{
		if(input_filename != NULL)
		{
			fprintf(stderr, "Must specify EITHER an input file OR an input string\n");
			return 1;
		}
		const char* input_string = argv[optind];
		return process_string(input_string, output_filename);
	}

	return process_stream(input_filename, output_filename);
}
