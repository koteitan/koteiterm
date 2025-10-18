/*
 * koteiterm - Input Module
 * キーボード入力処理
 */

#include "input.h"
#include "display.h"
#include "koteiterm.h"
#include "terminal.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>

/**
 * キーイベントを処理する
 */
bool input_handle_key(XKeyEvent *event)
{
    char buf[32];
    KeySym keysym;
    int len;
    Status status;

    /* XICがあればXmbLookupStringを使用（IME対応） */
    extern DisplayState g_display;
    extern bool g_debug_key;

    /* まずkeysymを取得（Ctrl+Shift+C/Vチェックのため） */
    keysym = XLookupKeysym(event, 0);

    /* Ctrl+Shift+Cの処理（CLIPBOARDにコピー） - 最優先でチェック */
    if ((event->state & ControlMask) && (event->state & ShiftMask) &&
        (keysym == XK_C || keysym == XK_c)) {
        extern char *selection_text;
        extern bool g_debug;
        if (g_debug) {
            fprintf(stderr, "DEBUG: Ctrl+Shift+C検出\n");
        }
        /* 選択テキストが既にある場合はCLIPBOARDも設定 */
        if (selection_text) {
            XSetSelectionOwner(g_display.display, g_display.clipboard_atom,
                              g_display.window, CurrentTime);

            /* WSLg互換性のため、clip.exeを使ってWindowsクリップボードにもコピー */
            FILE *clip = popen("clip.exe 2>/dev/null", "w");
            if (clip) {
                fwrite(selection_text, 1, strlen(selection_text), clip);
                pclose(clip);
                if (g_debug) {
                    fprintf(stderr, "DEBUG: clip.exe経由でWindowsクリップボードにコピー\n");
                }
            } else {
                /* clip.exeが使えない場合はxclipを試す */
                FILE *xclip = popen("xclip -selection clipboard 2>/dev/null", "w");
                if (xclip) {
                    fwrite(selection_text, 1, strlen(selection_text), xclip);
                    pclose(xclip);
                    if (g_debug) {
                        fprintf(stderr, "DEBUG: xclip経由でCLIPBOARDにコピー\n");
                    }
                }
            }
        }
        return true;
    }

    /* Ctrl+Shift+Vの処理（CLIPBOARDから貼り付け） - 最優先でチェック */
    if ((event->state & ControlMask) && (event->state & ShiftMask) &&
        (keysym == XK_V || keysym == XK_v)) {
        extern bool g_debug;
        if (g_debug) {
            fprintf(stderr, "DEBUG: Ctrl+Shift+V検出、CLIPBOARDから貼り付け要求 (atom=%lu)\n",
                   g_display.clipboard_atom);
        }
        XConvertSelection(g_display.display, g_display.clipboard_atom, XA_STRING,
                         g_display.clipboard_atom, g_display.window, CurrentTime);
        return true;
    }

    if (g_display.xic) {
        /* Alt+` (IME切り替えキー) を無視 */
        if ((event->state & Mod1Mask) && event->keycode == 49) {
            if (g_debug_key) {
                fprintf(stderr, "DEBUG: Alt+` を検出、IME切り替えとして処理\n");
            }
            return false;
        }

        len = XmbLookupString(g_display.xic, event, buf, sizeof(buf) - 1, &keysym, &status);

        if (g_debug_key) {
            fprintf(stderr, "DEBUG: XmbLookupString - status=%d, len=%d, keysym=0x%lx\n",
                    status, len, (unsigned long)keysym);
        }

        if (status == XBufferOverflow) {
            /* バッファが小さすぎる場合（通常は発生しない） */
            fprintf(stderr, "警告: IME入力バッファオーバーフロー\n");
            return false;
        }

        if (status == XLookupChars || status == XLookupBoth) {
            /* 文字が入力された */
            if (len > 0) {
                buf[len] = '\0';
                if (g_debug_key) {
                    fprintf(stderr, "DEBUG: IME入力: %d bytes\n", len);
                }
                pty_write(buf, len);
                return true;
            }
        }

        /* XLookupKeySym の場合は下の特殊キー処理へ */
    } else {
        /* XICがない場合は従来のXLookupStringを使用 */
        len = XLookupString(event, buf, sizeof(buf) - 1, &keysym, NULL);

        if (len > 0) {
            /* 通常の文字入力 */
            buf[len] = '\0';
            pty_write(buf, len);
            return true;
        }
    }

    /* 特殊キーの処理 */
    switch (keysym) {
        case XK_Return:
        case XK_KP_Enter:
            pty_write("\r", 1);
            return true;

        case XK_BackSpace:
            pty_write("\x7F", 1);  /* DEL */
            return true;

        case XK_Tab:
        case XK_KP_Tab:
            pty_write("\t", 1);
            return true;

        case XK_Escape:
            pty_write("\x1B", 1);  /* ESC */
            return true;

        case XK_Up:
        case XK_KP_Up:
            pty_write("\x1B[A", 3);  /* カーソル上 */
            return true;

        case XK_Down:
        case XK_KP_Down:
            pty_write("\x1B[B", 3);  /* カーソル下 */
            return true;

        case XK_Right:
        case XK_KP_Right:
            pty_write("\x1B[C", 3);  /* カーソル右 */
            return true;

        case XK_Left:
        case XK_KP_Left:
            pty_write("\x1B[D", 3);  /* カーソル左 */
            return true;

        case XK_Home:
        case XK_KP_Home:
            pty_write("\x1B[H", 3);  /* Home */
            return true;

        case XK_End:
        case XK_KP_End:
            pty_write("\x1B[F", 3);  /* End */
            return true;

        case XK_Page_Up:
        case XK_KP_Page_Up:
            /* Shift+PageUp: スクロールアップ */
            if (event->state & ShiftMask) {
                extern TerminalBuffer g_terminal;
                terminal_scroll_by(g_terminal.rows);
                return true;
            }
            pty_write("\x1B[5~", 4);  /* Page Up */
            return true;

        case XK_Page_Down:
        case XK_KP_Page_Down:
            /* Shift+PageDown: スクロールダウン */
            if (event->state & ShiftMask) {
                extern TerminalBuffer g_terminal;
                terminal_scroll_by(-g_terminal.rows);
                return true;
            }
            pty_write("\x1B[6~", 4);  /* Page Down */
            return true;

        case XK_Insert:
        case XK_KP_Insert:
            pty_write("\x1B[2~", 4);  /* Insert */
            return true;

        case XK_Delete:
        case XK_KP_Delete:
            pty_write("\x1B[3~", 4);  /* Delete */
            return true;

        default:
            /* その他のキーは無視 */
            return false;
    }
}
