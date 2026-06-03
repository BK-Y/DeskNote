# 2026-06-03 — 建立 .ai 记录体系

## 做了什么
- 创建 `insights.md` + `insights/`：记录聊天中沉淀的设计结论
- 创建 `change_logs.md` + `change_logs/`：工程笔记流水
- 创建 `arch_change_logs.md` + `arch_changelogs_detail/`：架构修改记录

## 背景
之前的讨论结论没有归档位置，重复讨论、遗失方案。这套体系解决：
- 聊天中灵光一现的结论 → insights
- 改了什么 → change_logs
- 架构变动 → arch_change_logs

## 文件结构
```
.ai/
  insights.md + insights/             设计结论索引 + 详情
  change_logs.md + change_logs/       工程流水索引 + 详情
  arch_change_logs.md + arch_changelogs_detail/  架构修改索引 + 详情
  plans.md + plans/                   计划索引 + 详情
  arch.md                             架构文档
  standards/                          标准规范
```
