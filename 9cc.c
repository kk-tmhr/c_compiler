#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

enum {
    TK_NUM = 256,
    TK_EOF,
};

enum {
    ND_NUM = 256
};

typedef struct
{
    int ty;
    int val;
    char *input;
} Token;

typedef struct Node
{
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;
} Node;

Node *expr();
Node *term();
Node *mul();
Node *unary();

char *user_input;

Token tokens[100];

int pos = 0;

void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *msg)
{
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

int consume(int ty){
    if (tokens[pos].ty != ty)
        return 0;
    pos++;
    return 1;
}

Node *expr() {
    Node *node = mul();

    for (;;) {
        if (consume('+'))
            node = new_node('+', node, mul());
        else if (consume('-'))
            node = new_node('-', node, mul());
        else
            return node;
    }
}

Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume('*'))
            node = new_node('*', node, unary());
        else if (consume('/'))
            node = new_node('/', node, unary());
        else 
            return node;
    }
}

Node *term() {
    if (consume('(')) {
        Node *node = expr();
        if (!consume(')'))
            error_at(tokens[pos].input, "開きカッコに対する閉じカッコがありません");
        return node;
    }

    if (tokens[pos].ty == TK_NUM)
        return new_node_num(tokens[pos++].val);

    error_at(tokens[pos].input, "数値でも開きカッコでもないトークンです");
}

Node *unary() {
    if (consume('+'))
        return term();
    if (consume('-'))
        return new_node('-', new_node_num(0), term());
    return term();
}

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->ty)
    {
    case '+':
        printf("  add rax, rdi\n");
        break;
    case '-':
        printf("  sub rax, rdi\n");
        break;
    case '*':
        printf("  imul rdi\n");
        break;
    case '/':
        printf("  cqo\n");
        printf("  idiv rdi\n");
    }
    printf("  push rax\n");
}

void tokenize(char *argv)
{
    char *p = argv;

    int i = 0;
    while (*p)
    {
        // 空白文字をスキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')')
        {
            tokens[i].ty = *p;
            tokens[i].input = p;
            i++;
            p++;
            continue;
        }

        if (isdigit(*p))
        {
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

int main(int argc, char const *argv[])
{
    if (argc !=2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];

    tokenize(argv[1]);
    Node *node = expr();
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
