/*
 * koteiterm - Display Module
 * X11ウィンドウ管理とレンダリング
 */

#include "display.h"
#include "font.h"
#include "terminal.h"
#include "input.h"
#include "pty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* グローバルディスプレイ状態 */
DisplayState g_display = {0};

/* ANSI 16色パレット (Xterm default colors) */
static const struct {
    unsigned short r, g, b;
} ansi_colors[16] = {
    {0x0000, 0x0000, 0x0000},  /* 0: 黒 */
    {0xcd00, 0x0000, 0x0000},  /* 1: 赤 */
    {0x0000, 0xcd00, 0x0000},  /* 2: 緑 */
    {0xcd00, 0xcd00, 0x0000},  /* 3: 黄 */
    {0x0000, 0x0000, 0xee00},  /* 4: 青 */
    {0xcd00, 0x0000, 0xcd00},  /* 5: マゼンタ */
    {0x0000, 0xcd00, 0xcd00},  /* 6: シアン */
    {0xe5e5, 0xe5e5, 0xe5e5},  /* 7: 白 */
    {0x7f7f, 0x7f7f, 0x7f7f},  /* 8: 明るい黒 (グレー) */
    {0xff00, 0x0000, 0x0000},  /* 9: 明るい赤 */
    {0x0000, 0xff00, 0x0000},  /* 10: 明るい緑 */
    {0xff00, 0xff00, 0x0000},  /* 11: 明るい黄 */
    {0x5c5c, 0x5c5c, 0xff00},  /* 12: 明るい青 */
    {0xff00, 0x0000, 0xff00},  /* 13: 明るいマゼンタ */
    {0x0000, 0xff00, 0xff00},  /* 14: 明るいシアン */
    {0xffff, 0xffff, 0xffff},  /* 15: 明るい白 */
};

/* Xftカラーキャッシュ（256色対応） */
static XftColor xft_color_cache[256];
static bool color_initialized[256] = {false};

/**
 * 256色インデックスをRGB値に変換
 * @param idx 色インデックス (0-255)
 * @param r, g, b 出力RGB値 (0-65535)
 */
static void color_256_to_rgb(uint8_t idx, unsigned short *r, unsigned short *g, unsigned short *b)
{
    if (idx < 16) {
        /* ANSI 16色 */
        *r = ansi_colors[idx].r;
        *g = ansi_colors[idx].g;
        *b = ansi_colors[idx].b;
    } else if (idx < 232) {
        /* 6x6x6 RGB色空間 (216色) */
        int color_idx = idx - 16;
        int r_level = (color_idx / 36) % 6;
        int g_level = (color_idx / 6) % 6;
        int b_level = color_idx % 6;

        /* 各レベル (0-5) を 0x0000-0xFFFF に変換 */
        const unsigned short levels[6] = {0x0000, 0x5F5F, 0x8787, 0xAFAF, 0xD7D7, 0xFFFF};
        *r = levels[r_level];
        *g = levels[g_level];
        *b = levels[b_level];
    } else {
        /* グレースケール (24段階) */
        int gray_idx = idx - 232;
        unsigned short gray = 0x0808 + (gray_idx * 0x0A0A);
        *r = *g = *b = gray;
    }
}

/**
 * 色を取得（必要に応じて初期化）
 * @param idx 色インデックス (0-255)
 * @return XftColorへのポインタ
 */
static XftColor *get_color(uint8_t idx)
{
    if (!color_initialized[idx]) {
        unsigned short r, g, b;
        color_256_to_rgb(idx, &r, &g, &b);

        XRenderColor xr_color = {r, g, b, 0xffff};
        Visual *visual = DefaultVisual(g_display.display, g_display.screen);
        Colormap colormap = DefaultColormap(g_display.display, g_display.screen);

        XftColorAllocValue(g_display.display, visual, colormap, &xr_color, &xft_color_cache[idx]);
        color_initialized[idx] = true;
    }

    return &xft_color_cache[idx];
}

