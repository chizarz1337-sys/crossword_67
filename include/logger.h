#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

/* Инициализация логирования (файл + консоль) */
void log_init(const char* log_file);
void log_close(void);

/* Уровни логирования */
void log_info (const char* module, const char* fmt, ...);
void log_warn (const char* module, const char* fmt, ...);
void log_error(const char* module, const char* fmt, ...);

#endif /* LOGGER_H */