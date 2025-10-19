/*
 * koteiterm - Linux Terminal Emulator
 * メインエントリポイント
 */

#include "koteiterm.h"
#include "display.h"
#include "terminal.h"
#include "pty.h"
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <X11/Xlib.h>

/* グローバルターミナル状態 */
TerminalState g_term = {
    .rows = DEFAULT_ROWS,
    .cols = DEFAULT_COLS,
    .cursor_x = 0,
    .cursor_y = 0,
    .running = true
};

/* デバッグフラグ */
bool g_debug = false;
bool g_debug_key = false;

/* Truecolor（24-bit RGB）モードフラグ */
bool g_truecolor_mode = true;

/* 色オプション設定（デフォルトはNULL = システムデフォルト） */
ColorOptions g_color_options = {
    .foreground = NULL,
    .background = NULL,
    .cursor = NULL,
    .sel_bg = NULL,
    .sel_fg = NULL,
    .underline = NULL
};

/* 表示オプション設定 */
DisplayOptions g_display_options = {
    .cursor_shape = TERM_CURSOR_BAR,
    .show_underline = false,
    .cursor_image_path = NULL,
    .cursor_offset_x = 0,
    .cursor_offset_y = 0,
    .cursor_scale = 1.0
};

/* シグナルハンドラ */
static void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        fprintf(stderr, "終了シグナルを受信しました (signal=%d)\n", sig);
        g_term.running = false;
    }
}

/* 初期化処理 */
static int init(void)
{
    /* シグナルハンドラの設定 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (g_debug) {
        printf("koteiterm v%s を初期化しています...\n", KOTEITERM_VERSION);
    }

    /* ディスプレイの初期化 */
    if (display_init(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT) != 0) {
        fprintf(stderr, "ディスプレイの初期化に失敗しました\n");
        return -1;
    }

    /* フォントの初期化 */
    extern DisplayState g_display;
    if (font_init(g_display.display, g_display.screen, DEFAULT_FONT, DEFAULT_FONT_SIZE) != 0) {
        fprintf(stderr, "フォントの初期化に失敗しました\n");
        display_cleanup();
        return -1;
    }

    /* ウィンドウサイズからターミナルサイズを計算 */
    int char_width = font_get_char_width();
    int char_height = font_get_char_height();
    int actual_cols = g_display.width / char_width;
    int actual_rows = g_display.height / char_height;

    if (actual_cols <= 0 || actual_rows <= 0) {
        fprintf(stderr, "ターミナルサイズの計算に失敗しました\n");
        font_cleanup(g_display.display);
        display_cleanup();
        return -1;
    }

    /* グローバル状態を更新 */
    g_term.rows = actual_rows;
    g_term.cols = actual_cols;

    if (g_debug) {
        printf("実際のターミナルサイズ: %dx%d (文字サイズ: %dx%d)\n",
               g_term.cols, g_term.rows, char_width, char_height);
    }

    /* ターミナルバッファの初期化 */
    if (terminal_init(g_term.rows, g_term.cols) != 0) {
        fprintf(stderr, "ターミナルバッファの初期化に失敗しました\n");
        font_cleanup(g_display.display);
        display_cleanup();
        return -1;
    }

    /* PTYの初期化とシェル起動 */
    if (pty_init(g_term.rows, g_term.cols) != 0) {
        fprintf(stderr, "PTYの初期化に失敗しました\n");
        terminal_cleanup();
        font_cleanup(g_display.display);
        display_cleanup();
        return -1;
    }

    return 0;
}

/* クリーンアップ処理 */
void cleanup(void)
{
    extern DisplayState g_display;

    if (g_debug) {
        printf("koteiterm をクリーンアップしています...\n");
    }

    /* PTYのクリーンアップ */
    pty_cleanup();

    /* ターミナルバッファのクリーンアップ */
    terminal_cleanup();

    /* フォントのクリーンアップ */
    if (g_display.display) {
        font_cleanup(g_display.display);
    }

    /* ディスプレイのクリーンアップ */
    display_cleanup();
}

/* stdin事前読み取りバッファ（グローバル） */
static char g_stdin_buffer[65536];
static size_t g_stdin_buffer_len = 0;
static size_t g_stdin_buffer_pos = 0;

/**
 * stdin から MC (Media Copy) シーケンスを検出する
 * @param data データバッファ
 * @param size データサイズ
 * @param mc_start MC シーケンスの開始位置を格納（出力パラメータ）
 * @param mc_type MC シーケンスの種類を格納（1=ESC[5i, 2=ESC[4i, 3=ESC[4;0i）
 * @return MC シーケンスの長さ（0 = MC シーケンスなし）
 */
