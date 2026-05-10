#include "libs.h"
#include "censtree.h"
#include "dictionary.h"
#include "logger.h"
#include "../files/generator.h"
#include "../files/display.h"
#include "../files/grid.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define DEFAULT_GRID_SIZE 20

static void print_help(void)
{
    printf("Генератор кроссвордов (жадный алгоритм)\n"
           "Использование: crossword_gen [опции]\n\n"
           "  --help                  показать справку\n"
           "  --authors               авторы проекта\n"
           "  --dict <file.txt>       загрузить словарь из текста (word|question)\n"
           "  --save-bin <file.bin>   сохранить словарь в бинарный файл\n"
           "  --load-bin <file.bin>   загрузить словарь из бинарного файла\n"
           "  --generate              запустить генерацию кроссворда\n"
           "  --size <N>              размер сетки N x N (по умолчанию: %d)\n"
           "  --out <file.txt>        сохранить кроссворд в файл\n"
           "  --log <file.log>        файл лога (по умолчанию: crossword.log)\n",
           DEFAULT_GRID_SIZE);
}

static void print_authors(void)
{
    printf("Авторы: [Ваши ФИО]\n"
           "Группа: [Номер группы]\n"
           "Дисциплина: [Название]\n"
           "Проект: Генератор кроссвордов (AVL-словарь + жадное размещение)\n");
}

static void free_tree(Node* n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    const char* log_file  = "crossword.log";
    const char* dict_text = NULL;
    const char* bin_save  = NULL;
    const char* bin_load  = NULL;
    const char* out_file  = NULL;
    int do_generate = 0;
    int grid_size   = DEFAULT_GRID_SIZE;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--help")     == 0) { print_help();    return EXIT_SUCCESS; }
        else if (strcmp(argv[i], "--authors")  == 0) { print_authors(); return EXIT_SUCCESS; }
        else if (strcmp(argv[i], "--generate") == 0) { do_generate = 1; }
        else if (strcmp(argv[i], "--log")      == 0 && i+1 < argc) log_file  = argv[++i];
        else if (strcmp(argv[i], "--dict")     == 0 && i+1 < argc) dict_text = argv[++i];
        else if (strcmp(argv[i], "--save-bin") == 0 && i+1 < argc) bin_save  = argv[++i];
        else if (strcmp(argv[i], "--load-bin") == 0 && i+1 < argc) bin_load  = argv[++i];
        else if (strcmp(argv[i], "--out")      == 0 && i+1 < argc) out_file  = argv[++i];
        else if (strcmp(argv[i], "--size")     == 0 && i+1 < argc) {
            grid_size = atoi(argv[++i]);
            if (grid_size < 5) {
                fprintf(stderr, "Ошибка: размер сетки не может быть меньше 5\n");
                return EXIT_FAILURE;
            }
        }
    }

    log_init(log_file);
    log_info("MAIN", "Программа запущена. Аргументов: %d", argc - 1);

    // загрузка словаря
    Node* dict = NULL;
    if (bin_load) {
        dict = dict_load_binary(bin_load);
    } else if (dict_text) {
        dict = dict_load_text(dict, dict_text);
    }

    if (!dict && (do_generate || bin_save)) {
        log_warn("MAIN", "Словарь не загружен");
        fprintf(stderr, "Словарь не загружен. Используйте --dict или --load-bin.\n");
        log_close();
        return EXIT_FAILURE;
    }

    // сохранение в бинарный файл
    if (bin_save && dict) {
        dict_save_binary(dict, bin_save);
    }

    // генерация кроссворда
    if (do_generate && dict) {
        log_info("MAIN", "Словарь: %d слов. Генерация, сетка %dx%d",
                 c_getSize(dict), grid_size, grid_size);

        Crossword cw;
        int placed = generate_crossword(&cw, dict, grid_size, NULL);

        if (placed == 0) {
            log_warn("MAIN", "Не удалось разместить ни одного слова");
            fprintf(stderr, "Генерация не дала результата для сетки %dx%d.\n",
                    grid_size, grid_size);
        } else {
            log_info("MAIN", "Размещено слов: %d", placed);
            display_all(&cw, stdout);
            if (out_file) {
                display_to_file(&cw, out_file);
                log_info("MAIN", "Кроссворд сохранён: %s", out_file);
            }
        }
        crossword_free(&cw);
    }

    if (dict) {
        log_info("MAIN", "Итоговый размер словаря: %d слов", c_getSize(dict));
        free_tree(dict);
    }

    if (!do_generate && !bin_save && !bin_load && !dict_text)
        print_help();

    log_close();
    return EXIT_SUCCESS;
}
