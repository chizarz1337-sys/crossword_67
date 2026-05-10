#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include "censtree.h"
#include "dictionary.h"
#include "logger.h"
#include "generator.h"
#include "grid.h"

#define RESULTS_DIR   "tests/results"
#define TMP_BIN       "tests/results/tmp_dict.bin"
#define BENCH_REPEATS 5

// размеры сеток для теста генерации
#define MIN_GRID_BENCH 10
#define MAX_GRID_BENCH 100
#define GRID_STEP      10

static double now_sec(void) {
#if defined(_WIN32)
    return (double)clock() / CLOCKS_PER_SEC;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

static void ensure_results_dir(void) {
#if defined(_WIN32)
    int r = system("if not exist tests\\results mkdir tests\\results"); (void)r;
#else
    int r = system("mkdir -p " RESULTS_DIR); (void)r;
#endif
}

static void free_tree(Node* n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

// запись в массив DictEntry — читает весь файл без лимита на количество
typedef struct {
    char word    [MAX_WORD_LEN     + 1];
    char question[MAX_QUESTION_LEN + 1];
} DictEntry;

static int load_entries(const char* filename, DictEntry** out) {
    FILE* fp = fopen(filename, "r");
    if (!fp) { fprintf(stderr, "Не удалось открыть %s\n", filename); return 0; }

    // первый проход — считаем строки
    int n = 0;
    char line[2048];
    while (fgets(line, sizeof(line), fp))
        if (strchr(line, '|')) n++;
    rewind(fp);

    *out = (DictEntry*)malloc((size_t)n * sizeof(DictEntry));
    if (!*out) { fclose(fp); return 0; }

    int idx = 0;
    while (fgets(line, sizeof(line), fp) && idx < n) {
        line[strcspn(line, "\r\n")] = '\0';
        char* sep = strchr(line, '|');
        if (!sep) continue;
        *sep = '\0';
        snprintf((*out)[idx].word,     MAX_WORD_LEN     + 1, "%s", line);
        snprintf((*out)[idx].question, MAX_QUESTION_LEN + 1, "%s", sep + 1);
        idx++;
    }
    fclose(fp);
    return idx;
}

// ТЕСТ 1: накопленное время вставки одного за одним
static void bench_insert(int n_entries, DictEntry* entries) {
    char path[512];
    snprintf(path, sizeof(path), "%s/insert_time.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen insert_time.csv"); return; }

    fprintf(csv, "n_words,time_sec,rotations\n");
    printf("[BENCH] Вставка %d слов...\n", n_entries);

    extern int c_rot;

    for (int r = 0; r < BENCH_REPEATS; r++) {
        Node* tree = NULL;
        c_rot = 0;
        double t0 = now_sec();
        for (int i = 0; i < n_entries; i++) {
            tree = c_insert(tree, entries[i].word, entries[i].question,
                            (int)strlen(entries[i].word));
            // пишем в CSV только при первом повторе
            if (r == 0)
                fprintf(csv, "%d,%.9f,%d\n", i + 1, now_sec() - t0, c_rot);
        }
        free_tree(tree);
    }
    fclose(csv);
    printf("[BENCH] insert_time.csv готов\n");
}

// ТЕСТ 2: загрузка из текстового файла
static void bench_load_text(const char* dict_txt) {
    char path[512];
    snprintf(path, sizeof(path), "%s/load_text_time.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen load_text_time.csv"); return; }

    fprintf(csv, "repeat,n_words,time_sec\n");
    printf("[BENCH] Загрузка из текста (%d повторов)...\n", BENCH_REPEATS);

    for (int r = 0; r < BENCH_REPEATS; r++) {
        double t0   = now_sec();
        Node*  tree = dict_load_text(NULL, dict_txt);
        double elapsed = now_sec() - t0;
        int n = c_getSize(tree);
        fprintf(csv, "%d,%d,%.9f\n", r + 1, n, elapsed);
        free_tree(tree);
    }
    fclose(csv);
    printf("[BENCH] load_text_time.csv готов\n");
}