static size_t find_mc_sequence(const char *data, size_t size, size_t *mc_start, int *mc_type)
{
    extern bool g_debug;

    /* バッファ内で ESC を探す */
    for (size_t i = 0; i < size; i++) {
        if (data[i] != '\x1B') {
            continue;
        }

        /* ESC[ で始まるかチェック */
        if (i + 1 >= size || data[i + 1] != '[') {
            continue;
        }

        /* ESC[5i - スクリーンキャプチャ */
        if (i + 3 < size && data[i + 2] == '5' && data[i + 3] == 'i') {
            *mc_start = i;
            *mc_type = 1;  /* ESC[5i */
            return 4;
        }

        /* ESC[4;0i - プレーンテキスト出力 */
        if (i + 5 < size && data[i + 2] == '4' && data[i + 3] == ';' &&
            data[i + 4] == '0' && data[i + 5] == 'i') {
            *mc_start = i;
            *mc_type = 3;  /* ESC[4;0i */
            return 6;
        }

        /* ESC[4i - ANSIエスケープ付き出力 */
        if (i + 3 < size && data[i + 2] == '4' && data[i + 3] == 'i') {
            *mc_start = i;
            *mc_type = 2;  /* ESC[4i */
            return 4;
        }
    }

    return 0;
}

/* メインループ */
static void main_loop(bool stdin_enabled)
{
    char buffer[4096];

    if (g_debug) {
        printf("メインループを開始します（ウィンドウを閉じるかCtrl+Cで終了）\n");
    }

    /* X11とPTYのファイルディスクリプタを取得 */
    extern DisplayState g_display;
    extern PtyState g_pty;
    int x11_fd = ConnectionNumber(g_display.display);
    int pty_fd = g_pty.master_fd;

    /* イベントループ */
    while (g_term.running) {
        fd_set readfds;
        struct timeval tv;
        int max_fd = 0;

        FD_ZERO(&readfds);

        /* X11のfdを監視 */
        FD_SET(x11_fd, &readfds);
        if (x11_fd > max_fd) max_fd = x11_fd;

        /* PTYのfdを監視 */
        FD_SET(pty_fd, &readfds);
        if (pty_fd > max_fd) max_fd = pty_fd;

        /* stdinのfdを監視（stdin入力モードの場合） */
        if (stdin_enabled) {
            FD_SET(STDIN_FILENO, &readfds);
            if (STDIN_FILENO > max_fd) max_fd = STDIN_FILENO;
        }

        /* タイムアウト設定（約60 FPS） */
        tv.tv_sec = 0;
        tv.tv_usec = 16666;

        int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;  /* シグナル割り込み、継続 */
            }
            perror("select");
            break;
        }

        /* X11イベントを処理 */
        if (FD_ISSET(x11_fd, &readfds) || XPending(g_display.display) > 0) {
            if (!display_handle_events()) {
                /* ウィンドウが閉じられた */
                g_term.running = false;
                break;
            }
        }

        /* PTYからデータを読み取る */
        if (FD_ISSET(pty_fd, &readfds)) {
            ssize_t n = pty_read(buffer, sizeof(buffer) - 1);
            if (n > 0) {
                /* ターミナルバッファに書き込む */
                terminal_write(buffer, n);
            }
        }

        /* stdinからデータを読み取る */
        if (stdin_enabled && FD_ISSET(STDIN_FILENO, &readfds)) {
            /* バッファに空きがあれば読み取る */
            size_t available = sizeof(g_stdin_buffer) - g_stdin_buffer_len;
            if (available > 0) {
                ssize_t n = read(STDIN_FILENO, g_stdin_buffer + g_stdin_buffer_len, available);
                if (n > 0) {
                    g_stdin_buffer_len += n;
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: stdinから%zd バイト読み取り（バッファ合計: %zu）\n",
                                n, g_stdin_buffer_len);
                    }
                } else if (n == 0) {
                    /* EOF検出、stdin入力終了 */
                    stdin_enabled = false;
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: stdinがEOFに達しました\n");
                    }
                }
            }
        }

        /* stdinバッファからデータを送信 */
        if (stdin_enabled && g_stdin_buffer_pos < g_stdin_buffer_len) {
            /* 少しずつ送信（一度に最大256バイト） */
            size_t chunk_size = g_stdin_buffer_len - g_stdin_buffer_pos;
            if (chunk_size > 256) chunk_size = 256;

            /* MC シーケンスをチェック（シェルに渡す前に傍受） */
            size_t mc_start = 0;
            int mc_type = 0;
            size_t mc_len = find_mc_sequence(
                g_stdin_buffer + g_stdin_buffer_pos, chunk_size, &mc_start, &mc_type);

            if (mc_len > 0) {
                /* MC シーケンスの前のデータを送信 */
                if (mc_start > 0) {
                    pty_write(g_stdin_buffer + g_stdin_buffer_pos, mc_start);

                    if (g_debug) {
                        fprintf(stderr, "DEBUG: stdinバッファから%zu バイト送信（MC前）: ",
                                mc_start);
                        for (size_t i = 0; i < mc_start && i < 40; i++) {
                            char ch = g_stdin_buffer[g_stdin_buffer_pos + i];
                            if (ch >= 32 && ch < 127) {
                                fprintf(stderr, "%c", ch);
                            } else if (ch == '\n') {
                                fprintf(stderr, "\\n");
                            } else if (ch == '\r') {
                                fprintf(stderr, "\\r");
                            } else {
                                fprintf(stderr, "<%02x>", (unsigned char)ch);
                            }
                        }
                        fprintf(stderr, "\n");
                    }

                    g_stdin_buffer_pos += mc_start;
                }

                /* ESC[5i - スクリーンキャプチャ */
                if (mc_type == 1) {
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: ESC[5i 検出、PTY残データを処理してキャプチャ\n");
                    }

                    /* PTYに読み取り可能なデータがあれば全て処理してからキャプチャ */
                    while (1) {
                        ssize_t n = pty_read(buffer, sizeof(buffer) - 1);
                        if (n > 0) {
                            terminal_write(buffer, n);
                            if (g_debug) {
                                fprintf(stderr, "DEBUG: PTYから%zd バイト読み取り\n", n);
                            }
                        } else {
                            break;  /* 読み取り可能なデータが無くなった */
                        }
                    }

                    terminal_capture_screen();
                } else if (mc_type == 2) {
                    /* ESC[4i - ANSI出力 */
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: ESC[4i 検出、ANSI出力実行\n");
                    }
                    terminal_print_screen(false);
                } else if (mc_type == 3) {
                    /* ESC[4;0i - プレーンテキスト出力 */
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: ESC[4;0i 検出、プレーンテキスト出力実行\n");
                    }
                    terminal_print_screen(true);
                }

                /* MC シーケンスをスキップ（握りつぶす） */
                g_stdin_buffer_pos += mc_len;
            } else {
                /* 通常のデータ、PTYに送信 */
                pty_write(g_stdin_buffer + g_stdin_buffer_pos, chunk_size);

                if (g_debug) {
                    fprintf(stderr, "DEBUG: stdinバッファから%zu バイト送信 (残り%zu): ",
                            chunk_size, g_stdin_buffer_len - g_stdin_buffer_pos - chunk_size);
                    for (size_t i = 0; i < chunk_size && i < 40; i++) {
                        char ch = g_stdin_buffer[g_stdin_buffer_pos + i];
                        if (ch >= 32 && ch < 127) {
                            fprintf(stderr, "%c", ch);
                        } else if (ch == '\n') {
                            fprintf(stderr, "\\n");
                        } else if (ch == '\r') {
                            fprintf(stderr, "\\r");
                        } else {
                            fprintf(stderr, "<%02x>", (unsigned char)ch);
                        }
                    }
                    fprintf(stderr, "\n");
                }

                g_stdin_buffer_pos += chunk_size;
            }

            /* バッファ内の処理済みデータをクリア */
            if (g_stdin_buffer_pos >= g_stdin_buffer_len) {
                /* 全て処理済み、バッファをリセット */
                g_stdin_buffer_pos = 0;
                g_stdin_buffer_len = 0;
                if (g_debug) {
                    fprintf(stderr, "DEBUG: stdinバッファをクリアしました\n");
                }
            } else if (g_stdin_buffer_pos > 0) {
                /* 処理済みデータを削除してバッファを前詰め */
                size_t remaining = g_stdin_buffer_len - g_stdin_buffer_pos;
                memmove(g_stdin_buffer, g_stdin_buffer + g_stdin_buffer_pos, remaining);
                g_stdin_buffer_len = remaining;
                g_stdin_buffer_pos = 0;
                if (g_debug) {
                    fprintf(stderr, "DEBUG: stdinバッファを前詰めしました（残り: %zu）\n", remaining);
                }
            }
        }

        /* 子プロセスの状態をチェック */
        if (!pty_is_child_running()) {
            if (g_debug) {
                printf("シェルが終了しました\n");
            }
            g_term.running = false;
            break;
        }

        /* GIFアニメーションカーソルを更新 */
        display_update_gif_cursor();

        /* 描画処理 */
        display_clear();
        display_render_terminal();
        display_flush();
    }
}

