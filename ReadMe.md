CPython Parser 分析
---

代码克隆自官方CPython的[V2.7.15](https://github.com/python/cpython/tree/2.7/)版的Parser目录，有删改和目录的变化。

忽略了所有.h文件（INCLUDE_DIRECTORIES引入），只使用.c文件。

---

### 结构分析：

``` text
asdl/   根据Python.asdl文件生成 Python-ast.c（AST树定义）
pgen/   根据Grammar来生成有 graminit.c （有限状态自动机的定义）
* acceler.c    加速器，本质上是一个反向索引，即将在数组中搜索变成根据下标查找。
bitset.c    字符级别的位操作
firstsets.c   计算firstsets，具体干什么的待定
* graminit.c    pgen生成的有限自动状态机的具体内容
grammar.c, grammar1.c    定义了自动机的一些操作
intlcheck.c    检测中断？
listnode.c    打印CST？
metagrammar.c    生成Meta自动机？暂时不知道有什么用？
myreadline.c    从输入流读内容，供分词使用
node.c    CST的节点定义
* parser.c, parser.h    根据token流在有限自动机上跑
* parsetok.c    Parser的对外接口定义
tokenizer.c, tokenizer.h    对python程序进行分词
token00.h   在Include目录下，被我拷贝过来方便查看token信息
main.c    自己添加的可执行程序
```

---

### 大体执行流：（来自pythonrun.c）

以 PyRun_SimpleStringFlags 为例（即使用 -c 命令执行一小段python代码）

``` text
PyRun_SimpleStringFlags
  - PyImport_AddModule  （添加__main__表示应立刻执行）
  - PyRun_StringFlags
    - PyParser_ASTFromString  （根据输入代码生成AST！）
      - PyParser_ParseStringFlagsFilenameEx  （生成CST的node）
        - PyTokenizer_FromString  （对代码进行分词）
          - 一个个字符进行读取并进行分词，主要在tok_get函数
        - parsetok   （一个个token依次加入并生成CST）
          - 依次一个个获得下一个token，对换行等进行处理
          - PyParser_AddToken    （向有限状态自动机内喂下一个token）
            - 【核心：有限状态自动机如何根据下一个token来进行状态的改变】
          - 根据最后自动机的状态，生成CST
      - PyAST_FromNode （从CST生成AST）
        - ...
    - run_mod（实际运行代码！）
      - ...
```

从执行流可见，我们主要关注的是 PyParser_AddToken 函数，即有限状态自动机根据下一个token来进行状态的改变。

---

### 有限状态自动机的定义

有限状态自动机是由 [`pgen/`](pgen) 下的代码，读取 [`Grammar`](pgen/Grammar) 文件所生成。

`Grammar` 中共定义了85个Statement，`pgen/` 对应生成了85个States，和 `Grammar` 一一对应。

从 [`pgen/pgen.c`](pgen/pgen.c) 的介绍可以看出，DFA的生成经过了四步：

 - PART ONE -- CONSTRUCT NFA -- Cf. Algorithm 3.2 from [Aho&Ullman 77]
 - PART TWO -- CONSTRUCT DFA -- Algorithm 3.1 from [Aho&Ullman 77]
 - PART THREE -- SIMPLIFY DFA
 - PART FOUR -- GENERATE PARSING TABLES

这里不对具体的DFA生成过程进行介绍，有兴趣可自行阅读 `pgen/pgen.c` 源码。

这里只重点关注一下生成结果 `graminit.h` 和 `graminit.c`。

#### graminit.h：State -> 数字的映射

[`graminit.h`](pgen/graminit.h) 将 State 到其代表的数字进行了一一映射。

``` c
#define single_input 256
#define file_input 257
...
#define funcdef 262
#define parameters 263
...
#define stmt 267
...
#define encoding_decl 339
#define yield_expr 340
```

可以看到，每个状态都被唯一映射到了一个数字，且从256开始（为什么是256之后会有说明）。

反之，也能知道每个数字所对应的状态。

#### graminit.c：具体的自动机内容

[`graminit.c`](pgen/graminit.c) 定义了自动机的具体内容。它由 [`pgenmain.c`](pgen/pgenmain.c) 生成，分成三个部分。

**Part 1：状态和边的邻接表定义**

第一段是对于状态和边的邻接表定义，其主要结构是：

``` c
static arc arcs_0_0[3] = {
    {2, 1},
    {3, 1},
    {4, 2},
};
static arc arcs_0_1[1] = {
    {0, 1},
};
static arc arcs_0_2[1] = {
    {2, 1},
};
static state states_0[3] = {
    {3, arcs_0_0},
    {1, arcs_0_1},
    {1, arcs_0_2},
};
```

从 [`printgrammar.c`](pgen/printgrammar.c#L62) 可以看出，对于每个状态，首先输入所有边，再输出状态。

每个状态 `states` 都是存在不同的子状态 `d_stats` 。如上面的 `states_0` 就有三个子状态 `states_0_0~2`，分别对应边集合 `arcs_0_0~2`。

每个边集合 `arcs` 包含若干个具体的边 `{label, target}`，其中前者为边的当前符号（也就是token对应的数字），后者为终点的状态ID。

上面这个例子就说明：
 - `states_0` 存在三个子状态：`states_0_0, states_0_1, states_0_2`
 - `states_0_0` 的边集合 `arcs_0_0` 存在三条边：
   - `{2,1}` 如果碰到一个为2的token，则转到状态1（即`states_1`）
   - `{3,1}` 如果碰到一个为3的token，则转到状态1（即`states_1`）
   - `{4,2}` 如果碰到一个标签为4的token，则转到状态2（即`states_2`）

通过这种邻接表的定义方式，就可以知道，如果当前是哪个状态（的子状态），碰到一个标签数字为多少的token，则跳转到哪个状态。

**Part 2：自动机状态的详细信息**

第二段是对于每个状态的详细信息。

``` c
static dfa dfas[85] = {
    {256, "single_input", 0, 3, states_0,
     "\004\050\060\000\000\000\000\124\360\024\114\220\023\040\010\000\200\041\044\015\002\001"}
}
```

从 [`printgrammar.c`](pgen/printgrammar.c#L62) 可以看出，每一项定义了一个状态。

``` c
for (i = 0; i < g->g_ndfas; i++, d++) {
    fprintf(fp, "    {%d, \"%s\", %d, %d, states_%d,\n",
        d->d_type, d->d_name, d->d_initial, d->d_nstates, i);
    fprintf(fp, "     \"");
    for (j = 0; j < NBYTES(g->g_ll.ll_nlabels); j++)
        fprintf(fp, "\\%03o", d->d_first[j] & 0xff);
    fprintf(fp, "\"},\n");
}
```

第一项是该状态对应的数字，从256开始，和`graminit.h`完全一致的；第二项是该状态的名称；第三项是该状态的初始子状态；第四项是该状态的子状态数。

第五项是状态名（在上面状态和边的邻接表定义中被定义）。

第5项是是个22个8进制数字，和firstsets有关，具体使用方式尚不明确。

**Part 3：token标签定义**

第三段是对于token的标签进行定义。

每个token都有唯一的一个数字（定义在`token.h`中），而一个token又会被映射到一个或多个LABEL。

``` c
static label labels[169] = {
    // ...
    {278, 0},
    {280, 0},
    {279, 0},
    {1, "break"},
    {1, "continue"},
    {1, "return"},
    {1, "raise"},
    {282, 0},
    {283, 0},
    {1, "import"},
    {287, 0},
    // ...
}
```

每一行`{x, y}`定义了一个符号。其中x为符号的数字。

**小于256的视为`TERMINAL`（终结符），大于256的视为`NON-TERMINAL`（非终结符）。**

```
#define NT_OFFSET		256
#define ISTERMINAL(x)		((x) < NT_OFFSET)
#define ISNONTERMINAL(x)	((x) >= NT_OFFSET)
```

第二项为该标签的对应关键词`keyword`内容，如果不为0则为一个字符串，且对应的第一项一定是1（`NAME`的`TERMINAL`）。可以利用这个来检测某个token是否是关键词。

### 有限状态自动机的执行流程



### 加速器accelerator




