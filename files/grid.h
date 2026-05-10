#ifndef GRID_H
#define GRID_H

#include "types.h"

// инициализация кроссворда — выделяет память под сетку size x size
int  crossword_init(Crossword *cw, int size);

// освобождение памяти кроссворда
void crossword_free(Crossword *cw);

// проверка возможности размещения слова (1 — можно, 0 — нельзя)
int  grid_can_place(const Crossword *cw, const char *word, int len,
                    int row, int col, Direction dir);

// размещение слова (вызывать только после grid_can_place == 1)
int  grid_place_word(Crossword *cw,
                     const char *word, const char *question, int len,
                     int row, int col, Direction dir);

#endif