/* UTF-8エンコード関数 */
static int utf8_encode(uint32_t codepoint, char *utf8)
{
    if (codepoint < 0x80) {
        /* 1バイト */
        utf8[0] = (char)codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        /* 2バイト */
        utf8[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
        utf8[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint < 0x10000) {
        /* 3バイト */
        utf8[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
        utf8[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        utf8[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else if (codepoint < 0x110000) {
        /* 4バイト */
        utf8[0] = 0xF0 | ((codepoint >> 18) & 0x07);
        utf8[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        utf8[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        utf8[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    }
    /* 不正なコードポイント */
    utf8[0] = '?';
    return 1;
}

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

    /* ANSI 16色を初期化 */
    for (int i = 0; i < 16; i++) {
        XRenderColor xr_color = {
            ansi_colors[i].r,
            ansi_colors[i].g,
            ansi_colors[i].b,
            0xffff
        };
        XftColorAllocValue(g_display.display, visual, colormap, &xr_color, &xft_color_cache[i]);
        color_initialized[i] = true;
    }

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

    /* 初期化済みの色を解放 */
    for (int i = 0; i < 256; i++) {
        if (color_initialized[i]) {
            XftColorFree(g_display.display, visual, colormap, &xft_color_cache[i]);
            color_initialized[i] = false;
        }
    }

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

                    /* 新しいターミナルサイズを計算 */
                    int char_width = font_get_char_width();
                    int char_height = font_get_char_height();

                    if (char_width > 0 && char_height > 0) {
                        int new_cols = g_display.width / char_width;
                        int new_rows = g_display.height / char_height;

                        if (new_cols > 0 && new_rows > 0) {
                            /* ターミナルバッファをリサイズ */
                            if (terminal_resize(new_rows, new_cols) == 0) {
                                /* PTYをリサイズ */
                                pty_resize(new_rows, new_cols);

                                /* 再描画 */
                                display_clear();
                                display_render_terminal();
                                display_flush();
                            }
                        }
                    }
                }
                break;

            case KeyPress:
                /* キー入力を処理 */
                input_handle_key(&event.xkey);
                break;

            case ButtonPress:
                /* マウスホイール: Button4=上, Button5=下 */
                if (event.xbutton.button == Button4) {
                    /* 上スクロール */
                    terminal_scroll_by(3);  /* 3行ずつスクロール */
                } else if (event.xbutton.button == Button5) {
                    /* 下スクロール */
                    terminal_scroll_by(-3);  /* 3行ずつスクロール */
                }
                /* その他のボタンは将来実装 */
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

    int scroll_offset = terminal_get_scroll_offset();

    /* 全てのセルを描画 */
    for (int y = 0; y < g_terminal.rows; y++) {
        for (int x = 0; x < g_terminal.cols; x++) {
            Cell *cell = NULL;

            /* スクロールオフセットを考慮してセルを取得 */
            if (scroll_offset > 0) {
                /* スクロールバックバッファから取得 */
                int scrollback_line_idx = g_terminal.scrollback.count - scroll_offset + y;

                if (scrollback_line_idx >= 0 && scrollback_line_idx < g_terminal.scrollback.count) {
                    /* スクロールバックバッファから */
                    ScrollbackLine *line = terminal_get_scrollback_line(scrollback_line_idx);
                    if (line && line->cells && x < line->cols) {
                        cell = &line->cells[x];
                    }
                } else if (scrollback_line_idx >= g_terminal.scrollback.count) {
                    /* 通常バッファから */
                    int buffer_y = scrollback_line_idx - g_terminal.scrollback.count;
                    if (buffer_y >= 0 && buffer_y < g_terminal.rows) {
                        cell = terminal_get_cell(x, buffer_y);
                    }
                }
            } else {
                /* スクロールオフセットなし：通常バッファから */
                cell = terminal_get_cell(x, y);
            }

            if (!cell) {
                continue;
            }

            /* 描画位置を計算 */
            int px = x * char_width;
            int py = y * char_height;

            /* 色を取得（256色対応） */
            uint8_t fg_idx = cell->attr.fg_color;
            uint8_t bg_idx = cell->attr.bg_color;

            /* 反転属性を適用 */
            if (cell->attr.flags & ATTR_REVERSE) {
                uint8_t tmp = fg_idx;
                fg_idx = bg_idx;
                bg_idx = tmp;
            }

            /* WIDE_CHAR_CONTINUATIONはスキップ（全角文字の2セル目） */
            if (cell->ch == WIDE_CHAR_CONTINUATION) {
                continue;
            }

            /* 背景色を描画 */
            if (bg_idx != 0) {  /* 背景が黒でない場合のみ描画 */
                XftColor *bg_color = get_color(bg_idx);
                XSetForeground(g_display.display, g_display.gc, bg_color->pixel);
                XFillRectangle(g_display.display, g_display.window, g_display.gc,
                              px, py, char_width, char_height);
            }

            /* 文字を描画 */
            if (cell->ch != ' ' && cell->ch != 0) {
                char utf8[5];
                int len;

                /* Unicode を UTF-8 に変換 */
                len = utf8_encode(cell->ch, utf8);
                utf8[len] = '\0';

                /* 前景色を選択 */
                XftColor *fg_color = get_color(fg_idx);

                /* 文字を描画 */
                XftDrawStringUtf8(g_display.xft_draw, fg_color,
                                 g_font.xft_font, px, py + g_font.ascent,
                                 (FcChar8 *)utf8, len);

                /* 下線を描画 */
                if (cell->attr.flags & ATTR_UNDERLINE) {
                    int uy = py + g_font.ascent + 1;
                    XSetForeground(g_display.display, g_display.gc, fg_color->pixel);
                    XDrawLine(g_display.display, g_display.window, g_display.gc,
                             px, uy, px + char_width - 1, uy);
                }
            }
        }
    }

    /* カーソルを描画 */
    if (g_terminal.cursor_visible) {
        int cx = g_terminal.cursor_x * char_width;
        int cy = g_terminal.cursor_y * char_height;

        /* カーソルを白い矩形として描画 */
        XSetForeground(g_display.display, g_display.gc,
                      WhitePixel(g_display.display, g_display.screen));
        XFillRectangle(g_display.display, g_display.window, g_display.gc,
                      cx, cy, char_width, 2);  /* 下線カーソル */
    }
}
