#include "zar.h"

unsigned int total_size;
unsigned int curr_size;
static unsigned long curr_time;

int sum(const char *fpath, const struct stat *sb, int typeflag)
{
	if (typeflag == 0)
		total_size += sb->st_size;
	return 0;
}

void showProgressBar(void)
{
	size_t amount = ((double) curr_size / total_size) * 10;

	printf("\r[%-10.*s] - %3d%%", amount, "##########", amount * 10);
	fflush(stdout);
}

int archive_file(const char *curr_path, const int to)
{
	struct stat buf_info;
	struct Header info;
	int file;

	if (strlen(curr_path) >= PATH_SIZE) {
		fprintf(stderr, "The path or name is too long\n");
		return -1;
	}

	file = open(curr_path, O_RDONLY);
	if (file == -1 || fstat(file, &buf_info) == -1) {
		perror("I/O error");
		return -1;
	}

	curr_size += buf_info.st_size;

	info.size = buf_info.st_size;
	info.s_mode = buf_info.st_mode;
	info.time.actime = buf_info.st_atimespec.tv_sec;
	info.time.modtime = buf_info.st_mtimespec.tv_sec;
	strcpy(&info.path, curr_path);

	if (write(to, &info, sizeof(struct Header)) == -1) {
		perror("I/O error");
		close(file);
		return -1;
	}

	if (move_file(file, to, info.size) == -1) {
		close(file);
		return -1;
	}

	close(file);
	return 0;
}

int move_file(const int from, const int to, int size_bytes)
{
	int n_bytes;
	int size_buf = 1024;

	char *tmp;

	if (size_bytes < -1) {
		fprintf(stderr, "Invalid argument for copy file\n");
		return -1;
	}
	if (size_bytes == -1) {
		struct stat info;

		fstat(from, &info);
		size_bytes = info.st_size;
	}

	char *buf = malloc(size_buf);

	if (buf == NULL) {
		perror("Memory Error");
		return -1;
	}
	tmp = buf;

	int mod;

	do {
		if (size_bytes == 0)
			break;

		mod = size_bytes % size_buf;
		n_bytes = mod ? mod : size_buf;
		size_bytes -= n_bytes;
		n_bytes = read(from, buf, n_bytes);

		if (n_bytes == -1 || write(to, buf, n_bytes) == -1) {
			perror("I/O error");
			free(buf);
			return -1;
		}

		// Ускоряем копирование с помощью удвоения буфера
		if (tmp != NULL && size_buf < MAX_MEMORY && size_buf < size_bytes - n_bytes) {
			tmp = realloc(buf, size_buf << 1);
			if (tmp != NULL) {
				buf = tmp;
				size_buf <<= 1;
			}
		}
	} while (n_bytes);

	free(buf);
	return 0;
}

int archive_dir(const char *path, const int to)
{
	DIR *dir;
	struct dirent *info_dir;
	struct timespec spec;
	long ms;

	dir = opendir(path);
	if (dir == NULL) {
		perror("Dir error");
		return -1;
	}

	while ((info_dir = readdir(dir)) != NULL) {
		char *next = malloc(strlen(path) + strlen(info_dir->d_name) + 2);

		if (next == NULL) {
			perror("Memory error");
			return -1;
		}
		sprintf(next, "%s/%s", path, info_dir->d_name);
		if (info_dir->d_type == DT_REG && archive_file(next, to) == -1)
			return -1;
		else if (info_dir->d_type == DT_DIR && strcmp(info_dir->d_name, ".")
			&& strcmp(info_dir->d_name, "..") && archive_dir(next, to) == -1)
			return -1;
		free(next);

		clock_gettime(CLOCK_REALTIME, &spec);
		ms = round(spec.tv_nsec / 1.0e6);
		if (ms - curr_time >= TIME_BAR) {
			showProgressBar();
			curr_time = ms;
		}
	}

	closedir(dir);
	return 0;
}

int recovery_path(const char path[])
{
	DIR *dir;

	size_t len = strlen(path);

	if (len == 0) {
		fprintf(stderr, "Invalid argument: path is empty\n");
		return -1;
	}

	dir = opendir(path);
	if (dir != NULL) {
		closedir(dir);
		return 0;
	}

	char *copy = malloc(len + 2);

	if (copy == NULL)
		goto dir_error;
	strcpy(copy, path);

	char *tmp = copy;

	if (copy[len - 1] != '/')
		strcat(copy, "/");
	if (copy[0] == '/')
		copy++;


	char *ptr = copy;

	while ((ptr = strchr(ptr, '/')) != NULL) {
		*ptr = 0;
		dir = opendir(copy);
		*ptr = '/';
		if (dir == NULL)
			break;
		ptr++;
		closedir(dir);
	}

	while ((ptr = strchr(ptr, '/')) != NULL) {
		*ptr = 0;
		if (mkdir(copy, 0775) == -1) {
			free(tmp);
			goto dir_error;
		}
		*ptr = '/';
		ptr++;
	}

	free(tmp);
	return 0;

dir_error:
		perror("Dir error");
		return -1;
}

int unpack(const int archive_file)
{
	struct Header info;
	struct timespec spec;
	long ms;

	if (read(archive_file, &total_size, sizeof(total_size)) == -1)
		goto IO_error;

	int out_file;
	int n_bytes;
	char *ptr;

	while ((n_bytes = read(archive_file, &info, sizeof(struct Header)))) {
		if (n_bytes == -1)
			goto IO_error;

		ptr = strrchr(info.path, '/');
		*ptr = 0;
		recovery_path(info.path);
		*ptr = '/';

		out_file = open(info.path, O_WRONLY | O_CREAT);
		if (out_file == -1)
			goto IO_error;

		curr_size += info.size;
		if (move_file(archive_file, out_file, info.size) == -1)
			return -1;

		if (fchmod(out_file, info.s_mode) == -1
		|| utime(info.path, &info.time) == -1
		|| close(out_file) == -1)
			goto IO_error;

		clock_gettime(CLOCK_REALTIME, &spec);
		ms = round(spec.tv_nsec / 1.0e6);
		if (ms - curr_time >= TIME_BAR) {
			showProgressBar();
			curr_time = ms;
		}
	}

	return 0;

IO_error:
		perror("I/O error");
		return -1;
}
