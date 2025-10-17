#ifndef INPUT_H
#define INPUT_H

#include <X11/Xlib.h>
#include <stdbool.h>

/**
 * キーイベントを処理する
 * @param event X11 KeyPressイベント
 * @return 処理された場合true
 */
bool input_handle_key(XKeyEvent *event);

#endif /* INPUT_H */
