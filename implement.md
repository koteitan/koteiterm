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
├── Makefile
├── README.md
├── spec.md             # 仕様書（日本語）
├── implement.md        # 実装計画（日本語）
└── current.md          # 現在の作業状況（日本語）
```
