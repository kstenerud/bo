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

static void teardown(void* context, FILE* in_stream, FILE* out_stream)
{
	if(context != NULL)
	{
		bo_destroy_context(context);
	}
	if(in_stream != NULL && in_stream != stdin)
	{
		fclose(in_stream);
	}
	if(out_stream != NULL && out_stream != stdout)
	{
		fclose(out_stream);
	}
}

int main(int argc, char* argv[])
{
	void* context = NULL;
	FILE* in_stream = NULL;
	FILE* out_stream = stdout;
	int opt;
    while((opt = getopt (argc, argv, "i:o:")) != -1)
    {
    	switch(opt)
        {
		    case 'i':
        		in_stream = new_input_stream(optarg);
        		break;
		    case 'o':
		    	out_stream = new_output_stream(optarg);
        		break;
      		default:
				print_usage();
				return 1;
      	}
	}
	const char* in_string = optind < argc ? argv[optind] : NULL;

	if(in_string == NULL && in_stream == NULL)
	{
		fprintf(stderr, "Must specify input string and/or input stream\n");
		goto failed;
	}

	context = bo_new_stream_context(out_stream, on_error);

	if(in_string != NULL && bo_process_string(in_string, context) != 0)
	{
		goto failed;
	}

	if(in_stream != NULL && !bo_process_stream(in_stream, context))
	{
		goto failed;
	}

	teardown(context, in_stream, out_stream);
	return 0;

failed:
    printf("failed\n");fflush(stdout);
	teardown(context, in_stream, out_stream);
	return 1;
}
