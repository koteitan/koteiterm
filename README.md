# koteiterm

シンプルなLinuxターミナルエミュレータ

![koteiterm demo](image/preview.gif)

## 概要

koteitermは、X11とXftを使用したC言語製のターミナルエミュレータです。VT100エスケープシーケンス、ANSI 16色、UTF-8/日本語表示、Nerd Fontsアイコンをサポートしています。

## ビルド

```bash
sudo apt-get install \
  libx11-dev libxft-dev libfontconfig1-dev \
  libfreetype6-dev libimlib2-dev libgif-dev
make
./koteiterm
```
## マウス操作

 |操作|機能|
  |---|---|
  |左ボタンドラッグ|テキスト選択|
  |中ボタンクリック|貼り付け（PRIMARYクリップボード）|
  |マウスホイール|スクロール|
- 選択されたテキストは自動的にPRIMARYクリップボードにコピーされます

## 起動オプション

```bash
./koteiterm --help           # ヘルプを表示
./koteiterm --version        # バージョン情報を表示

# 色の指定
./koteiterm -fg <color>      # 前景色を指定
./koteiterm -bg <color>      # 背景色を指定
./koteiterm -cr <color>      # カーソル色を指定
./koteiterm -selbg <color>   # 選択背景色を指定
./koteiterm -selfg <color>   # 選択前景色を指定

# カーソル形状の指定
./koteiterm --cursor bar        # 左縦線（デフォルト）
./koteiterm --cursor underline  # 短いアンダーライン
./koteiterm --cursor hollow     # 中抜き四角
./koteiterm --cursor block      # 中埋め四角

# 画像(透過PNG/透過アニメGIF, 左下が基準点です)
./koteiterm --cursor "path/to/image.png" 
./koteiterm --cursor "path/to/image.png:2:4"     # オフセットx:y [pixels]
./koteiterm --cursor "path/to/image.png:2:4:0.5" # スケール [倍]

# 256色モードの切り替え
./koteiterm --256color  # 256色モード(デフォルトはtruecolor)
./koteiterm --debug     # stdoutにデバッグ情報を表示
./koteiterm --debug-key # stdoujにキー入力のデバッグ情報を表示
```

### 色の指定方法

 | 指定方法|例|
  |---|---|
  |色名|`red`, `blue`, `white`, `black` など|
  |#RGB形式|`"#f00"` (赤), `"#0f0"` (緑), `"#00f"` (青)|
  |#RRGGBB形式|`"#ff0000"` (赤), `"#00ff00"` (緑), `"#0000ff"` (青)|
  |rgb:RR/GG/BB形式|`rgb:ff/00/00` (赤)|
  |rgb:RRRR/GGGG/BBBB形式|`rgb:ffff/0000/0000` (赤)|

---

## 現在サポートしている機能

### テストされたOS
- ✅ Ubuntu 22.04 LTS (WSL2)
- ✅ OpenBSD

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
- ✅ 高度な機能
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
