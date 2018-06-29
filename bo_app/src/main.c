#include <bo/bo.h>
#include <stdio.h>
#include <stdarg.h>

static void print_usage()
{
	printf("Version: %s\n", bo_version());
	printf("Usage: bo <input>\n");
}

static void on_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("BO Error: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		print_usage();
		return 1;
	}
	char* input = argv[1];

	int buffer_size = 100000;
	char* buffer = malloc(buffer_size);
	int bytes_written = bo_process_string(input, buffer, buffer_size-1, on_error);
	if(bytes_written > 0)
	{
		write(1, buffer, bytes_written);
	}
	free(buffer);
	return 0;
}
