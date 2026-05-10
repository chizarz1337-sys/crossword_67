#include "display.h"
#include <stdio.h>

#define EMPTY_CELL '.'

void display_grid(const Crossword *cw, FILE *out) {
    fprintf(out, "\nCROSSWORD %dx%d\n\n", cw->size, cw->size);

    // шапка с номерами столбцов (повторяем каждые 10)
    fprintf(out, "     ");
    for (int c = 0; c < cw->size; c++)
        fprintf(out, "%2d", c % 10);
    fprintf(out, "\n     ");
    for (int c = 0; c < cw->size; c++)
        fprintf(out, "--");
    fprintf(out, "\n");

    for (int r = 0; r < cw->size; r++) {
        fprintf(out, "%3d |", r);
        for (int c = 0; c < cw->size; c++) {
            char ch = cw->grid[r][c].letter;
            fprintf(out, " %c", ch ? ch : EMPTY_CELL);
        }
        // номера слов, начинающихся в этой строке
        fprintf(out, "  |");
        for (int c = 0; c < cw->size; c++) {
            int num = cw->grid[r][c].word_num;
            if (num > 0) fprintf(out, " [%d]", num);
        }
        fprintf(out, "\n");
    }

    fprintf(out, "     ");
    for (int c = 0; c < cw->size; c++)
        fprintf(out, "--");
    fprintf(out, "\n\n");
}

void display_questions(const Crossword *cw, FILE *out) {
    fprintf(out, "=== ВОПРОСЫ ===\n");
    for (int i = 0; i < cw->word_count; i++) {
        const PlacedWord *pw = &cw->words[i];
        fprintf(out, "%4d. [%s] %s\n",
                pw->number,
                pw->dir == HORIZONTAL ? "→" : "↓",
                pw->question[0] ? pw->question : "(нет вопроса)");
    }
    fprintf(out, "\n");
}

void display_answers(const Crossword *cw, FILE *out) {
    fprintf(out, "=== ОТВЕТЫ ===\n");
    for (int i = 0; i < cw->word_count; i++) {
        const PlacedWord *pw = &cw->words[i];
        fprintf(out, "%4d. %s\n", pw->number, pw->word);
    }
    fprintf(out, "\n");
}

void display_all(const Crossword *cw, FILE *out) {
    display_grid(cw, out);
    display_questions(cw, out);
    display_answers(cw, out);
}

void display_to_file(const Crossword *cw, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "[DISPLAY] Не удалось открыть '%s'\n", filename);
        return;
    }
    display_all(cw, f);
    fclose(f);
    fprintf(stdout, "[DISPLAY] Сохранено в '%s'\n", filename);
}
