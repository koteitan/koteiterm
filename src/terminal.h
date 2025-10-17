#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

/* 特殊文字コード */
#define WIDE_CHAR_CONTINUATION 0xFFFFFFFE  /* 全角文字の2セル目 */

/* 文字セル */
typedef struct {
    uint32_t ch;            /* Unicode文字 */
    CellAttr attr;          /* 属性 */
} Cell;

/* スクロールバック行 */
typedef struct {
    Cell *cells;            /* セル配列 */
    int cols;               /* この行の列数 */
} ScrollbackLine;

/* スクロールバックバッファ */
typedef struct {
    ScrollbackLine *lines;  /* 行配列 */
    int capacity;           /* 最大行数 */
    int count;              /* 現在の行数 */
    int head;               /* リングバッファの先頭位置 */
} ScrollbackBuffer;

/* ターミナルバッファ */
typedef struct {
    Cell *cells;            /* セル配列（rows * cols） */
    int rows;               /* 行数 */
    int cols;               /* 列数 */
    int cursor_x;           /* カーソルX座標 */
    int cursor_y;           /* カーソルY座標 */
    bool cursor_visible;    /* カーソル表示 */
    ScrollbackBuffer scrollback;  /* スクロールバック履歴 */
    int scroll_offset;      /* スクロールオフセット（0=最下部） */
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

/**
 * バイト列を処理してターミナルバッファに書き込む
 * @param data データ
 * @param size データサイズ
 */
void terminal_write(const char *data, size_t size);

/**
 * 1文字をカーソル位置に書き込んで進める
 * @param ch 文字
 */
void terminal_put_char_at_cursor(uint32_t ch);

/**
 * 改行処理
 */
void terminal_newline(void);

/**
 * キャリッジリターン処理
 */
void terminal_carriage_return(void);

/**
 * 画面を1行上にスクロール
 */
void terminal_scroll_up(void);

/**
 * ターミナルバッファをリサイズする
 * @param new_rows 新しい行数
 * @param new_cols 新しい列数
 * @return 成功時0、失敗時-1
 */
int terminal_resize(int new_rows, int new_cols);

/**
 * スクロールアップ（行数指定）
 * @param lines スクロールする行数
 */
void terminal_scroll_by(int lines);

/**
 * スクロールオフセットを設定
 * @param offset オフセット（0=最下部）
 */
void terminal_set_scroll_offset(int offset);

/**
 * スクロールオフセットを取得
 * @return 現在のスクロールオフセット
 */
int terminal_get_scroll_offset(void);

/**
 * 最下部までスクロール
 */
void terminal_scroll_to_bottom(void);

/**
 * スクロールバックから指定行を取得
 * @param line_index スクロールバック内の行インデックス（0=最古）
 * @return 行へのポインタ、範囲外の場合NULL
 */
ScrollbackLine *terminal_get_scrollback_line(int line_index);

#endif /* TERMINAL_H */
