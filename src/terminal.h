#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stdbool.h>

/* セル属性 */
typedef struct {
    uint8_t fg_color;       /* 前景色（0-15: ANSI色） */
    uint8_t bg_color;       /* 背景色（0-15: ANSI色） */
    uint8_t flags;          /* 属性フラグ */
} CellAttr;

/* 属性フラグ */
#define ATTR_BOLD      (1 << 0)
#define ATTR_ITALIC    (1 << 1)
#define ATTR_UNDERLINE (1 << 2)
#define ATTR_REVERSE   (1 << 3)

/* 文字セル */
typedef struct {
    uint32_t ch;            /* Unicode文字 */
    CellAttr attr;          /* 属性 */
} Cell;

/* ターミナルバッファ */
typedef struct {
    Cell *cells;            /* セル配列（rows * cols） */
    int rows;               /* 行数 */
    int cols;               /* 列数 */
    int cursor_x;           /* カーソルX座標 */
    int cursor_y;           /* カーソルY座標 */
    bool cursor_visible;    /* カーソル表示 */
} TerminalBuffer;

/* グローバルターミナルバッファ */
extern TerminalBuffer g_terminal;

/* 関数プロトタイプ */

/**
 * ターミナルバッファを初期化する
 * @param rows 行数
 * @param cols 列数
 * @return 成功時0、失敗時-1
 */
int terminal_init(int rows, int cols);

/**
 * ターミナルバッファをクリーンアップする
 */
void terminal_cleanup(void);

/**
 * 指定位置のセルを取得する
 * @param x X座標
 * @param y Y座標
 * @return セルへのポインタ、範囲外の場合NULL
 */
Cell *terminal_get_cell(int x, int y);

/**
 * 指定位置に文字を書き込む
 * @param x X座標
 * @param y Y座標
 * @param ch 文字
 * @param attr 属性
 */
void terminal_put_char(int x, int y, uint32_t ch, CellAttr attr);

/**
 * 画面をクリアする
 */
void terminal_clear(void);

/**
 * カーソル位置を設定する
 * @param x X座標
 * @param y Y座標
 */
void terminal_set_cursor(int x, int y);

/**
 * カーソル位置を取得する
 * @param x X座標を格納する変数へのポインタ
 * @param y Y座標を格納する変数へのポインタ
 */
void terminal_get_cursor(int *x, int *y);

#endif /* TERMINAL_H */