// ТЕСТ 3: сохранение в бинарный файл
static void bench_save_binary(const char* dict_txt) {
    Node* tree = dict_load_text(NULL, dict_txt);
    if (!tree) { fprintf(stderr, "[BENCH] Словарь пуст, пропуск\n"); return; }
    int n = c_getSize(tree);

    char path[512];
    snprintf(path, sizeof(path), "%s/save_binary_time.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen save_binary_time.csv"); free_tree(tree); return; }

    fprintf(csv, "repeat,n_words,time_sec,file_bytes\n");
    printf("[BENCH] Сохранение в бинарный файл (%d повторов)...\n", BENCH_REPEATS);

    for (int r = 0; r < BENCH_REPEATS; r++) {
        double t0 = now_sec();
        dict_save_binary(tree, TMP_BIN);
        double elapsed = now_sec() - t0;
        struct stat st;
        long bytes = (stat(TMP_BIN, &st) == 0) ? (long)st.st_size : -1;
        fprintf(csv, "%d,%d,%.9f,%ld\n", r + 1, n, elapsed, bytes);
    }
    fclose(csv);
    free_tree(tree);
    printf("[BENCH] save_binary_time.csv готов\n");
}

// ТЕСТ 4: загрузка из бинарного файла
static void bench_load_binary(const char* dict_txt) {
    Node* tree = dict_load_text(NULL, dict_txt);
    if (!tree) { fprintf(stderr, "[BENCH] Словарь пуст, пропуск\n"); return; }
    dict_save_binary(tree, TMP_BIN);
    free_tree(tree);

    char path[512];
    snprintf(path, sizeof(path), "%s/load_binary_time.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen load_binary_time.csv"); return; }

    fprintf(csv, "repeat,n_words,time_sec\n");
    printf("[BENCH] Загрузка из бинарного файла (%d повторов)...\n", BENCH_REPEATS);

    for (int r = 0; r < BENCH_REPEATS; r++) {
        double t0     = now_sec();
        Node*  loaded = dict_load_binary(TMP_BIN);
        double elapsed = now_sec() - t0;
        int n = c_getSize(loaded);
        fprintf(csv, "%d,%d,%.9f\n", r + 1, n, elapsed);
        free_tree(loaded);
    }
    fclose(csv);
    printf("[BENCH] load_binary_time.csv готов\n");
}

// ТЕСТ 5: сравнение text vs binary загрузки
static void bench_load_comparison(const char* dict_txt) {
    Node* tmp = dict_load_text(NULL, dict_txt);
    if (!tmp) return;
    dict_save_binary(tmp, TMP_BIN);
    int n = c_getSize(tmp);
    free_tree(tmp);

    char path[512];
    snprintf(path, sizeof(path), "%s/load_comparison.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen load_comparison.csv"); return; }

    fprintf(csv, "method,repeat,n_words,time_sec\n");
    printf("[BENCH] text vs binary загрузка...\n");

    for (int r = 0; r < BENCH_REPEATS; r++) {
        double t0;

        t0 = now_sec();
        Node* t1 = dict_load_text(NULL, dict_txt);
        fprintf(csv, "text,%d,%d,%.9f\n", r + 1, n, now_sec() - t0);
        free_tree(t1);

        t0 = now_sec();
        Node* t2 = dict_load_binary(TMP_BIN);
        fprintf(csv, "binary,%d,%d,%.9f\n", r + 1, n, now_sec() - t0);
        free_tree(t2);
    }
    fclose(csv);
    printf("[BENCH] load_comparison.csv готов\n");
}

