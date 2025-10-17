/*
 * koteiterm - Display Module
 * X11ウィンドウ管理とレンダリング
 */

#include "display.h"
#include "font.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* グローバルディスプレイ状態 */
DisplayState g_display = {0};

/**
 * ディスプレイを初期化する
 */
int display_init(int width, int height)
{
    /* X11ディスプレイを開く */
    g_display.display = XOpenDisplay(NULL);
    if (!g_display.display) {
        fprintf(stderr, "エラー: X11ディスプレイを開けません\n");
        fprintf(stderr, "DISPLAY環境変数が設定されているか確認してください\n");
        return -1;
    }

    g_display.screen = DefaultScreen(g_display.display);
    g_display.width = width;
    g_display.height = height;

    /* ウィンドウを作成 */
    unsigned long black = BlackPixel(g_display.display, g_display.screen);
    unsigned long white = WhitePixel(g_display.display, g_display.screen);

    g_display.window = XCreateSimpleWindow(
        g_display.display,
        RootWindow(g_display.display, g_display.screen),
        0, 0,                    /* x, y */
        width, height,           /* width, height */
        0,                       /* border_width */
        white,                   /* border */
        black                    /* background */
    );

    /* ウィンドウプロパティを設定 */
    XStoreName(g_display.display, g_display.window, "koteiterm");

    /* WM_DELETE_WINDOWメッセージを受け取るように設定 */
    g_display.wm_delete_window = XInternAtom(g_display.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display.display, g_display.window, &g_display.wm_delete_window, 1);

    /* イベントマスクを設定 */
    XSelectInput(g_display.display, g_display.window,
                 ExposureMask | KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask | StructureNotifyMask);

    /* グラフィックスコンテキストを作成 */
    g_display.gc = XCreateGC(g_display.display, g_display.window, 0, NULL);
    XSetForeground(g_display.display, g_display.gc, white);
    XSetBackground(g_display.display, g_display.gc, black);

    /* ウィンドウをマップ（表示） */
    XMapWindow(g_display.display, g_display.window);
    XFlush(g_display.display);

    /* Xft描画コンテキストを作成 */
    Visual *visual = DefaultVisual(g_display.display, g_display.screen);
    Colormap colormap = DefaultColormap(g_display.display, g_display.screen);

    g_display.xft_draw = XftDrawCreate(g_display.display, g_display.window,
                                       visual, colormap);
    if (!g_display.xft_draw) {
        fprintf(stderr, "エラー: XftDraw の作成に失敗しました\n");
        XDestroyWindow(g_display.display, g_display.window);
        XCloseDisplay(g_display.display);
        return -1;
    }

    /* デフォルト色を設定（白の前景、黒の背景） */
    XRenderColor xr_fg = {0xffff, 0xffff, 0xffff, 0xffff};  /* 白 */
    XRenderColor xr_bg = {0x0000, 0x0000, 0x0000, 0xffff};  /* 黒 */
    XftColorAllocValue(g_display.display, visual, colormap, &xr_fg, &g_display.xft_fg);
    XftColorAllocValue(g_display.display, visual, colormap, &xr_bg, &g_display.xft_bg);

    printf("X11ディスプレイを初期化しました (%dx%d)\n", width, height);

    return 0;
}

/**
 * ディスプレイをクリーンアップする
 */
void display_cleanup(void)
{
    if (!g_display.display) {
        return;
    }

    /* Xft リソースをクリーンアップ */
    if (g_display.xft_draw) {
        XftDrawDestroy(g_display.xft_draw);
    }

    Visual *visual = DefaultVisual(g_display.display, g_display.screen);
    Colormap colormap = DefaultColormap(g_display.display, g_display.screen);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_fg);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_bg);

    if (g_display.gc) {
        XFreeGC(g_display.display, g_display.gc);
    }

    if (g_display.window) {
        XDestroyWindow(g_display.display, g_display.window);
    }

    XCloseDisplay(g_display.display);

    printf("X11ディスプレイをクリーンアップしました\n");

    memset(&g_display, 0, sizeof(g_display));
}

/**
 * イベントを処理する
 */
bool display_handle_events(void)
{
    XEvent event;

    /* 全ての保留中のイベントを処理 */
    while (XPending(g_display.display) > 0) {
        XNextEvent(g_display.display, &event);

        switch (event.type) {
            case Expose:
                /* 再描画が必要 */
                if (event.xexpose.count == 0) {
                    /* 最後のExposeイベントの時だけ再描画 */
                    display_clear();
                    display_flush();
                }
                break;

            case ClientMessage:
                /* ウィンドウマネージャからのメッセージ */
                if ((Atom)event.xclient.data.l[0] == g_display.wm_delete_window) {
                    printf("ウィンドウクローズが要求されました\n");
                    return false;  /* 終了 */
                }
                break;

            case ConfigureNotify:
                /* ウィンドウサイズが変更された */
                if (event.xconfigure.width != g_display.width ||
                    event.xconfigure.height != g_display.height) {
                    g_display.width = event.xconfigure.width;
                    g_display.height = event.xconfigure.height;
                    printf("ウィンドウサイズ変更: %dx%d\n",
                           g_display.width, g_display.height);
                }
                break;

            case KeyPress:
                /* キー入力（将来実装） */
                break;

            case ButtonPress:
                /* マウスボタン押下（将来実装） */
                break;

            default:
                /* その他のイベントは無視 */
                break;
        }
    }

    return true;  /* 継続 */
}

/**
 * 画面をクリアする
 */
void display_clear(void)
{
    if (!g_display.display || !g_display.window) {
        return;
    }

    /* ウィンドウ全体を背景色で塗りつぶす */
    XClearWindow(g_display.display, g_display.window);
}

/**
 * 画面を更新する
 */
void display_flush(void)
{
    if (!g_display.display) {
        return;
    }

    XFlush(g_display.display);
}

/**
 * ターミナルバッファの内容を描画する
 */
void display_render_terminal(void)
{
    if (!g_display.display || !g_display.xft_draw) {
        return;
    }

    extern FontState g_font;
    extern TerminalBuffer g_terminal;

    if (!g_font.xft_font) {
        return;
    }

    int char_width = font_get_char_width();
    int char_height = font_get_char_height();

    /* 全てのセルを描画 */
    for (int y = 0; y < g_terminal.rows; y++) {
        for (int x = 0; x < g_terminal.cols; x++) {
            Cell *cell = terminal_get_cell(x, y);
            if (!cell) {
                continue;
            }

            /* 描画位置を計算 */
            int px = x * char_width;
            int py = y * char_height + g_font.ascent;

            /* 文字を描画（現時点では白文字のみ） */
            if (cell->ch != ' ' && cell->ch != 0) {
                char utf8[5];
                int len = 0;

                /* Unicode を UTF-8 に変換（簡易版：ASCII のみ） */
                if (cell->ch < 0x80) {
                    utf8[0] = (char)cell->ch;
                    len = 1;
                } else {
                    /* 将来: 完全なUTF-8変換を実装 */
                    utf8[0] = '?';
                    len = 1;
                }
                utf8[len] = '\0';

                XftDrawStringUtf8(g_display.xft_draw, &g_display.xft_fg,
                                 g_font.xft_font, px, py,
                                 (FcChar8 *)utf8, len);
            }
        }
    }
}
