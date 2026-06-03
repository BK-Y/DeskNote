# it-001 — ini2arr：ini 文件到内存数组的读写模块

## 问题
StateStore_Save 整表覆写 state.ini，丢弃所有注释和空行。

## 方案
一个只处理 ini 文件读写的小模块，将 ini 内容转换为内存中的 key-value 数组。

## 命名
`ini2arr` — ini file to array。功能就是 ini 到数组的转换。

## 文件位置
`src/storage/ini2arr.c` / `ini2arr.h`

## 数据结构

```c
typedef struct {
    char key[64];
    int  val;
} KeyValue;
```

### 读取逻辑

```c
#define MAX_LINES  128
#define MAX_LINE_LEN 256

char raw_lines[MAX_LINES][MAX_LINE_LEN];
int  line_count = 0;

// 一次读入全部行到临时缓冲区
FILE* f = fopen("state.ini", "r");
if (!f) return;
while (fgets(raw_lines[line_count], MAX_LINE_LEN, f) && line_count < MAX_LINES)
    line_count++;
fclose(f);

// 二次扫描，解析配置行到 key_value 表
KeyValue* table = malloc(sizeof(KeyValue) * (line_count + 1));
int config_count = 0;

for (int i = 0; i < line_count; i++) {
    char key[64];
    int  val;
    if (sscanf(raw_lines[i], " %63[^=]=%d", key, &val) == 2) {
        strcpy(table[config_count].key, key);
        table[config_count].val = val;
        config_count++;
    }
}
// raw_lines 在读取解析完后即可释放，运行时只保留 key_value 表
```

### 写入逻辑

行级增改，不是整表覆盖：
- 找到 key 所在行 → 只替换那行的值
- 没找到 → 在文件末尾追加
- 注释行、空行、其他 key 的原行完全不动

### 伴生行为

通过 on_change 回调分离，本模块不涉及任何业务逻辑（如注册 AppBar）。


## 与现有 state_store 的关系

本模块是底层通用 ini 读写机制，state_store 在它之上封装预定义的字段集合。

## 讨论记录
2026-06-03 与用户讨论确定读取逻辑、行级写入策略、命名 ini2arr。
