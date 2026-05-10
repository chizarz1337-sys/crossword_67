#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"
#include <stdio.h>

// вывести сетку кроссворда
void display_grid(const Crossword *cw, FILE *out);

// вывести список вопросов
void display_questions(const Crossword *cw, FILE *out);

// вывести список ответов
void display_answers(const Crossword *cw, FILE *out);

// вывести сетку + вопросы + ответы
void display_all(const Crossword *cw, FILE *out);

// сохранить в файл
void display_to_file(const Crossword *cw, const char *filename);

#endif
