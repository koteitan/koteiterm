/*
 * koteiterm - Display Module
 * X11ウィンドウ管理とレンダリング
 */

#include "display.h"
#include "font.h"
#include "terminal.h"
#include "input.h"
#include "pty.h"
#include "color.h"
#include "koteiterm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gif_lib.h>

/* グローバルディスプレイ状態 */
DisplayState g_display = {0};

/* マウス選択状態 */
static bool mouse_selecting = false;
static int selection_start_x = 0;
static int selection_start_y = 0;

/* クリップボード用の選択テキスト */
char *selection_text = NULL;  /* 選択されたテキスト（グローバル） */

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

/**
 * 24-bit RGB値からXftColorを作成（Truecolorモード用）
 * @param rgb RGB値（0x00RRGGBB形式）
 * @param xft_color 出力先XftColor
 */
static void get_rgb_color(uint32_t rgb, XftColor *xft_color)
{
    unsigned short r = ((rgb >> 16) & 0xFF) * 257;  /* 0-255 → 0-65535 */
    unsigned short g = ((rgb >> 8) & 0xFF) * 257;
    unsigned short b = (rgb & 0xFF) * 257;

    XRenderColor xr_color = {r, g, b, 0xffff};
    Visual *visual = DefaultVisual(g_display.display, g_display.screen);
    Colormap colormap = DefaultColormap(g_display.display, g_display.screen);

    XftColorAllocValue(g_display.display, visual, colormap, &xr_color, xft_color);
}

/**
 * 色文字列をパースしてXftColorを割り当てる
 * @param color_str 色文字列（NULL可）
 * @param default_r デフォルトR値
 * @param default_g デフォルトG値
 * @param default_b デフォルトB値
 * @param xft_color 出力先XftColor
 * @return 成功時true、失敗時false
 */
