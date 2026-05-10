#include "libs.h"
#include "logger.h"

static FILE* log_fp = NULL;

static void log_write(const char* level, const char* module, const char* fmt, va_list args)
{
    time_t now = time(NULL);
    char tstr[64];
    strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    if (log_fp) {
        // va_list нельзя использовать дважды — копируем
        va_list args_copy;
        va_copy(args_copy, args);
        fprintf(stderr,  "[%s] [%s] %-12s: ", tstr, level, module);
        vfprintf(stderr,  fmt, args);
        fprintf(stderr,  "\n");
        fprintf(log_fp,  "[%s] [%s] %-12s: ", tstr, level, module);
        vfprintf(log_fp,  fmt, args_copy);
        fprintf(log_fp,  "\n");
        fflush(log_fp);
        va_end(args_copy);
    } else {
        fprintf(stderr, "[%s] [%s] %-12s: ", tstr, level, module);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }
}

void log_init(const char* log_file)
{
    log_fp = fopen(log_file, "a");
    if (!log_fp)
        fprintf(stderr, "[WARN] Не удалось открыть файл лога '%s'\n", log_file);
    else
        log_info("LOGGER", "Логирование инициализировано. Файл: %s", log_file);
}

void log_close(void)
{
    if (log_fp) {
        log_info("LOGGER", "Логирование завершено.");
        fclose(log_fp);
        log_fp = NULL;
    }
}

void log_info (const char* m, const char* f, ...) { va_list a; va_start(a, f); log_write("INFO",  m, f, a); va_end(a); }
void log_warn (const char* m, const char* f, ...) { va_list a; va_start(a, f); log_write("WARN",  m, f, a); va_end(a); }
void log_error(const char* m, const char* f, ...) { va_list a; va_start(a, f); log_write("ERROR", m, f, a); va_end(a); }
