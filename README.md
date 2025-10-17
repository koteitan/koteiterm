# koteiterm

シンプルなLinuxターミナルエミュレータ

## 概要

koteitermは、X11とXftを使用したC言語製のターミナルエミュレータです。VT100エスケープシーケンス、ANSI 16色、UTF-8/日本語表示、Nerd Fontsアイコンをサポートしています。

## ビルド

### 必要な依存関係

```bash
sudo apt-get install libx11-dev libxft-dev libfontconfig1-dev libfreetype6-dev
```

### ビルド方法

```bash
make
```

## 実行

```bash
./koteiterm
```

## 現在サポートしている機能

### 基本機能
- ✅ X11ウィンドウ表示（動的リサイズ対応）
- ✅ PTY作成とシェル起動
- ✅ Xftフォント描画（Nerd Fonts対応）
- ✅ 動的ターミナルバッファ
- ✅ カーソル表示（下線）
- ✅ スクロールバックバッファ（1000行履歴）

### キーボード入力
- ✅ ASCII文字入力
- ✅ Enter、Backspace、Tab、Escape
- ✅ 矢印キー (Up/Down/Left/Right)
- ✅ Home、End、Page Up/Down
- ✅ Insert、Delete

### VT100エスケープシーケンス
- ✅ カーソル移動
  - CUU (ESC[A): カーソルを上に移動
  - CUD (ESC[B): カーソルを下に移動
  - CUF (ESC[C): カーソルを右に移動
  - CUB (ESC[D): カーソルを左に移動
  - CUP (ESC[H): カーソル位置設定
- ✅ 画面クリア
  - ED (ESC[J): 画面の一部または全体をクリア
  - EL (ESC[K): 行の一部または全体をクリア
- ✅ SGR（色と文字属性）
  - ANSI 16色対応（前景色・背景色）
  - 太字、イタリック、下線、反転表示
- ✅ 基本的な制御文字
  - \\n (改行)
  - \\r (キャリッジリターン)
  - \\b (バックスペース)
  - \\t (タブ)

### 文字エンコーディング
- ✅ UTF-8完全対応（1-4バイト）
- ✅ 日本語表示（ひらがな、カタカナ、漢字）
- ✅ 全角文字対応（East Asian Width検出、2セル表示）
- ✅ Nerd Fontsアイコン表示（vim-airline系）

### スクロール機能
- ✅ Shift+PageUp/PageDown（1画面分スクロール）
- ✅ マウスホイール（3行ずつスクロール）
- ✅ スクロールバック履歴（1000行）

### フォント
- ✅ デフォルト: FiraCode Nerd Font Mono
- ✅ Nerd Fonts対応（Powerline記号、アイコン）
- ✅ WSL環境でWindowsフォントにアクセス可能
- ✅ fontconfig言語ヒント（`:lang=ja`）で日本語フォント優先

## 既知の制限事項

### 未実装機能
- ❌ マウス選択とクリップボード操作
- ❌ 256色/TrueColorサポート
- ❌ 設定ファイル
- ❌ タブ機能
- ❌ 分割ウィンドウ

## プロジェクト構成

```
koteiterm/
├── src/
│   ├── main.c          # メインエントリポイント
│   ├── display.c/h     # X11ウィンドウ管理
│   ├── pty.c/h         # PTYとシェル管理
│   ├── font.c/h        # フォント描画
│   ├── terminal.c/h    # ターミナルバッファとVT100パーサー
│   └── input.c/h       # キーボード入力処理
├── include/
│   └── koteiterm.h     # 共通ヘッダー
├── Makefile
├── README.md
├── spec.md             # 仕様書（日本語）
├── implement.md        # 実装計画（日本語）
└── current.md          # 現在の作業状況（日本語）
```

## 開発状況

**フェーズ1: MVP** ✅ 完了
- [x] 1.1-1.8: 基本的なターミナル機能

**フェーズ2: 実用機能** (進行中)
- [x] 2.1: ANSI 16色対応
- [x] 2.2: 特殊キー対応
- [x] 2.3: ウィンドウリサイズ対応
- [x] 2.4: スクロールバックバッファ
- [ ] 2.5: マウス選択とクリップボード ← **次**
- [ ] 2.6: 256色サポート
- [ ] 2.7: VT100完全互換性
- [ ] 2.8: フェーズ2統合テスト

進捗: 4/8 完了 (50%)

## テスト

### Nerd Fontsアイコンテスト

```bash
./test_nerd_icons.sh
```

vim-airlineで使用されるPowerline記号やファイルタイプアイコンが正しく表示されることを確認できます。

## ライセンス

(未定)

## 作者

koteitan
