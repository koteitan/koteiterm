/*
 * winclip.exe - Windows Clipboard Utility for koteiterm
 *
 * Usage:
 *   winclip.exe get    - Read clipboard as UTF-8 to stdout
 *   winclip.exe set    - Write clipboard from UTF-8 stdin
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

/* Convert UTF-8 to UTF-16 */
static WCHAR* utf8_to_utf16(const char* utf8_str, int utf8_len) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
    if (wlen == 0) return NULL;

    WCHAR* wstr = (WCHAR*)malloc((wlen + 1) * sizeof(WCHAR));
    if (!wstr) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, wstr, wlen);
    wstr[wlen] = L'\0';
    return wstr;
}

/* Convert UTF-16 to UTF-8 */
static char* utf16_to_utf8(const WCHAR* wstr) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len == 0) return NULL;

    char* str = (char*)malloc(len);
    if (!str) return NULL;

    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    return str;
}

/* Get clipboard text as UTF-8 */
static int clipboard_get(void) {
    if (!OpenClipboard(NULL)) {
        fprintf(stderr, "Error: Cannot open clipboard\n");
        return 1;
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        /* Empty clipboard is not an error */
        return 0;
    }

    WCHAR* wstr = (WCHAR*)GlobalLock(hData);
    if (!wstr) {
        CloseClipboard();
        fprintf(stderr, "Error: Cannot lock clipboard data\n");
        return 1;
    }

    char* utf8_str = utf16_to_utf8(wstr);
    GlobalUnlock(hData);
    CloseClipboard();

    if (!utf8_str) {
        fprintf(stderr, "Error: UTF-16 to UTF-8 conversion failed\n");
        return 1;
    }

    /* Output UTF-8 to stdout (binary mode to preserve encoding) */
    _setmode(_fileno(stdout), _O_BINARY);
    fputs(utf8_str, stdout);
    free(utf8_str);

    return 0;
}

/* Set clipboard text from UTF-8 */
static int clipboard_set(void) {
    /* Read all stdin data */
    char buffer[65536];
    size_t total = 0;
    size_t n;

    _setmode(_fileno(stdin), _O_BINARY);
    while ((n = fread(buffer + total, 1, sizeof(buffer) - total - 1, stdin)) > 0) {
        total += n;
        if (total >= sizeof(buffer) - 1) break;
    }
    buffer[total] = '\0';

    if (total == 0) {
        /* Empty input, clear clipboard */
        if (!OpenClipboard(NULL)) {
            fprintf(stderr, "Error: Cannot open clipboard\n");
            return 1;
        }
        EmptyClipboard();
        CloseClipboard();
        return 0;
    }

    /* Convert UTF-8 to UTF-16 */
    WCHAR* wstr = utf8_to_utf16(buffer, total);
    if (!wstr) {
        fprintf(stderr, "Error: UTF-8 to UTF-16 conversion failed\n");
        return 1;
    }

    /* Allocate global memory for clipboard */
    size_t wlen = wcslen(wstr);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wlen + 1) * sizeof(WCHAR));
    if (!hMem) {
        free(wstr);
        fprintf(stderr, "Error: Cannot allocate global memory\n");
        return 1;
    }

    WCHAR* pMem = (WCHAR*)GlobalLock(hMem);
    if (!pMem) {
        GlobalFree(hMem);
        free(wstr);
        fprintf(stderr, "Error: Cannot lock global memory\n");
        return 1;
    }

    wcscpy(pMem, wstr);
    GlobalUnlock(hMem);
    free(wstr);

    /* Set clipboard data */
    if (!OpenClipboard(NULL)) {
        GlobalFree(hMem);
        fprintf(stderr, "Error: Cannot open clipboard\n");
        return 1;
    }

    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        CloseClipboard();
        GlobalFree(hMem);
        fprintf(stderr, "Error: Cannot set clipboard data\n");
        return 1;
    }

    CloseClipboard();
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s {get|set}\n", argv[0]);
        fprintf(stderr, "  get - Read clipboard as UTF-8 to stdout\n");
        fprintf(stderr, "  set - Write clipboard from UTF-8 stdin\n");
        return 1;
    }

    if (strcmp(argv[1], "get") == 0) {
        return clipboard_get();
    } else if (strcmp(argv[1], "set") == 0) {
        return clipboard_set();
    } else {
        fprintf(stderr, "Error: Invalid command '%s'\n", argv[1]);
        fprintf(stderr, "Use 'get' or 'set'\n");
        return 1;
    }
}
