# koteiterm

シンプルなLinuxターミナルエミュレータ

## 概要

koteiter mは、X11とXftを使用したC言語製のターミナルエミュレータです。基本的なVT100エスケープシーケンスをサポートしています。

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

## 現在サポートしている機能 (MVP)

### 基本機能
- ✅ X11ウィンドウ表示 (800x600)
- ✅ PTY作成とシェル起動
- ✅ Xftフォント描画 (monospace 12pt)
- ✅ 80x24 ターミナルバッファ
- ✅ カーソル表示（下線）

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
- ✅ 基本的な制御文字
  - \\n (改行)
  - \\r (キャリッジリターン)
  - \\b (バックスペース)
  - \\t (タブ)

## 既知の制限事項

### 未実装機能
- ❌ ANSI色（SGRシーケンス）
- ❌ 太字、下線、反転などの文字属性
- ❌ スクロールバック履歴
- ❌ マウスサポート
- ❌ ウィンドウリサイズ対応
- ❌ UTF-8完全サポート（ASCIIのみ）
- ❌ クリップボード操作
- ❌ 設定ファイル

### 制約
- ウィンドウサイズ固定 (800x600)
- ターミナルサイズ固定 (80x24)
- フォント固定 (monospace 12pt)
- 色固定 (白文字・黒背景)

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

**フェーズ1: MVP** (完了予定)
- [x] 1.1: プロジェクト基盤構築
- [x] 1.2: X11ウィンドウ作成
- [x] 1.3: PTY作成とシェル起動
- [x] 1.4: 基本的なテキスト描画
- [x] 1.5: PTY出力を画面に表示
- [x] 1.6: キーボード入力の基本実装
- [x] 1.7: VT100基本エスケープシーケンス
- [x] 1.8: MVP統合テストと修正

進捗: 8/8 完了 (100%)

## ライセンス

(未定)

## 作者

koteitan
