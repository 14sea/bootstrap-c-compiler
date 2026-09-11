**简体中文** · [English](README.en.md)

# 从零自举的 C 子集编译器（Ubuntu / x86-64）

<p align="center">
  <a href="ref/idea.jpeg"><img src="ref/idea.jpeg" height="380" alt="出题的微博原帖"></a>
  <a href="ref/result.02.png"><img src="ref/result.02.png" height="380" alt="build.sh 完整输出"></a>
  <a href="ref/result.01.png"><img src="ref/result.01.png" height="380" alt="会话交付小结"></a>
</p>

> **缘起。** 这个仓库是对微博用户 tombkeeper 于 2026-09-11 所出题目的一次完整解答：
> 不借助任何现成的编译器或汇编器，从一份手写的十六进制种子出发，一步步造出能自举的
> C 子集编译器。原题的目标平台是 Windows x64，本仓库做的是 Ubuntu x86-64。
> 全部代码由 Claude Code（Opus 5）在一次会话中写成。
>
> `ref/` 目录保留了这次工作的引用材料，各用一句话说明：
>
> | 材料 | 内容 |
> |---|---|
> | [`ref/idea.jpeg`](ref/idea.jpeg) | 出题的微博原帖截图：六条约束的题面，以及出题者自己那版 Windows 实现的构建日志（种子 → 模板展开器 → 引导编译器 → c0/c1/c2，SHA256 逐字节一致）。 |
> | [`ref/prompt.txt`](ref/prompt.txt) | 实际交给 Claude Code 的提示词原文，也就是本仓库自始至终严格遵循的那六条规则。 |
> | [`ref/result.01.png`](ref/result.01.png) | 会话结束时的交付小结截图：四阶段引导链表格、`cc1 == cc2 == cc3` 的同一个 md5，以及逐条要求的落实说明。 |
> | [`ref/result.02.png`](ref/result.02.png) | `./build.sh` 的完整输出截图：从种子重建全部四个阶段、不动点校验通过、7 项功能测试全绿。 |

全部产物都由本仓库自己的工具链生成。整条链的唯一起点是 `seed/hexa.hex`
——一份手写的十六进制机器码文本。链上除了"把十六进制文本转成字节"
这一步之外，没有任何外部程序参与代码生成。

```
seed/hexa.hex ──hex2bin──▶ hexa      阶段 A  最小汇编器（手写机器码）
stage1/hex0.hasm ──hexa──▶ hex0      阶段 B  带命名标签与宏的汇编器
stage1/cc0.hasm  ──hex0──▶ cc0       阶段 C  C 子集编译器（用 hex0 汇编写）
stage2/cc.c      ──cc0 ──▶ cc1       阶段 D  自举编译器（用它支持的 C 子集写）
stage2/cc.c      ──cc1 ──▶ cc2
stage2/cc.c      ──cc2 ──▶ cc3       验证 cc2 == cc3（不动点）
tools/hex2bin.c  ──cc  ──▶ hex2bin   自制的十六进制→二进制转换器
```

实测结果：`cc1 == cc2 == cc3`，md5 `28e0088aa8da1e6a3f9ea3e890583c91`。
连 cc0 与 cc 的代码生成也逐字节一致。

## 一、构建与测试

```sh
./build.sh                 # 从种子完整重建并自检（约 1 秒）
./tests/run.sh ./build/cc  # 单独跑测试套件
tools/ccc.sh prog.c prog   # 用最终编译器编译一个程序
```

`build.sh` 会依次完成：建 venv → 种子转二进制 → 逐级自举 → 连续两次自编译
并比对 → 用自制 hex2bin 复现全部二进制 → 运行 7 个功能测试。

单步等价于：

```sh
env/bin/python tools/hex2bin.py seed/hexa.hex build/hexa
./build/hexa < stage1/hex0.hasm > build/hex0.hex
env/bin/python tools/hex2bin.py build/hex0.hex build/hex0
cat stage1/prelude.hasm stage1/cc0.hasm | ./build/hex0 > build/cc0.hex
env/bin/python tools/hex2bin.py build/cc0.hex build/cc0
./build/cc0 < stage2/cc.c > build/cc1.hasm
./build/hex0 < build/cc1.hasm > build/cc1.hex
env/bin/python tools/hex2bin.py build/cc1.hex build/cc1
# ... cc1 -> cc2 -> cc3，cmp build/cc2 build/cc3
```

## 二、关于"不借助外部工具"

| 环节 | 用到的东西 |
|---|---|
| 种子二进制 | `tools/hex2bin.py`（31 行，只做 hex 文本 → 字节，无符号解析、无代码生成） |
| 其余所有二进制 | 一律由前一级自制工具生成；`tools/hex2bin.c` 是同一转换器的自举版本，`build.sh` 会验证它与 Python 版对所有产物逐字节一致 |
| gcc / as / ld / nasm | 完全没有使用 |
| objdump | 仅用于人工核对种子（检查，不参与生成） |
| cat / sh | 只做文本拼接与流程调度 |