static bool parse_and_alloc_color(const char *color_str,
                                  uint16_t default_r, uint16_t default_g, uint16_t default_b,
                                  XftColor *xft_color)
{
    Visual *visual = DefaultVisual(g_display.display, g_display.screen);
    Colormap colormap = DefaultColormap(g_display.display, g_display.screen);
    XRenderColor xr_color;

    if (color_str) {
        /* 色文字列をパース */
        ColorRGB rgb;
        if (color_parse(color_str, &rgb)) {
            xr_color.red = rgb.r;
            xr_color.green = rgb.g;
            xr_color.blue = rgb.b;
            xr_color.alpha = 0xffff;
        } else {
            fprintf(stderr, "警告: 色 '%s' のパースに失敗しました。デフォルト色を使用します\n", color_str);
            xr_color.red = default_r;
            xr_color.green = default_g;
            xr_color.blue = default_b;
            xr_color.alpha = 0xffff;
        }
    } else {
        /* デフォルト色を使用 */
        xr_color.red = default_r;
        xr_color.green = default_g;
        xr_color.blue = default_b;
        xr_color.alpha = 0xffff;
    }

    return XftColorAllocValue(g_display.display, visual, colormap, &xr_color, xft_color) != 0;
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
 * GIFアニメーションを読み込む
 * @param path GIFファイルのパス
 * @return 成功時0、失敗時-1
 */
static int load_gif_animation(const char *path)
{
    int error_code;
    GifFileType *gif = DGifOpenFileName(path, &error_code);
    if (!gif) {
        fprintf(stderr, "警告: GIFファイルを開けませんでした: %s (error: %d)\n", path, error_code);
        return -1;
    }

    /* GIFの全フレームを読み込む */
    if (DGifSlurp(gif) == GIF_ERROR) {
        fprintf(stderr, "警告: GIF読み込みに失敗しました: %s\n", path);
        DGifCloseFile(gif, &error_code);
        return -1;
    }

    int frame_count = gif->ImageCount;
    if (frame_count <= 0) {
        fprintf(stderr, "警告: GIFにフレームがありません: %s\n", path);
        DGifCloseFile(gif, &error_code);
        return -1;
    }

    /* メモリを確保 */
    g_display.cursor_gif_frames = calloc(frame_count, sizeof(Pixmap));
    g_display.cursor_gif_masks = calloc(frame_count, sizeof(Pixmap));
    g_display.cursor_gif_delays = calloc(frame_count, sizeof(int));
    g_display.cursor_gif_frame_count = frame_count;
    g_display.cursor_gif_current_frame = 0;

    if (!g_display.cursor_gif_frames || !g_display.cursor_gif_masks || !g_display.cursor_gif_delays) {
        fprintf(stderr, "警告: GIFフレーム用のメモリ確保に失敗しました\n");
        free(g_display.cursor_gif_frames);
        free(g_display.cursor_gif_masks);
        free(g_display.cursor_gif_delays);
        DGifCloseFile(gif, &error_code);
        return -1;
    }

    Visual *visual = DefaultVisual(g_display.display, g_display.screen);
    Colormap colormap = DefaultColormap(g_display.display, g_display.screen);

    /* Imlib2コンテキストを設定 */
    imlib_context_set_display(g_display.display);
    imlib_context_set_visual(visual);
    imlib_context_set_colormap(colormap);
    imlib_context_set_drawable(g_display.window);

    int width = gif->SWidth;
    int height = gif->SHeight;

    /* スケーリング計算 */
    int scaled_width = (int)(width * g_display_options.cursor_scale);
    int scaled_height = (int)(height * g_display_options.cursor_scale);
    if (scaled_width <= 0) scaled_width = 1;
    if (scaled_height <= 0) scaled_height = 1;

    g_display.cursor_image_width = scaled_width;
    g_display.cursor_image_height = scaled_height;

    /* 各フレームを処理 */
    uint8_t *canvas = calloc(width * height, 4);  /* RGBA */
    if (!canvas) {
        fprintf(stderr, "警告: GIFキャンバス用のメモリ確保に失敗しました\n");
        free(g_display.cursor_gif_frames);
        free(g_display.cursor_gif_masks);
        free(g_display.cursor_gif_delays);
        DGifCloseFile(gif, &error_code);
        return -1;
    }

    for (int i = 0; i < frame_count; i++) {
        SavedImage *frame = &gif->SavedImages[i];
        GifImageDesc *desc = &frame->ImageDesc;

        /* フレームの遅延時間と透明色インデックスを取得 */
        int delay = 10;  /* デフォルト100ms */
        int transparent_idx = -1;  /* 透明色なし */
        for (int j = 0; j < frame->ExtensionBlockCount; j++) {
            ExtensionBlock *ext = &frame->ExtensionBlocks[j];
            if (ext->Function == GRAPHICS_EXT_FUNC_CODE && ext->ByteCount >= 4) {
                /* Graphics Control Extension の構造:
                 * Byte 0: フラグ (bit 0 = 透明色有効)
                 * Byte 1-2: 遅延時間（10ms単位）
                 * Byte 3: 透明色インデックス
                 */
                uint8_t flags = ext->Bytes[0];
                delay = (ext->Bytes[2] << 8) | ext->Bytes[1];
                if (delay == 0) delay = 10;  /* 0の場合はデフォルト */

                /* 透明色フラグをチェック（bit 0） */
                if (flags & 0x01) {
                    transparent_idx = ext->Bytes[3];
                }
                break;
            }
        }
        g_display.cursor_gif_delays[i] = delay;

        /* カラーマップを取得 */
        ColorMapObject *cmap = desc->ColorMap ? desc->ColorMap : gif->SColorMap;
        if (!cmap) {
            fprintf(stderr, "警告: GIFフレーム%dにカラーマップがありません\n", i);
            continue;
        }

        /* フレームをBGRAキャンバスに描画（Imlib2用） */
        for (int y = 0; y < desc->Height; y++) {
            for (int x = 0; x < desc->Width; x++) {
                int canvas_x = desc->Left + x;
                int canvas_y = desc->Top + y;
                if (canvas_x >= width || canvas_y >= height) continue;

                int pixel_idx = y * desc->Width + x;
                uint8_t color_idx = frame->RasterBits[pixel_idx];

                int canvas_idx = (canvas_y * width + canvas_x) * 4;

                /* 透明色かどうかチェック */
                if (color_idx == transparent_idx) {
                    /* 透明ピクセル: アルファ=0 */
                    canvas[canvas_idx + 0] = 0;
                    canvas[canvas_idx + 1] = 0;
                    canvas[canvas_idx + 2] = 0;
                    canvas[canvas_idx + 3] = 0;
                } else if (color_idx < cmap->ColorCount) {
                    /* 不透明ピクセル: BGRAフォーマット（Imlib2用） */
                    GifColorType *color = &cmap->Colors[color_idx];
                    canvas[canvas_idx + 0] = color->Blue;   /* B */
                    canvas[canvas_idx + 1] = color->Green;  /* G */
                    canvas[canvas_idx + 2] = color->Red;    /* R */
                    canvas[canvas_idx + 3] = 255;           /* A */
                }
            }
        }

        /* Imlib2画像を作成 */
        Imlib_Image img = imlib_create_image_using_copied_data(width, height, (uint32_t *)canvas);
        if (!img) {
            fprintf(stderr, "警告: GIFフレーム%dのImlib2画像作成に失敗しました\n", i);
            continue;
        }

        imlib_context_set_image(img);
        imlib_image_set_has_alpha(1);

        /* スケーリング */
        Imlib_Image scaled_img = imlib_create_cropped_scaled_image(
            0, 0, width, height, scaled_width, scaled_height);
        imlib_free_image();

        if (!scaled_img) {
            fprintf(stderr, "警告: GIFフレーム%dのスケーリングに失敗しました\n", i);
            continue;
        }

        /* PixmapとMaskを生成 */
        imlib_context_set_image(scaled_img);
        imlib_render_pixmaps_for_whole_image(&g_display.cursor_gif_frames[i],
                                             &g_display.cursor_gif_masks[i]);
        imlib_free_image();
    }

    free(canvas);
    DGifCloseFile(gif, &error_code);

    /* 初回のフレームを設定 */
    if (frame_count > 0) {
        g_display.cursor_pixmap = g_display.cursor_gif_frames[0];
        g_display.cursor_mask = g_display.cursor_gif_masks[0];
    }

    /* タイマー初期化 */
    clock_gettime(CLOCK_MONOTONIC, &g_display.cursor_gif_last_update);

    if (g_debug) {
        printf("GIFアニメーションを読み込みました: %s (%dフレーム, サイズ: %dx%d → %dx%d)\n",
               path, frame_count, width, height, scaled_width, scaled_height);
    }

    return 0;
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

    /* CLIPBOARDアトムを取得 */
    g_display.clipboard_atom = XInternAtom(g_display.display, "CLIPBOARD", False);

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

    /* 色を設定（コマンドラインオプションまたはデフォルト） */
    parse_and_alloc_color(g_color_options.foreground, 0xffff, 0xffff, 0xffff, &g_display.xft_fg);  /* デフォルト: 白 */
    parse_and_alloc_color(g_color_options.background, 0x0000, 0x0000, 0x0000, &g_display.xft_bg);  /* デフォルト: 黒 */
    parse_and_alloc_color(g_color_options.cursor, 0xff00, 0x0000, 0x0000, &g_display.xft_cursor);  /* デフォルト: 赤 */
    parse_and_alloc_color(g_color_options.sel_bg, 0x5c5c, 0x5c5c, 0x5c5c, &g_display.xft_sel_bg);  /* デフォルト: グレー */
    parse_and_alloc_color(g_color_options.sel_fg, 0xffff, 0xffff, 0xffff, &g_display.xft_sel_fg);  /* デフォルト: 白 */
    parse_and_alloc_color(g_color_options.underline, 0xffff, 0xffff, 0xffff, &g_display.xft_underline);  /* デフォルト: 白 */

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

    /* XIM (Input Method) を初期化 */
    g_display.xim = XOpenIM(g_display.display, NULL, NULL, NULL);
    if (!g_display.xim) {
        fprintf(stderr, "警告: XIM (Input Method) を開けませんでした\n");
        fprintf(stderr, "日本語入力が利用できない可能性があります\n");
        g_display.xic = NULL;
    } else {
        /* XIC (Input Context) を作成 */
        g_display.xic = XCreateIC(g_display.xim,
                                  XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                  XNClientWindow, g_display.window,
                                  XNFocusWindow, g_display.window,
                                  NULL);
        if (!g_display.xic) {
            fprintf(stderr, "警告: XIC (Input Context) を作成できませんでした\n");
            fprintf(stderr, "日本語入力が利用できない可能性があります\n");
            XCloseIM(g_display.xim);
            g_display.xim = NULL;
        } else {
            /* フォーカスを設定 */
            XSetICFocus(g_display.xic);
            if (g_debug) {
                printf("XIM/XICを初期化しました（日本語入力が利用可能）\n");
            }
        }
    }

    if (g_debug) {
        printf("X11ディスプレイを初期化しました (%dx%d)\n", width, height);
    }

    /* 画像カーソルのロード */
    if (g_display_options.cursor_shape == TERM_CURSOR_IMAGE && g_display_options.cursor_image_path) {
        /* 拡張子でGIFかどうか判定 */
        const char *ext = strrchr(g_display_options.cursor_image_path, '.');
        bool is_gif = (ext && strcasecmp(ext, ".gif") == 0);

        if (is_gif) {
            /* GIFアニメーションを読み込む */
            if (load_gif_animation(g_display_options.cursor_image_path) != 0) {
                /* 失敗した場合はデフォルトカーソルにフォールバック */
                g_display_options.cursor_shape = TERM_CURSOR_BAR;
            }
        } else {
            /* 静止画像（PNG等）をImlib2で読み込む */
            /* Imlib2コンテキストを設定 */
            imlib_context_set_display(g_display.display);
            imlib_context_set_visual(visual);
            imlib_context_set_colormap(colormap);
            imlib_context_set_drawable(g_display.window);

            /* 画像をロード */
            Imlib_Image image = imlib_load_image(g_display_options.cursor_image_path);
            if (!image) {
                fprintf(stderr, "警告: カーソル画像の読み込みに失敗しました: %s\n", g_display_options.cursor_image_path);
                /* デフォルトカーソルにフォールバック */
                g_display_options.cursor_shape = TERM_CURSOR_BAR;
            } else {
                imlib_context_set_image(image);

                int orig_width = imlib_image_get_width();
                int orig_height = imlib_image_get_height();

                /* スケーリング計算 */
                int scaled_width = (int)(orig_width * g_display_options.cursor_scale);
                int scaled_height = (int)(orig_height * g_display_options.cursor_scale);

                if (scaled_width <= 0) scaled_width = 1;
                if (scaled_height <= 0) scaled_height = 1;

                /* スケーリング済み画像を作成 */
                Imlib_Image scaled_image = imlib_create_cropped_scaled_image(
                    0, 0, orig_width, orig_height, scaled_width, scaled_height);

                /* 元の画像を解放 */
                imlib_free_image();

                if (!scaled_image) {
                    fprintf(stderr, "警告: カーソル画像のスケーリングに失敗しました\n");
                    g_display_options.cursor_shape = TERM_CURSOR_BAR;
                } else {
                    imlib_context_set_image(scaled_image);

                    /* Pixmap と Mask を作成 */
                    imlib_render_pixmaps_for_whole_image(&g_display.cursor_pixmap, &g_display.cursor_mask);

                    /* サイズを保存 */
                    g_display.cursor_image_width = scaled_width;
                    g_display.cursor_image_height = scaled_height;

                    /* Imlib_Imageを保存 */
                    g_display.cursor_image = scaled_image;

                    if (g_debug) {
                        printf("カーソル画像を読み込みました: %s (元: %dx%d, スケール: %.2f, 結果: %dx%d)\n",
                               g_display_options.cursor_image_path, orig_width, orig_height,
                               g_display_options.cursor_scale, scaled_width, scaled_height);
                    }
                }
            }
        }
    }

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

    /* XIC/XIMをクリーンアップ */
    if (g_display.xic) {
        XDestroyIC(g_display.xic);
        g_display.xic = NULL;
    }
    if (g_display.xim) {
        XCloseIM(g_display.xim);
        g_display.xim = NULL;
    }

    /* Xft リソースをクリーンアップ */
    if (g_display.xft_draw) {
        XftDrawDestroy(g_display.xft_draw);
    }

    Visual *visual = DefaultVisual(g_display.display, g_display.screen);
    Colormap colormap = DefaultColormap(g_display.display, g_display.screen);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_fg);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_bg);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_cursor);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_sel_bg);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_sel_fg);
    XftColorFree(g_display.display, visual, colormap, &g_display.xft_underline);

    /* 初期化済みの色を解放 */
    for (int i = 0; i < 256; i++) {
        if (color_initialized[i]) {
            XftColorFree(g_display.display, visual, colormap, &xft_color_cache[i]);
            color_initialized[i] = false;
        }
    }

    /* Imlib2画像リソースを解放 */
    if (g_display.cursor_image) {
        imlib_context_set_image(g_display.cursor_image);
        imlib_free_image();
        g_display.cursor_image = NULL;
    }

    /* GIFアニメーションフレームを解放 */
    if (g_display.cursor_gif_frames) {
        for (int i = 0; i < g_display.cursor_gif_frame_count; i++) {
            if (g_display.cursor_gif_frames[i]) {
                XFreePixmap(g_display.display, g_display.cursor_gif_frames[i]);
            }
            if (g_display.cursor_gif_masks && g_display.cursor_gif_masks[i]) {
                XFreePixmap(g_display.display, g_display.cursor_gif_masks[i]);
            }
        }
        free(g_display.cursor_gif_frames);
        free(g_display.cursor_gif_masks);
        free(g_display.cursor_gif_delays);
        g_display.cursor_gif_frames = NULL;
        g_display.cursor_gif_masks = NULL;
        g_display.cursor_gif_delays = NULL;
        /* cursor_pixmap/cursor_maskはGIFフレームの一部なので、二重解放を防ぐため0にする */
        g_display.cursor_pixmap = 0;
        g_display.cursor_mask = 0;
    }

    /* Pixmapを解放（GIF以外の静止画像用） */
    if (g_display.cursor_pixmap && !g_display.cursor_gif_frames) {
        XFreePixmap(g_display.display, g_display.cursor_pixmap);
        g_display.cursor_pixmap = 0;
    }
    if (g_display.cursor_mask && !g_display.cursor_gif_frames) {
        XFreePixmap(g_display.display, g_display.cursor_mask);
        g_display.cursor_mask = 0;
    }

    if (g_display.gc) {
        XFreeGC(g_display.display, g_display.gc);
    }

    if (g_display.window) {
        XDestroyWindow(g_display.display, g_display.window);
    }

    XCloseDisplay(g_display.display);

    /* 選択テキストを解放 */
    if (selection_text) {
        free(selection_text);
        selection_text = NULL;
    }

    if (g_debug) {
        printf("X11ディスプレイをクリーンアップしました\n");
    }

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

        /* IMEにイベントを渡す */
        if (XFilterEvent(&event, g_display.window)) {
            /* IMEがイベントを処理した場合はスキップ */
            extern bool g_debug_key;
            if (g_debug_key && event.type == KeyPress) {
                fprintf(stderr, "DEBUG: XFilterEvent がキーイベントをフィルタしました\n");
            }
            continue;
        }

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
                    if (g_debug) {
                        printf("ウィンドウクローズが要求されました\n");
                    }
                    return false;  /* 終了 */
                }
                break;

            case ConfigureNotify:
                /* ウィンドウサイズが変更された */
                if (event.xconfigure.width != g_display.width ||
                    event.xconfigure.height != g_display.height) {
                    g_display.width = event.xconfigure.width;
                    g_display.height = event.xconfigure.height;
                    if (g_debug) {
                        printf("ウィンドウサイズ変更: %dx%d\n",
                               g_display.width, g_display.height);
                    }

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
                } else if (event.xbutton.button == Button1) {
                    /* 左ボタン: 選択開始 */
                    int char_width = font_get_char_width();
                    int char_height = font_get_char_height();
                    int x = event.xbutton.x / char_width;
                    int y = event.xbutton.y / char_height;

                    /* 開始位置を記録 */
                    selection_start_x = x;
                    selection_start_y = y;

                    terminal_selection_start(x, y);
                    mouse_selecting = true;
                }
                break;

            case ButtonRelease:
                if (event.xbutton.button == Button1 && mouse_selecting) {
                    /* 終了位置を計算 */
                    int char_width = font_get_char_width();
                    int char_height = font_get_char_height();
                    int end_x = event.xbutton.x / char_width;
                    int end_y = event.xbutton.y / char_height;

                    /* ドラッグしていない場合（開始位置と終了位置が同じ）は選択をクリア */
                    if (end_x == selection_start_x && end_y == selection_start_y) {
                        /* 画面上の選択表示のみクリア（クリップボードは保持） */
                        terminal_selection_clear();
                        mouse_selecting = false;
                        /* 注: selection_text はクリアしない（前回の選択内容を保持） */
                    } else {
                        /* ドラッグした場合は選択を確定 */
                        terminal_selection_end();
                        mouse_selecting = false;

                        /* 選択されたテキストをPRIMARY選択にコピー */
                        if (selection_text) {
                            free(selection_text);
                        }
                        selection_text = terminal_get_selected_text();
                        extern bool g_debug;
                        if (g_debug) {
                            fprintf(stderr, "DEBUG: terminal_get_selected_text() returned: %s\n",
                                   selection_text ? selection_text : "NULL");
                            if (selection_text) {
                                fprintf(stderr, "DEBUG: selection_text hex dump: ");
                                for (size_t i = 0; i < strlen(selection_text) && i < 32; i++) {
                                    fprintf(stderr, "%02x ", (unsigned char)selection_text[i]);
                                }
                                fprintf(stderr, "\n");
                            }
                        }
                        if (selection_text) {
                            /* PRIMARYとCLIPBOARDの両方を設定（WSLg互換性のため） */
                            XSetSelectionOwner(g_display.display, XA_PRIMARY,
                                              g_display.window, CurrentTime);
                            XSetSelectionOwner(g_display.display, g_display.clipboard_atom,
                                              g_display.window, CurrentTime);

                            /* WSLg互換性のため、winclip.exeを使用（セキュリティ上PowerShell非使用） */
                            FILE *clip = popen("winclip.exe set 2>/dev/null", "w");
                            if (clip) {
                                fwrite(selection_text, 1, strlen(selection_text), clip);
                                int status = pclose(clip);
                                if (status == 0 && g_debug) {
                                    fprintf(stderr, "DEBUG: マウス選択でwinclip.exe経由でWindowsクリップボードにコピー\n");
                                } else if (g_debug) {
                                    fprintf(stderr, "DEBUG: winclip.exe failed (status=%d)\n", status);
                                }
                            } else if (g_debug) {
                                fprintf(stderr, "DEBUG: winclip.exe not available\n");
                            }

                            if (g_debug) {
                                fprintf(stderr, "DEBUG: 選択テキストをCLIPBOARDにコピー: [%s]\n", selection_text);
                                fprintf(stderr, "DEBUG: PRIMARY owner: %lu, CLIPBOARD owner: %lu\n",
                                       XGetSelectionOwner(g_display.display, XA_PRIMARY),
                                       XGetSelectionOwner(g_display.display, g_display.clipboard_atom));
                            }
                        } else {
                            if (g_debug) {
                                fprintf(stderr, "DEBUG: 選択テキストが空のためコピーできません\n");
                            }
                        }
                    }
                } else if (event.xbutton.button == Button2) {
                    /* 中ボタン: Windowsクリップボードから貼り付け（WSLg互換性） */
                    extern bool g_debug;
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: 中ボタンクリック、Windowsクリップボードから貼り付け\n");
                    }

                    /* WSLg互換性のため、winclip.exeを使用（セキュリティ上PowerShell非使用） */
                    FILE *paste = popen("winclip.exe get 2>/dev/null", "r");
                    if (paste) {
                        char buffer[8192];
                        size_t total_len = 0;
                        size_t n;

                        /* すべてのデータを読み取る */
                        while ((n = fread(buffer + total_len, 1, sizeof(buffer) - total_len - 1, paste)) > 0) {
                            total_len += n;
                        }
                        int status = pclose(paste);

                        if (total_len > 0 && status == 0) {
                            if (g_debug) {
                                fprintf(stderr, "DEBUG: winclip.exeから読み取ったデータ (hex): ");
                                for (size_t i = 0; i < total_len && i < 32; i++) {
                                    fprintf(stderr, "%02x ", (unsigned char)buffer[i]);
                                }
                                fprintf(stderr, "\n");
                            }

                            /* Windowsの改行(CRLF)をUnix形式(LF)に変換 */
                            char output[8192];
                            size_t out_len = 0;
                            for (size_t i = 0; i < total_len; i++) {
                                if (buffer[i] == '\r' && i + 1 < total_len && buffer[i + 1] == '\n') {
                                    /* CRをスキップ、次のLFのみ使う */
                                    continue;
                                } else if (buffer[i] == '\r') {
                                    /* 単独のCRをLFに変換 */
                                    output[out_len++] = '\n';
                                } else {
                                    output[out_len++] = buffer[i];
                                }
                            }

                            if (out_len > 0) {
                                pty_write(output, out_len);
                            }

                            if (g_debug) {
                                fprintf(stderr, "DEBUG: 中ボタンでwinclip.exe経由で貼り付け (読み取り: %zu bytes, 出力: %zu bytes)\n", total_len, out_len);
                            }
                        } else {
                            /* winclip.exeが失敗した場合はX11 PRIMARY選択にフォールバック */
                            if (g_debug) {
                                fprintf(stderr, "DEBUG: winclip.exe失敗、PRIMARY選択にフォールバック\n");
                            }
                            XConvertSelection(g_display.display, XA_PRIMARY, XA_STRING,
                                             XA_PRIMARY, g_display.window, CurrentTime);
                        }
                    } else {
                        /* winclip.exeが使えない場合はX11 PRIMARY選択にフォールバック */
                        if (g_debug) {
                            fprintf(stderr, "DEBUG: winclip.exe使用不可、PRIMARY選択にフォールバック\n");
                        }
                        XConvertSelection(g_display.display, XA_PRIMARY, XA_STRING,
                                         XA_PRIMARY, g_display.window, CurrentTime);
                    }
                }
                break;

            case SelectionRequest:
            {
                /* 他のアプリケーションが選択を要求 */
                XSelectionRequestEvent *req = &event.xselectionrequest;
                XSelectionEvent ev;
                ev.type = SelectionNotify;
                ev.requestor = req->requestor;
                ev.selection = req->selection;
                ev.target = req->target;
                ev.time = req->time;
                ev.property = None;

                if (req->target == XA_STRING && selection_text) {
                    extern bool g_debug;
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: SelectionRequest受信 - selection=%lu, target=%lu\n",
                               req->selection, req->target);
                    }
                    XChangeProperty(g_display.display, req->requestor,
                                   req->property, XA_STRING, 8,
                                   PropModeReplace,
                                   (unsigned char *)selection_text,
                                   strlen(selection_text));
                    ev.property = req->property;
                }

                XSendEvent(g_display.display, req->requestor, False, 0, (XEvent *)&ev);
                break;
            }

            case SelectionNotify:
            {
                /* 選択データを受信 */
                if (event.xselection.property != None) {
                    Atom actual_type;
                    int actual_format;
                    unsigned long nitems, bytes_after;
                    unsigned char *data = NULL;

                    extern bool g_debug;
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: SelectionNotify受信 - selection=%lu, property=%lu\n",
                               event.xselection.selection, event.xselection.property);
                    }
                    if (XGetWindowProperty(g_display.display, g_display.window,
                                          event.xselection.property, 0, 8192,
                                          False, AnyPropertyType,
                                          &actual_type, &actual_format,
                                          &nitems, &bytes_after, &data) == Success) {
                        if (data && nitems > 0) {
                            if (g_debug) {
                                fprintf(stderr, "DEBUG: 貼り付けデータ受信: %lu bytes\n", nitems);
                            }
                            /* ターミナルに貼り付け */
                            pty_write((const char *)data, nitems);
                        }
                        if (data) {
                            XFree(data);
                        }
                    }
                    XDeleteProperty(g_display.display, g_display.window,
                                   event.xselection.property);
                }
                break;
            }

            case MotionNotify:
                if (mouse_selecting) {
                    /* 選択を更新 */
                    int char_width = font_get_char_width();
                    int char_height = font_get_char_height();
                    int x = event.xmotion.x / char_width;
                    int y = event.xmotion.y / char_height;
                    terminal_selection_update(x, y);
                }
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

    /* パス1: 全ての背景を描画 */
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

            /* 選択範囲のハイライト */
            bool is_selected = terminal_is_selected(x, y);

            /* 反転属性を適用 */
            if (cell->attr.flags & ATTR_REVERSE) {
                uint8_t tmp = fg_idx;
                fg_idx = bg_idx;
                bg_idx = tmp;
            }

            /* 背景色を決定 */
            XftColor *bg_color = NULL;
            XftColor temp_bg_color;  /* Truecolorモード用の一時カラー */

            if (is_selected) {
                /* 選択範囲は設定色を使用 */
                bg_color = &g_display.xft_sel_bg;
            } else {
                /* 背景色を決定 */
                if (cell->attr.flags & ATTR_BG_TRUECOLOR) {
                    /* Truecolor背景色 */
                    get_rgb_color(cell->attr.bg_rgb, &temp_bg_color);
                    bg_color = &temp_bg_color;
                } else if (bg_idx == 0) {
                    /* デフォルト黒はカスタム色を使用 */
                    bg_color = &g_display.xft_bg;
                } else {
                    /* 256色パレット */
                    bg_color = get_color(bg_idx);
                }
            }

            /* 背景色を描画 */
            /* 選択範囲、または背景色がデフォルト以外、またはTruecolor背景、またはカスタム背景色が設定されている場合に描画 */
            if (is_selected || bg_idx != 0 || (cell->attr.flags & ATTR_BG_TRUECOLOR) || g_color_options.background != NULL) {
                XSetForeground(g_display.display, g_display.gc, bg_color->pixel);
                XFillRectangle(g_display.display, g_display.window, g_display.gc,
                              px, py, char_width, char_height);
            }
        }
    }

    /* パス2: 全ての文字を描画 */
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

            /* WIDE_CHAR_CONTINUATIONは文字描画をスキップ（全角文字の2セル目） */
            if (cell->ch == WIDE_CHAR_CONTINUATION) {
                continue;
            }

            /* 描画位置を計算 */
            int px = x * char_width;
            int py = y * char_height;

            /* 色を取得（256色対応） */
            uint8_t fg_idx = cell->attr.fg_color;
            uint8_t bg_idx = cell->attr.bg_color;

            /* 選択範囲のハイライト */
            bool is_selected = terminal_is_selected(x, y);

            /* 反転属性を適用 */
            if (cell->attr.flags & ATTR_REVERSE) {
                uint8_t tmp = fg_idx;
                fg_idx = bg_idx;
                bg_idx = tmp;
            }

            /* 前景色を決定 */
            XftColor *fg_color = NULL;
            XftColor temp_fg_color;  /* Truecolorモード用の一時カラー */

            if (is_selected) {
                /* 選択範囲は設定色を使用 */
                fg_color = &g_display.xft_sel_fg;
            } else {
                /* 前景色を決定 */
                if (cell->attr.flags & ATTR_FG_TRUECOLOR) {
                    /* Truecolor前景色 */
                    get_rgb_color(cell->attr.fg_rgb, &temp_fg_color);
                    fg_color = &temp_fg_color;
                } else if (fg_idx == 7) {
                    /* デフォルト白はカスタム色を使用 */
                    fg_color = &g_display.xft_fg;
                } else {
                    /* 256色パレット */
                    fg_color = get_color(fg_idx);
                }
            }

            /* 文字を描画 */
            if (cell->ch != ' ' && cell->ch != 0) {
                char utf8[5];
                int len;

                /* Unicode を UTF-8 に変換 */
                len = utf8_encode(cell->ch, utf8);
                utf8[len] = '\0';

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

    /* 全幅アンダーラインを描画 */
    if (g_display_options.show_underline && g_terminal.cursor_y >= 0 && g_terminal.cursor_y < g_terminal.rows) {
        int uly = g_terminal.cursor_y * char_height + char_height - 1;
        XSetForeground(g_display.display, g_display.gc, g_display.xft_underline.pixel);
        XDrawLine(g_display.display, g_display.window, g_display.gc,
                 0, uly, g_display.width, uly);
    }

    /* カーソルを描画 */
    if (g_terminal.cursor_visible) {
        int cx = g_terminal.cursor_x * char_width;
        int cy = g_terminal.cursor_y * char_height;

        XSetForeground(g_display.display, g_display.gc, g_display.xft_cursor.pixel);

        switch (g_display_options.cursor_shape) {
            case TERM_CURSOR_UNDERLINE:
                /* 短いアンダーライン（文字セルの下部） */
                XFillRectangle(g_display.display, g_display.window, g_display.gc,
                              cx, cy + char_height - 2, char_width, 2);
                break;

            case TERM_CURSOR_BAR:
                /* 左縦線 */
                XFillRectangle(g_display.display, g_display.window, g_display.gc,
                              cx, cy, 2, char_height);
                break;

            case TERM_CURSOR_HOLLOW_BLOCK:
                /* 中抜き四角 */
                XDrawRectangle(g_display.display, g_display.window, g_display.gc,
                              cx, cy, char_width - 1, char_height - 1);
                break;

            case TERM_CURSOR_BLOCK:
                /* 中埋め四角 */
                XFillRectangle(g_display.display, g_display.window, g_display.gc,
                              cx, cy, char_width, char_height);
                break;

            case TERM_CURSOR_IMAGE:
                /* 画像カーソル */
                if (g_display.cursor_pixmap) {
                    /* 画像描画位置を計算（左下基準でオフセット適用） */
                    int img_x = cx + g_display_options.cursor_offset_x;
                    int img_y = cy + char_height - g_display.cursor_image_height + g_display_options.cursor_offset_y;

                    if (g_display.cursor_mask) {
                        /* マスクを使って透過描画 */
                        XSetClipMask(g_display.display, g_display.gc, g_display.cursor_mask);
                        XSetClipOrigin(g_display.display, g_display.gc, img_x, img_y);
                    }

                    /* Pixmapをコピー */
                    XCopyArea(g_display.display, g_display.cursor_pixmap, g_display.window,
                              g_display.gc, 0, 0, g_display.cursor_image_width,
                              g_display.cursor_image_height, img_x, img_y);

                    if (g_display.cursor_mask) {
                        /* クリップマスクをリセット */
                        XSetClipMask(g_display.display, g_display.gc, None);
                    }
                }
                break;
        }
    }
}

