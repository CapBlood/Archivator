#include "zar.h"

#define UNPACK 1
#define PACK 2
#define HELP 3

unsigned int total_size;
unsigned int curr_size;

int main(int argc, char *argv[])
{
	mode_t flags = 0775;
	char *target = NULL;
	char *out_dir = NULL;
	char *out_name = "archive.zar";
	char *help = "archive [options] [target_dir]\nOptions:\nd - archive the dir\n\
u - unpack dir\nh - help\no - output path";
	int file;
	int opt;
	int mode = 0;

	while ((opt = getopt(argc, argv, "u:o:d:h")) != -1) {
		switch (opt) {
		case 'o':
			out_name = optarg;
			out_dir = optarg;
			break;
		case 'u':
			mode = UNPACK;
			target = optarg;
			break;
		case 'd':
			mode = PACK;
			target = optarg;
			break;
		case 'h':
			mode = HELP;
			break;
		}
	}

	if (mode == HELP) {
		printf("%s\n", help);
		return 0;
	}

	if (target == NULL) {
		fprintf(stderr, "Invalid arguments: not found path of dir for archive\n");
		return -1;
	}

	char *ptr = strrchr(target, '/');

	if (mode == PACK) {
		if (chdir(target) == -1 || chdir("..") == -1)
			goto IO_error;
		file = open(out_name, O_WRONLY | O_CREAT, flags);
		if (file == -1 || ftw(target, &sum, 1) == -1)
			goto IO_error;
		if (write(file, &total_size, sizeof(total_size)) == -1 || archive_dir(ptr + 1, file) == -1)
			return -1;
		showProgressBar();
		printf("\nSuccessful\n");
		close(file);
	} else if (mode == UNPACK) {
		file = open(target, O_RDONLY);
		if (file == -1)
			goto IO_error;
		if (out_dir != NULL) {
			if (chdir(out_dir) == -1)
				goto IO_error;
		} else if (ptr != NULL) {
			*ptr = 0;
			if (chdir(target) == -1)
				goto IO_error;
			*ptr = '/';
		}
		if (unpack(file) == -1)
			return -1;
		close(file);
		showProgressBar();
		printf("\nSuccessful\n");
	}

	return 0;

IO_error:
		perror("IO error");
		return -1;
}
