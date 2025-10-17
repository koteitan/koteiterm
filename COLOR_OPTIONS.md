# Color Options Implementation

koteitermにxterm互換の色設定オプションを実装しました。

## 実装した機能

### 1. 色パース機能 (color.c/h)

以下の色指定形式をサポート:

- **色名**: `red`, `blue`, `white`, `black`, `green`, `yellow`, `cyan`, `magenta`, `orange`, `purple`, `pink`, `brown`, `gray`, `darkgray`, `lightgray`
- **#RGB形式**: `#f00`, `#0f0`, `#00f` (4ビット/チャンネル)
- **#RRGGBB形式**: `#ff0000`, `#00ff00`, `#0000ff` (8ビット/チャンネル)
- **rgb:RR/GG/BB形式**: `rgb:ff/00/00` (8ビット/チャンネル)
- **rgb:RRRR/GGGG/BBBB形式**: `rgb:ffff/0000/0000` (16ビット/チャンネル)

### 2. コマンドラインオプション

- `-fg <color>` - 前景色（テキストの色）
- `-bg <color>` - 背景色
- `-cr <color>` - カーソル色
- `-selbg <color>` - 選択範囲の背景色
- `-selfg <color>` - 選択範囲の前景色

### 3. デフォルト色

オプション指定がない場合のデフォルト:

- 前景色: 白 (`#ffffff`)
- 背景色: 黒 (`#000000`)
- カーソル色: 白 (`#ffffff`)
- 選択背景色: グレー (`#5c5c5c`)
- 選択前景色: 白 (`#ffffff`)

## 使用例

### 基本的な使用

```bash
# 緑の文字に黒背景
./koteiterm -fg green -bg black

# 赤いカーソル
./koteiterm -cr red

# 青い選択範囲（白い文字）
./koteiterm -selbg blue -selfg white
```

### 色コード指定

```bash
# #RRGGBB形式
./koteiterm -fg "#00ff00" -bg "#000000"

# #RGB形式（短縮形）
./koteiterm -fg "#0f0" -bg "#000"

# rgb:形式（16ビット精度）
./koteiterm -fg "rgb:0000/ffff/0000" -bg "rgb:0000/0000/0000"
```

### 組み合わせ

```bash
# ダークテーマ風
./koteiterm -fg cyan -bg "#1a1a1a" -cr orange -selbg "#404040" -selfg yellow

# ライトテーマ風
./koteiterm -fg black -bg white -cr blue -selbg "#aaaaff" -selfg black

# クラシックなターミナル風
./koteiterm -fg green -bg black -cr green -selbg green -selfg black
```

## 技術詳細

### ファイル構成

- `src/color.h` - 色パース関数のインターフェース定義
- `src/color.c` - 色パース実装
- `src/display.h` - DisplayStateに色フィールド追加
- `src/display.c` - 色の初期化と描画での使用
- `include/koteiterm.h` - ColorOptions構造体とグローバル変数宣言
- `src/main.c` - コマンドライン引数パース

### 色の内部表現

- パース結果: `ColorRGB` 構造体（16ビット/チャンネル: 0x0000-0xFFFF）
- X11描画: `XRenderColor`（16ビット/チャンネル）→ `XftColor`に変換

### エラーハンドリング

- 無効な色指定の場合は警告を表示してデフォルト色を使用
- 色パースに失敗しても起動は継続

## テスト

```bash
# ヘルプメッセージで色オプションを確認
./koteiterm --help

# テストガイドを実行
./test_colors.sh

# 実際にターミナルを起動してテスト（X11環境が必要）
./koteiterm -fg green -bg black -cr red
```

## 制限事項

- TrueColor (24ビットRGB) のエスケープシーケンスは未対応
  - 内部的には16ビット精度で色を管理
  - ANSI 256色パレットは完全対応
- 設定ファイルによる色設定は未実装（将来の機能）

## 参考

xtermの色オプションと互換性があります:

- [xterm man page](https://invisible-island.net/xterm/manpage/xterm.html)
- X11カラーデータベース (`/usr/share/X11/rgb.txt`)