// ТЕСТ 6: время генерации для разных размеров сетки
static void bench_generation(const char* dict_txt) {
    char path[512];
    snprintf(path, sizeof(path), "%s/generation_time.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen generation_time.csv"); return; }

    fprintf(csv, "grid_size,repeat,words_placed,time_sec,fill_ratio\n");
    printf("[BENCH] Генерация для сеток %d..%d шаг %d (%d повторов)...\n",
           MIN_GRID_BENCH, MAX_GRID_BENCH, GRID_STEP, BENCH_REPEATS);

    for (int size = MIN_GRID_BENCH; size <= MAX_GRID_BENCH; size += GRID_STEP) {
        printf("  %dx%d ...\n", size, size);
        for (int r = 0; r < BENCH_REPEATS; r++) {
            // свежее дерево на каждый запуск — флаги used чистые
            Node* tree = dict_load_text(NULL, dict_txt);
            if (!tree) break;

            Crossword cw;
            double t0     = now_sec();
            int    placed = generate_crossword(&cw, tree, size, NULL);
            double elapsed = now_sec() - t0;

            // доля заполненных клеток
            int filled = 0;
            for (int i = 0; i < size; i++)
                for (int j = 0; j < size; j++)
                    if (cw.grid[i][j].letter != '\0') filled++;
            double fill = (double)filled / ((double)size * size);

            fprintf(csv, "%d,%d,%d,%.9f,%.4f\n", size, r + 1, placed, elapsed, fill);
            crossword_free(&cw);
            free_tree(tree);
        }
    }
    fclose(csv);
    printf("[BENCH] generation_time.csv готов\n");
}

// ТЕСТ 7: рост высоты AVL-дерева при добавлении N слов
static void bench_avl_height(int n_entries, DictEntry* entries) {
    char path[512];
    snprintf(path, sizeof(path), "%s/avl_height.csv", RESULTS_DIR);
    FILE* csv = fopen(path, "w");
    if (!csv) { perror("fopen avl_height.csv"); return; }

    fprintf(csv, "n_words,height,log2_n,rotations_total\n");
    printf("[BENCH] Рост высоты AVL (%d слов)...\n", n_entries);

    extern int c_rot;
    Node* tree = NULL;
    c_rot = 0;

    for (int i = 0; i < n_entries; i++) {
        tree = c_insert(tree, entries[i].word, entries[i].question,
                        (int)strlen(entries[i].word));
        int    h  = c_getHeight(tree);
        double lg = (i + 1 > 1) ? (log((double)(i + 1)) / log(2.0)) : 0.0;
        fprintf(csv, "%d,%d,%.3f,%d\n", i + 1, h, lg, c_rot);
    }
    free_tree(tree);
    fclose(csv);
    printf("[BENCH] avl_height.csv готов\n");
}

int main(int argc, char* argv[]) {
    const char* dict_txt = (argc > 1) ? argv[1] : "tests/dict_test.txt";

    printf("=== BENCHMARK: Crossword Generator ===\n\n");
    printf("Словарь : %s\n", dict_txt);
    printf("Повторов: %d\n\n", BENCH_REPEATS);

#if defined(_WIN32)
    log_init("nul");
#else
    log_init("/dev/null");
#endif

    ensure_results_dir();

    DictEntry* entries = NULL;
    int n = load_entries(dict_txt, &entries);
    if (n == 0) {
        fprintf(stderr, "Не удалось загрузить словарь: %s\n", dict_txt);
        return EXIT_FAILURE;
    }
    printf("Загружено слов: %d\n\n", n);

    bench_insert         (n, entries);
    bench_load_text      (dict_txt);
    bench_save_binary    (dict_txt);
    bench_load_binary    (dict_txt);
    bench_load_comparison(dict_txt);
    bench_generation     (dict_txt);
    bench_avl_height     (n, entries);

    free(entries);
    log_close();

    printf("\n=== CSV-файлы в %s/ ===\n", RESULTS_DIR);
    printf("  insert_time.csv       накопленное время вставки\n");
    printf("  load_text_time.csv    загрузка из текста\n");
    printf("  save_binary_time.csv  сохранение в бинарный\n");
    printf("  load_binary_time.csv  загрузка из бинарного\n");
    printf("  load_comparison.csv   text vs binary\n");
    printf("  generation_time.csv   генерация для разных размеров\n");
    printf("  avl_height.csv        рост высоты AVL\n");

    return EXIT_SUCCESS;
}
