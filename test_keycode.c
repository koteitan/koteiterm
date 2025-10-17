/*
 * キーコード判別ツール
 * X11のキーイベントを表示してキーコードを確認する
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

int main(void)
{
    Display *display;
    Window window;
    XEvent event;
    int screen;

    /* X11接続 */
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "X11ディスプレイを開けませんでした\n");
        return 1;
    }

    screen = DefaultScreen(display);

    /* ウィンドウ作成 */
    window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        100, 100,  /* 位置 */
        400, 300,  /* サイズ */
        1,
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );

    /* ウィンドウタイトル */
    XStoreName(display, window, "Key Code Tester - Press any key");

    /* イベントマスク */
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask | ExposureMask);

    /* ウィンドウ表示 */
    XMapWindow(display, window);
    XFlush(display);

    printf("========================================\n");
    printf("キーコードテスター\n");
    printf("========================================\n");
    printf("ウィンドウにフォーカスして任意のキーを押してください\n");
    printf("終了するにはウィンドウを閉じるかCtrl+Cを押してください\n");
    printf("\n");

    /* イベントループ */
    while (1) {
        XNextEvent(display, &event);

        switch (event.type) {
            case KeyPress:
            case KeyRelease:
            {
                XKeyEvent *key_event = (XKeyEvent *)&event;
                KeySym keysym;
                char buffer[128];
                int len;

                /* KeySymを取得 */
                len = XLookupString(key_event, buffer, sizeof(buffer) - 1, &keysym, NULL);
                buffer[len] = '\0';

                /* KeySymの名前を取得 */
                char *keysym_name = XKeysymToString(keysym);

                /* 結果を表示 */
                printf("----------------------------------------\n");
                printf("イベント: %s\n",
                       event.type == KeyPress ? "KeyPress" : "KeyRelease");
                printf("  keycode:    0x%04x (%d)\n",
                       key_event->keycode, key_event->keycode);
                printf("  state:      0x%04x\n", key_event->state);
                printf("  KeySym:     0x%08lx (%ld)\n", keysym, keysym);
                printf("  KeySym名:   %s\n",
                       keysym_name ? keysym_name : "(unknown)");

                /* 修飾キー状態 */
                printf("  修飾キー:   ");
                if (key_event->state & ShiftMask)   printf("Shift ");
                if (key_event->state & ControlMask) printf("Ctrl ");
                if (key_event->state & Mod1Mask)    printf("Alt ");
                if (key_event->state & Mod4Mask)    printf("Super ");
                if (key_event->state == 0)          printf("(none)");
                printf("\n");

                /* 文字列 */
                if (len > 0) {
                    printf("  文字列:     \"");
                    for (int i = 0; i < len; i++) {
                        if (buffer[i] >= 0x20 && buffer[i] < 0x7F) {
                            printf("%c", buffer[i]);
                        } else {
                            printf("\\x%02x", (unsigned char)buffer[i]);
                        }
                    }
                    printf("\"\n");
                }

                /* 一般的なIMEキーの検出 */
                if (keysym == XK_Henkan || keysym == XK_Henkan_Mode) {
                    printf("  → これは「変換」キーです（IME ON）\n");
                } else if (keysym == XK_Muhenkan) {
                    printf("  → これは「無変換」キーです（IME OFF）\n");
                } else if (keysym == XK_Hiragana_Katakana || keysym == XK_Hiragana) {
                    printf("  → これは「ひらがな/カタカナ」キーです\n");
                } else if (keysym == XK_Zenkaku_Hankaku) {
                    printf("  → これは「全角/半角」キーです\n");
                } else if (keysym == XK_Eisu_toggle || keysym == XK_Eisu_Shift) {
                    printf("  → これは「英数」キーです\n");
                } else if (keysym == XK_Kanji) {
                    printf("  → これは「漢字」キーです\n");
                }

                printf("\n");
                break;
            }

            case Expose:
                /* 再描画 */
                break;
        }
    }

    /* クリーンアップ */
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
