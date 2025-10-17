#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xlib.h>
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

#endif /* DISPLAY_H */
