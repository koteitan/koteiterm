#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>
#include <stdbool.h>
#include <time.h>

/* ディスプレイ状態 */
typedef struct {
    Display *display;        /* X11ディスプレイ */
    Window window;           /* メインウィンドウ */
    int screen;              /* スクリーン番号 */
    GC gc;                   /* グラフィックスコンテキスト */
    int width;               /* ウィンドウ幅 */
    int height;              /* ウィンドウ高さ */
    Atom wm_delete_window;   /* ウィンドウ削除メッセージ */
    Atom clipboard_atom;     /* CLIPBOARDアトム */
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
    /* GIFアニメーション用 */
    Pixmap *cursor_gif_frames;       /* GIFフレームのPixmap配列 */
    Pixmap *cursor_gif_masks;        /* GIFフレームのマスク配列 */
    int *cursor_gif_delays;          /* 各フレームの遅延時間（10ms単位） */
    int cursor_gif_frame_count;      /* フレーム数 */
    int cursor_gif_current_frame;    /* 現在のフレームインデックス */
    struct timespec cursor_gif_last_update;  /* 最後のフレーム更新時刻 */
} DisplayState;

/* グローバルディスプレイ状態 */
extern DisplayState g_display;

/* 選択テキスト（グローバル） */
extern char *selection_text;

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

/**
 * GIFアニメーションカーソルを更新する
 * メインループから定期的に呼び出される
 */
void display_update_gif_cursor(void);

#endif /* DISPLAY_H */
