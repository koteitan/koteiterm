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

    return 0;
}

/* クリーンアップ処理 */
void cleanup(void)
{
    printf("koteiterm をクリーンアップしています...\n");

    /* 将来: display_cleanup(), pty_cleanup(), terminal_cleanup() */
}

/* メインループ */
static void main_loop(void)
{
    printf("メインループを開始します（Ctrl+Cで終了）\n");

    /* 現時点では何もせずに待機 */
    while (g_term.running) {
        sleep(1);
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