/**
 * GIFアニメーションカーソルを更新する
 * メインループから定期的に呼び出される
 */
void display_update_gif_cursor(void)
{
    /* GIFアニメーションがない場合は何もしない */
    if (!g_display.cursor_gif_frames || g_display.cursor_gif_frame_count <= 1) {
        return;
    }

    /* 現在時刻を取得 */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    /* 前回の更新からの経過時間を計算（ミリ秒単位） */
    long elapsed_ms = (now.tv_sec - g_display.cursor_gif_last_update.tv_sec) * 1000 +
                      (now.tv_nsec - g_display.cursor_gif_last_update.tv_nsec) / 1000000;

    /* 現在のフレームの遅延時間（10ms単位 → ms）*/
    int current_delay_ms = g_display.cursor_gif_delays[g_display.cursor_gif_current_frame] * 10;

    /* フレーム切り替えタイミングをチェック */
    if (elapsed_ms >= current_delay_ms) {
        /* 次のフレームに進む */
        g_display.cursor_gif_current_frame++;
        if (g_display.cursor_gif_current_frame >= g_display.cursor_gif_frame_count) {
            /* ループ */
            g_display.cursor_gif_current_frame = 0;
        }

        /* 現在のフレームのPixmapとMaskを設定 */
        g_display.cursor_pixmap = g_display.cursor_gif_frames[g_display.cursor_gif_current_frame];
        g_display.cursor_mask = g_display.cursor_gif_masks[g_display.cursor_gif_current_frame];

        /* タイマーを更新 */
        g_display.cursor_gif_last_update = now;
    }
}
