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

/* 現在の描画属性 */
static CellAttr g_current_attr = {
    .fg_color = 7,  /* 白 */
    .bg_color = 0,  /* 黒 */
    .flags = 0
};

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
 * ターミナルバッファをリサイズする
 */
int terminal_resize(int new_rows, int new_cols)
{
    if (new_rows <= 0 || new_cols <= 0) {
        fprintf(stderr, "エラー: 無効なターミナルサイズ (%dx%d)\n", new_cols, new_rows);
        return -1;
    }

    /* 新しいバッファを確保 */
    Cell *new_cells = calloc(new_rows * new_cols, sizeof(Cell));
    if (!new_cells) {
        fprintf(stderr, "エラー: リサイズ用バッファのメモリ確保に失敗しました\n");
        return -1;
    }

    /* デフォルト属性で初期化 */
    CellAttr default_attr = {
        .fg_color = 7,  /* 白 */
        .bg_color = 0,  /* 黒 */
        .flags = 0
    };

    for (int i = 0; i < new_rows * new_cols; i++) {
        new_cells[i].ch = ' ';
        new_cells[i].attr = default_attr;
    }

    /* 既存の内容をコピー */
    int copy_rows = (new_rows < g_terminal.rows) ? new_rows : g_terminal.rows;
    int copy_cols = (new_cols < g_terminal.cols) ? new_cols : g_terminal.cols;

    for (int y = 0; y < copy_rows; y++) {
        for (int x = 0; x < copy_cols; x++) {
            int old_idx = y * g_terminal.cols + x;
            int new_idx = y * new_cols + x;
            new_cells[new_idx] = g_terminal.cells[old_idx];
        }
    }

    /* 古いバッファを解放 */
    free(g_terminal.cells);

    /* 新しいバッファに切り替え */
    g_terminal.cells = new_cells;
    g_terminal.rows = new_rows;
    g_terminal.cols = new_cols;

    /* カーソル位置を調整 */
    if (g_terminal.cursor_x >= new_cols) {
        g_terminal.cursor_x = new_cols - 1;
    }
    if (g_terminal.cursor_y >= new_rows) {
        g_terminal.cursor_y = new_rows - 1;
    }

    printf("ターミナルバッファをリサイズしました (%dx%d)\n", new_cols, new_rows);

    return 0;
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
    Cell *cell = terminal_get_cell(g_terminal.cursor_x, g_terminal.cursor_y);
    if (cell) {
        cell->ch = ch;
        cell->attr = g_current_attr;  /* 現在の属性を使用 */
    }

    /* カーソルを右に進める */
    g_terminal.cursor_x++;
    if (g_terminal.cursor_x >= g_terminal.cols) {
        /* 行末に達したら次の行へ */
        g_terminal.cursor_x = 0;
        terminal_newline();
    }
}

/* CSIパラメータをパースする */
static void parse_csi_params(const char *param_buf, int *params, int *param_count, int max_params)
{
    *param_count = 0;
    int value = 0;
    bool has_value = false;

    for (const char *p = param_buf; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
            has_value = true;
        } else if (*p == ';') {
            if (*param_count < max_params) {
                params[(*param_count)++] = has_value ? value : 0;
            }
            value = 0;
            has_value = false;
        }
    }

    /* 最後のパラメータ */
    if (*param_count < max_params) {
        params[(*param_count)++] = has_value ? value : 0;
    }
}

