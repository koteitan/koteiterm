#ifndef KOTEITERM_H
#define KOTEITERM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* バージョン情報 */
#define KOTEITERM_VERSION "0.1.0"
#define KOTEITERM_VERSION_MAJOR 0
#define KOTEITERM_VERSION_MINOR 1
#define KOTEITERM_VERSION_PATCH 0

/* デフォルト設定 */
#define DEFAULT_ROWS 24
#define DEFAULT_COLS 80
#define DEFAULT_SCROLLBACK 1000
#define DEFAULT_FONT "monospace"
#define DEFAULT_FONT_SIZE 12
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

/* 色定義（ANSI 16色） */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

/* ターミナル状態 */
typedef struct {
    int rows;           /* ターミナルの行数 */
    int cols;           /* ターミナルの列数 */
    int cursor_x;       /* カーソルのX座標 */
    int cursor_y;       /* カーソルのY座標 */
    bool running;       /* アプリケーション実行中フラグ */
} TerminalState;

/* グローバル状態 */
extern TerminalState g_term;

/* 関数プロトタイプ（後で各モジュールで実装） */

/* main.c */
void cleanup(void);

/* display.c */
int display_init(int width, int height);
void display_cleanup(void);
bool display_handle_events(void);
void display_clear(void);
void display_flush(void);

/* pty.c */
int pty_init(int rows, int cols);
void pty_cleanup(void);
ssize_t pty_read(char *buffer, size_t size);
ssize_t pty_write(const char *data, size_t size);
int pty_resize(int rows, int cols);
bool pty_is_child_running(void);

/* terminal.c（将来実装） */
// int terminal_init(int rows, int cols);
// void terminal_cleanup(void);

/* input.c（将来実装） */
// void input_handle(void);

#endif /* KOTEITERM_H */
