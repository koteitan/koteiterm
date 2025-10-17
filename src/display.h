#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>
#include <stdbool.h>

/* ディスプレイ状態 */
typedef struct {
    Display *display;        /* X11ディスプレイ */
    Window window;           /* メインウィンドウ */
    int screen;              /* スクリーン番号 */
    GC gc;                   /* グラフィックスコンテキスト */
    int width;               /* ウィンドウ幅 */
    int height;              /* ウィンドウ高さ */
    Atom wm_delete_window;   /* ウィンドウ削除メッセージ */
    XftDraw *xft_draw;       /* Xft描画コンテキスト */
    XftColor xft_fg;         /* 前景色 */
    XftColor xft_bg;         /* 背景色 */
    XftColor xft_cursor;     /* カーソル色 */
    XftColor xft_sel_bg;     /* 選択背景色 */
    XftColor xft_sel_fg;     /* 選択前景色 */
    XftColor xft_underline;  /* アンダーライン色 */
    XIM xim;                 /* Input Method */
    XIC xic;                 /* Input Context */
    Imlib_Image cursor_image;  /* カーソル画像（Imlib2） */
    Pixmap cursor_pixmap;      /* カーソル画像のPixmap */
    Pixmap cursor_mask;        /* カーソル画像のマスク */
    int cursor_image_width;    /* カーソル画像の幅 */
    int cursor_image_height;   /* カーソル画像の高さ */
} DisplayState;

/* グローバルディスプレイ状態 */
extern DisplayState g_display;

/* 関数プロトタイプ */

/**
 * ディスプレイを初期化する
 * @param width ウィンドウの幅
 * @param height ウィンドウの高さ
 * @return 成功時0、失敗時-1
 */
int display_init(int width, int height);

/**
 * ディスプレイをクリーンアップする
 */
void display_cleanup(void);

/**
 * イベントを処理する
 * @return 終了要求があった場合false、それ以外true
 */
bool display_handle_events(void);

/**
 * 画面をクリアする
 */
void display_clear(void);

/**
 * 画面を更新する（バッファをフラッシュ）
 */
void display_flush(void);

/**
 * ターミナルバッファの内容を描画する
 */
void display_render_terminal(void);

#endif /* DISPLAY_H */
