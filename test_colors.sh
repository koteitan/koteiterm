#!/bin/bash
# 色設定オプションのテストスクリプト

echo "koteiterm 色設定オプションのテスト"
echo "====================================="
echo

echo "1. ヘルプメッセージの確認:"
./koteiterm --help | grep -A 10 "色設定:"
echo

echo "2. 色名のテスト:"
echo "   ./koteiterm -fg green -bg black -cr red"
echo "   (色のパースに成功すればプログラムが起動します)"
echo

echo "3. #RGB形式のテスト:"
echo "   ./koteiterm -fg \"#0f0\" -bg \"#000\" -cr \"#f00\""
echo

echo "4. #RRGGBB形式のテスト:"
echo "   ./koteiterm -fg \"#00ff00\" -bg \"#000000\" -cr \"#ff0000\""
echo

echo "5. rgb:形式のテスト:"
echo "   ./koteiterm -fg \"rgb:00/ff/00\" -bg \"rgb:00/00/00\" -cr \"rgb:ff/00/00\""
echo

echo "6. 選択範囲の色設定テスト:"
echo "   ./koteiterm -selbg blue -selfg white"
echo

echo "7. 組み合わせテスト:"
echo "   ./koteiterm -fg cyan -bg \"#1a1a1a\" -cr orange -selbg \"#404040\" -selfg yellow"
echo

echo "テストを実行するには、上記のコマンドを手動で実行してください。"
echo "プログラムが正常に起動すれば、色のパースは成功しています。"
