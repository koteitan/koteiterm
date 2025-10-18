# 実装仕様

## プロジェクト構成

```
koteiterm/
├── src/
│   ├── main.c          # メインエントリポイント
│   ├── display.c/h     # X11ウィンドウ管理
│   ├── pty.c/h         # PTYとシェル管理
│   ├── font.c/h        # フォント描画
│   ├── terminal.c/h    # ターミナルバッファとVT100パーサー
│   ├── input.c/h       # キーボード入力処理
│   └── color.c/h       # 色パース処理
├── include/
│   └── koteiterm.h     # 共通ヘッダー
├── winclip/
│   └── winclip.c       # Windowsクリップボードヘルパー
├── Makefile
├── README.md
├── spec.md             # 仕様書（日本語）
├── implement.md        # 実装計画（日本語）
└── current.md          # 現在の作業状況（日本語）
```

## 関数一覧

### main.c - メインエントリポイント
- `main(argc, argv)` - メインエントリポイント
- `init()` - 初期化処理
- `cleanup()` - クリーンアップ処理
- `main_loop()` - メインイベントループ（select()ベース、stdin/X11/PTY監視）
- `signal_handler(sig)` - シグナルハンドラ
- `print_usage(prog_name)` - ヘルプメッセージ表示

### display.c - X11ウィンドウ管理とレンダリング
- `display_init(width, height)` - X11ディスプレイ初期化
- `display_cleanup()` - ディスプレイクリーンアップ
- `display_handle_events()` - X11イベント処理
- `display_clear()` - 画面クリア
- `display_flush()` - 画面更新
- `display_render_terminal()` - ターミナル描画
- `display_update_gif_cursor()` - GIFアニメーションカーソル更新
- `color_256_to_rgb(idx, r, g, b)` - 256色インデックスをRGBに変換（内部）
- `get_color(idx)` - 256色パレットから色取得（内部）
- `get_rgb_color(rgb, xft_color)` - 24-bit RGBからXftColor作成（内部）
- `parse_and_alloc_color(color_str, ...)` - 色文字列パースと割り当て（内部）
- `utf8_encode(codepoint, utf8)` - UTF-8エンコード（内部）
- `load_gif_animation(path)` - GIFアニメーション読み込み（内部）

### pty.c - 疑似端末管理
- `pty_init(rows, cols)` - PTY初期化とシェル起動
- `pty_cleanup()` - PTYクリーンアップ
- `pty_read(buffer, size)` - PTYからデータ読み取り
- `pty_write(data, size)` - PTYへデータ書き込み
- `pty_resize(rows, cols)` - PTYウィンドウサイズ変更
- `pty_is_child_running()` - 子プロセス実行中チェック

