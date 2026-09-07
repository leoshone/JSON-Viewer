# 推上游记录（leoshone fork → NPP-JSONViewer/JSON-Viewer）

这份文档只记**推上游这件事本身**：每个 PR 的来龙去脉、评审原文、我们的决策与理由、
当前状态和下一步。代码层面的同步操作、冲突高危文件在
[`FORK-MAINTENANCE.md`](FORK-MAINTENANCE.md)，两份文档不重复。

上游仓库：`NPP-JSONViewer/JSON-Viewer`　我们的 fork：`leoshone/JSON-Viewer`
基线：`c448336`（2026-08-14，上游最后一个非 dependabot 提交是 2026-05-30 的 v2.2.0.0）

---

## 一、当前状态速览

| PR | 内容 | 提交 | 状态 |
|---|---|---|---|
| [#251](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/251) | R1 树缩放固化（65 行 / 6 文件） | 09-03 | ✅ **MERGED + APPROVED**（09-05 整改后获批，**第一个进上游的成果**） |
| [#252](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/252) | R3 Refresh 保留展开态 | 09-03 | 无人评审；09-07 确认 MERGEABLE（无需动） |
| [#253](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/253) | R2+R5 按 TAB 缓存 / 打开画树 | 09-04 | 无人评审；09-07 rebase 到 fa8d2b9，MERGEABLE |
| [#254](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/254) | R7 高 DPI 树底行裁切修复 | 09-04 | rebase 完成、before/after 对比已发（09-07），等回应 |
| [#255](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/255) | R6 窄版 jsonc 识别 | 09-07 | **OPEN**，正文首段直接引用 #251 的评审意见 |

**总体判断更新（09-07）**：上游并不完全停滞——#251 被合并，且合并后上游自己又做了跟进
（0f096dc 重写了设置保存逻辑 + 改了我们 ini 键名，见下）。**小而纯的 PR 策略已被验证有效**。

---

## 二、#251 的评审与我们的处置（2026-09-05）

maintainer **SinghRajenM** 留了两条行内意见（review 无固定 body，只有这两条）。

### 意见 1 — `ScintillaEditor.cpp:65`

> Let's remove `JSON5` as it is not supported currently by the plugin.

**他的意思**：我们的 jsonc 改动让 `IsJsonFile()` 接受 `L_JSON5`，但插件解析器不支持真正的
JSON5 语法（无引号 key、单引号字符串），这等于宣称支持了并不支持的东西。

**我们的分析**：顾虑成立，而且比他说的还严重一点——真正的 `.json5` 文件原本被插件安静
忽略，改动后会被拉去当 json 解析并弹错，**这是行为倒退**。

**处置**：照做。`fix/persist-tree-zoom` reset 到 `7771ca0`、`fix/per-tab-tree-snapshot`
reset 到 `ed4c3d8`，force-push。两个 PR 都不再含 jsonc 提交（各只动 `ScintillaEditor.cpp`
一个文件）。jsonc 改动留在 fork 的 `integration/all-features`（合并历史独立，
分支 reset 不影响它）。

**回复原文**：
> Agreed - full JSON5 syntax (unquoted keys, single-quoted strings) is not supported by the parser, and the change was out of scope for this PR anyway.
> Removed: `IsJsonFile()` is back to `L_JSON` only, and the extra commit is gone from this branch (and from #253). jsonc recognition stays in my fork.

### 意见 2 — `Define.h:125`

> What is the purpose and where it is used?

**他问的是** `Setting::configPath`，**这是我们加的**（用于对话框写回 `TREE_ZOOM`：
`JsonViewDlg.cpp:1363` → `ProfileSetting(m_pSetting->configPath).SetSettings(...)`；
上游原先只把 ini 路径单独传给 `SettingsDlg`）。

**这条疑问一半是我们自找的**：09-04 那次给 #251 "追加" jsonc 说明时，用
`gh pr edit --body-file` 是**整体替换**，把原正文冲掉了，PR 只剩一段 jsonc 文字，
reviewer 看不到任何背景。已重写完整正文（R1 描述 + 拖拽修复说明 + 验证章节）。

**回复原文**：
> `configPath` is the full path of JSONViewer.ini.
> The tree dialog needs it because the zoom level has to be written back to the ini when the user moves the zoom slider (`JsonViewDlg.cpp` -> `ProfileSetting(m_pSetting->configPath).SetSettings(...)`), and the dialog had no access to that path before - upstream only hands it to `SettingsDlg` through that dialog's own `m_configPath` (`NppJsonPlugin.cpp`). It is assigned once in `NppJsonPlugin.cpp` and is not itself persisted to the ini.
> I also rewrote the PR description, which had lost its original content - sorry about the missing context.

### 为什么是"撤掉"而不是"反驳"

- jsonc 本来就是顺手加的，不是 #251 的主题；为它挡住唯一有希望的 PR 不划算
- #251 是本批唯一被 review 的，说明**只有小 PR 会被看**——保持它小才有机会
- 我们自己的发布包照旧带 jsonc，撤 PR 不影响自己用

---

## 三、窄版 jsonc（R6 的当前形态，2026-09-05）

撤掉之后我们把 fork 里的实现改成了**窄版**：

```cpp
if (languageType == LangType::L_JSON)   return true;
if (languageType == LangType::L_JSON5)  return 文件名以 .jsonc 结尾;   // 只放 .jsonc
return false;
```

与宽版（`L_JSON || L_JSON5`）的差别：**真正的 `.json5` 文件行为零变化**，依旧被忽略，
不会被拉去解析然后报错。这正好回应了意见 1 的实质顾虑。

### 已实测坐实的事实（E2E，写进了 `e2e-jsonc.ps1` 的 C5）

- Notepad++ 把 `.jsonc` 和 `.json5` 都报为**同一个语言：86 = L_JSON5**
  （`langs.xml` 里 `json5 ext="json5 jsonc"` 共用一条记录）
- 内容完全相同的情况下：`.jsonc` 画树（4 项），`.json5` 不画（1 项）
- **画树成功后插件会把缓冲区语言改写成 L_JSON（57）**——`HighlightAsJson()` →
  `SetLangAsJson()`。所以任何语言探测都必须在 DRAW_ON_OPEN **关闭**的状态下做，
  否则读到的是改写后的值，会得出"两个扩展名语言不同"的错误结论（我第一版 C5
  就是这样误判的）
- 该断言对宽版会失败（宽版把 `.json5` 画成 6 项），所以它是真正的回归护栏，不是空断言

### 窄版的边界

只支持 jsonc（注释 + 尾逗号）。完整 JSON5 语法不支持、也不假装支持。

---

## 四、窄版推上游（#255）—— ✅ 已于 09-07 执行

按 09-05 定稿的计划执行完毕，实际动作：

1. **#255 已开**：[Recognize .jsonc files without implying JSON5 support](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/255)，
   分支 `feature/jsonc-narrow`（upstream/master fa8d2b9 + 窄版提交），
   正文首段直接引用 #251 的评审意见。
2. **#251 的 thread 追加了指向**（comment `3939416226`，PATCH 编辑成功——按计划，开完
   #255 后才改的，不是之前就改）：
   > Follow-up: #255 now proposes jsonc recognition in the narrow form discussed here -
   > it does not accept the JSON5 language at all, only the `.jsonc` extension, and a
   > real `.json5` file keeps behaving exactly as before.
3. cherry-pick 窄版提交到新分支时有 2 处冲突（include 区 + IsJsonFile 本体），均按
   "保留窄版语义 + 顺应上游新 include 顺序"解决，本地 MinGW 编译通过。

**风险与退路（仍有效）**：插件历来只按"语言类型"判定，改成看扩展名是引入第二条判据，
maintainer 可能以一致性为由拒绝。被拒也不亏——fork 留着，或者退到零代码方案
（在说明里让用户把 `jsonc` 加进 Notepad++ 的 json 语言用户扩展名，比改 langs.xml 正当）。

---

## 五、上游动态（09-07 观察，影响下次同步）

上游 master 前进到 `fa8d2b9`，三个新提交，**对 integration 分支有直接影响**：

| 提交 | 内容 | 对我们的影响 |
|---|---|---|
| `0f096dc` | Code improvement：**设置改为退出时保存且仅在变化时写**（`writeIfChanged`）+ include 顺序调整 | integration 的 `Profile.cpp` 写入路径必须顺应 `writeIfChanged` 模式 |
| `7605bf1` | Update submodule | submodule 指针，同步时自动处理 |
| `fa8d2b9` | Header file order change（9 个 .cpp 的 include 重排） | 与我们所有 PR 的 include 冲突，rebase 时逐个解决即可 |

**⚠️ 重要：上游把我们的 ini 键名改了** —— `TREE_ZOOM` → **`TREE_ZOOM_LEVEL`**
（`Define.h: STR_INI_OTHER_TREE_ZOOM[] = TEXT("TREE_ZOOM_LEVEL")`，上游合并 #251 后的跟进）。

后果：
- integration 分支目前仍写 `TREE_ZOOM`，**同步上游后必须跟随改名**；
- 改名意味着**已有 ini 里的 `TREE_ZOOM=200` 会被忽略**（回退到 100），用户需重新设置一次。
  如在意，可在同步时加一步向后兼容读取（先读 `TREE_ZOOM_LEVEL`，读不到再读 `TREE_ZOOM`）——
  是否值得加，同步时再定。

09-07 已把 **#253/#254 rebase 到 `fa8d2b9`**（冲突解法：`Profile.cpp` 写入侧顺应
`writeIfChanged`、DRAW_ON_OPEN 用同样模式；`Define.h`/`ProfileTest.cpp` 两侧都保留），
三个分支均 `mergeable=true`，MinGW 编译 + 单测全过。

---

## 六、环境事实（每次推 PR 前都适用）

| 事实 | 影响 |
|---|---|
| 上游 master 停在 `c448336`（2026-08-14） | 我们的分支基线不用经常动 |
| **上游不为 fork PR 跑 CI** | PR 页面显示 "no checks"，不是我们构建失败；要 CI 得靠 fork 自己的 draft PR（leoshone#5，指向 `integration/all-features`） |
| **fork 账号无法 `requested_reviewers`**（404，无写权限） | 回应完只能等对方收到通知，不能主动催 |
| **`gh pr edit --body-file` 是整体替换** | 追加正文必须先取回原 body 再拼接，改完回读校验（#251 正文就是这么丢的） |
| 只有小 PR 会被 review | 见第一节；#251（65 行）被看，#253（20 文件）没人碰 |
| **自己发的评论可改可删**（`PATCH`/`DELETE` `/pulls/comments/{id}`） | 但**编辑不发通知**，且会留 edited 标记；别人的评论改不了 |
| **push 后别急着取 CI 产物** | `gh run list --limit 1` 可能拿到上一次的 run（新的还没登记），`watch` 会秒回 "already completed"。必须核对 `headSha` == 本地 HEAD 再下载 |

### 写 PR 正文的硬要求

PR 正文是 reviewer 唯一的导览，**必须自包含**（未来他不会看我们的对话）：

1. 说清"改了什么"和"为什么"，不要只贴 diff
2. 说清**验证方式**（单测覆盖了什么、E2E 在真实 NPP 里验了什么）
3. 每个新字段/新函数都要能回答"它用在哪、为什么必须有"（意见 2 就是这么来的）
4. 一个 PR 一个主题，顺手改动一律另开
5. cherry-pick 提交到别的分支后，**正文必须同步补**（否则 diff 里有解释不到的提交）
