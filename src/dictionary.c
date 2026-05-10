#include "libs.h"
#include "censtree.h"
#include "dictionary.h"
#include "logger.h"

static char* trim(char* str)
{
    while (*str && isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

Node* dict_load_text(Node* root, const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        log_error("DICT", "Не удалось открыть текстовый словарь: %s", filename);
        return root;
    }
    char line[2048];
    int lines = 0, errors = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        char* t = trim(line);
        if (*t == '\0' || *t == '#') continue;
        char* sep = strchr(t, '|');
        if (!sep) { log_warn("DICT", "Строка без '|': %s", t); errors++; continue; }
        *sep = '\0';
        char* word  = trim(t);
        char* quest = trim(sep + 1);
        if (*word == '\0') { errors++; continue; }
        if (*quest == '\0') quest = "(вопрос не указан)";
        root = c_insert(root, word, quest, (int)strlen(word));
        lines++;
    }
    fclose(fp);
    log_info("DICT", "Загружено %d строк, ошибок формата: %d", lines, errors);
    return root;
}

// запись бинарного файла: сначала int count, затем массив записей
typedef struct {
    char word    [MAX_WORD_LEN     + 1];
    char question[MAX_QUESTION_LEN + 1];
    int  length;
} BinRecord;

static void collect_nodes(Node* n, FILE* fp, int* count)
{
    if (!n) return;
    collect_nodes(n->left, fp, count);
    BinRecord rec;
    memcpy(rec.word,     n->word,     sizeof(rec.word));
    memcpy(rec.question, n->question, sizeof(rec.question));
    rec.length = n->length;
    fwrite(&rec, sizeof(BinRecord), 1, fp);
    (*count)++;
    collect_nodes(n->right, fp, count);
}

int dict_save_binary(Node* root, const char* filename)
{
    FILE* fp = fopen(filename, "wb");
    if (!fp) { log_error("DICT", "Ошибка создания бинарного файла: %s", filename); return -1; }
    // первый проход — считаем количество
    int count = 0;
    collect_nodes(root, fp, &count);
    fclose(fp);
    // второй проход — пишем count в заголовок, затем данные
    fp = fopen(filename, "wb");
    if (!fp) return -1;
    fwrite(&count, sizeof(int), 1, fp);
    int dummy = 0;
    collect_nodes(root, fp, &dummy);
    fclose(fp);
    log_info("DICT", "Бинарный словарь сохранён: %d слов -> %s", count, filename);
    return 0;
}

Node* dict_load_binary(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) { log_error("DICT", "Не удалось открыть бинарный словарь: %s", filename); return NULL; }
    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        log_error("DICT", "Повреждённый заголовок бинарного файла");
        fclose(fp); return NULL;
    }
    Node* root = NULL;
    BinRecord rec;
    for (int i = 0; i < count; i++) {
        if (fread(&rec, sizeof(BinRecord), 1, fp) != 1) break;
        root = c_insert(root, rec.word, rec.question, rec.length);
    }
    fclose(fp);
    log_info("DICT", "Бинарный словарь загружен: %d слов", count);
    return root;
}