### terminal.c - ターミナルバッファとVT100パーサー
- `terminal_init(rows, cols)` - ターミナルバッファ初期化
- `terminal_cleanup()` - ターミナルバッファクリーンアップ
- `terminal_get_cell(x, y)` - セル取得
- `terminal_put_char(x, y, ch, attr)` - セルに文字書き込み
- `terminal_clear()` - 画面クリア
- `terminal_set_cursor(x, y)` - カーソル位置設定
- `terminal_get_cursor(x, y)` - カーソル位置取得
- `terminal_scroll_up()` - 1行上にスクロール
- `terminal_resize(new_rows, new_cols)` - バッファリサイズ
- `terminal_carriage_return()` - キャリッジリターン処理
- `terminal_newline()` - 改行処理
- `terminal_put_char_at_cursor(ch)` - カーソル位置に文字書き込み
- `terminal_write(data, size)` - バイト列処理（VT100パーサー）
- `terminal_scroll_by(lines)` - 指定行数スクロール
- `terminal_set_scroll_offset(offset)` - スクロールオフセット設定
- `terminal_get_scroll_offset()` - スクロールオフセット取得
- `terminal_scroll_to_bottom()` - 最下部までスクロール
- `terminal_get_scrollback_line(line_index)` - スクロールバック行取得
- `terminal_selection_start(x, y)` - 選択開始
- `terminal_selection_update(x, y)` - 選択更新
- `terminal_selection_end()` - 選択終了
- `terminal_selection_clear()` - 選択クリア
- `terminal_is_selected(x, y)` - 選択範囲判定
- `terminal_get_selected_text()` - 選択テキスト取得
- `terminal_capture_screen()` - 画面スクリーンショットをキャプチャ (ESC[5i)
- `terminal_print_screen(plain_text)` - スクリーンショットを出力 (ESC[4i)
- `utf8_decode(data, size, codepoint)` - UTF-8デコード（内部）
- `get_char_width(ch)` - 文字幅取得（内部）
- `parse_csi_params(param_buf, params, ...)` - CSIパラメータパース（内部）
- `handle_csi_command(cmd, param_buf)` - CSIコマンド処理（内部）

### input.c - キーボード入力処理
- `input_handle_key(event)` - キーイベント処理

### font.c - フォント管理
- `font_init(display, screen, font_name, font_size)` - フォント初期化
- `font_cleanup(display)` - フォントクリーンアップ
- `font_get_char_width()` - 文字幅取得
- `font_get_char_height()` - 文字高さ取得

### color.c - 色パース処理
- `color_parse(color_str, rgb)` - 色文字列パース
- `parse_named_color(name, rgb)` - 色名パース（内部）
- `parse_hex_color(str, rgb)` - #RGB/#RRGGBB形式パース（内部）
- `parse_rgb_color(str, rgb)` - rgb:形式パース（内部）
- `hex_to_int(c)` - 16進数文字を数値変換（内部）
- `scale_8_to_16(val)` - 8bit→16bit変換（内部）

### winclip/winclip.c - Windowsクリップボードヘルパー
- `main(argc, argv)` - クリップボード操作（get/set）

## データフロー

### 起動フロー
```
main()
  → init()
    → display_init() (X11初期化)
    → font_init() (フォント読み込み)
    → terminal_init() (バッファ確保)
    → pty_init() (シェル起動)
  → main_loop()
```

### 入力処理フロー

**キーボード入力（X11経由）:**
```
キーボード入力
  → XEvent (KeyPress)
    → display_handle_events()
      → input_handle_key()
        → pty_write()
          → write(master_fd)
            → シェル (stdin)
```

**stdin入力（パイプ/リダイレクト経由）:**
```
stdin（パイプまたはファイルリダイレクト）
  → isatty(STDIN_FILENO) == false を検出
  → main_loop()内のselect()で監視
    → read(STDIN_FILENO)
      → pty_write()
        → write(master_fd)
          → シェル (stdin)
```

### 出力処理フロー
```
シェル (stdout/stderr)
  → read(master_fd)
    → pty_read()
      → main_loop()
        → terminal_write()
          ├── VT100パーサー (状態機械)
          ├── handle_csi_command() (CSIシーケンス処理)
          └── terminal_put_char_at_cursor() (文字描画)
```

### 描画フロー
```
main_loop()
  → display_clear() (XClearWindow)
  → display_render_terminal()
    ├── パス1: 背景描画 (XFillRectangle)
    ├── パス2: 文字描画 (XftDrawStringUtf8)
    └── カーソル描画 (XFillRectangle / XCopyArea)
  → display_flush() (XFlush)
```

### クリップボード連携フロー (WSL)
```
マウス選択 (Button1ドラッグ)
  → terminal_selection_update()
  → ButtonRelease
    → terminal_get_selected_text()
    → XSetSelectionOwner() (PRIMARY/CLIPBOARD)
    → popen("winclip.exe set") (WSL環境)

中ボタンクリック (Button2)
  → popen("winclip.exe get") (WSL環境)
  → CRLF → LF 変換
  → pty_write()
```

### クリップボード連携フロー (ネイティブLinux)
```
マウス選択 (Button1ドラッグ)
  → terminal_selection_update()
  → ButtonRelease
    → terminal_get_selected_text()
    → XSetSelectionOwner() (PRIMARY/CLIPBOARD)

中ボタンクリック (Button2)
  → XConvertSelection() (CLIPBOARD)
  → SelectionNotify
    → XGetWindowProperty()
    → pty_write()
```

### Media Copy (スクリーンショット) フロー
```
シェルまたはstdinから ESC[5i
  → terminal_write()
    → handle_csi_command('i', "5")
      → terminal_capture_screen()
        ├── 現在の画面バッファをコピー（rows * cols）
        ├── screenshot.cells にメモリ確保
        └── screenshot.captured = true

シェルまたはstdinから ESC[4i または ESC[4;0i
  → terminal_write()
    → handle_csi_command('i', "4" or "4;0")
      → terminal_print_screen(plain_text)
        ├── screenshot.captured チェック
        ├── plain_text == false の場合：
        │   └── ANSIエスケープ付きで stdout に出力
        │       （色・太字・下線などの属性を SGR で再現）
        └── plain_text == true の場合：
            └── プレーンテキストで stdout に出力
                （文字のみ、色・属性なし）
```
