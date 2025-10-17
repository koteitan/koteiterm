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

/**
 * 画面を1行上にスクロール
 */
void terminal_scroll_up(void)
{
    /* 最初の行を削除、全ての行を1行上に移動 */
    int line_size = g_terminal.cols * sizeof(Cell);
    memmove(g_terminal.cells,
            g_terminal.cells + g_terminal.cols,
            (g_terminal.rows - 1) * line_size);

    /* 最後の行をクリア */
    CellAttr default_attr = {
        .fg_color = 7,  /* 白 */
        .bg_color = 0,  /* 黒 */
        .flags = 0
    };

    int last_row_start = (g_terminal.rows - 1) * g_terminal.cols;
    for (int x = 0; x < g_terminal.cols; x++) {
        g_terminal.cells[last_row_start + x].ch = ' ';
        g_terminal.cells[last_row_start + x].attr = default_attr;
    }
}

/**
 * キャリッジリターン処理
 */
void terminal_carriage_return(void)
{
    g_terminal.cursor_x = 0;
}

/**
 * 改行処理
 */
void terminal_newline(void)
{
    g_terminal.cursor_y++;
    if (g_terminal.cursor_y >= g_terminal.rows) {
        /* スクロール */
        terminal_scroll_up();
        g_terminal.cursor_y = g_terminal.rows - 1;
    }
}

/**
 * 1文字をカーソル位置に書き込んで進める
 */
void terminal_put_char_at_cursor(uint32_t ch)
{
    CellAttr default_attr = {
        .fg_color = 7,  /* 白 */
        .bg_color = 0,  /* 黒 */
        .flags = 0
    };

    Cell *cell = terminal_get_cell(g_terminal.cursor_x, g_terminal.cursor_y);
    if (cell) {
        cell->ch = ch;
        cell->attr = default_attr;
    }

    /* カーソルを右に進める */
    g_terminal.cursor_x++;
    if (g_terminal.cursor_x >= g_terminal.cols) {
        /* 行末に達したら次の行へ */
        g_terminal.cursor_x = 0;
        terminal_newline();
    }
}

/**
 * バイト列を処理してターミナルバッファに書き込む
 * 簡易版: エスケープシーケンスは無視、ASCII文字のみ処理
 */
void terminal_write(const char *data, size_t size)
{
    static enum {
        STATE_NORMAL,
        STATE_ESC,
        STATE_CSI,
    } state = STATE_NORMAL;

    for (size_t i = 0; i < size; i++) {
        unsigned char ch = (unsigned char)data[i];

        switch (state) {
            case STATE_NORMAL:
                if (ch == 0x1B) {  /* ESC */
                    state = STATE_ESC;
                } else if (ch == '\n') {
                    terminal_newline();
                } else if (ch == '\r') {
                    terminal_carriage_return();
                } else if (ch == '\b') {
                    /* バックスペース */
                    if (g_terminal.cursor_x > 0) {
                        g_terminal.cursor_x--;
                    }
                } else if (ch == '\t') {
                    /* タブ: 次の8の倍数位置へ */
                    int next_tab = ((g_terminal.cursor_x / 8) + 1) * 8;
                    if (next_tab >= g_terminal.cols) {
                        next_tab = g_terminal.cols - 1;
                    }
                    g_terminal.cursor_x = next_tab;
                } else if (ch >= 0x20) {
                    /* 表示可能文字 */
                    terminal_put_char_at_cursor(ch);
                }
                /* その他の制御文字は無視 */
                break;

            case STATE_ESC:
                if (ch == '[') {
                    state = STATE_CSI;
                } else {
                    /* その他のエスケープシーケンスは無視 */
                    state = STATE_NORMAL;
                }
                break;

            case STATE_CSI:
                /* CSIシーケンスの終端文字を待つ */
                if ((ch >= 0x40 && ch <= 0x7E) || ch < 0x20) {
                    /* 終端文字: @A-Z[\]^_`a-z{|}~ または制御文字 */
                    state = STATE_NORMAL;
                }
                /* パラメータとして0-9;?などは無視して読み飛ばす */
                break;
        }
    }
}
