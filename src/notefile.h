// notefile.h - 便签文件读写
#ifndef NOTEFILE_H
#define NOTEFILE_H

#include <windows.h>

// 初始化：读取 last_file 配置，打开对应的 .md 文件
// 返回：文件内容（需调用后复制），hwndEditor 用于显示
// 返回值：0=成功, 1=失败
int NoteFile_OpenLast(HWND hEditor);

// 保存当前编辑器内容到文件
void NoteFile_Save(HWND hEditor);

// 关闭时调用：保存 + 更新 last_file 配置
void NoteFile_Close(HWND hEditor);

// 获取当前文件名（不含路径，用于标题栏显示）
const wchar_t* NoteFile_GetName(void);

#endif
