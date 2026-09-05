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
| [#251](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/251) | R1 树缩放固化（65 行 / 6 文件） | 09-03 | **CHANGES_REQUESTED** → 已整改并逐条回复，等对方回应 |
| [#252](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/252) | R3 Refresh 保留展开态 | 09-03 | 无人评审 |
| [#253](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/253) | R2+R5 按 TAB 缓存 / 打开画树 | 09-04 | 无人评审（已按 #251 意见撤下 jsonc） |
| [#254](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/254) | R7 高 DPI 树底行裁切修复 | 09-04 | 无人评审 |
| **待开 #255** | R6 窄版 jsonc 识别 | — | **计划已定（09-05）**：等 #251 有结果再开，见第四节 |

**总体判断**：上游实质停滞，三个 PR 挂着无人评审。真正的策略是**把上游当主线、定期反向
同步**，而不是等它吸收我们。但 #251 是个例外——它小而纯，是唯一有希望进上游的，
值得为它多花点功夫。

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

## 四、待办：窄版推上游（#255）

**计划已于 2026-09-05 定稿：要推，但现在不开；#251 的 thread 现在也不改。**

- **推的理由**：窄版是正面回应意见 1 的——它压根不接受 JSON5 语言，只在扩展名是
  `.jsonc` 时破例，`.json5` 行为零变化。这是把他的顾虑原样解决的方案，通过概率
  明显高于之前那个宽版。
- **不开的理由（时机）**：我们刚在同一条 thread 里说"Removed, jsonc stays in my fork"，
  扭头又提 jsonc 显得反复；而且同时挂两个相关 PR 容易让人困惑。
- **触发条件**：#251 被批准或合并 → 立刻开；或 #251 两周无回音 → 开。
- **开法**：**新 PR（预计 #255），不要塞进 #251**（#251 必须保持小而纯）。
  正文第一段直说：
  > Following your comment on #251 — here is a version that does not accept JSON5
  > at all; it only recognises the `.jsonc` extension, and a real `.json5` file
  > keeps behaving exactly as before.

- **#251 的 thread 现在不动**：那条 "Removed … jsonc recognition stays in my fork"
  目前仍然属实（窄版确实只在 fork 里）。而且**编辑评论不会重新发通知**，改了他也不一定
  看得到，收益极低；刚说完 Removed 就改口还显得犹豫。

**风险与退路**：插件历来只按"语言类型"判定，改成看扩展名是引入第二条判据，
maintainer 可能以一致性为由拒绝。被拒也不亏——fork 留着，或者退到零代码方案
（在说明里让用户把 `jsonc` 加进 Notepad++ 的 json 语言用户扩展名，比改 langs.xml 正当）。

### 开完 #255 之后的收尾（别忘了）

在 #251 那条回复（comment id `3939416226`）**末尾追加**一句指向新 PR，让后来读 thread
的人有去处：

```bash
gh api -X PATCH repos/NPP-JSONViewer/JSON-Viewer/pulls/comments/3939416226 \
  -f body="$(cat new-body.md)"
```

追加的内容：
> Superseded by #255, which recognises .jsonc without accepting JSON5 at all —
> a real .json5 file keeps behaving exactly as before.

（另两条备用 id：我们的 `configPath` 回复是 `3939416311`；他那两条原始意见是
`3939342913` / `3939344367`，**那两条我们不能改**。）

---

## 五、环境事实（每次推 PR 前都适用）

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
