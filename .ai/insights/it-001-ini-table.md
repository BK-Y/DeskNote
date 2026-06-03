# it-001 — ini_table：行级增改的配置存储方案

## 问题
StateStore_Save 整表覆写 state.ini，丢弃所有注释和空行。

## 方案
ini_table 模块，独立于任何业务语义：
- 读取：一次读入全部行到内存数组，二次扫描解析 key=value
- 运行时：维护 key_value 内存表，不保留 raw_lines
- 写入：临时重新读文件，找到目标行替换值，其余原样保留；新 key 追加
- 注释保留：写入时只改值所在行，注释行和空行原封不动
- 伴生行为：通过 on_change 回调分离，ini_table 不直接调 AppBar

## 文件位置
`src/storage/ini_table.c` / `ini_table.h`

## 与现有 state_store 的关系
ini_table 是底层通用机制，state_store 在它之上封装预定义的字段集合。

## 讨论记录
2026-06-03 与用户讨论确定方向。
