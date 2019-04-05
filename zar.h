#ifndef ZAR
#define ZAR

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ftw.h>
#include <math.h>
#include <utime.h>

#define PATH_SIZE 200
#define MAX_MEMORY 1073741824 // 1 Гб
#define TIME_BAR 1000 // Время обновления статус бара в мс

extern unsigned int total_size; // Общий размер файлов в директории
extern unsigned int curr_size;

struct Header {
	char    path[PATH_SIZE]; // Путь файла
        off_t  size;        // Размер
        mode_t s_mode;      // Protection mode
        struct utimbuf time; // Дата изменения/доступа
};

int recovery_path(const char path[]);
/* Восстанавливает путь к заданной директории
 * Аргументы:
 * path - путь к директории
 * Возвращает:
 * 0 - успешно
 * -1 - ошибка
 */

int archive_file(const char *curr_path, const int to);
/* Архивирует заданный файл
 * Аргументы:
 * curr_path - путь к файлу
 * to - дескриптор архива
 * Возвращает:
 * 0 - успешно
 * -1 - ошибка
 */

int move_file(const int from, const int to, int size_bytes);
/* Копирует заданное кол-во байт из одного файла в другой
 * Аргументы:
 * from - дескриптор копируемого файла
 * to - дескриптор конечного файла
 * size_bytes - кол-во копируемых байт
 * Возвращает:
 * 0 - успешно
 * -1 - ошибка
 */

int archive_dir(const char *path, const int to);
/* Архивирует заданную директорию
 * Аргументы:
 * path - путь к директории
 * to - дескриптор архива
 * Возвращает:
 * 0 - успешно
 * -1 - ошибка
 */

int unpack(const int archive_file);
/* Распаковывает заданный файл
 * Аргументы:
 * archive_file - дескриптор архива
 * Возвращает:
 * 0 - успешно
 * -1 - ошибка
 */

int sum(const char *fpath, const struct stat *sb, int typeflag);
/* Суммирует размеры файлов в директории
 * Аргументы:
 * fpath - путь к объекту(файлу или директории)
 * sb - структура описания файла
 * typeflag - тип объекта
 * Возвращает:
 * 0 - успешно
 * -1 - ошибка
 */

void showProgressBar();
/* Отображает прогресс архивации/распаковки
 */

#endif