/* CSIコマンドを処理する */
static void handle_csi_command(char cmd, const char *param_buf)
{
    int params[16];
    int param_count;

    parse_csi_params(param_buf, params, &param_count, 16);

    switch (cmd) {
        case 'A':  /* CUU: Cursor Up */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            g_terminal.cursor_y -= n;
            if (g_terminal.cursor_y < 0) g_terminal.cursor_y = 0;
            break;
        }

        case 'B':  /* CUD: Cursor Down */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            g_terminal.cursor_y += n;
            if (g_terminal.cursor_y >= g_terminal.rows) {
                g_terminal.cursor_y = g_terminal.rows - 1;
            }
            break;
        }

        case 'C':  /* CUF: Cursor Forward */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            g_terminal.cursor_x += n;
            if (g_terminal.cursor_x >= g_terminal.cols) {
                g_terminal.cursor_x = g_terminal.cols - 1;
            }
            break;
        }

        case 'D':  /* CUB: Cursor Back */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            g_terminal.cursor_x -= n;
            if (g_terminal.cursor_x < 0) g_terminal.cursor_x = 0;
            break;
        }

        case 'H':  /* CUP: Cursor Position */
        case 'f':  /* HVP: Horizontal and Vertical Position */
        {
            int row = (param_count > 0 && params[0] > 0) ? params[0] - 1 : 0;
            int col = (param_count > 1 && params[1] > 0) ? params[1] - 1 : 0;
            terminal_set_cursor(col, row);
            break;
        }

        case 'J':  /* ED: Erase in Display */
        {
            int n = (param_count > 0) ? params[0] : 0;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            if (n == 0) {
                /* カーソルから下をクリア */
                for (int y = g_terminal.cursor_y; y < g_terminal.rows; y++) {
                    int start_x = (y == g_terminal.cursor_y) ? g_terminal.cursor_x : 0;
                    for (int x = start_x; x < g_terminal.cols; x++) {
                        Cell *cell = terminal_get_cell(x, y);
                        if (cell) {
                            cell->ch = ' ';
                            cell->attr = default_attr;
                        }
                    }
                }
            } else if (n == 1) {
                /* カーソルから上をクリア */
                for (int y = 0; y <= g_terminal.cursor_y; y++) {
                    int end_x = (y == g_terminal.cursor_y) ? g_terminal.cursor_x : g_terminal.cols - 1;
                    for (int x = 0; x <= end_x; x++) {
                        Cell *cell = terminal_get_cell(x, y);
                        if (cell) {
                            cell->ch = ' ';
                            cell->attr = default_attr;
                        }
                    }
                }
            } else if (n == 2 || n == 3) {
                /* 画面全体をクリア */
                terminal_clear();
            }
            break;
        }

        case 'K':  /* EL: Erase in Line */
        {
            int n = (param_count > 0) ? params[0] : 0;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            if (n == 0) {
                /* カーソルから行末までクリア */
                for (int x = g_terminal.cursor_x; x < g_terminal.cols; x++) {
                    Cell *cell = terminal_get_cell(x, g_terminal.cursor_y);
                    if (cell) {
                        cell->ch = ' ';
                        cell->attr = default_attr;
                    }
                }
            } else if (n == 1) {
                /* 行頭からカーソルまでクリア */
                for (int x = 0; x <= g_terminal.cursor_x; x++) {
                    Cell *cell = terminal_get_cell(x, g_terminal.cursor_y);
                    if (cell) {
                        cell->ch = ' ';
                        cell->attr = default_attr;
                    }
                }
            } else if (n == 2) {
                /* 行全体をクリア */
                for (int x = 0; x < g_terminal.cols; x++) {
                    Cell *cell = terminal_get_cell(x, g_terminal.cursor_y);
                    if (cell) {
                        cell->ch = ' ';
                        cell->attr = default_attr;
                    }
                }
            }
            break;
        }

        case 'm':  /* SGR: Select Graphic Rendition */
        {
            if (param_count == 0) {
                /* パラメータなし: リセット */
                g_current_attr.fg_color = 7;
                g_current_attr.bg_color = 0;
                g_current_attr.flags = 0;
            } else {
                for (int i = 0; i < param_count; i++) {
                    int p = params[i];

                    if (p == 0) {
                        /* リセット */
                        g_current_attr.fg_color = 7;
                        g_current_attr.bg_color = 0;
                        g_current_attr.flags = 0;
                    } else if (p == 1) {
                        /* 太字 */
                        g_current_attr.flags |= ATTR_BOLD;
                    } else if (p == 3) {
                        /* イタリック */
                        g_current_attr.flags |= ATTR_ITALIC;
                    } else if (p == 4) {
                        /* 下線 */
                        g_current_attr.flags |= ATTR_UNDERLINE;
                    } else if (p == 7) {
                        /* 反転 */
                        g_current_attr.flags |= ATTR_REVERSE;
                    } else if (p == 22) {
                        /* 太字解除 */
                        g_current_attr.flags &= ~ATTR_BOLD;
                    } else if (p == 23) {
                        /* イタリック解除 */
                        g_current_attr.flags &= ~ATTR_ITALIC;
                    } else if (p == 24) {
                        /* 下線解除 */
                        g_current_attr.flags &= ~ATTR_UNDERLINE;
                    } else if (p == 27) {
                        /* 反転解除 */
                        g_current_attr.flags &= ~ATTR_REVERSE;
                    } else if (p >= 30 && p <= 37) {
                        /* 前景色: 30-37 */
                        g_current_attr.fg_color = p - 30;
                    } else if (p == 39) {
                        /* デフォルト前景色 */
                        g_current_attr.fg_color = 7;
                    } else if (p >= 40 && p <= 47) {
                        /* 背景色: 40-47 */
                        g_current_attr.bg_color = p - 40;
                    } else if (p == 49) {
                        /* デフォルト背景色 */
                        g_current_attr.bg_color = 0;
                    } else if (p >= 90 && p <= 97) {
                        /* 明るい前景色: 90-97 */
                        g_current_attr.fg_color = (p - 90) + 8;
                    } else if (p >= 100 && p <= 107) {
                        /* 明るい背景色: 100-107 */
                        g_current_attr.bg_color = (p - 100) + 8;
                    }
                }
            }
            break;
        }

        default:
            /* その他のコマンドは無視 */
            break;
    }
}

/**
 * バイト列を処理してターミナルバッファに書き込む
 */
void terminal_write(const char *data, size_t size)
{
    static enum {
        STATE_NORMAL,
        STATE_ESC,
        STATE_CSI,
    } state = STATE_NORMAL;

    static char csi_buf[256];
    static int csi_len = 0;

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
                    csi_len = 0;
                    memset(csi_buf, 0, sizeof(csi_buf));
                } else {
                    /* その他のエスケープシーケンスは無視 */
                    state = STATE_NORMAL;
                }
                break;

            case STATE_CSI:
                /* CSIシーケンスのパラメータと終端文字を収集 */
                if (ch >= 0x40 && ch <= 0x7E) {
                    /* 終端文字: @A-Z[\]^_`a-z{|}~ */
                    csi_buf[csi_len] = '\0';
                    handle_csi_command(ch, csi_buf);
                    state = STATE_NORMAL;
                } else if (ch >= 0x20 && ch < 0x40) {
                    /* パラメータ文字: 0-9;:<=>? など */
                    if (csi_len < (int)sizeof(csi_buf) - 1) {
                        csi_buf[csi_len++] = ch;
                    }
                } else {
                    /* 予期しない文字、中断 */
                    state = STATE_NORMAL;
                }
                break;
        }
    }
}
