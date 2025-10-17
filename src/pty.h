#ifndef PTY_H
#define PTY_H

#include <stdbool.h>
#include <sys/types.h>

/* PTY状態 */
typedef struct {
    int master_fd;      /* PTYマスタのファイルディスクリプタ */
    int slave_fd;       /* PTYスレーブのファイルディスクリプタ */
    pid_t child_pid;    /* 子プロセス（シェル）のPID */
    bool child_running; /* 子プロセスが実行中かどうか */
} PtyState;

/* グローバルPTY状態 */
extern PtyState g_pty;

/* 関数プロトタイプ */

/**
 * PTYを初期化してシェルを起動する
 * @param rows ターミナルの行数
 * @param cols ターミナルの列数
 * @return 成功時0、失敗時-1
 */
int pty_init(int rows, int cols);

/**
 * PTYをクリーンアップする
 */
void pty_cleanup(void);

/**
 * PTYマスタからデータを読み取る（ノンブロッキング）
 * @param buffer データを格納するバッファ
 * @param size バッファサイズ
 * @return 読み取ったバイト数、エラー時-1
 */
ssize_t pty_read(char *buffer, size_t size);

/**
 * PTYマスタにデータを書き込む
 * @param data 書き込むデータ
 * @param size データサイズ
 * @return 書き込んだバイト数、エラー時-1
 */
ssize_t pty_write(const char *data, size_t size);

/**
 * PTYのウィンドウサイズを変更する
 * @param rows 新しい行数
 * @param cols 新しい列数
 * @return 成功時0、失敗時-1
 */
int pty_resize(int rows, int cols);

/**
 * 子プロセスが実行中かチェックする
 * @return 実行中ならtrue、それ以外false
 */
bool pty_is_child_running(void);

#endif /* PTY_H */
