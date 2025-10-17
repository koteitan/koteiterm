/*
 * koteiterm - Linux Terminal Emulator
 * メインエントリポイント
 */

#include "koteiterm.h"
#include <signal.h>
#include <unistd.h>

/* グローバルターミナル状態 */
TerminalState g_term = {
    .rows = DEFAULT_ROWS,
    .cols = DEFAULT_COLS,
    .cursor_x = 0,
    .cursor_y = 0,
    .running = true
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

    printf("koteiterm v%s を初期化しています...\n", KOTEITERM_VERSION);
    printf("ターミナルサイズ: %dx%d\n", g_term.cols, g_term.rows);

    /* ディスプレイの初期化 */
    if (display_init(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT) != 0) {
        fprintf(stderr, "ディスプレイの初期化に失敗しました\n");
        return -1;
    }

    /* PTYの初期化とシェル起動 */
    if (pty_init(g_term.rows, g_term.cols) != 0) {
        fprintf(stderr, "PTYの初期化に失敗しました\n");
        display_cleanup();
        return -1;
    }

    return 0;
}

/* クリーンアップ処理 */
void cleanup(void)
{
    printf("koteiterm をクリーンアップしています...\n");

    /* PTYのクリーンアップ */
    pty_cleanup();

    /* ディスプレイのクリーンアップ */
    display_cleanup();

    /* 将来: terminal_cleanup() */
}

/* メインループ */
static void main_loop(void)
{
    char buffer[4096];

    printf("メインループを開始します（ウィンドウを閉じるかCtrl+Cで終了）\n");

    /* イベントループ */
    while (g_term.running) {
        /* X11イベントを処理 */
        if (!display_handle_events()) {
            /* ウィンドウが閉じられた */
            g_term.running = false;
            break;
        }

        /* PTYからデータを読み取る */
        ssize_t n = pty_read(buffer, sizeof(buffer) - 1);
        if (n > 0) {
            /* デバッグ: 標準出力に表示 */
            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }

        /* 子プロセスの状態をチェック */
        if (!pty_is_child_running()) {
            printf("シェルが終了しました\n");
            g_term.running = false;
            break;
        }

        /* 描画処理（将来実装） */
        /* 現時点では何も描画しない */

        /* CPUを使いすぎないように少し待機 */
        usleep(16666);  /* 約60 FPS */
    }
}

/* ヘルプメッセージ */
static void print_usage(const char *prog_name)
{
    printf("使い方: %s [オプション]\n", prog_name);
    printf("\nオプション:\n");
    printf("  -h, --help     このヘルプメッセージを表示\n");
    printf("  -v, --version  バージョン情報を表示\n");
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
        } else {
            fprintf(stderr, "不明なオプション: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* 初期化 */
    if (init() != 0) {
        fprintf(stderr, "初期化に失敗しました\n");
        return 1;
    }

    /* メインループ */
    main_loop();

    /* クリーンアップ */
    cleanup();

    printf("koteiterm を終了しました\n");
    return ret;
}
