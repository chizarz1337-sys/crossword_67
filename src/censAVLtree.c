#include "libs.h"
#include "censtree.h"

// счётчики операций
int c_ins = 0, c_del = 0, c_srch = 0, c_sz = 0, c_hgt = 0, c_prn = 0, c_rot = 0;

static Node* createNode(const char* word, const char* question, int length)
{
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) { perror("malloc failed"); exit(EXIT_FAILURE); }
    strncpy(node->word,     word,     MAX_WORD_LEN);
    node->word[MAX_WORD_LEN] = '\0';
    strncpy(node->question, question, MAX_QUESTION_LEN);
    node->question[MAX_QUESTION_LEN] = '\0';
    node->length = length;
    node->used   = 0;
    node->hight  = 1;
    node->left   = NULL;
    node->right  = NULL;
    return node;
}

static int getHeight(Node* n)  { return n ? n->hight : 0; }
static int imax(int a, int b)  { return a > b ? a : b; }
static int getBalance(Node* n) { return n ? getHeight(n->left) - getHeight(n->right) : 0; }

static void updateHeight(Node* n) {
    if (n) n->hight = imax(getHeight(n->left), getHeight(n->right)) + 1;
}

static Node* rightRotate(Node* y) {
    c_rot++;
    Node* x  = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left  = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
}

static Node* leftRotate(Node* x) {
    if (!x || !x->right) return x;
    c_rot++;
    Node* y  = x->right;
    Node* T2 = y->left;
    y->left  = x;
    x->right = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
}

static Node* balance(Node* node, const char* key) {
    updateHeight(node);
    int b = getBalance(node);
    // LL
    if (b > 1 && strcasecmp(key, node->left->word) < 0)
        return rightRotate(node);
    // RR
    if (b < -1 && strcasecmp(key, node->right->word) > 0)
        return leftRotate(node);
    // LR
    if (b > 1 && strcasecmp(key, node->left->word) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    // RL
    if (b < -1 && strcasecmp(key, node->right->word) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

static Node* insert(Node* node, const char* word, const char* question, int length)
{
    if (!node) return createNode(word, question, length);
    int cmp = strcasecmp(word, node->word);
    if      (cmp < 0) node->left  = insert(node->left,  word, question, length);
    else if (cmp > 0) node->right = insert(node->right, word, question, length);
    else              return node;
    return balance(node, word);
}

static Node* minValueNode(Node* node) {
    while (node->left) node = node->left;
    return node;
}

static Node* rebalance(Node* root) {
    if (!root) return root;
    updateHeight(root);
    int b = getBalance(root);
    if (b > 1) {
        if (getBalance(root->left) < 0)
            root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (b < -1) {
        if (getBalance(root->right) > 0)
            root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

static Node* deleteNode(Node* root, const char* word)
{
    if (!root) return NULL;
    int cmp = strcasecmp(word, root->word);
    if      (cmp < 0) root->left  = deleteNode(root->left,  word);
    else if (cmp > 0) root->right = deleteNode(root->right, word);
    else {
        if (!root->left || !root->right) {
            Node* tmp = root->left ? root->left : root->right;
            if (!tmp) { free(root); return NULL; }
            *root = *tmp;
            free(tmp);
        } else {
            Node* tmp = minValueNode(root->right);
            strncpy(root->word,     tmp->word,     MAX_WORD_LEN);
            root->word[MAX_WORD_LEN] = '\0';
            strncpy(root->question, tmp->question, MAX_QUESTION_LEN);
            root->question[MAX_QUESTION_LEN] = '\0';
            root->length = tmp->length;
            root->used   = tmp->used;
            root->right  = deleteNode(root->right, tmp->word);
        }
    }
    return rebalance(root);
}

static Node* search(Node* root, const char* word)
{
    while (root) {
        int cmp = strcasecmp(word, root->word);
        if      (cmp == 0) return root;
        else if (cmp < 0)  root = root->left;
        else               root = root->right;
    }
    return NULL;
}

static int getSize(Node* root) {
    if (!root) return 0;
    return 1 + getSize(root->left) + getSize(root->right);
}

static void printTree(Node* root)
{
    if (!root) return;
    printTree(root->left);
    printf("%-*s | len=%-3d | used=%d | h=%-2d | Q: %s\n",
           MAX_WORD_LEN, root->word,
           root->length, root->used,
           root->hight,  root->question);
    printTree(root->right);
}

static void resetUsed(Node* root) {
    if (!root) return;
    root->used = 0;
    resetUsed(root->left);
    resetUsed(root->right);
}

// публичные обёртки со счётчиками
Node* c_insert    (Node* n, const char* w, const char* q, int l) { c_ins++;  return insert    (n, w, q, l); }
Node* c_deleteNode(Node* r, const char* w)                       { c_del++;  return deleteNode (r, w);       }
Node* c_search    (Node* r, const char* w)                       { c_srch++; return search     (r, w);       }
int   c_getSize   (Node* r)                                      { c_sz++;   return getSize    (r);          }
int   c_getHeight (Node* r)                                      { c_hgt++;  return getHeight  (r);          }
void  c_printTree (Node* r)                                      { c_prn++;  printTree         (r);          }
void  c_resetUsed (Node* r)                                      {           resetUsed         (r);          }
