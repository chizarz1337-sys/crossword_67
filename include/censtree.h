#ifndef CENSTREE_H
#define CENSTREE_H

#include <stdlib.h>

#define MAX_WORD_LEN      50
#define MAX_QUESTION_LEN  1000

// AVL-узел словаря
typedef struct Node {
    char  word    [MAX_WORD_LEN     + 1];
    char  question[MAX_QUESTION_LEN + 1];
    int   length;
    int   used;
    int   hight;
    struct Node* left;
    struct Node* right;
} Node;

// направление слова в сетке
typedef enum { HORIZONTAL, VERTICAL } Direction;

// одна клетка сетки
typedef struct {
    char letter;
    int  word_num;
} Cell;

// слово, размещённое в кроссворде
typedef struct {
    int       number;
    int       row, col;
    Direction dir;
    int       length;
    char      word    [MAX_WORD_LEN     + 1];
    char      question[MAX_QUESTION_LEN + 1];
} PlacedWord;

// кроссворд — сетка и список слов полностью динамические
typedef struct {
    int        size;
    Cell     **grid;
    Cell      *grid_data;
    PlacedWord *words;
    int        word_count;
    int        word_cap;
} Crossword;

// публичные функции AVL-дерева
Node* c_insert    (Node* node, const char* word, const char* question, int length);
Node* c_deleteNode(Node* root, const char* word);
Node* c_search    (Node* root, const char* word);
int   c_getSize   (Node* root);
int   c_getHeight (Node* root);
void  c_printTree (Node* root);
void  c_resetUsed (Node* root);

#endif
