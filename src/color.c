/*
 * koteiterm - Color Module
 * 色文字列パース処理
 */

#include "color.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * 基本色名のテーブル（X11色名の一部）
 */
typedef struct {
    const char *name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} NamedColor;

static const NamedColor named_colors[] = {
    /* 基本16色 */
    {"black",   0x00, 0x00, 0x00},
    {"red",     0xff, 0x00, 0x00},
    {"green",   0x00, 0xff, 0x00},
    {"yellow",  0xff, 0xff, 0x00},
    {"blue",    0x00, 0x00, 0xff},
    {"magenta", 0xff, 0x00, 0xff},
    {"cyan",    0x00, 0xff, 0xff},
    {"white",   0xff, 0xff, 0xff},

    /* 拡張色 */
    {"gray",    0x80, 0x80, 0x80},
    {"grey",    0x80, 0x80, 0x80},
    {"darkgray",  0x40, 0x40, 0x40},
    {"darkgrey",  0x40, 0x40, 0x40},
    {"lightgray", 0xc0, 0xc0, 0xc0},
    {"lightgrey", 0xc0, 0xc0, 0xc0},
    {"orange",  0xff, 0xa5, 0x00},
    {"purple",  0x80, 0x00, 0x80},
    {"brown",   0xa5, 0x2a, 0x2a},
    {"pink",    0xff, 0xc0, 0xcb},

    {NULL, 0, 0, 0}  /* 終端 */
};

/**
 * 16進数文字を数値に変換
 */
static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/**
 * 8ビット値を16ビット値に変換（0-255 -> 0-65535）
 */
static uint16_t scale_8_to_16(uint8_t val)
{
    return (uint16_t)val * 257;  /* 65535 / 255 ≈ 257 */
}

/**
 * 色名からRGB値を取得
 */
static bool parse_named_color(const char *name, ColorRGB *rgb)
{
    /* 小文字に変換して比較 */
    char lower_name[64];
    size_t i;

    if (strlen(name) >= sizeof(lower_name)) {
        return false;
    }

    for (i = 0; name[i]; i++) {
        lower_name[i] = tolower((unsigned char)name[i]);
    }
    lower_name[i] = '\0';

    /* テーブルを検索 */
    for (i = 0; named_colors[i].name != NULL; i++) {
        if (strcmp(lower_name, named_colors[i].name) == 0) {
            rgb->r = scale_8_to_16(named_colors[i].r);
            rgb->g = scale_8_to_16(named_colors[i].g);
            rgb->b = scale_8_to_16(named_colors[i].b);
            return true;
        }
    }

    return false;
}

/**
 * #RGB または #RRGGBB 形式をパース
 */
static bool parse_hex_color(const char *str, ColorRGB *rgb)
{
    size_t len = strlen(str);
    int r1, r2, g1, g2, b1, b2;

    if (str[0] != '#') {
        return false;
    }

    if (len == 4) {
        /* #RGB 形式 */
        r1 = hex_to_int(str[1]);
        g1 = hex_to_int(str[2]);
        b1 = hex_to_int(str[3]);

        if (r1 < 0 || g1 < 0 || b1 < 0) {
            return false;
        }

        /* 4ビット値を8ビット値に変換（0xF -> 0xFF） */
        rgb->r = scale_8_to_16((uint8_t)((r1 << 4) | r1));
        rgb->g = scale_8_to_16((uint8_t)((g1 << 4) | g1));
        rgb->b = scale_8_to_16((uint8_t)((b1 << 4) | b1));
        return true;

    } else if (len == 7) {
        /* #RRGGBB 形式 */
        r1 = hex_to_int(str[1]);
        r2 = hex_to_int(str[2]);
        g1 = hex_to_int(str[3]);
        g2 = hex_to_int(str[4]);
        b1 = hex_to_int(str[5]);
        b2 = hex_to_int(str[6]);

        if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) {
            return false;
        }

        rgb->r = scale_8_to_16((uint8_t)((r1 << 4) | r2));
        rgb->g = scale_8_to_16((uint8_t)((g1 << 4) | g2));
        rgb->b = scale_8_to_16((uint8_t)((b1 << 4) | b2));
        return true;
    }

    return false;
}

/**
 * rgb:RR/GG/BB または rgb:RRRR/GGGG/BBBB 形式をパース
 */
static bool parse_rgb_color(const char *str, ColorRGB *rgb)
{
    char buf[64];
    char *r_str, *g_str, *b_str;
    char *saveptr;
    unsigned long r, g, b;
    size_t comp_len;

    /* "rgb:" で始まることを確認 */
    if (strncmp(str, "rgb:", 4) != 0) {
        return false;
    }

    /* コピーして分割 */
    if (strlen(str + 4) >= sizeof(buf)) {
        return false;
    }
    strcpy(buf, str + 4);

    /* '/' で分割 */
    r_str = strtok_r(buf, "/", &saveptr);
    g_str = strtok_r(NULL, "/", &saveptr);
    b_str = strtok_r(NULL, "/", &saveptr);

    if (!r_str || !g_str || !b_str) {
        return false;
    }

    /* 各成分の長さをチェック（2桁または4桁） */
    comp_len = strlen(r_str);
    if (comp_len != 2 && comp_len != 4) {
        return false;
    }
    if (strlen(g_str) != comp_len || strlen(b_str) != comp_len) {
        return false;
    }

    /* 16進数として解析 */
    char *endptr;
    r = strtoul(r_str, &endptr, 16);
    if (*endptr != '\0') return false;

    g = strtoul(g_str, &endptr, 16);
    if (*endptr != '\0') return false;

    b = strtoul(b_str, &endptr, 16);
    if (*endptr != '\0') return false;

    if (comp_len == 2) {
        /* 8ビット値を16ビットに変換 */
        rgb->r = scale_8_to_16((uint8_t)r);
        rgb->g = scale_8_to_16((uint8_t)g);
        rgb->b = scale_8_to_16((uint8_t)b);
    } else {
        /* 16ビット値をそのまま使用 */
        rgb->r = (uint16_t)r;
        rgb->g = (uint16_t)g;
        rgb->b = (uint16_t)b;
    }

    return true;
}

/**
 * 色文字列をパースしてRGB値を取得する
 */
bool color_parse(const char *color_str, ColorRGB *rgb)
{
    if (!color_str || !rgb) {
        return false;
    }

    /* 空文字列チェック */
    if (color_str[0] == '\0') {
        return false;
    }

    /* 形式に応じて適切なパーサーを呼び出す */
    if (color_str[0] == '#') {
        /* #RGB または #RRGGBB 形式 */
        return parse_hex_color(color_str, rgb);
    } else if (strncmp(color_str, "rgb:", 4) == 0) {
        /* rgb:RR/GG/BB または rgb:RRRR/GGGG/BBBB 形式 */
        return parse_rgb_color(color_str, rgb);
    } else {
        /* 色名 */
        return parse_named_color(color_str, rgb);
    }
}