/* ヘルプメッセージ */
static void print_usage(const char *prog_name)
{
    printf("使い方: %s [オプション]\n", prog_name);
    printf("\nオプション:\n");
    printf("  -h, --help       このヘルプメッセージを表示\n");
    printf("  -v, --version    バージョン情報を表示\n");
    printf("      --debug      デバッグ情報を表示\n");
    printf("      --debug-key  キー入力のデバッグ情報を表示\n");
    printf("\n");
    printf("色設定:\n");
    printf("  -fg <color>      前景色を指定\n");
    printf("  -bg <color>      背景色を指定\n");
    printf("  -cr <color>      カーソル色を指定\n");
    printf("  -selbg <color>   選択背景色を指定\n");
    printf("  -selfg <color>   選択前景色を指定\n");
    printf("  -ul <color>      アンダーライン色を指定\n");
    printf("\n");
    printf("色の指定方法:\n");
    printf("  - 色名: red, blue, white, black など\n");
    printf("  - #RGB: #f00, #0f0, #00f\n");
    printf("  - #RRGGBB: #ff0000, #00ff00, #0000ff\n");
    printf("  - rgb:RR/GG/BB: rgb:ff/00/00\n");
    printf("  - rgb:RRRR/GGGG/BBBB: rgb:ffff/0000/0000\n");
    printf("\n");
    printf("表示設定:\n");
    printf("  --cursor <shape>       カーソル形状を指定\n");
    printf("    <shape> = bar        左縦線（デフォルト）\n");
    printf("    <shape> = underline  短いアンダーライン\n");
    printf("    <shape> = hollow     中抜き四角\n");
    printf("    <shape> = block      中埋め四角\n");
    printf("  --cursor \"path/to/file.png\"   (PNG/GIF対応)\n");
    printf("  --cursor \"path/to/file.gif\"   (アニメーションGIF対応)\n");
    printf("  --cursor \"path/to/file.png:x:y\"\n");
    printf("  --cursor \"path/to/file.png:x:y:scale\"\n");
    printf("    x,y  : オフセット（ピクセル、デフォルト0,0）\n");
    printf("    scale: スケール  （0.0-1.0、 デフォルト1.0）\n");
    printf("  --underline      行全体にアンダーラインを表示\n");
    printf("\n");
    printf("キーボード操作:\n");
    printf("  Shift+PageUp       上にスクロール（1画面分）\n");
    printf("  Shift+PageDown     下にスクロール（1画面分）\n");
    printf("  矢印キー           カーソル移動\n");
    printf("  Ctrl+C             割り込み\n");
    printf("  Ctrl+D             EOF（終了）\n");
    printf("\n");
    printf("マウス操作:\n");
    printf("  左ボタンドラッグ   テキスト選択\n");
    printf("  中ボタンクリック   貼り付け（PRIMARY選択）\n");
    printf("  マウスホイール上   上にスクロール（3行）\n");
    printf("  マウスホイール下   下にスクロール（3行）\n");
    printf("\n");
    printf("カラーモード:\n");
    printf("  --truecolor      24-bit RGBカラー（約1677万色）を有効化（デフォルト）\n");
    printf("  --256color       256色モードに戻す\n");
    printf("\n");
    printf("機能:\n");
    printf("  - VT100/ANSI完全互換\n");
    printf("  - 24-bit Truecolor対応（デフォルト、または--256colorで256色モード）\n");
    printf("  - UTF-8/日本語表示\n");
    printf("  - 全角文字対応（2セル幅）\n");
    printf("  - スクロールバック履歴（1000行）\n");
    printf("  - 代替スクリーンバッファ（vim、less等に対応）\n");
    printf("  - マウス選択とクリップボード連携\n");
    printf("\n");
}

