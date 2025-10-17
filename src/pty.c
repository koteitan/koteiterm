/*
 * koteiterm - PTY Module
 * 疑似端末（PTY）の管理とシェルプロセスの制御
 */

#include "pty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pty.h>

/* グローバルPTY状態 */
PtyState g_pty = {
    .master_fd = -1,
    .slave_fd = -1,
    .child_pid = -1,
    .child_running = false
};

/**
 * PTYを初期化してシェルを起動する
 */
int pty_init(int rows, int cols)
{
    struct winsize ws;

    /* ウィンドウサイズを設定 */
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = rows;
    ws.ws_col = cols;

    /* PTYを作成 */
    if (openpty(&g_pty.master_fd, &g_pty.slave_fd, NULL, NULL, &ws) < 0) {
        fprintf(stderr, "エラー: PTYの作成に失敗しました: %s\n", strerror(errno));
        return -1;
    }

    /* マスタFDをノンブロッキングに設定 */
    int flags = fcntl(g_pty.master_fd, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "エラー: fcntl(F_GETFL)に失敗しました: %s\n", strerror(errno));
        close(g_pty.master_fd);
        close(g_pty.slave_fd);
        return -1;
    }
    if (fcntl(g_pty.master_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        fprintf(stderr, "エラー: fcntl(F_SETFL)に失敗しました: %s\n", strerror(errno));
        close(g_pty.master_fd);
        close(g_pty.slave_fd);
        return -1;
    }

    /* 子プロセスをfork */
    g_pty.child_pid = fork();
    if (g_pty.child_pid < 0) {
        fprintf(stderr, "エラー: forkに失敗しました: %s\n", strerror(errno));
        close(g_pty.master_fd);
        close(g_pty.slave_fd);
        return -1;
    }

    if (g_pty.child_pid == 0) {
        /* 子プロセス: シェルを実行 */

        /* マスタFDを閉じる */
        close(g_pty.master_fd);

        /* 新しいセッションを作成 */
        if (setsid() < 0) {
            perror("setsid");
            exit(1);
        }

        /* スレーブを制御端末として設定 */
        if (ioctl(g_pty.slave_fd, TIOCSCTTY, 0) < 0) {
            perror("ioctl(TIOCSCTTY)");
            exit(1);
        }

        /* 標準入出力をスレーブにリダイレクト */
        dup2(g_pty.slave_fd, STDIN_FILENO);
        dup2(g_pty.slave_fd, STDOUT_FILENO);
        dup2(g_pty.slave_fd, STDERR_FILENO);

        /* スレーブFDを閉じる（stdin/stdout/stderrで使用中） */
        if (g_pty.slave_fd > STDERR_FILENO) {
            close(g_pty.slave_fd);
        }

        /* シェルを起動 */
        const char *shell = getenv("SHELL");
        if (!shell) {
            shell = "/bin/bash";
        }

        /* TERM環境変数を設定 */
        setenv("TERM", "xterm-256color", 1);

        /* シェルを実行 */
        execl(shell, shell, NULL);

        /* execが失敗した場合 */
        perror("exec");
        exit(1);
    }

    /* 親プロセス */
    g_pty.child_running = true;

    /* スレーブFDを閉じる（親では不要） */
    close(g_pty.slave_fd);
    g_pty.slave_fd = -1;

    printf("PTYを初期化しました (fd=%d, pid=%d, %dx%d)\n",
           g_pty.master_fd, g_pty.child_pid, cols, rows);

    return 0;
}

/**
 * PTYをクリーンアップする
 */
void pty_cleanup(void)
{
    if (g_pty.child_running && g_pty.child_pid > 0) {
        /* 子プロセスに終了シグナルを送信 */
        printf("子プロセス (PID=%d) を終了しています...\n", g_pty.child_pid);
        kill(g_pty.child_pid, SIGTERM);

        /* 子プロセスの終了を待つ（タイムアウト付き） */
        int status;
        for (int i = 0; i < 10; i++) {
            if (waitpid(g_pty.child_pid, &status, WNOHANG) > 0) {
                break;
            }
            usleep(100000);  /* 100ms待機 */
        }

        /* まだ終了していなければ強制終了 */
        if (g_pty.child_running) {
            kill(g_pty.child_pid, SIGKILL);
            waitpid(g_pty.child_pid, &status, 0);
        }

        g_pty.child_running = false;
    }

    if (g_pty.master_fd >= 0) {
        close(g_pty.master_fd);
        g_pty.master_fd = -1;
    }

    if (g_pty.slave_fd >= 0) {
        close(g_pty.slave_fd);
        g_pty.slave_fd = -1;
    }

    g_pty.child_pid = -1;

    printf("PTYをクリーンアップしました\n");
}

/**
 * PTYマスタからデータを読み取る
 */
ssize_t pty_read(char *buffer, size_t size)
{
    if (g_pty.master_fd < 0) {
        return -1;
    }

    ssize_t n = read(g_pty.master_fd, buffer, size);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* データがない場合（ノンブロッキング） */
            return 0;
        }
        return -1;
    }

    return n;
}

/**
 * PTYマスタにデータを書き込む
 */
ssize_t pty_write(const char *data, size_t size)
{
    if (g_pty.master_fd < 0) {
        return -1;
    }

    ssize_t n = write(g_pty.master_fd, data, size);
    if (n < 0) {
        fprintf(stderr, "エラー: PTYへの書き込みに失敗しました: %s\n", strerror(errno));
        return -1;
    }

    return n;
}

/**
 * PTYのウィンドウサイズを変更する
 */
int pty_resize(int rows, int cols)
{
    if (g_pty.master_fd < 0) {
        return -1;
    }

    struct winsize ws;
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = rows;
    ws.ws_col = cols;

    if (ioctl(g_pty.master_fd, TIOCSWINSZ, &ws) < 0) {
        fprintf(stderr, "エラー: ウィンドウサイズの変更に失敗しました: %s\n", strerror(errno));
        return -1;
    }

    printf("PTYウィンドウサイズを変更しました (%dx%d)\n", cols, rows);

    return 0;
}

/**
 * 子プロセスが実行中かチェックする
 */
bool pty_is_child_running(void)
{
    if (!g_pty.child_running || g_pty.child_pid <= 0) {
        return false;
    }

    /* 子プロセスの状態をチェック（ノンブロッキング） */
    int status;
    pid_t result = waitpid(g_pty.child_pid, &status, WNOHANG);

    if (result > 0) {
        /* 子プロセスが終了した */
        printf("子プロセス (PID=%d) が終了しました ", g_pty.child_pid);
        if (WIFEXITED(status)) {
            printf("(exit status=%d)\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("(signal=%d)\n", WTERMSIG(status));
        } else {
            printf("\n");
        }
        g_pty.child_running = false;
        return false;
    } else if (result < 0) {
        /* エラー */
        if (errno != ECHILD) {
            fprintf(stderr, "エラー: waitpidに失敗しました: %s\n", strerror(errno));
        }
        g_pty.child_running = false;
        return false;
    }

    /* 子プロセスは実行中 */
    return true;
}
