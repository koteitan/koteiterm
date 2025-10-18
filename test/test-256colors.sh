#!/bin/bash

# 256色テストスクリプト

echo "=== 256色テスト ==="
echo ""

# ANSI 16色
echo "ANSI 16色:"
for i in {0..15}; do
    printf "\e[48;5;%dm  %3d  \e[0m" $i $i
    if [ $(( ($i + 1) % 8 )) -eq 0 ]; then
        echo ""
    fi
done
echo ""

# 216色 (6x6x6 RGB)
echo "216色 RGB色空間:"
for i in {16..231}; do
    printf "\e[48;5;%dm  \e[0m" $i
    if [ $(( ($i - 16 + 1) % 36 )) -eq 0 ]; then
        echo ""
    fi
done
echo ""

# グレースケール 24段階
echo "グレースケール 24段階:"
for i in {232..255}; do
    printf "\e[48;5;%dm  %3d  \e[0m" $i $i
done
echo ""
echo ""

# 前景色テスト
echo "前景色テスト:"
for i in {0..15}; do
    printf "\e[38;5;%dmColor %d\e[0m " $i $i
    if [ $(( ($i + 1) % 4 )) -eq 0 ]; then
        echo ""
    fi
done
echo ""

echo "=== テスト完了 ==="
