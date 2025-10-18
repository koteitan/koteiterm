# テストスクリプト

このディレクトリには、koteitermの機能をテストするためのスクリプトが含まれています。

## 主要なテストスクリプト

### test-color.sh
24-bit Truecolorと256色モードの包括的なテストスクリプト。

**使用方法:**
```bash
# Truecolorモード（デフォルト）でテスト
./koteiterm
./test/test-color.sh

# 256色モードでテスト
./koteiterm --256color
./test/test-color.sh
```

**テスト内容:**
- セクション1: 256色テスト
  1. 256色パレット全色表示（8行、全256色）
  2. 基本的な前景色（6色）
  3. 基本的な背景色（3色）
  4. 前景色と背景色の組み合わせ

- セクション2: Truecolorテスト
  5. 赤→青グラデーション（前景色）
  6. 緑→赤グラデーション（背景色）
  7. 微細な色の違い（1段階ずつ変化）

### test-integration.sh
VT100エスケープシーケンスと端末機能の統合テスト。

**使用方法:**
```bash
./test/test-integration.sh
```

**テスト内容:**
- カーソル移動
- テキスト属性（太字、下線、反転など）
- 画面クリア
- スクロール領域
- 代替スクリーンバッファ

### test-256colors.sh
256色パレットの全色表示テスト。

**使用方法:**
```bash
./test/test-256colors.sh
```

### test-colors.sh
ANSI 16色の基本テスト。

**使用方法:**
```bash
./test/test-colors.sh
```

### test-nerd-icons.sh
Nerd Fontsアイコンの表示テスト。

**使用方法:**
```bash
./test/test-nerd-icons.sh
```

## その他のテストファイル

- **test-cursor-save.sh**: カーソル位置保存/復元のテスト
- **test-redraw.sh**: 画面再描画のテスト
- **test-keycode.c**: キーコードテスト用Cプログラム
- **little-test.sh**: 簡易テストスクリプト

## テストの実行

すべてのテストスクリプトは、koteitermを起動してから実行してください：

```bash
# 1. koteiteimを起動
./koteiterm

# 2. koteiteimのシェル内でテストスクリプトを実行
./test/test-color.sh
```
