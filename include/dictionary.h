#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "censtree.h"

/* Загрузка из текстового файла (word|question) */
Node* dict_load_text(Node* root, const char* filename);

/* Сохранение в бинарный файл (быстрая загрузка) */
int   dict_save_binary(Node* root, const char* filename);

/* Загрузка из бинарного файла */
Node* dict_load_binary(const char* filename);

#endif /* DICTIONARY_H */