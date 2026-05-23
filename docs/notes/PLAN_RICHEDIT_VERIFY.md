# 阶段 2-2：Rich Edit 能力验证 (已完成)

## 验证结果

| 测试 | 方法 | 结果 |
|------|------|------|
| 控件创建 | `CreateWindowExW(L"RichEdit20W", ...)` | ✅ 成功 |
| 文本输入 | `SetWindowTextW` / 直接打字 | ✅ 正常 |
| EM_GETTEXTMODE | 返回值 42 (TM_RICHTEXT) | ✅ 富文本模式 |
| EM_GETLIMITTEXT | 返回值 32767 | ✅ 正常 |
| CHARFORMAT2W size | 116 字节 | ✅ 正确 |
| **字号** | `CFM_SIZE`, `yHeight = 480` | ✅ **可见变化** |
| **粗体** | `CFM_WEIGHT`, `wWeight = FW_BOLD` | ✅ **可见变化** |
| **斜体** | `CFM_ITALIC`, `dwEffects = CFE_ITALIC` | ✅ **可见变化** |
| **颜色** | `CFM_COLOR`, `crTextColor = RGB(255,0,0)` | ✅ **可见变化** |
| **粗体（旧方式）** | `CFM_BOLD`, `dwEffects = CFE_BOLD` | ❌ **无效** |

## 关键发现

**粗体必须用 `CFM_WEIGHT` 而非 `CFM_BOLD`：**

```c
// ❌ 无效
cf.dwMask = CFM_BOLD;
cf.dwEffects = CFE_BOLD;

// ✅ 有效
cf.dwMask = CFM_WEIGHT;
cf.wWeight = FW_BOLD;  // 700
```

其余格式（斜体、颜色、字号）使用标准方式即可。

## 下一步

清理测试代码 → 重新引入 md4c → 实现实时 Markdown 渲染
