#include <bo/bo.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

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

#define PERROR_EXIT(FMT, ...) do { fprintf(stderr, FMT, __VA_ARGS__); perror(""); exit(1); } while(false)

FILE* new_output_stream(const char* filename)
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
		PERROR_EXIT("Could not open %s for writing", filename);
	}
	return stream;
}

FILE* new_input_stream(const char* filename)
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
		PERROR_EXIT("Could not open %s for reading", filename);
	}
	return stream;
}

void close_stream(FILE* stream)
{
	if(stream == NULL || stream == stdin || stream == stdout)
	{
		return;
	}

	fclose(stream);
}

static void teardown(void* context, FILE* out_stream, const char** in_filenames, int in_filenames_count)
{
	if(context != NULL)
	{
		bo_destroy_context(context);
	}
	close_stream(out_stream);
	for(int i = 0; i < in_filenames_count; i++)
	{
		free((void*)in_filenames[i]);
	}
}

int main(int argc, char* argv[])
{
	void* context = NULL;
	const int max_file_count = 1000;
	const char* in_filenames[max_file_count];
	int in_file_count = 0;
	FILE* out_stream = stdout;
	int opt;
    while((opt = getopt (argc, argv, "i:o:")) != -1)
    {
    	switch(opt)
        {
		    case 'i':
		    	if(in_file_count >= max_file_count)
		    	{
					fprintf(stderr, "Too many input files. Maximum allowed is %d.\n", max_file_count);
					return 1;
		    	}
		    	in_filenames[in_file_count++] = strdup(optarg);
        		break;
		    case 'o':
		    	out_stream = new_output_stream(optarg);
        		break;
      		default:
				print_usage();
				return 1;
      	}
	}
	bool has_args = optind < argc;

	if(!has_args && in_file_count == 0)
	{
		fprintf(stderr, "Must specify input string and/or input stream\n");
		goto failed;
	}

	context = bo_new_stream_context(out_stream, on_error);

	for(int i = optind; i < argc; i++)
	{
		if(bo_process_string(argv[i], context) < 0)
		{
			goto failed;
		}
	}

	for(int i = 0; i < in_file_count; i++)
	{
		FILE* in_stream = new_input_stream(in_filenames[i]);
		bool result = bo_process_stream(in_stream, context);
		close_stream(in_stream);
		if(!result)
		{
			goto failed;
		}
	}

	teardown(context, out_stream, in_filenames, in_file_count);
	return 0;

failed:
    printf("failed\n");fflush(stdout);
	teardown(context, out_stream, in_filenames, in_file_count);
	return 1;
}