## 三、阶段 A：种子汇编器 `hexa`（584 字节）

`seed/hexa.hex` 是逐条手工编码的 x86-64 机器码，每行都带有地址与助记符注释；
跳转位移与调用偏移全部手算，可用 `objdump -D -b binary -m i386:x86-64 build/hexa`
复核。它从 stdin 读汇编源，把解析好的机器码**以十六进制文本写到 stdout**。

语法（两遍扫描）：

| 记号 | 含义 |
|---|---|
| `XX` | 一个字面字节 |
| `:ab` | 定义标签 `ab`（恰好两个字符，直接索引表） |
| `@ab` | 4 字节，相对位移 rel32（相对于该字段之后） |
| `&ab` | 4 字节，标签的绝对虚拟地址 |
| `~ab` | 4 字节，标签的文件偏移（地址 − 0x400000） |
| `$ab` | 8 字节，标签的绝对虚拟地址 |
| `# …` | 注释到行尾 |

程序加载到 0x400000，单个 RWX `PT_LOAD` 段，BSS 由 `p_memsz` 提供。

## 四、阶段 B：汇编器 `hex0`（1412 字节）

`stage1/hex0.hasm` 用 hexa 的语言写成，功能是 hexa 的超集：

* 任意长度的标签名（开放定址哈希表，2^18 槽）
* `%NAME bb bb …` 宏定义，`NAME` 处展开为这串字节
* `"文本"` 字符串字面量，直接发射 ASCII 字节
* `@name &name ~name $name`、`#` 与 `;` 注释
* 未定义名字会报错并以状态 3 退出

`stage1/prelude.hasm` 是一套 x86-64 指令宏（`PROLOG`、`LDA V3`、`ADD_AC`、`JNE` …），
使得阶段 C 可以用可读的方式书写。

## 五、阶段 C：`cc0`（12667 字节）

`stage1/cc0.hasm` 是用 hex0 宏汇编手写的 C 子集编译器，约 2700 行。
它读 C 源码，输出 hex0 汇编文本。存在的唯一目的是编译 `stage2/cc.c`。
它支持的语法与下一节完全一致，只是**不含 `for`**。

## 六、阶段 D：最终编译器 `stage2/cc.c`

用它自己支持的子集写成，1070 行。`cc.c` 读 stdin 的 C 源码，输出 hex0 汇编文本。

### 支持的语言

* **类型**：`char`(1 字节，有符号)、`int` / `long` / `void`(均 8 字节)、
  任意层指针、一维数组（全局与局部均可）。
  类型内部编码为一个整数：`0`=char，`1`=int，`t+2`=指向 `t` 的指针。
* **声明**：全局变量与数组、函数定义、函数原型、逗号分隔的多声明符
  （各声明符共用同一基类型与指针层数，限制见"已知限制"）、块内任意位置的局部声明。
* **语句**：`{}`、`if` / `else`、`while`、`for`、`return`、`break`、
  `continue`、表达式语句、空语句。
* **表达式**：`=`，`||`，`&&`，`|`，`^`，`&`，`==` `!=`，`<` `>` `<=` `>=`，
  `<<` `>>`，`+` `-`，`*` `/` `%`，一元 `- ! ~ * & +`，下标 `a[i]`，
  函数调用（最多 6 个参数）。指针加减会按元素大小缩放，指针相减给出元素个数。
* **字面量**：十进制、`0x` 十六进制、字符（支持 `\n \t \r \0 \\ \' \"`）、字符串。
* **注释**：`//` 与 `/* */`。
* **内建函数**（直接展开为系统调用）：
  `sys_read(fd,buf,n)`、`sys_write(fd,buf,n)`、`sys_open(path,flags,mode)`、
  `sys_close(fd)`、`sys_lseek(fd,off,whence)`、`sys_exit(code)`。
* **入口**：`int main(int argc, char **argv)`，返回值即进程退出码。

### 刻意不支持

预处理器（无 `#include` / `#define`）、结构体、联合、枚举、`typedef`、
`switch`、`goto`、`do-while`、三元 `?:`、逗号运算符、`++` / `--`、
复合赋值、`sizeof`、类型转换、无符号类型、浮点、可变参数、初始化式。

### 代码生成

单遍、直接输出汇编文本，无 AST：

* 值放在 `rax`；`curlv` 标记 `rax` 里是不是"左值的地址"，需要右值时再取内容。
* 二元运算：左操作数 `push`，算右操作数，`mov rcx,rax; pop rax`，再发射运算。
* 栈帧固定 4096 字节，局部量在 `[rbp-8k]`，参数按 SysV 寄存器顺序存入帧内。
* 目标程序布局：0x400000 处单个 RWX 段，`p_memsz` = 0x10000000；
  全局变量放在 0x08000000 起的 BSS 区；字符串字面量作为 `:S<n>` 标签排在代码尾部。
