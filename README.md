# koteiterm

シンプルな Linux ターミナルエミュレータ

![koteiterm demo](image/preview.gif)

## 概要

koteiterm は、X11 と Xft を使用した C 言語製のターミナルエミュレータです。VT100 エスケープシーケンス、ANSI 16 色、UTF-8/日本語表示、Nerd Fonts アイコンをサポートしています。

## ビルド

### 基本的な依存関係

```bash
sudo apt-get install \
  libx11-dev libxft-dev libfontconfig1-dev \
  libfreetype6-dev libimlib2-dev libgif-dev
```

### WSL/Windows 環境でのクリップボード連携

WSL 環境で Windows クリップボードと連携する場合、MinGW クロスコンパイラが必要です：

```bash
sudo apt-get install mingw-w64
```

Makefile が自動的に WSL 環境を検出し、winclip.exe をビルドします。

### ビルドと実行

```bash
make
./koteiterm
```
## マウス操作

 |操作|機能|
  |---|---|
  |左ボタンドラッグ|テキスト選択|
  |中ボタンクリック|貼り付け（クリップボード）|
  |マウスホイール|スクロール|

### クリップボード動作
- **ネイティブ Linux 環境**: X11 の PRIMARY/CLIPBOARD 選択を使用（標準的な Linux 動作）
- **WSL 環境**: Windows クリップボードと X11 クリップボードの両方に対応
  - 選択したテキストが Windows クリップボードにも自動コピーされます
  - 中ボタンクリックで Windows クリップボードから貼り付け可能

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
./koteiterm --debug     # stdout にデバッグ情報を表示
./koteiterm --debug-key # stdout にキー入力のデバッグ情報を表示
```

## stdin 入力と Media Copy 機能

koteiterm は、stdin からパイプやファイルリダイレクト経由でキー入力を受け取ることができます。
また、Media Copy (スクリーンショット) のエスケープシーケンスを送ることによって、画面内容をテキストとして取得できます。

これを使って、LLM に任意のコマンドを実行させ、人が見るのと同じターミナルの画面内容を取得させるテストが可能です。

### Media Copy エスケープシーケンス
下記の VT220 の Media Copy 機能により、画面内容をテキストとして出力できます。koteiterm は、stdin と stdout（シェル出力）の**両方**から MC シーケンスを処理します。
- `\x1b[5i` - 現在の画面内容をキャプチャ
- `\x1b[4i` - キャプチャした内容を stdout に出力(エスケープシーケンス付き)
- `\x1b[4;0i` - キャプチャした内容を stdout に出力(plain text)

### テストの例

ls コマンドのスクリーンショット
```bash
(
  echo "ls --color=always"
  sleep 1
  printf "\x1b[5i\x1b[4i"
  sleep 1
  echo "exit"
) | ./koteiterm
```

vim のウェルカム画面スクリーンショット
```bash
(
  echo "vim"
  sleep 1
  printf "\x1b[5i\x1b[4i"
  sleep 1
  echo ":q!"
  sleep 1
  echo "exit"
) | ./koteiterm
```

### データフロー

koteiterm 内部でのデータの流れを以下に示します：

```mermaid
flowchart TD
    Start([stdin]) --> StdinBuf[入力バッファ]

    StdinBuf --> MainLoop{メインループ}

    CheckMC -->|通常データ| PTYMaster[PTY マスター fd write]
    CheckMC -->|MediaCopy シーケンス| MCFunc
    PTYMaster --> PTYSlave[PTY スレーブ]
    PTYSlave --> |shell の stdin|Shell{シェル}
    
    MainLoop -->|256バイトずつ処理| CheckMC[MediaCopy シーケンス検出]
    MCFunc["MediaCopy 処理"] --> 破棄


    Shell --> |"shell の std{out,err}"| PTYSlave2[PTY スレーブ]
    PTYSlave2 --> PTYMaster2[PTY マスター fd read]

    VT100[VT100 パーサー]
    VT100 --> TermBuf[スクリーンバッファ]

    PTYMaster2 --> VT100

    TermBuf -->|"ESC[5i でコピー"| Screenshot[スクリーンショットバッファ]
    Screenshot -->|"ESC[4i で出力"| StdOut([stdout へ出力])

    TermBuf --> X11Output([X11 ウィンドウ画面表示])
    カーソル--> X11Output([X11 ウィンドウ画面表示])

```

**主要な処理フロー:**
1. **stdin → koteiterm**: パイプ入力を `g_stdin_buffer` に読み取り
2. **MC シーケンス検出**: `find_mc_sequence()` で ESC[5i/4i/4;0i を検出・分岐
3. **PTY 経由**: 通常データは `pty_write()` でシェルに送信
4. **シェル実行**: bash/zsh がコマンドを実行し、結果を出力
5. **PTY から受信**: `pty_read()` でシェルの出力を受け取り
6. **VT100 パース**: `terminal_write()` でエスケープシーケンスを解析
7. **スクリーンバッファ**: `g_terminal.cells[]` にセル情報を格納
8. **画面描画**: X11 + Xft で文字と色を描画
9. **MC 処理**: ESC[5i でキャプチャ、ESC[4i で stdout に出力

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

### VT100 エスケープシーケンス
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
  - **256 色対応**（前景色・背景色、ESC[38;5;Nm と ESC[48;5;Nm）
  - ANSI 16 色、6x6x6 RGB 色空間（216 色）、グレースケール（24 段階）
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
- ✅ スクロールバック履歴（1000 行）

### フォント
- ✅ デフォルト: HackGen Console NF（日本語 + Nerd Fonts 統合フォント）
- ✅ 日本語と Nerd Fonts アイコンを同一フォントで表示
- ✅ WSL 環境で Windows フォントにアクセス可能
- ✅ fontconfig 言語ヒント（`:lang=ja`）で日本語フォント優先

## ライセンス

MIT License

Copyright (c) 2025 koteitan

詳細は [LICENSE](LICENSE) ファイルを参照してください。

## 免責事項

本ソフトウェアは「現状のまま」提供されており、明示的または黙示的を問わず、いかなる保証も行いません。本ソフトウェアの使用により生じたいかなる損害についても、著作権者および貢献者は一切の責任を負いません。

使用者は本ソフトウェアを自己の責任において使用するものとします。