/* メイン関数 */
int main(int argc, char *argv[])
{
    int ret = 0;

    /* コマンドライン引数の解析 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("koteiterm version %s\n", KOTEITERM_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--debug") == 0) {
            g_debug = true;
        } else if (strcmp(argv[i], "--debug-key") == 0) {
            g_debug_key = true;
        } else if (strcmp(argv[i], "--truecolor") == 0) {
            g_truecolor_mode = true;
        } else if (strcmp(argv[i], "--256color") == 0) {
            g_truecolor_mode = false;
        } else if (strcmp(argv[i], "-fg") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: -fg オプションには色の指定が必要です\n");
                return 1;
            }
            g_color_options.foreground = argv[++i];
        } else if (strcmp(argv[i], "-bg") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: -bg オプションには色の指定が必要です\n");
                return 1;
            }
            g_color_options.background = argv[++i];
        } else if (strcmp(argv[i], "-cr") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: -cr オプションには色の指定が必要です\n");
                return 1;
            }
            g_color_options.cursor = argv[++i];
        } else if (strcmp(argv[i], "-selbg") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: -selbg オプションには色の指定が必要です\n");
                return 1;
            }
            g_color_options.sel_bg = argv[++i];
        } else if (strcmp(argv[i], "-selfg") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: -selfg オプションには色の指定が必要です\n");
                return 1;
            }
            g_color_options.sel_fg = argv[++i];
        } else if (strcmp(argv[i], "-ul") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: -ul オプションには色の指定が必要です\n");
                return 1;
            }
            g_color_options.underline = argv[++i];
        } else if (strcmp(argv[i], "--cursor") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "エラー: --cursor オプションには形状の指定が必要です\n");
                return 1;
            }
            const char *shape = argv[++i];
            if (strcmp(shape, "underline") == 0) {
                g_display_options.cursor_shape = TERM_CURSOR_UNDERLINE;
            } else if (strcmp(shape, "bar") == 0) {
                g_display_options.cursor_shape = TERM_CURSOR_BAR;
            } else if (strcmp(shape, "hollow") == 0) {
                g_display_options.cursor_shape = TERM_CURSOR_HOLLOW_BLOCK;
            } else if (strcmp(shape, "block") == 0) {
                g_display_options.cursor_shape = TERM_CURSOR_BLOCK;
            } else if (strchr(shape, '.') != NULL) {
                /* 画像ファイルパス（拡張子を含む） */
                g_display_options.cursor_shape = TERM_CURSOR_IMAGE;

                /* パス:x:y:scaleの形式をパース */
                char *path_copy = strdup(shape);
                char *token = strtok(path_copy, ":");

                if (token) {
                    g_display_options.cursor_image_path = strdup(token);

                    /* オフセットXをパース */
                    token = strtok(NULL, ":");
                    if (token) {
                        g_display_options.cursor_offset_x = atoi(token);

                        /* オフセットYをパース */
                        token = strtok(NULL, ":");
                        if (token) {
                            g_display_options.cursor_offset_y = atoi(token);

                            /* スケールをパース */
                            token = strtok(NULL, ":");
                            if (token) {
                                g_display_options.cursor_scale = atof(token);
                                /* スケールを0.0-1.0にクランプ */
                                if (g_display_options.cursor_scale < 0.0) {
                                    g_display_options.cursor_scale = 0.0;
                                } else if (g_display_options.cursor_scale > 1.0) {
                                    g_display_options.cursor_scale = 1.0;
                                }
                            }
                        }
                    }
                }
                free(path_copy);
            } else {
                fprintf(stderr, "エラー: 不明なカーソル形状: %s\n", shape);
                fprintf(stderr, "使用可能な形状: underline, bar, hollow, block, または画像ファイルパス\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--underline") == 0) {
            g_display_options.show_underline = true;
        } else {
            fprintf(stderr, "不明なオプション: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* 環境変数を設定してシェルに伝える */
    if (g_truecolor_mode) {
        setenv("KOTEITERM_TRUECOLOR", "1", 1);
        setenv("COLORTERM", "truecolor", 1);
    }

    /* stdinが端末でない場合（パイプやファイルリダイレクト）、ノンブロッキングモードに設定 */
    bool stdin_enabled = false;
    if (!isatty(STDIN_FILENO)) {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        }
        stdin_enabled = true;

        if (g_debug) {
            fprintf(stderr, "DEBUG: stdin入力モードが有効です\n");
        }
    }

    /* 初期化 */
    if (init() != 0) {
        fprintf(stderr, "初期化に失敗しました\n");
        return 1;
    }

    /* メインループ */
    main_loop(stdin_enabled);

    /* クリーンアップ */
    cleanup();

    if (g_debug) {
        printf("koteiterm を終了しました\n");
    }
    return ret;
}