* 跳转与调用全部发射为 `@L<n>` / `@F_<name>` 符号引用，由 hex0 解析，
  因此前向引用天然可用，不需要函数原型。

## 七、目录

```
seed/hexa.hex        手写种子（唯一的人工二进制来源）
stage1/hex0.hasm     阶段 B 汇编器源码（hexa 语言）
stage1/prelude.hasm  x86-64 指令宏
stage1/cc0.hasm      阶段 C 编译器源码（hex0 宏汇编）
stage2/cc.c          最终自举编译器源码
tools/hex2bin.py     hex 文本 → 二进制（仅用于种子；31 行）
tools/hex2bin.c      同一转换器的自举版本
tools/ccc.sh         编译驱动：cc → hex0 → hex2bin
tests/*.c *.exp      功能测试与期望输出
tests/run.sh         测试运行器
build.sh             从种子开始的完整可复现构建
ref/                 出题原帖、提示词与运行结果截图
build/               构建产物（不入库，由 build.sh 生成）
env/                 Python 虚拟环境（不入库）
```

## 八、已知限制

* 每个函数的局部变量总量不得超过 4096 字节（栈帧固定），大缓冲区请用全局数组。
* 函数参数最多 6 个；全局符号 4096 个、每函数局部 1024 个、字符串 8192 条、
  字符串字节总量 1 MB。**其中只有符号个数与栈帧大小会被检查并报错**，
  字符串条数与字节数超限不会得到任何诊断。
* 源文件上限 4 MB，生成的汇编文本上限 16 MB，同样不做检查。
* 局部声明的作用域是整个函数（块内不回收帧空间）。
* 无类型检查：赋值与实参传递只按 8 字节（或 1 字节）搬运。
* **函数调用的结果一律按 `int` 处理**，被调用者声明的返回类型会丢失。
  于是返回指针的函数，其结果不能直接用 `[]`、`*` 或参与指针算术
  （`char *f(); f()[0]` 会被拒；`long *g(); g() + 1` 会按字节而非元素步进）。
  先把结果赋给一个指针变量即可绕开。
* **逗号分隔的声明符共用同一基类型与指针层数**：`long *p, *q;` 会被拒，
  而 `long *p, n;` 会把 `n` 也当成指针。稳妥做法是每行只声明一个名字。
* 编译器与 `hex2bin` 都只做一次 `write` 且不检查返回值：写入失败（例如目标设备已满）
  不会报错，进程仍以 0 退出。

## 九、修改记录

### 2026-09-11 —— 依据 [`review.txt`](review.txt) 的复核结果

一次独立代码复核（全文见 [`review.txt`](review.txt)）重跑了整条链，得到同样的结果
（`cc1 == cc2 == cc3`，md5 `28e0088aa8da1e6a3f9ea3e890583c91`，7 项测试全过），
自举结论不变。其中两条属于**失败检测**缺陷，已修复；另有若干 README 说法宽于实现，已改准。

* **`build.sh` 不再吞掉不动点比对失败。** 原先写作 `cmp A B && echo ...`，
  而 `&&` 左侧失败的命令不受 `set -e` 约束，于是即便自举产物不一致，脚本仍会走完
  并返回 0。现在两处 `cmp` 都是独立命令。验证方式：用一个只在比较 cc2 与 cc3 时
  返回 1 的 `cmp` 桩，构建即在不动点检查处中止，根本走不到测试套件。
* **`tests/run.sh` 开始检查退出码。** 原先只比对输出，程序打印正确却以非零状态退出
  也算通过；转换步骤同样未检查，失败时可能拿旧产物继续跑。验证方式：让某个测试
  照常打印但返回 7，现在会报 `FAIL t01_arith (exit status 7)` 并使脚本返回非零。
* **README 四处说法改准**：逗号声明符的限制、函数返回指针在调用处丢类型、
  哪些容量上限其实没有检查、以及输出写入不检查错误。

复核中未修复、但已写进"已知限制"的问题：调用丢弃被调用者的返回类型、
逗号声明符不各自解析 `*`、字符串缓冲区无边界检查、输出 `write` 不查返回值。
这些都不在 `cc.c` 自身的用法范围内（它不返回指针，每条声明只有一个声明符），
因此不影响自举与不动点。

## 十、许可

代码与文档以 MIT 许可发布，见 [LICENSE](LICENSE)。

`ref/` 目录是例外：其中 `ref/idea.jpeg` 是第三方微博内容的截图，仅作为出处引用收录，
版权归原作者所有，不在 MIT 许可范围内。
