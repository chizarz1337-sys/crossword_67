#include "generator.h"
#include "grid.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// динамический пул указателей на узлы дерева
typedef struct {
    Node **nodes;
    int    count;
    int    cap;
} WordPool;

static int pool_init(WordPool *p, int initial_cap) {
    p->nodes = (Node**)malloc((size_t)initial_cap * sizeof(Node*));
    if (!p->nodes) { perror("malloc pool"); return -1; }
    p->count = 0;
    p->cap   = initial_cap;
    return 0;
}

static int pool_push(WordPool *p, Node *n) {
    if (p->count >= p->cap) {
        int new_cap = p->cap * 2;
        Node **tmp = (Node**)realloc(p->nodes, (size_t)new_cap * sizeof(Node*));
        if (!tmp) { perror("realloc pool"); return -1; }
        p->nodes = tmp;
        p->cap   = new_cap;
    }
    p->nodes[p->count++] = n;
    return 0;
}

static void pool_free(WordPool *p) {
    free(p->nodes);
    p->nodes = NULL;
    p->count = p->cap = 0;
}

// in-order обход: собираем все незанятые слова
static int collect_words(Node *node, WordPool *pool) {
    if (!node) return 0;
    if (collect_words(node->left, pool) < 0) return -1;
    if (!node->used)
        if (pool_push(pool, node) < 0) return -1;
    if (collect_words(node->right, pool) < 0) return -1;
    return 0;
}

// сортировка по убыванию длины
static int cmp_len_desc(const void *a, const void *b) {
    const Node *na = *(const Node **)a;
    const Node *nb = *(const Node **)b;
    return nb->length - na->length;
}

static int try_place(Crossword *cw, Node *node, FILE *log) {
    const char *word = node->word;
    int         len  = node->length;

    // слово длиннее сетки — пропустить
    if (len > cw->size) {
        if (log) fprintf(log, "[GEN] SKIP    \"%s\" — длиннее сетки\n", word);
        return 0;
    }

    // первое слово — горизонтально по центру
    if (cw->word_count == 0) {
        int row = cw->size / 2;
        int col = (cw->size - len) / 2;
        if (col < 0) col = 0;
        if (grid_can_place(cw, word, len, row, col, HORIZONTAL)) {
            grid_place_word(cw, word, node->question, len, row, col, HORIZONTAL);
            node->used = 1;
            if (log)
                fprintf(log, "[GEN] PLACED  #%d \"%s\" (%d,%d) HORIZONTAL (первое)\n",
                        cw->word_count, word, row, col);
            return 1;
        }
        return 0;
    }

    // последующие слова — ищем пересечение с уже стоящими
    for (int w = 0; w < cw->word_count; w++) {
        PlacedWord *pw     = &cw->words[w];
        Direction   newdir = (pw->dir == HORIZONTAL) ? VERTICAL : HORIZONTAL;
        int dr_pw  = (pw->dir == VERTICAL)   ? 1 : 0;
        int dc_pw  = (pw->dir == HORIZONTAL) ? 1 : 0;
        int dr_new = (newdir == VERTICAL)     ? 1 : 0;
        int dc_new = (newdir == HORIZONTAL)   ? 1 : 0;

        for (int j = 0; j < pw->length; j++) {
            int  ir      = pw->row + dr_pw * j;
            int  ic      = pw->col + dc_pw * j;
            char cell_ch = pw->word[j];

            for (int i = 0; i < len; i++) {
                if (word[i] != cell_ch) continue;
                int row = ir - dr_new * i;
                int col = ic - dc_new * i;
                if (grid_can_place(cw, word, len, row, col, newdir)) {
                    grid_place_word(cw, word, node->question, len, row, col, newdir);
                    node->used = 1;
                    if (log)
                        fprintf(log,
                            "[GEN] PLACED  #%d \"%s\" (%d,%d) %s (пересечение с \"%s\" (%d,%d) '%c')\n",
                            cw->word_count, word, row, col,
                            newdir == HORIZONTAL ? "HORIZONTAL" : "VERTICAL",
                            pw->word, ir, ic, cell_ch);
                    return 1;
                }
            }
        }
    }

    if (log) fprintf(log, "[GEN] SKIP    \"%s\" — нет позиции\n", word);
    return 0;
}

static void check_length_constraints(const Crossword *cw, FILE *log) {
    if (!log) return;
    int s = 0, m = 0, l = 0;
    for (int i = 0; i < cw->word_count; i++) {
        int len = cw->words[i].length;
        if      (len <= 4) s++;
        else if (len <= 7) m++;
        else               l++;
    }
    fprintf(log, "[GEN] Длины: коротких(<=4)=%d/%d  средних(5-7)=%d/%d  длинных(8+)=%d/%d\n",
            s, MIN_SHORT_WORDS, m, MIN_MEDIUM_WORDS, l, MIN_LONG_WORDS);
}

int generate_crossword(Crossword *cw, Node *root, int size, FILE *log) {
    clock_t t_start = clock();

    if (crossword_init(cw, size) < 0) return 0;

    if (!root) {
        if (log) fprintf(log, "[GEN] ERROR: словарь пуст\n");
        return 0;
    }

    WordPool pool;
    if (pool_init(&pool, 4096) < 0) return 0;
    if (collect_words(root, &pool) < 0) { pool_free(&pool); return 0; }

    if (pool.count == 0) {
        if (log) fprintf(log, "[GEN] ERROR: все слова уже использованы\n");
        pool_free(&pool);
        return 0;
    }

    qsort(pool.nodes, (size_t)pool.count, sizeof(Node*), cmp_len_desc);

    if (log)
        fprintf(log, "[GEN] START  size=%d  доступно слов=%d\n", size, pool.count);

    // жадный алгоритм: многопроходный
    // каждый проход — пробуем все незанятые слова
    // стоп — если за проход ничего не добавилось
    int placed_this_pass;
    do {
        placed_this_pass = 0;
        for (int i = 0; i < pool.count; i++) {
            if (!pool.nodes[i]->used)
                if (try_place(cw, pool.nodes[i], log))
                    placed_this_pass++;
        }
    } while (placed_this_pass > 0);

    clock_t t_end = clock();
    double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;

    check_length_constraints(cw, log);
    if (log)
        fprintf(log, "[GEN] DONE   размещено=%d  время=%.4fs\n",
                cw->word_count, elapsed);

    pool_free(&pool);
    return cw->word_count;
}
