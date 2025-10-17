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

/* UTF-8デコード関数 */
static int utf8_decode(const unsigned char *data, size_t size, uint32_t *codepoint)
{
    if (size < 1) {
        return 0;
    }

    unsigned char first = data[0];

    /* 1バイト文字 (ASCII) */
    if ((first & 0x80) == 0) {
        *codepoint = first;
        return 1;
    }

    /* 2バイト文字 */
    if ((first & 0xE0) == 0xC0) {
        if (size < 2) return 0;
        if ((data[1] & 0xC0) != 0x80) return 0;
        *codepoint = ((first & 0x1F) << 6) | (data[1] & 0x3F);
        return 2;
    }

    /* 3バイト文字 */
    if ((first & 0xF0) == 0xE0) {
        if (size < 3) return 0;
        if ((data[1] & 0xC0) != 0x80) return 0;
        if ((data[2] & 0xC0) != 0x80) return 0;
        *codepoint = ((first & 0x0F) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
        return 3;
    }

    /* 4バイト文字 */
    if ((first & 0xF8) == 0xF0) {
        if (size < 4) return 0;
        if ((data[1] & 0xC0) != 0x80) return 0;
        if ((data[2] & 0xC0) != 0x80) return 0;
        if ((data[3] & 0xC0) != 0x80) return 0;
        *codepoint = ((first & 0x07) << 18) | ((data[1] & 0x3F) << 12) |
                     ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
        return 4;
    }

    /* 不正なUTF-8シーケンス */
    return 0;
}

/* 文字幅を取得（East Asian Width） */
static int get_char_width(uint32_t ch)
{
    /* ASCII範囲 */
    if (ch < 0x7F) {
        return 1;
    }

    /* 全角文字の範囲を判定 */
    if ((ch >= 0x1100 && ch <= 0x115F) ||   /* ハングル */
        (ch >= 0x2E80 && ch <= 0x2EFF) ||   /* CJK部首補助 */
        (ch >= 0x2F00 && ch <= 0x2FDF) ||   /* 康熙部首 */
        (ch >= 0x3000 && ch <= 0x303F) ||   /* CJK記号・句読点 */
        (ch >= 0x3040 && ch <= 0x309F) ||   /* ひらがな */
        (ch >= 0x30A0 && ch <= 0x30FF) ||   /* カタカナ */
        (ch >= 0x3100 && ch <= 0x312F) ||   /* 注音符号 */
        (ch >= 0x3130 && ch <= 0x318F) ||   /* ハングル互換字母 */
        (ch >= 0x3190 && ch <= 0x319F) ||   /* 漢文用記号 */
        (ch >= 0x31A0 && ch <= 0x31BF) ||   /* 注音符号拡張 */
        (ch >= 0x31C0 && ch <= 0x31EF) ||   /* CJK筆画 */
        (ch >= 0x3200 && ch <= 0x32FF) ||   /* 囲み文字 */
        (ch >= 0x3300 && ch <= 0x33FF) ||   /* CJK互換 */
        (ch >= 0x3400 && ch <= 0x4DBF) ||   /* CJK統合漢字拡張A */
        (ch >= 0x4E00 && ch <= 0x9FFF) ||   /* CJK統合漢字 */
        (ch >= 0xAC00 && ch <= 0xD7AF) ||   /* ハングル音節 */
        (ch >= 0xF900 && ch <= 0xFAFF) ||   /* CJK互換漢字 */
        (ch >= 0xFE30 && ch <= 0xFE4F) ||   /* CJK互換形式 */
        (ch >= 0xFF00 && ch <= 0xFF60) ||   /* 全角ASCII、全角記号 */
        (ch >= 0xFFE0 && ch <= 0xFFE6) ||   /* 全角記号 */
        (ch >= 0x20000 && ch <= 0x2FFFD) || /* CJK統合漢字拡張B-F */
        (ch >= 0x30000 && ch <= 0x3FFFD)) { /* CJK統合漢字拡張G */
        return 2;
    }

    /* その他は半角 */
    return 1;
}

/**
 * ターミナルバッファを初期化する
 */
int terminal_init(int rows, int cols)
{
    /* メインバッファを確保 */
    g_terminal.cells = calloc(rows * cols, sizeof(Cell));
    if (!g_terminal.cells) {
        fprintf(stderr, "エラー: ターミナルバッファのメモリ確保に失敗しました\n");
        return -1;
    }

    /* 代替バッファは後で必要に応じて確保 */
    g_terminal.alternate_cells = NULL;
    g_terminal.using_alternate = false;

    g_terminal.rows = rows;
    g_terminal.cols = cols;
    g_terminal.cursor_x = 0;
    g_terminal.cursor_y = 0;
    g_terminal.cursor_visible = true;

    /* スクロール領域をデフォルト（全画面）に設定 */
    g_terminal.scroll_top = 0;
    g_terminal.scroll_bottom = rows - 1;

    /* 保存されたカーソル位置を初期化 */
    g_terminal.saved_cursor_x = 0;
    g_terminal.saved_cursor_y = 0;
    g_terminal.saved_attr.fg_color = 7;
    g_terminal.saved_attr.bg_color = 0;
    g_terminal.saved_attr.flags = 0;

    /* スクロールバックバッファを初期化 */
    g_terminal.scrollback.capacity = 1000;  /* 1000行の履歴 */
    g_terminal.scrollback.count = 0;
    g_terminal.scrollback.head = 0;
    g_terminal.scrollback.lines = calloc(g_terminal.scrollback.capacity, sizeof(ScrollbackLine));
    if (!g_terminal.scrollback.lines) {
        fprintf(stderr, "エラー: スクロールバックバッファのメモリ確保に失敗しました\n");
        free(g_terminal.cells);
        return -1;
    }
    g_terminal.scroll_offset = 0;  /* 最下部から開始 */

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

    /* 代替バッファをクリーンアップ */
    if (g_terminal.alternate_cells) {
        free(g_terminal.alternate_cells);
        g_terminal.alternate_cells = NULL;
    }

    /* スクロールバックバッファをクリーンアップ */
    if (g_terminal.scrollback.lines) {
        for (int i = 0; i < g_terminal.scrollback.count; i++) {
            int idx = (g_terminal.scrollback.head + i) % g_terminal.scrollback.capacity;
            if (g_terminal.scrollback.lines[idx].cells) {
                free(g_terminal.scrollback.lines[idx].cells);
            }
        }
        free(g_terminal.scrollback.lines);
        g_terminal.scrollback.lines = NULL;
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
    /* 最初の行をスクロールバックバッファに保存 */
    if (g_terminal.scrollback.lines) {
        /* リングバッファの次の位置を計算 */
        int write_idx;
        if (g_terminal.scrollback.count < g_terminal.scrollback.capacity) {
            /* まだ容量に余裕がある */
            write_idx = g_terminal.scrollback.count;
            g_terminal.scrollback.count++;
        } else {
            /* 容量いっぱい、最古の行を上書き */
            write_idx = g_terminal.scrollback.head;
            g_terminal.scrollback.head = (g_terminal.scrollback.head + 1) % g_terminal.scrollback.capacity;

            /* 既存の行のメモリを解放 */
            if (g_terminal.scrollback.lines[write_idx].cells) {
                free(g_terminal.scrollback.lines[write_idx].cells);
            }
        }

        /* 最初の行をコピー */
        g_terminal.scrollback.lines[write_idx].cols = g_terminal.cols;
        g_terminal.scrollback.lines[write_idx].cells = malloc(g_terminal.cols * sizeof(Cell));
        if (g_terminal.scrollback.lines[write_idx].cells) {
            memcpy(g_terminal.scrollback.lines[write_idx].cells,
                   g_terminal.cells,
                   g_terminal.cols * sizeof(Cell));
        }
    }

    /* 全ての行を1行上に移動 */
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

    /* 新しい出力があったらスクロールオフセットをリセット（最下部に移動） */
    if (g_terminal.scroll_offset == 0) {
        /* すでに最下部にいる場合は何もしない（自動スクロール） */
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
    int char_width = get_char_width(ch);

    Cell *cell = terminal_get_cell(g_terminal.cursor_x, g_terminal.cursor_y);
    if (cell) {
        cell->ch = ch;
        cell->attr = g_current_attr;  /* 現在の属性を使用 */
    }

    /* 全角文字の場合、次のセルに継続マーカーを設定 */
    if (char_width == 2 && g_terminal.cursor_x + 1 < g_terminal.cols) {
        Cell *next_cell = terminal_get_cell(g_terminal.cursor_x + 1, g_terminal.cursor_y);
        if (next_cell) {
            next_cell->ch = WIDE_CHAR_CONTINUATION;
            next_cell->attr = g_current_attr;
        }
    }

    /* カーソルを文字幅分進める */
    g_terminal.cursor_x += char_width;
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
                    } else if (p == 38 && i + 2 < param_count) {
                        /* 256色前景色: 38;5;N */
                        if (params[i + 1] == 5) {
                            g_current_attr.fg_color = params[i + 2];
                            i += 2;  /* パラメータ2つ分スキップ */
                        }
                    } else if (p == 48 && i + 2 < param_count) {
                        /* 256色背景色: 48;5;N */
                        if (params[i + 1] == 5) {
                            g_current_attr.bg_color = params[i + 2];
                            i += 2;  /* パラメータ2つ分スキップ */
                        }
                    }
                }
            }
            break;
        }

        case 'r':  /* DECSTBM: Set Scrolling Region */
        {
            int top = (param_count > 0 && params[0] > 0) ? params[0] - 1 : 0;
            int bottom = (param_count > 1 && params[1] > 0) ? params[1] - 1 : g_terminal.rows - 1;

            /* 範囲チェック */
            if (top < 0) top = 0;
            if (top >= g_terminal.rows) top = g_terminal.rows - 1;
            if (bottom < 0) bottom = 0;
            if (bottom >= g_terminal.rows) bottom = g_terminal.rows - 1;
            if (top > bottom) {
                int tmp = top;
                top = bottom;
                bottom = tmp;
            }

            g_terminal.scroll_top = top;
            g_terminal.scroll_bottom = bottom;

            /* カーソルをホームポジション (1,1) に移動 */
            g_terminal.cursor_x = 0;
            g_terminal.cursor_y = 0;
            break;
        }

        case 'L':  /* IL: Insert Line */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            /* カーソル行からスクロール領域下端まで下にシフト */
            for (int i = 0; i < n && g_terminal.cursor_y <= g_terminal.scroll_bottom; i++) {
                /* 最下行から上に向かって1行ずつ下にコピー */
                for (int y = g_terminal.scroll_bottom; y > g_terminal.cursor_y; y--) {
                    for (int x = 0; x < g_terminal.cols; x++) {
                        Cell *dst = terminal_get_cell(x, y);
                        Cell *src = terminal_get_cell(x, y - 1);
                        if (dst && src) {
                            *dst = *src;
                        }
                    }
                }

                /* カーソル行をクリア */
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

        case 'M':  /* DL: Delete Line */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            /* カーソル行を削除し、下の行を上にシフト */
            for (int i = 0; i < n && g_terminal.cursor_y <= g_terminal.scroll_bottom; i++) {
                /* カーソル行から上に向かって下の行をコピー */
                for (int y = g_terminal.cursor_y; y < g_terminal.scroll_bottom; y++) {
                    for (int x = 0; x < g_terminal.cols; x++) {
                        Cell *dst = terminal_get_cell(x, y);
                        Cell *src = terminal_get_cell(x, y + 1);
                        if (dst && src) {
                            *dst = *src;
                        }
                    }
                }

                /* 最下行をクリア */
                for (int x = 0; x < g_terminal.cols; x++) {
                    Cell *cell = terminal_get_cell(x, g_terminal.scroll_bottom);
                    if (cell) {
                        cell->ch = ' ';
                        cell->attr = default_attr;
                    }
                }
            }
            break;
        }

        case '@':  /* ICH: Insert Character */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            /* カーソル位置から右の文字を右にシフト */
            for (int i = 0; i < n; i++) {
                for (int x = g_terminal.cols - 1; x > g_terminal.cursor_x; x--) {
                    Cell *dst = terminal_get_cell(x, g_terminal.cursor_y);
                    Cell *src = terminal_get_cell(x - 1, g_terminal.cursor_y);
                    if (dst && src) {
                        *dst = *src;
                    }
                }

                /* カーソル位置に空白を挿入 */
                Cell *cell = terminal_get_cell(g_terminal.cursor_x, g_terminal.cursor_y);
                if (cell) {
                    cell->ch = ' ';
                    cell->attr = default_attr;
                }
            }
            break;
        }

        case 'P':  /* DCH: Delete Character */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            /* カーソル位置から文字を削除し、右の文字を左にシフト */
            for (int i = 0; i < n; i++) {
                for (int x = g_terminal.cursor_x; x < g_terminal.cols - 1; x++) {
                    Cell *dst = terminal_get_cell(x, g_terminal.cursor_y);
                    Cell *src = terminal_get_cell(x + 1, g_terminal.cursor_y);
                    if (dst && src) {
                        *dst = *src;
                    }
                }

                /* 行末に空白を追加 */
                Cell *cell = terminal_get_cell(g_terminal.cols - 1, g_terminal.cursor_y);
                if (cell) {
                    cell->ch = ' ';
                    cell->attr = default_attr;
                }
            }
            break;
        }

        case 'E':  /* CNL: Cursor Next Line */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            g_terminal.cursor_y += n;
            if (g_terminal.cursor_y >= g_terminal.rows) {
                g_terminal.cursor_y = g_terminal.rows - 1;
            }
            g_terminal.cursor_x = 0;
            break;
        }

        case 'F':  /* CPL: Cursor Previous Line */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            g_terminal.cursor_y -= n;
            if (g_terminal.cursor_y < 0) {
                g_terminal.cursor_y = 0;
            }
            g_terminal.cursor_x = 0;
            break;
        }

        case 'G':  /* CHA/HPA: Cursor Horizontal Absolute */
        {
            int col = (param_count > 0 && params[0] > 0) ? params[0] - 1 : 0;
            if (col >= 0 && col < g_terminal.cols) {
                g_terminal.cursor_x = col;
            }
            break;
        }

        case 'd':  /* VPA: Line Position Absolute */
        {
            int row = (param_count > 0 && params[0] > 0) ? params[0] - 1 : 0;
            if (row >= 0 && row < g_terminal.rows) {
                g_terminal.cursor_y = row;
            }
            break;
        }

        case 'S':  /* SU: Scroll Up */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            for (int i = 0; i < n; i++) {
                /* スクロール領域を1行上にスクロール */
                for (int y = g_terminal.scroll_top; y < g_terminal.scroll_bottom; y++) {
                    for (int x = 0; x < g_terminal.cols; x++) {
                        Cell *dst = terminal_get_cell(x, y);
                        Cell *src = terminal_get_cell(x, y + 1);
                        if (dst && src) {
                            *dst = *src;
                        }
                    }
                }

                /* 最下行をクリア */
                for (int x = 0; x < g_terminal.cols; x++) {
                    Cell *cell = terminal_get_cell(x, g_terminal.scroll_bottom);
                    if (cell) {
                        cell->ch = ' ';
                        cell->attr = default_attr;
                    }
                }
            }
            break;
        }

        case 'T':  /* SD: Scroll Down */
        {
            int n = (param_count > 0 && params[0] > 0) ? params[0] : 1;
            CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};

            for (int i = 0; i < n; i++) {
                /* スクロール領域を1行下にスクロール */
                for (int y = g_terminal.scroll_bottom; y > g_terminal.scroll_top; y--) {
                    for (int x = 0; x < g_terminal.cols; x++) {
                        Cell *dst = terminal_get_cell(x, y);
                        Cell *src = terminal_get_cell(x, y - 1);
                        if (dst && src) {
                            *dst = *src;
                        }
                    }
                }

                /* 最上行をクリア */
                for (int x = 0; x < g_terminal.cols; x++) {
                    Cell *cell = terminal_get_cell(x, g_terminal.scroll_top);
                    if (cell) {
                        cell->ch = ' ';
                        cell->attr = default_attr;
                    }
                }
            }
            break;
        }

        case 'n':  /* DSR: Device Status Report */
        {
            if (param_count > 0 && params[0] == 6) {
                /* CPR: Cursor Position Report */
                /* ESC[<row>;<col>R を返す（1ベース） */
                extern void pty_write(const char *data, size_t len);
                char response[32];
                int len = snprintf(response, sizeof(response), "\033[%d;%dR",
                                 g_terminal.cursor_y + 1, g_terminal.cursor_x + 1);
                pty_write(response, len);
            }
            break;
        }

        case 'h':  /* SM: Set Mode */
        case 'l':  /* RM: Reset Mode */
        {
            bool set_mode = (cmd == 'h');

            /* プライベートモードかチェック（パラメータが?で始まる） */
            bool is_private = (param_buf[0] == '?');
            int mode_params[16];
            int mode_param_count = 0;

            if (is_private) {
                /* ?の後のパラメータをパース */
                parse_csi_params(param_buf + 1, mode_params, &mode_param_count, 16);
            } else {
                /* 通常のパラメータ */
                for (int i = 0; i < param_count; i++) {
                    mode_params[i] = params[i];
                }
                mode_param_count = param_count;
            }

            for (int i = 0; i < mode_param_count; i++) {
                int mode = mode_params[i];

                if (is_private) {
                    /* プライベートモード */
                    if (mode == 25) {
                        /* DECTCEM: カーソル表示/非表示 */
                        g_terminal.cursor_visible = set_mode;
                    } else if (mode == 1049) {
                        /* 代替スクリーンバッファ */
                        if (set_mode) {
                            /* 代替バッファに切り替え */
                            if (!g_terminal.alternate_cells) {
                                /* 代替バッファを初期化 */
                                g_terminal.alternate_cells = calloc(g_terminal.rows * g_terminal.cols, sizeof(Cell));
                                if (g_terminal.alternate_cells) {
                                    CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};
                                    for (int j = 0; j < g_terminal.rows * g_terminal.cols; j++) {
                                        g_terminal.alternate_cells[j].ch = ' ';
                                        g_terminal.alternate_cells[j].attr = default_attr;
                                    }
                                }
                            }

                            if (g_terminal.alternate_cells && !g_terminal.using_alternate) {
                                /* カーソル位置を保存 */
                                g_terminal.saved_cursor_x = g_terminal.cursor_x;
                                g_terminal.saved_cursor_y = g_terminal.cursor_y;
                                g_terminal.saved_attr = g_current_attr;

                                /* バッファを入れ替え */
                                Cell *tmp = g_terminal.cells;
                                g_terminal.cells = g_terminal.alternate_cells;
                                g_terminal.alternate_cells = tmp;
                                g_terminal.using_alternate = true;

                                /* カーソルをホームに移動 */
                                g_terminal.cursor_x = 0;
                                g_terminal.cursor_y = 0;
                            }
                        } else {
                            /* メインバッファに戻る */
                            if (g_terminal.using_alternate && g_terminal.alternate_cells) {
                                /* バッファを入れ替え */
                                Cell *tmp = g_terminal.cells;
                                g_terminal.cells = g_terminal.alternate_cells;
                                g_terminal.alternate_cells = tmp;
                                g_terminal.using_alternate = false;

                                /* カーソル位置を復元 */
                                g_terminal.cursor_x = g_terminal.saved_cursor_x;
                                g_terminal.cursor_y = g_terminal.saved_cursor_y;
                                g_current_attr = g_terminal.saved_attr;
                            }
                        }
                    } else if (mode == 47 || mode == 1047) {
                        /* 代替スクリーンバッファ（カーソル保存なし） */
                        if (set_mode) {
                            /* 代替バッファに切り替え */
                            if (!g_terminal.alternate_cells) {
                                g_terminal.alternate_cells = calloc(g_terminal.rows * g_terminal.cols, sizeof(Cell));
                                if (g_terminal.alternate_cells) {
                                    CellAttr default_attr = {.fg_color = 7, .bg_color = 0, .flags = 0};
                                    for (int j = 0; j < g_terminal.rows * g_terminal.cols; j++) {
                                        g_terminal.alternate_cells[j].ch = ' ';
                                        g_terminal.alternate_cells[j].attr = default_attr;
                                    }
                                }
                            }

                            if (g_terminal.alternate_cells && !g_terminal.using_alternate) {
                                Cell *tmp = g_terminal.cells;
                                g_terminal.cells = g_terminal.alternate_cells;
                                g_terminal.alternate_cells = tmp;
                                g_terminal.using_alternate = true;
                            }
                        } else {
                            /* メインバッファに戻る */
                            if (g_terminal.using_alternate && g_terminal.alternate_cells) {
                                Cell *tmp = g_terminal.cells;
                                g_terminal.cells = g_terminal.alternate_cells;
                                g_terminal.alternate_cells = tmp;
                                g_terminal.using_alternate = false;
                            }
                        }
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
        STATE_OSC,
    } state = STATE_NORMAL;

    static char csi_buf[256];
    static int csi_len = 0;
    static char osc_buf[512];
    static int osc_len = 0;

    for (size_t i = 0; i < size; ) {
        unsigned char ch = (unsigned char)data[i];

        switch (state) {
            case STATE_NORMAL:
                if (ch == 0x1B) {  /* ESC */
                    state = STATE_ESC;
                    i++;
                } else if (ch == '\n') {
                    terminal_newline();
                    i++;
                } else if (ch == '\r') {
                    terminal_carriage_return();
                    i++;
                } else if (ch == '\b') {
                    /* バックスペース */
                    if (g_terminal.cursor_x > 0) {
                        g_terminal.cursor_x--;
                    }
                    i++;
                } else if (ch == '\t') {
                    /* タブ: 次の8の倍数位置へ */
                    int next_tab = ((g_terminal.cursor_x / 8) + 1) * 8;
                    if (next_tab >= g_terminal.cols) {
                        next_tab = g_terminal.cols - 1;
                    }
                    g_terminal.cursor_x = next_tab;
                    i++;
                } else if (ch >= 0x20 || (ch & 0x80)) {
                    /* UTF-8文字をデコード */
                    uint32_t codepoint;
                    int bytes = utf8_decode((const unsigned char *)&data[i], size - i, &codepoint);
                    if (bytes > 0) {
                        terminal_put_char_at_cursor(codepoint);
                        i += bytes;
                    } else {
                        /* デコード失敗、スキップ */
                        i++;
                    }
                } else {
                    /* その他の制御文字は無視 */
                    i++;
                }
                break;

            case STATE_ESC:
                if (ch == '[') {
                    state = STATE_CSI;
                    csi_len = 0;
                    memset(csi_buf, 0, sizeof(csi_buf));
                } else if (ch == ']') {
                    /* OSC: Operating System Command */
                    state = STATE_OSC;
                    osc_len = 0;
                    memset(osc_buf, 0, sizeof(osc_buf));
                } else if (ch == '7') {
                    /* DECSC: カーソル位置と属性を保存 */
                    g_terminal.saved_cursor_x = g_terminal.cursor_x;
                    g_terminal.saved_cursor_y = g_terminal.cursor_y;
                    g_terminal.saved_attr = g_current_attr;
                    state = STATE_NORMAL;
                } else if (ch == '8') {
                    /* DECRC: カーソル位置と属性を復元 */
                    g_terminal.cursor_x = g_terminal.saved_cursor_x;
                    g_terminal.cursor_y = g_terminal.saved_cursor_y;
                    g_current_attr = g_terminal.saved_attr;
                    state = STATE_NORMAL;
                } else {
                    /* その他のエスケープシーケンスは無視 */
                    state = STATE_NORMAL;
                }
                i++;
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
                i++;
                break;

            case STATE_OSC:
                /* OSCシーケンスを収集（BELまたはESC\で終了） */
                if (ch == 0x07) {
                    /* BEL (0x07) で終了 */
                    osc_buf[osc_len] = '\0';
                    /* OSCシーケンスは無視（ウィンドウタイトル設定など） */
                    state = STATE_NORMAL;
                    i++;
                } else if (ch == 0x1B) {
                    /* ESC: 次が \ なら ST (String Terminator) */
                    if (i + 1 < size && (unsigned char)data[i + 1] == '\\') {
                        /* ESC \ で終了 */
                        osc_buf[osc_len] = '\0';
                        state = STATE_NORMAL;
                        i += 2;  /* ESC と \ をスキップ */
                    } else {
                        /* 単なるESC、バッファに追加 */
                        if (osc_len < (int)sizeof(osc_buf) - 1) {
                            osc_buf[osc_len++] = ch;
                        }
                        i++;
                    }
                } else {
                    /* OSCシーケンスの文字を収集 */
                    if (osc_len < (int)sizeof(osc_buf) - 1) {
                        osc_buf[osc_len++] = ch;
                    }
                    i++;
                }
                break;
        }
    }
}

/**
 * スクロールアップ（行数指定）
 */
void terminal_scroll_by(int lines)
{
    g_terminal.scroll_offset += lines;

    /* 範囲チェック */
    int max_offset = g_terminal.scrollback.count;
    if (g_terminal.scroll_offset > max_offset) {
        g_terminal.scroll_offset = max_offset;
    }
    if (g_terminal.scroll_offset < 0) {
        g_terminal.scroll_offset = 0;
    }
}

/**
 * スクロールオフセットを設定
 */
void terminal_set_scroll_offset(int offset)
{
    g_terminal.scroll_offset = offset;

    /* 範囲チェック */
    int max_offset = g_terminal.scrollback.count;
    if (g_terminal.scroll_offset > max_offset) {
        g_terminal.scroll_offset = max_offset;
    }
    if (g_terminal.scroll_offset < 0) {
        g_terminal.scroll_offset = 0;
    }
}

/**
 * スクロールオフセットを取得
 */
int terminal_get_scroll_offset(void)
{
    return g_terminal.scroll_offset;
}

/**
 * 最下部までスクロール
 */
void terminal_scroll_to_bottom(void)
{
    g_terminal.scroll_offset = 0;
}

/**
 * スクロールバックから指定行を取得
 */
ScrollbackLine *terminal_get_scrollback_line(int line_index)
{
    if (line_index < 0 || line_index >= g_terminal.scrollback.count) {
        return NULL;
    }

    int idx = (g_terminal.scrollback.head + line_index) % g_terminal.scrollback.capacity;
    return &g_terminal.scrollback.lines[idx];
}

/**
 * 選択を開始
 */
void terminal_selection_start(int x, int y)
{
    g_terminal.selection.active = true;
    g_terminal.selection.start_x = x;
    g_terminal.selection.start_y = y;
    g_terminal.selection.end_x = x;
    g_terminal.selection.end_y = y;
}

/**
 * 選択を更新（ドラッグ中）
 */
void terminal_selection_update(int x, int y)
{
    if (!g_terminal.selection.active) {
        return;
    }

    g_terminal.selection.end_x = x;
    g_terminal.selection.end_y = y;
}

/**
 * 選択を終了
 */
void terminal_selection_end(void)
{
    /* 選択状態はactiveのままにして、範囲を保持 */
}

/**
 * 選択をクリア
 */
void terminal_selection_clear(void)
{
    g_terminal.selection.active = false;
    g_terminal.selection.start_x = 0;
    g_terminal.selection.start_y = 0;
    g_terminal.selection.end_x = 0;
    g_terminal.selection.end_y = 0;
}

/**
 * 指定位置が選択範囲内かチェック
 */
bool terminal_is_selected(int x, int y)
{
    if (!g_terminal.selection.active) {
        return false;
    }

    int start_x = g_terminal.selection.start_x;
    int start_y = g_terminal.selection.start_y;
    int end_x = g_terminal.selection.end_x;
    int end_y = g_terminal.selection.end_y;

    /* 開始と終了を正規化（開始 < 終了） */
    if (start_y > end_y || (start_y == end_y && start_x > end_x)) {
        int tmp;
        tmp = start_x; start_x = end_x; end_x = tmp;
        tmp = start_y; start_y = end_y; end_y = tmp;
    }

    /* 範囲チェック */
    if (y < start_y || y > end_y) {
        return false;
    }

    if (y == start_y && y == end_y) {
        /* 同じ行 */
        return x >= start_x && x <= end_x;
    } else if (y == start_y) {
        /* 開始行 */
        return x >= start_x;
    } else if (y == end_y) {
        /* 終了行 */
        return x <= end_x;
    } else {
        /* 中間行 */
        return true;
    }
}

/**
 * 選択されたテキストを取得
 */
char *terminal_get_selected_text(void)
{
    if (!g_terminal.selection.active) {
        return NULL;
    }

    int start_x = g_terminal.selection.start_x;
    int start_y = g_terminal.selection.start_y;
    int end_x = g_terminal.selection.end_x;
    int end_y = g_terminal.selection.end_y;

    /* 開始と終了を正規化 */
    if (start_y > end_y || (start_y == end_y && start_x > end_x)) {
        int tmp;
        tmp = start_x; start_x = end_x; end_x = tmp;
        tmp = start_y; start_y = end_y; end_y = tmp;
    }

    /* バッファサイズを推定 */
    int estimated_size = (end_y - start_y + 1) * (g_terminal.cols + 1) * 4 + 1;
    char *text = malloc(estimated_size);
    if (!text) {
        return NULL;
    }

    char *p = text;
    for (int y = start_y; y <= end_y; y++) {
        int col_start = (y == start_y) ? start_x : 0;
        int col_end = (y == end_y) ? end_x : g_terminal.cols - 1;

        for (int x = col_start; x <= col_end; x++) {
            Cell *cell = terminal_get_cell(x, y);
            if (cell && cell->ch != ' ' && cell->ch != WIDE_CHAR_CONTINUATION) {
                /* UTF-8エンコード */
                uint32_t ch = cell->ch;
                if (ch < 0x80) {
                    *p++ = (char)ch;
                } else if (ch < 0x800) {
                    *p++ = 0xC0 | ((ch >> 6) & 0x1F);
                    *p++ = 0x80 | (ch & 0x3F);
                } else if (ch < 0x10000) {
                    *p++ = 0xE0 | ((ch >> 12) & 0x0F);
                    *p++ = 0x80 | ((ch >> 6) & 0x3F);
                    *p++ = 0x80 | (ch & 0x3F);
                } else {
                    *p++ = 0xF0 | ((ch >> 18) & 0x07);
                    *p++ = 0x80 | ((ch >> 12) & 0x3F);
                    *p++ = 0x80 | ((ch >> 6) & 0x3F);
                    *p++ = 0x80 | (ch & 0x3F);
                }
            }
        }

        /* 行末に改行を追加（最終行以外） */
        if (y < end_y) {
            *p++ = '\n';
        }
    }

    *p = '\0';
    return text;
}
