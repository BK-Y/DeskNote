#ifndef APP_H
#define APP_H

#include <windows.h>

int App_Run(void);
int App_Init(HWND hwnd);
void App_Shutdown(void);
int App_OnResize(unsigned int width, unsigned int height);
void App_OnPaint(void);
void App_OnChar(wchar_t ch);
void App_OnKeyDown(unsigned int key);
void App_OnFocusGained(void);
void App_OnFocusLost(void);
void App_OnImeStartComposition(void);

#endif
