#ifndef GENERATOR_H
#define GENERATOR_H

#include "types.h"
#include <stdio.h>

// минимальное количество слов каждой длины в готовом кроссворде
#define MIN_SHORT_WORDS  2   // длина <= 4
#define MIN_MEDIUM_WORDS 3   // длина 5-7
#define MIN_LONG_WORDS   1   // длина >= 8

// генерировать кроссворд size x size из AVL-дерева root
// log — открытый файл для логирования (NULL — без лога)
// возвращает количество размещённых слов
int generate_crossword(Crossword *cw, Node *root, int size, FILE *log);

#endif
