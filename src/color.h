#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdint.h>

/**
 * RGB色を表す構造体（16bit値）
 */
typedef struct {
    uint16_t r;  /* 0x0000-0xFFFF */
    uint16_t g;  /* 0x0000-0xFFFF */
    uint16_t b;  /* 0x0000-0xFFFF */
} ColorRGB;

/**
 * 色文字列をパースしてRGB値を取得する
 *
 * サポートする形式:
 * - 色名: "red", "blue", "white", "black", etc.
 * - #RGB: "#f00", "#0f0", "#00f"
 * - #RRGGBB: "#ff0000", "#00ff00", "#0000ff"
 * - rgb:RR/GG/BB: "rgb:ff/00/00"
 * - rgb:RRRR/GGGG/BBBB: "rgb:ffff/0000/0000"
 *
 * @param color_str 色文字列
 * @param rgb 出力先RGB構造体
 * @return 成功時true、失敗時false
 */
bool color_parse(const char *color_str, ColorRGB *rgb);

#endif /* COLOR_H */
