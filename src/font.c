/*
 * koteiterm - Font Module
 * フォント管理とメトリクス
 */

#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* グローバルフォント状態 */
FontState g_font = {0};

/**
 * フォントを初期化する
 */
int font_init(Display *display, int screen, const char *font_name, int font_size)
{
    char font_pattern[256];
    XGlyphInfo extents;

    /* フォントパターンを構築（日本語対応） */
    snprintf(font_pattern, sizeof(font_pattern), "%s:size=%d:antialias=true:lang=ja",
             font_name, font_size);

    /* Xftフォントを開く */
    g_font.xft_font = XftFontOpenName(display, screen, font_pattern);
    if (!g_font.xft_font) {
        fprintf(stderr, "エラー: フォントを開けません: %s\n", font_pattern);
        return -1;
    }

    /* フォントメトリクスを取得 */
    g_font.ascent = g_font.xft_font->ascent;
    g_font.descent = g_font.xft_font->descent;
    g_font.char_height = g_font.ascent + g_font.descent;

    /* 文字 'M' の幅を測定（等幅フォントの幅） */
    XftTextExtentsUtf8(display, g_font.xft_font,
                       (FcChar8 *)"M", 1, &extents);
    g_font.char_width = extents.xOff;

    printf("フォントを初期化しました: %s (幅=%d, 高さ=%d)\n",
           font_pattern, g_font.char_width, g_font.char_height);

    return 0;
}

/**
 * フォントをクリーンアップする
 */
void font_cleanup(Display *display)
{
    if (g_font.xft_font) {
        XftFontClose(display, g_font.xft_font);
        g_font.xft_font = NULL;
    }

    memset(&g_font, 0, sizeof(g_font));

    printf("フォントをクリーンアップしました\n");
}

/**
 * 文字の幅を取得する
 */
int font_get_char_width(void)
{
    return g_font.char_width;
}

/**
 * 文字の高さを取得する
 */
int font_get_char_height(void)
{
    return g_font.char_height;
}
