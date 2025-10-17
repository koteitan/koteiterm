#ifndef FONT_H
#define FONT_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

/* フォント状態 */
typedef struct {
    XftFont *xft_font;      /* Xftフォント */
    int char_width;         /* 文字の幅（ピクセル） */
    int char_height;        /* 文字の高さ（ピクセル） */
    int ascent;             /* ベースラインから上部まで */
    int descent;            /* ベースラインから下部まで */
} FontState;

/* グローバルフォント状態 */
extern FontState g_font;

/* 関数プロトタイプ */

/**
 * フォントを初期化する
 * @param display X11ディスプレイ
 * @param screen スクリーン番号
 * @param font_name フォント名（例: "monospace"）
 * @param font_size フォントサイズ
 * @return 成功時0、失敗時-1
 */
int font_init(Display *display, int screen, const char *font_name, int font_size);

/**
 * フォントをクリーンアップする
 * @param display X11ディスプレイ
 */
void font_cleanup(Display *display);

/**
 * 文字の幅を取得する
 * @return 文字の幅（ピクセル）
 */
int font_get_char_width(void);

/**
 * 文字の高さを取得する
 * @return 文字の高さ（ピクセル）
 */
int font_get_char_height(void);

#endif /* FONT_H */
