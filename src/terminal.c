/*
 * koteiterm - Terminal Module
 * ターミナルバッファと画面管理
 */

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* グローバルターミナルバッファ */
TerminalBuffer g_terminal = {0};

/**
 * ターミナルバッファを初期化する
 */
int terminal_init(int rows, int cols)
{
    /* バッファを確保 */
    g_terminal.cells = calloc(rows * cols, sizeof(Cell));
    if (!g_terminal.cells) {
        fprintf(stderr, "エラー: ターミナルバッファのメモリ確保に失敗しました\n");
        return -1;
    }

    g_terminal.rows = rows;
    g_terminal.cols = cols;
    g_terminal.cursor_x = 0;
    g_terminal.cursor_y = 0;
    g_terminal.cursor_visible = true;

    /* 初期化: 空白で埋める */
    CellAttr default_attr = {
        .fg_color = 7,  /* 白 */
        .bg_color = 0,  /* 黒 */
        .flags = 0
    };

    for (int i = 0; i < rows * cols; i++) {
        g_terminal.cells[i].ch = ' ';
        g_terminal.cells[i].attr = default_attr;
    }

    printf("ターミナルバッファを初期化しました (%dx%d)\n", cols, rows);

    return 0;
}

/**
 * ターミナルバッファをクリーンアップする
 */
void terminal_cleanup(void)
{
    if (g_terminal.cells) {
        free(g_terminal.cells);
        g_terminal.cells = NULL;
    }

    memset(&g_terminal, 0, sizeof(g_terminal));

    printf("ターミナルバッファをクリーンアップしました\n");
}

/**
 * 指定位置のセルを取得する
 */
Cell *terminal_get_cell(int x, int y)
{
    if (x < 0 || x >= g_terminal.cols || y < 0 || y >= g_terminal.rows) {
        return NULL;
    }

    return &g_terminal.cells[y * g_terminal.cols + x];
}

/**
 * 指定位置に文字を書き込む
 */
void terminal_put_char(int x, int y, uint32_t ch, CellAttr attr)
{
    Cell *cell = terminal_get_cell(x, y);
    if (cell) {
        cell->ch = ch;
        cell->attr = attr;
    }
}

/**
 * 画面をクリアする
 */
void terminal_clear(void)
{
    CellAttr default_attr = {
        .fg_color = 7,  /* 白 */
        .bg_color = 0,  /* 黒 */
        .flags = 0
    };

    for (int i = 0; i < g_terminal.rows * g_terminal.cols; i++) {
        g_terminal.cells[i].ch = ' ';
        g_terminal.cells[i].attr = default_attr;
    }

    g_terminal.cursor_x = 0;
    g_terminal.cursor_y = 0;
}

/**
 * カーソル位置を設定する
 */
void terminal_set_cursor(int x, int y)
{
    if (x >= 0 && x < g_terminal.cols) {
        g_terminal.cursor_x = x;
    }
    if (y >= 0 && y < g_terminal.rows) {
        g_terminal.cursor_y = y;
    }
}

/**
 * カーソル位置を取得する
 */
void terminal_get_cursor(int *x, int *y)
{
    if (x) {
        *x = g_terminal.cursor_x;
    }
    if (y) {
        *y = g_terminal.cursor_y;
    }
}
