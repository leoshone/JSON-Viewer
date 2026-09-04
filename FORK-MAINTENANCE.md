# 本 fork 的自定义改动与上游同步指南

> 本文件由 leoshone/JSON-Viewer fork 维护。
> 日期基准：2026-09-04。上游基线：`NPP-JSONViewer/JSON-Viewer@` **c448336**（v2.2.0.0 + 两个 dependabot 提交）。

---

## 一、本 fork 相对上游的全部改动

四条需求，全部实现、验证并部署。改动**全部是新增分支**（新增设置项、新增代码路径），
不删除、不改名任何上游已有的用户可见配置——这样每次同步上游时冲突面最小。

| 功能 | 开关（ini `[Others]`） | 默认 | 上游 PR |
|------|------------------------|------|---------|
| R1 树面板字体缩放固化（80%~250%，重启恢复） | `TREE_ZOOM` | `100` | [#251](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/251) |
| R2 按 TAB 缓存树快照（绝不自动解析，Refresh 才画；切 TAB 回放快照不重解析；关 TAB 清缓存） | 原有 `FOLLOW_TAB=0` 的新行为 | `0` | [#253](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/253) |
| R3 Refresh 保留展开态与选中（按节点路径匹配，路径失效的丢弃） | 无（直接生效） | — | [#252](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/252) |
| R5 打开 json 文件时自动画一次树（之后仍走 R2 快照，不重解析） | `DRAW_ON_OPEN` | `0` | 并入 [#253](https://github.com/NPP-JSONViewer/JSON-Viewer/pull/253) |
| R6 识别 jsonc 文件（`.jsonc` 与 `.json` 同等对待：自动画树/切 TAB 跟随/auto-format 均生效） | 无（直接生效） | — | 并入 #251/#253（`IsJsonFile` 同时接受 `L_JSON5`） |

jsonc 支持边界：注释 + 尾逗号（解析层本就支持且默认开启）；完整 JSON5 语法
（无引号 key、单引号字符串）**不在支持范围**，会照常报解析错误。
`IsJsonFile()` 是原版就有的判定点，此改动同时惠及 FOLLOW_TAB 与 auto-format。

### 分支拓扑

```
c448336 (上游 master 基线)
├── fix/persist-tree-zoom          = R1                    → PR #251
└── fix/keep-expansion-on-refresh  = R3                    → PR #252
        └── fix/per-tab-tree-snapshot = R2 + R5            → PR #253
                └── integration/all-features = R1+R3+R2+R5 合并（本地全集，不开上游 PR）
```

- **`integration/all-features`** 是"全家桶"：合并 R1 进 #253 链，解决过 3 处
  相邻插入冲突（`Define.h` / `Profile.cpp` / `ProfileTest.cpp` 的
  `TREE_ZOOM` vs `DRAW_ON_OPEN`）。**平时自己用、要部署 DLL，认准这条分支。**
- fork 上的 draft PR（leoshone#5 等）仅为触发 CI，**永不合并**。

### 与上游的关键行为差异（同步代码前先读懂）

1. `DrawJsonTree()` 签名变了：`DrawJsonTree(bool bPreserveExpansion = false, bool bSilent = false)`。
   `bSilent=true` 时解析错误只写成树内错误节点，不弹模态框（用于 R5 的自动画树）。
2. `FormatJson()` 走 `ReDrawJsonTree(true, true)`（重绘并保留展开态），与 R3 的 Refresh 行为一致。
3. `HandleFileOpened()` 承接了 auto-format（原版在 `HandleTabActivated` 里做），
   使"打开文件"不再触发解析。
4. `NPPN_READY` 会调 `RestoreCurrentTabTree()`（会话恢复场景画树入口）。
5. 新文件：`TreeExpansion.h/.cpp`（R3 路径工具）、`TreeState.h/.cpp`（R2 快照模型）。

---

## 二、上游大概率不合并：为什么要"反向同步"

上游实质停滞（最后一个功能提交 2026-05-30，其后只有 dependabot），三个 PR 挂着
无人评审。**实际策略是把上游当作"别人的主线"，定期把上游的新提交吸进来**，
而不是等上游吸收我们。

我们的优势：所有改动都是**纯增量**（新开关、新分支、新文件），与上游新代码的
天然冲突面很小；同步时真正要盯的只有下面第四节列的高危文件。

---

## 三、从上游同步的标准操作

```bash
cd D:\AiSpaces\Code\JSON-Viewer\upstream-jsonviewer

# 0) 一次性配置（只需做一次）
git remote add upstream https://github.com/NPP-JSONViewer/JSON-Viewer.git

# 1) 取上游
git fetch upstream

# 2) 看上游有什么新东西（决定要不要同步）
git log --oneline integration/all-features..upstream/master

# 3) 把上游合进集成分支
git checkout integration/all-features
git merge upstream/master
#    - 若无冲突：跑第四节的自检清单，然后 push
#    - 有冲突：见第四节逐文件策略

# 4) 如果上游吸收了我们的某个 PR（比如先合了 #251）：
#    合并会自动去重；确认 integration 分支里该功能的代码仍只剩一份：
git grep -n "TREE_ZOOM" -- src/        # 应只有 Define.h/Profile.cpp/JsonViewDlg.cpp/SettingsDlg.cpp 各一处定义
```

### 同步后必跑的验证（缺一不可）

```bash
# 单元测试（MinGW 本地，~1 分钟）
cd D:\AiSpaces\Code\JSON-Viewer\_build
bash build-tests-prc.sh     # 期望 168/168 PASSED
bash build-prc.sh           # DLL 编译通过（只验编译，产物不用）

# CI 出正式 DLL
git push origin integration/all-features
# （fork 已有 base=master 的 draft PR 时，push 自动触发 CI）
gh run watch --repo leoshone/JSON-Viewer $(gh run list --repo leoshone/JSON-Viewer --branch integration/all-features --limit 1 --json databaseId -q '.[0].databaseId')
# 六 job 全绿后下载：
gh run download <RUN_ID> --repo leoshone/JSON-Viewer -n NppJSONViewer_x64_Release -D ci-integration

# 真机 E2E（把 ci-integration\NPPJSONViewer.dll 拷进 _npp-test 后跑）
powershell -File _build\e2e-integration.ps1     # 期望 20/20
powershell -File _build\e2e-r2.ps1              # 期望 21/21
powershell -File _build\e2e-drawonopen.ps1      # 期望 20/20
powershell -File _build\e2e-r3.ps1              # 期望 10/10
powershell -File _build\e2e-r1.ps1              # 期望 6/6
```

### 部署到日常 NPP

```powershell
# NPP 必须已关闭！
$dir = "D:\Software\Notepad\plugins\NPPJSONViewer"
Copy-Item "D:\AiSpaces\Code\JSON-Viewer\_build\ci-integration\NPPJSONViewer.dll" "$dir\NPPJSONViewer.dll" -Force
```

日常 NPP：`D:\Software\Notepad`（doLocalConf，x64）。原版 DLL 备份在同目录
`NPPJSONViewer.dll.official-20250223`。

---

## 四、同步冲突高危文件与处置策略

按优先级排列。前三个是**已知的惯犯**（R1 与 R5 的相邻插入在此冲突过一次）：

| 文件 | 冲突形态 | 处置 |
|------|----------|------|
| `src/NppJsonViewer/Define.h` | 上游改 `Setting` 结构体 / 我们加了两个常量和一个字段 | 保两边；`TREE_ZOOM` 常量在上、`DRAW_ON_OPEN` 在下的顺序保持稳定 |
| `src/NppJsonViewer/Profile.cpp` | `GetSettings`/`SetSettings` 读写链相邻行 | 保两边；注意 `bRetVal &&` 链别断（虽然上游 `ReadValue(int)` 恒真，链断了也不报错——这正是它隐蔽的地方） |
| `tests/UnitTest/ProfileTest.cpp` | 默认值断言块 + 文件尾部新测试 | 保两边；`TreeZoom_RoundTrip` 和 `DrawOnOpen_RoundTrip` 两个 TEST 都要在 |
| `src/NppJsonViewer/JsonViewDlg.cpp` | 上游若改 `display()`/`HandleTabActivated()`/`DrawJsonTree()` 会撞我们的大改动 | **人工逐段合**。改动核心都在这几处：`display()` 的 `bShow` 分支、`HandleTabActivated` 的 else 分支、`RestoreTabState` 的无快照分支、`DrawJsonTree` 的静默开关 |
| `src/NppJsonViewer/resource.rc` | 上游动设置对话框布局 | 我们的复选框 `IDC_CHK_DRAW_ON_OPEN` 在 y=85；若上游也加了控件注意 y 坐标排布。**该文件含非 ASCII 字节（©=0xa9），必须用字节级工具修冲突，禁用会转码的编辑器** |
| `NPPJSONViewer.vcxproj` / `UnitTest.vcxproj` | 上游增删源文件 | 我们的新文件（TreeState/TreeExpansion）条目要保留；文件带 UTF-8 BOM，别弄丢 |

### 提交前的固定自查（曾两次救命）

1. **编码完整性**：改动文件逐个比对非 ASCII 字节数（对照 HEAD 版本）。
   `resource.rc` 必须恰好 1 个 `\xa9`；vcxproj 必须恰好 1 组 `ef bb bf`（BOM）。
   其他文件应为 0。发现 U+FFFD（`ef bf bd`）= 编码往返损坏，用 Python 字节级替换修复。
2. **同文件多处 Edit 必须串行**，改完按清单 grep 核对每一处。
3. 单测**不编译** `JsonViewDlg.cpp`——它编译通过必须靠 `build-prc.sh`（DLL 链接）验证。

---

## 五、已知限制（有意不修，防止遗忘）

1. **key 含 `.` 的 JSON**：R3/R2 的路径匹配用 `.` 分隔，key 本身带点时该节点的
   展开/选中恢复会失配（数据无损，仅恢复精度）。若要修：SplitPath/JoinPath
   换转义方案或长度前缀方案（`TreeExpansion.cpp:5`、`JsonViewDlg.cpp:764`）。
2. **面板隐藏时打开文件不触发 auto-format**：与上游原版行为一致（原版同样要求
   `isVisible()`），不是回归。
3. **上游既有问题**（不修，除非顺手）：`Profile::ReadValue(int)` 恒返回 true
   （错误链失效）；`SetTreeViewZoom` 的 `static HFONT` 缓存初始字体、句柄不释放。
4. `SetSettings_Positive` 测试里 `nTreeZoom` 断言是恒真的（没赋值就断言），
   真实覆盖靠 `TreeZoom_RoundTrip`。上游 `bAutoFormat` 重复赋值同款瑕疵。
   修这两个要在对应 PR 分支上单独做，**不要在 integration 分支顺手修**——
   它必须保持为三个 PR 的严格并集。

---

## 六、历史档案

- 开发全过程日志：`.workbuddy/memory/2026-09-03.md`、`2026-09-04.md`
- 上游 PR 跟踪（每日自动检查）：`.workbuddy/memory/upstream-pr-watch.md`
- E2E 脚手架说明与踩坑全集：`.workbuddy/skills/win-gui-e2e-messaging/SKILL.md`
- 测试环境：`_npp-test\`（便携版 NPP 8.9.6.2 x64 + `-multiInst -nosession` 隔离实例）
- 本地构建脚本：`_build\build-prc.sh`（DLL）、`build-tests-prc.sh`（单测），
  需 MinGW（`C:/Users/xiongbin/WorkBuddy/2026-08-27-20-02-13/toolchain/mingw64`）
- CI：fork 的 GitHub Actions 已启用；上游 CI 只认 master 的 push/PR，
  fork 分支必须开 base=master 的 draft PR 才触发
