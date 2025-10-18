#!/bin/bash
#
# フェーズ2統合テスト
# koteitermの各機能を総合的にテストする
#

set -e

echo "=========================================="
echo "koteiterm フェーズ2統合テスト"
echo "=========================================="
echo ""

# テスト結果を記録
PASSED=0
FAILED=0
TOTAL=0

# テスト結果を記録する関数
test_result() {
    TOTAL=$((TOTAL + 1))
    if [ $1 -eq 0 ]; then
        echo "✅ $2"
        PASSED=$((PASSED + 1))
    else
        echo "❌ $2"
        FAILED=$((FAILED + 1))
    fi
}

# 1. ビルドチェック
echo "1. ビルドチェック"
echo "----------------------------------------"
if make clean && make; then
    test_result 0 "ビルド成功"
else
    test_result 1 "ビルド失敗"
    exit 1
fi
echo ""

# 2. 実行ファイルの存在確認
echo "2. 実行ファイル確認"
echo "----------------------------------------"
if [ -x ./koteiterm ]; then
    test_result 0 "実行ファイルが存在"
else
    test_result 1 "実行ファイルが見つかりません"
    exit 1
fi
echo ""

# 3. ヘルプメッセージ
echo "3. ヘルプメッセージ表示"
echo "----------------------------------------"
if ./koteiterm --help > /dev/null 2>&1; then
    test_result 0 "ヘルプメッセージが表示される"
else
    test_result 1 "ヘルプメッセージが表示されない"
fi
echo ""

# 4. バージョン情報
echo "4. バージョン情報"
echo "----------------------------------------"
if ./koteiterm --version | grep -q "koteiterm version"; then
    test_result 0 "バージョン情報が表示される"
else
    test_result 1 "バージョン情報が表示されない"
fi
echo ""

# 5. 必要なコマンドの存在確認
echo "5. 必要なコマンド確認"
echo "----------------------------------------"
COMMANDS="vim less htop"
for cmd in $COMMANDS; do
    if command -v $cmd > /dev/null 2>&1; then
        test_result 0 "$cmd コマンドが利用可能"
    else
        test_result 1 "$cmd コマンドが見つかりません（テストスキップの可能性）"
    fi
done
echo ""

# 6. 256色テストスクリプト
echo "6. 256色テストスクリプト確認"
echo "----------------------------------------"
if [ -x ./test_256colors.sh ]; then
    test_result 0 "256色テストスクリプトが存在"
else
    test_result 1 "256色テストスクリプトが見つかりません"
fi
echo ""

# 7. Nerd Fontsテストスクリプト
echo "7. Nerd Fontsテストスクリプト確認"
echo "----------------------------------------"
if [ -x ./test_nerd_icons.sh ]; then
    test_result 0 "Nerd Fontsテストスクリプトが存在"
else
    test_result 1 "Nerd Fontsテストスクリプトが見つかりません"
fi
echo ""

# 8. ドキュメント確認
echo "8. ドキュメント確認"
echo "----------------------------------------"
DOCS="README.md spec.md implement.md current.md LICENSE"
for doc in $DOCS; do
    if [ -f $doc ]; then
        test_result 0 "$doc が存在"
    else
        test_result 1 "$doc が見つかりません"
    fi
done
echo ""

# 9. ソースファイル確認
echo "9. ソースファイル確認"
echo "----------------------------------------"
SRC_FILES="src/main.c src/display.c src/terminal.c src/pty.c src/font.c src/input.c"
for src in $SRC_FILES; do
    if [ -f $src ]; then
        test_result 0 "$src が存在"
    else
        test_result 1 "$src が見つかりません"
    fi
done
echo ""

# 10. ヘッダーファイル確認
echo "10. ヘッダーファイル確認"
echo "----------------------------------------"
HDR_FILES="include/koteiterm.h src/display.h src/terminal.h src/pty.h src/font.h src/input.h"
for hdr in $HDR_FILES; do
    if [ -f $hdr ]; then
        test_result 0 "$hdr が存在"
    else
        test_result 1 "$hdr が見つかりません"
    fi
done
echo ""

# テスト結果のサマリー
echo "=========================================="
echo "テスト結果サマリー"
echo "=========================================="
echo "合計: $TOTAL テスト"
echo "成功: $PASSED"
echo "失敗: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✅ 全てのテストが成功しました！"
    exit 0
else
    echo "❌ $FAILED 個のテストが失敗しました"
    exit 1
fi
