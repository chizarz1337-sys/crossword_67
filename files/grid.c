#include "grid.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int crossword_init(Crossword *cw, int size) {
    cw->size       = size;
    cw->word_count = 0;
    cw->word_cap   = 256;
    cw->grid       = NULL;
    cw->grid_data  = NULL;
    cw->words      = NULL;

    // непрерывный блок для всех клеток сетки
    cw->grid_data = (Cell*)calloc((size_t)size * size, sizeof(Cell));
    if (!cw->grid_data) { perror("calloc grid_data"); return -1; }

    // массив указателей на строки
    cw->grid = (Cell**)malloc((size_t)size * sizeof(Cell*));
    if (!cw->grid) { perror("malloc grid rows"); free(cw->grid_data); return -1; }

    for (int i = 0; i < size; i++)
        cw->grid[i] = cw->grid_data + (size_t)i * size;

    // начальный буфер для размещённых слов — растёт через realloc
    cw->words = (PlacedWord*)malloc((size_t)cw->word_cap * sizeof(PlacedWord));
    if (!cw->words) { perror("malloc words"); free(cw->grid); free(cw->grid_data); return -1; }

    return 0;
}

void crossword_free(Crossword *cw) {
    free(cw->grid_data);
    free(cw->grid);
    free(cw->words);
    cw->grid_data  = NULL;
    cw->grid       = NULL;
    cw->words      = NULL;
    cw->size       = 0;
    cw->word_count = 0;
    cw->word_cap   = 0;
}

int grid_can_place(const Crossword *cw, const char *word, int len,
                   int row, int col, Direction dir) {
    int dr = (dir == VERTICAL)   ? 1 : 0;
    int dc = (dir == HORIZONTAL) ? 1 : 0;
    int sz = cw->size;

    // 1. границы сетки
    int end_r = row + dr * (len - 1);
    int end_c = col + dc * (len - 1);
    if (row < 0 || col < 0 || end_r >= sz || end_c >= sz)
        return 0;

    // 2. клетки непосредственно до и после слова должны быть пустыми
    int pr = row - dr, pc = col - dc;
    if (pr >= 0 && pc >= 0 && cw->grid[pr][pc].letter != '\0')
        return 0;
    int nr = row + dr * len, nc = col + dc * len;
    if (nr < sz && nc < sz && cw->grid[nr][nc].letter != '\0')
        return 0;

    int has_intersection = 0;

    for (int i = 0; i < len; i++) {
        int  r        = row + dr * i;
        int  c        = col + dc * i;
        char existing = cw->grid[r][c].letter;

        if (existing == '\0') {
            // 3. боковые соседи пустой клетки должны быть пустыми
            int s1r = r + dc, s1c = c + dr;
            int s2r = r - dc, s2c = c - dr;
            if (s1r >= 0 && s1r < sz && s1c >= 0 && s1c < sz &&
                cw->grid[s1r][s1c].letter != '\0') return 0;
            if (s2r >= 0 && s2r < sz && s2c >= 0 && s2c < sz &&
                cw->grid[s2r][s2c].letter != '\0') return 0;
        } else if (existing == word[i]) {
            // 4. пересечение совпадает — допустимо
            has_intersection = 1;
        } else {
            // конфликт букв
            return 0;
        }
    }

    return has_intersection || (cw->word_count == 0);
}

int grid_place_word(Crossword *cw,
                    const char *word, const char *question, int len,
                    int row, int col, Direction dir) {
    // расширяем массив words при необходимости
    if (cw->word_count >= cw->word_cap) {
        int new_cap = cw->word_cap * 2;
        PlacedWord *tmp = (PlacedWord*)realloc(cw->words,
                                               (size_t)new_cap * sizeof(PlacedWord));
        if (!tmp) { perror("realloc words"); return -1; }
        cw->words    = tmp;
        cw->word_cap = new_cap;
    }

    int dr  = (dir == VERTICAL)   ? 1 : 0;
    int dc  = (dir == HORIZONTAL) ? 1 : 0;
    int num = cw->word_count + 1;

    cw->grid[row][col].word_num = num;
    for (int i = 0; i < len; i++)
        cw->grid[row + dr * i][col + dc * i].letter = word[i];

    PlacedWord *pw = &cw->words[cw->word_count++];
    pw->number = num;
    pw->row    = row;
    pw->col    = col;
    pw->dir    = dir;
    pw->length = len;
    strncpy(pw->word,     word,     MAX_WORD_LEN);     pw->word[MAX_WORD_LEN]         = '\0';
    strncpy(pw->question, question, MAX_QUESTION_LEN); pw->question[MAX_QUESTION_LEN] = '\0';
    return 0;
}
