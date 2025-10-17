# koteiterm

シンプルなLinuxターミナルエミュレータ

![koteiterm demo](image/preview.gif)

## 概要

koteitermは、X11とXftを使用したC言語製のターミナルエミュレータです。VT100エスケープシーケンス、ANSI 16色、UTF-8/日本語表示、Nerd Fontsアイコンをサポートしています。

## ビルド

### 必要な依存関係

```bash
sudo apt-get install libx11-dev libxft-dev libfontconfig1-dev libfreetype6-dev libimlib2-dev libgif-dev
```

### ビルド方法

```bash
make
```

## 実行

```bash
./koteiterm
```

### オプション

#### 基本オプション

```bash
./koteiterm --help       # ヘルプを表示
./koteiterm --version    # バージョン情報を表示
./koteiterm --debug-key  # キー入力のデバッグ情報を表示
```

#### 色設定オプション

```bash
./koteiterm -fg <color>      # 前景色を指定
./koteiterm -bg <color>      # 背景色を指定
./koteiterm -cr <color>      # カーソル色を指定
./koteiterm -selbg <color>   # 選択背景色を指定
./koteiterm -selfg <color>   # 選択前景色を指定
```

**色の指定方法:**
- 色名: `red`, `blue`, `white`, `black`, `green`, `yellow`, `cyan`, `magenta`, `orange`, `purple`, `pink`, `brown`, `gray` など
- #RGB形式: `#f00` (赤), `#0f0` (緑), `#00f` (青)
- #RRGGBB形式: `#ff0000` (赤), `#00ff00` (緑), `#0000ff` (青)
- rgb:RR/GG/BB形式: `rgb:ff/00/00` (赤)
- rgb:RRRR/GGGG/BBBB形式: `rgb:ffff/0000/0000` (赤)

**使用例:**

```bash
# 緑の文字に黒背景、赤いカーソル
./koteiterm -fg green -bg black -cr red

# 色コード指定
./koteiterm -fg "#00ff00" -bg "#000000" -cr "#ff0000"

# 選択範囲の色をカスタマイズ
./koteiterm -selbg blue -selfg white

# 組み合わせ
./koteiterm -fg cyan -bg "#1a1a1a" -cr orange -selbg "#404040" -selfg yellow
```

#### カーソルカスタマイズ

**カーソル形状:**

```bash
./koteiterm --cursor bar        # 左縦線（デフォルト）
./koteiterm --cursor underline  # 短いアンダーライン
./koteiterm --cursor hollow     # 中抜き四角
./koteiterm --cursor block      # 中埋め四角
```

**画像カーソル（PNG/GIF対応）:**

カーソルとして画像ファイルを使用できます：

```bash
# 基本的な使い方
./koteiterm --cursor "path/to/image.png"

# オフセット指定（x:y、ピクセル単位）
./koteiterm --cursor "sushi.png:2:4"

# オフセットとスケール指定（0.0-1.0）
./koteiterm --cursor "sushi.png:0:0:0.5"
```

**パラメータ:**
- **x, y**: カーソル画像の位置オフセット（ピクセル、デフォルト: 0, 0）
- **scale**: 画像サイズのスケール（0.0〜1.0、デフォルト: 1.0）

**使用例:**

```bash
# 寿司アイコンをカーソルとして使用
./koteiterm --cursor "sushi.png"

# スケールを50%に縮小
./koteiterm --cursor "sushi.png:0:0:0.5"
```

## 操作方法

### マウス操作

**テキスト選択:**
- 左ボタンドラッグ - テキストを選択
- 選択されたテキストは自動的にPRIMARYクリップボードにコピーされます

**貼り付け:**
- 中ボタンクリック - PRIMARYクリップボードから貼り付け

**スクロール:**
- マウスホイール - スクロール（3行）

## 現在サポートしている機能

### VT100エスケープシーケンス
- ✅ カーソル移動
  - CUU (ESC[A): カーソルを上に移動
  - CUD (ESC[B): カーソルを下に移動
  - CUF (ESC[C): カーソルを右に移動
  - CUB (ESC[D): カーソルを左に移動
  - CUP (ESC[H): カーソル位置設定
  - CNL/CPL/HPA/VPA: 追加のカーソル制御
- ✅ 画面クリア
  - ED (ESC[J): 画面の一部または全体をクリア
  - EL (ESC[K): 行の一部または全体をクリア
- ✅ SGR（色と文字属性）
  - **256色対応**（前景色・背景色、ESC[38;5;NmとESC[48;5;Nm）
  - ANSI 16色、6x6x6 RGB色空間（216色）、グレースケール（24段階）
  - 太字、イタリック、下線、反転表示
- ✅ 高度な機能（VT100完全互換）
  - **代替スクリーンバッファ** (ESC[?1049h/l, ESC[?47h/l)
  - **スクロール領域設定** (DECSTBM: ESC[r)
  - **カーソル位置保存・復元** (DECSC: ESC 7, DECRC: ESC 8)
  - **カーソル表示/非表示** (ESC[?25h/l)
  - **行の挿入・削除** (IL: ESC[L, DL: ESC[M)
  - **文字の挿入・削除** (ICH: ESC[@, DCH: ESC[P)
  - **デバイス問い合わせ** (DSR: ESC[6n)
  - **スクロールコマンド** (SU: ESC[S, SD: ESC[T)
- ✅ 基本的な制御文字
  - \\n (改行)
  - \\r (キャリッジリターン)
  - \\b (バックスペース)
  - \\t (タブ)

### 文字エンコーディング
- ✅ 全角文字対応（East Asian Width検出、2セル表示）
- ✅ Nerd Fontsアイコン表示（vim-airline系）

### スクロール機能
- ✅ スクロールバック履歴（1000行）

### フォント
- ✅ デフォルト: HackGen Console NF（日本語+Nerd Fonts統合フォント）
- ✅ 日本語とNerd Fontsアイコンを同一フォントで表示
- ✅ WSL環境でWindowsフォントにアクセス可能
- ✅ fontconfig言語ヒント（`:lang=ja`）で日本語フォント優先

## ライセンス

MIT License

Copyright (c) 2025 koteitan

詳細は [LICENSE](LICENSE) ファイルを参照してください。

## 免責事項

本ソフトウェアは「現状のまま」提供されており、明示的または黙示的を問わず、いかなる保証も行いません。本ソフトウェアの使用により生じたいかなる損害についても、著作権者および貢献者は一切の責任を負いません。

使用者は本ソフトウェアを自己の責任において使用するものとします。
