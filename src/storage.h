// storage.h - 文件路径 + 配置读写
#ifndef STORAGE_H
#define STORAGE_H

#include <windows.h>

// 初始化存储：确保目录存在，加载配置
// 返回值：0=成功, 1=失败
int Storage_Init(void);

// 获取便签文件存储目录（末尾带反斜杠）
const wchar_t* Storage_GetNotesDir(void);

// 获取配置文件的完整路径
const wchar_t* Storage_GetSettingsPath(void);

// 读取配置中的字符串值（key:value 形式，JSON 简易解析）
// buf: 输出缓冲区, bufsize: 缓冲区大小(字符数)
// 返回值：读取到的字符串长度，0 表示未找到
int Storage_ReadSetting(const wchar_t* key, wchar_t* buf, int bufsize);

// 写入配置中的字符串值
// 如果文件不存在则创建
int Storage_WriteSetting(const wchar_t* key, const wchar_t* value);

#endif
