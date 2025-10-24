#ifndef KOTEITERM_H
#define KOTEITERM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* バージョン情報 */
#define KOTEITERM_NAME "koteiterm"
#define KOTEITERM_VERSION "0.1.0"
#define KOTEITERM_VERSION_MAJOR 0
#define KOTEITERM_VERSION_MINOR 1
#define KOTEITERM_VERSION_PATCH 0

/* デフォルト設定 */
#define DEFAULT_ROWS 24
#define DEFAULT_COLS 80
#define DEFAULT_SCROLLBACK 1000
#define DEFAULT_FONT "HackGen Console NF"
#define DEFAULT_FONT_SIZE 12
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

/* 色定義（ANSI 16色） */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

/* カーソル形状 */
typedef enum {
    TERM_CURSOR_UNDERLINE,    /* 短いアンダーライン（デフォルト） */
    TERM_CURSOR_BAR,          /* 左縦線 */
    TERM_CURSOR_HOLLOW_BLOCK, /* 中抜き四角 */
    TERM_CURSOR_BLOCK,        /* 中埋め四角 */
    TERM_CURSOR_IMAGE         /* 画像ファイル */
} TermCursorShape;

/* 色オプション設定 */
typedef struct {
    const char *foreground;    /* 前景色 (-fg) */
    const char *background;    /* 背景色 (-bg) */
    const char *cursor;        /* カーソル色 (-cr) */
    const char *sel_bg;        /* 選択背景色 (-selbg) */
    const char *sel_fg;        /* 選択前景色 (-selfg) */
    const char *underline;     /* アンダーライン色 (-ul) */
} ColorOptions;

/* 表示オプション設定 */
typedef struct {
    TermCursorShape cursor_shape;  /* カーソル形状 */
    bool show_underline;           /* 全幅アンダーラインを表示 */
    const char *cursor_image_path; /* カーソル画像ファイルパス */
    int cursor_offset_x;           /* カーソル画像のXオフセット（ピクセル） */
    int cursor_offset_y;           /* カーソル画像のYオフセット（ピクセル） */
    double cursor_scale;           /* カーソル画像のスケール（0.0-1.0） */
} DisplayOptions;

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

/* デバッグフラグ */
extern bool g_debug;        /* 一般的なデバッグ出力 */
extern bool g_debug_key;    /* キー入力のデバッグ出力 */

/* 色オプション設定 */
extern ColorOptions g_color_options;

/* 表示オプション設定 */
extern DisplayOptions g_display_options;

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

/* terminal.c */
int terminal_init(int rows, int cols);
void terminal_cleanup(void);
void terminal_clear(void);

/* font.c */
typedef struct _XDisplay Display;  /* Forward declaration */
int font_init(Display *display, int screen, const char *font_name, int font_size);
void font_cleanup(Display *display);
int font_get_char_width(void);
int font_get_char_height(void);

/* display.c - additional */
void display_render_terminal(void);

/* input.c（将来実装） */
// void input_handle(void);

#endif /* KOTEITERM_H */
