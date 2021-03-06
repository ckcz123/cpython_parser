# CPython Parser 分析

代码克隆自官方CPython的[V2.7.15](https://github.com/python/cpython/tree/2.7/)版的Parser目录，有删改和目录的变化。

忽略了所有.h文件（INCLUDE_DIRECTORIES引入），只使用.c文件。

---

## 结构分析：

``` text
asdl/   根据Python.asdl文件生成 Python-ast.c（AST树定义）
pgen/   根据Grammar来生成有 graminit.c （有限状态自动机的定义）
* acceler.c    加速器，本质上是一个反向索引，即将在数组中搜索变成根据下标查找。
bitset.c    字符级别的位操作
firstsets.c   计算firstsets，即每个状态的可进入token
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

## 大体执行流：（来自pythonrun.c）

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

## 有限状态自动机的定义

有限状态自动机是由 [`pgen/`](pgen) 下的代码，读取 [`Grammar`](pgen/Grammar) 文件所生成。

`Grammar` 中共定义了85个Statement，`pgen/` 对应生成了85个States（非终结符），一一对应。

从 [`pgen/pgen.c`](pgen/pgen.c) 的介绍可以看出，DFA的生成经过了四步：

 - PART ONE -- CONSTRUCT NFA -- Cf. Algorithm 3.2 from [Aho&Ullman 77]
 - PART TWO -- CONSTRUCT DFA -- Algorithm 3.1 from [Aho&Ullman 77]
 - PART THREE -- SIMPLIFY DFA
 - PART FOUR -- GENERATE PARSING TABLES

这里不对具体的DFA生成过程进行介绍，有兴趣可自行阅读 `pgen/pgen.c` 源码。

这里只重点关注一下该自动机的生成结果。

### 终结符与非终结符

DFA是存在终结符（不可再今夕任何推导的符号）和非终结符的。

在CPython的parser里，将**0\~256以下的数字视为终结符，256以上的均视为非终结符**。

``` c
#define NT_OFFSET		256
#define ISTERMINAL(x)		((x) < NT_OFFSET)
#define ISNONTERMINAL(x)	((x) >= NT_OFFSET)
```

终结符在 [`token.h`](token00.h) 中定义：

``` c
#define ENDMARKER	0
#define NAME		1
#define NUMBER		2
#define STRING		3
...
#define LESS		20
...
#define DOUBLESLASHEQUAL 49
#define AT              50
```

非终结符则在 [`graminit.h`](pgen/graminit.h) 中定义：

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

每个非终结符都和自动机的一个中间状态相对应。具体自动机的内容在下面有详细介绍。

### 状态，子状态，边，状态栈

**每个非终结符对应一个状态，且每个状态都对应若干个子状态，子状态之间用边相连。**

子状态描述的是**当前该状态走到了哪个位置**，边描述的是**从该子状态到下一个子状态应当怎么走**。

另外，整个程序还在维护一个状态栈。当从某个状态进入到其他状态时需要进行压栈操作，从其他状态离开时需要进行出栈操作。

### graminit.c：具体的有限状态自动机的内容

[`graminit.c`](pgen/graminit.c) 定义了自动机的具体内容。它由 [`pgenmain.c`](pgen/pgenmain.c) 生成，分成三个部分。

#### Part 1：状态和边的邻接表定义

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

每个边集合 `arcs` 包含若干个具体的边 `{label, target}`，其中前者为LABEL的索引，后者为终点的子状态。

上面这个例子就说明：
 - `states_0` 存在三个子状态：`states_0_0, states_0_1, states_0_2`
 - 子状态0 `states_0_0` 的边集合 `arcs_0_0` 存在三条边：
   - `{2,1}` 如果碰到一个LABEL索引为2的token，则转到子状态1（即`states_0_1`）
   - `{3,1}` 如果碰到一个LABEL索引为3的token，则转到子状态1（即`states_0_1`）
   - `{4,2}` 如果碰到一个LABEL索引为4的token，则转到子状态2（即`states_0_2`）

通过这种邻接表的定义方式，就可以知道，如果当前是该状态的哪个的子状态，碰到一个标签索引是啥的token，则应跳转到哪个子状态。

#### Part 2：状态的详细信息，和非终结符的对应关系

第二段是对于每个状态的详细信息。

``` c
static dfa dfas[85] = {
    {256, "single_input", 0, 3, states_0,
     "\004\050\060\000\000\000\000\124\360\024\114\220\023\040\010\000\200\041\044\015\002\001"}
}
```

从 [`printgrammar.c`](pgen/printgrammar.c#L62) 可以看出，每一项定义了一个状态（且和一个非终结符一一对应）。

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

第一项为该状态的数字，从256开始（表示是非终结符）。第二项代表该状态的非终结符的名称（如`single_input`）等。

第三项为该状态的初始子状态，第四项为该状态的子状态数，第五项为状态名（在上面状态和边的邻接表中被定义的）。

最后一项是是个22个8进制数字，和firstsets有关，表示哪些LABEL可以进入该状态。

上面的例子说明：**`single_input` 这个非终结符对应的数字是256，对应的自动机状态是states_0，且存在3个子状态，初始子状态是0。**

#### Part 3：token标签（LABEL）定义

第三段是对于token的标签进行定义。

``` c
static label labels[169] = {
    {0, "EMPTY"},
    {256, 0},
    {4, 0},
    ...
    {1, "import"},
    {287, 0},
    ...
    {339, 0},
    {1, "yield"}
}
```

LABEL共有 `169=31+53+85` 项，每一项其必为 `{x, y}` 形式，且每一项一定对应下列三种情况之一：
 - 关键字 `{1, keyword}`，如 `{1, "break"}`。由于关键字一定是 `NAME`（终结符），所以第一项一定是1，第二项为关键字的内容。
 - 终结符 `{x, 0}`，其中x为终结符对应的数字（小于256）。如 `{23, 0}` 对应的是`DOT`终结符。
 - 非终结符 `{y, 0}`，其中y为非终结符对应的数字（大于等于256）。如 `{268, 0}` 对应的是 `simple_stmt` 非终结符。

**该LABEL的索引（index）对应的是上面边里面的 `{label, target}` 中的第一项。**

### 实例：`with_stmt`

现在重点以 `with_stmt` 为例详细介绍一下整个流程。

`with_stmt` 在 `Grammar` 中的定义是 `with_stmt: 'with' with_item (',' with_item)*  ':' suite`。

其在自动机中对应的是 `states_41`。

``` c
{297, "with_stmt", 0, 5, states_41,
 "\000\000\000\000\000\000\000\000\000\000\000\000\020\000\000\000\000\000\000\000\000\000"}
```

它有五个子状态：
 - 子状态0 `states_41_0`：即将进入 `with_stmt` 语句。
 - 子状态1 `states_41_1`：已经接受了 `'with'` 关键字，即将进入 `with_item`。
 - 子状态2 `states_41_2`：已经接受了 `with_item`，即将进入 `(',' with_item)*` 或者 `':'`。
 - 子状态3 `states_41_3`：已经接受了 `':'`，即将进入`suite`。
 - 子状态4 `states_41_4`：已经接受了 `suite`。

``` c
static arc arcs_41_0[1] = {
    {100, 1},
};
static arc arcs_41_1[1] = {
    {101, 2},
};
static arc arcs_41_2[2] = {
    {29, 1},
    {23, 3},
};
static arc arcs_41_3[1] = {
    {24, 4},
};
static arc arcs_41_4[1] = {
    {0, 4},
};
static state states_41[5] = {
    {1, arcs_41_0},
    {1, arcs_41_1},
    {2, arcs_41_2},
    {1, arcs_41_3},
    {1, arcs_41_4},
};
```

这段定义说明了：
 - 当前子状态为0时：
   - 接收一个索引为100的LABEL（即`{1, "with"}`），则进入子状态为1
 - 当前子状态为1时：
   - 接受一个索引为101的LABEL（即`{298, 0}`，对应的是非终结符 `with_item`），则进入子状态为2
 - 当前子状态为2时：
   - 接受一个索引为29的LABEL（即`{12, 0}`，对应的是终结符 `COMMA`，即`,`），则回到子状态为1
   - 接收一个索引为23的LABEL（即`{11, 0}`，对应的是终结符 `COLON`，即`:`），则进入子状态为3
 - 当前子状态为3时：
   - 接收一个索引为24的LABEL（即`{300, 0}`，对应的是非终结符 `suite`），则进入子状态为4
 - 当前子状态为4时：
   - 接收换一个索引为0的LABEL（即`{0, "EMPTY"}`），则标记为`ACCEPTED`，并且仍然回到子状态4

以下面是一个最简单的代码为例：

``` python
with a, b:
    pass
```

状态变化大致是：

``` text
            with         a          ,          b          :        pass
PUSH -> s0 ------> s1 ------> s2 ------> s1 ------> s2 ------> s3 ------> s4 -> POP
```

下面是代码实际输出的结果，删减了中间的一些的信息，可以重点看标注出来的几句话。和上面的分析是完全一致的。

``` text
Token NAME/'with' ... It's a keyword
 DFA 'file_input', state 0: Push ...
 DFA 'stmt', state 0: Push ...
 DFA 'compound_stmt', state 0: Push ...
 DFA 'with_stmt', state 0: Shift.        <------
Token NAME/'a' ... It's a token we know
 DFA 'with_stmt', state 1: Push ...      <------
 DFA 'with_item', state 0: Push ...
 ......
 DFA 'power', state 0: Push ...
 DFA 'atom', state 0: Shift.
  DFA 'atom', state 5: Direct pop.
Token COMMA/',' ... It's a token we know
 DFA 'power', state 1: Pop ...
 ......
 DFA 'with_item', state 1: Pop ...
 DFA 'with_stmt', state 2: Shift.        <------
Token NAME/'b' ... It's a token we know
 DFA 'with_stmt', state 1: Push ...      <------
 DFA 'with_item', state 0: Push ...
 ......
 DFA 'atom', state 0: Shift.
  DFA 'atom', state 5: Direct pop.
Token COLON/':' ... It's a token we know
 DFA 'power', state 1: Pop ...
 ......
 DFA 'with_item', state 1: Pop ...
 DFA 'with_stmt', state 2: Shift.        <------
Token NEWLINE/'' ... It's a token we know
 DFA 'with_stmt', state 3: Push ...      <------
 DFA 'suite', state 0: Shift.
Token INDENT/'' ... It's a token we know
 DFA 'suite', state 2: Shift.
Token NAME/'pass' ... It's a keyword
 DFA 'suite', state 3: Push ...
 ......
  DFA 'small_stmt', state 1: Direct pop.
Token NEWLINE/'' ... It's a token we know
 DFA 'simple_stmt', state 1: Shift.
  DFA 'simple_stmt', state 3: Direct pop.
  DFA 'stmt', state 1: Direct pop.
Token DEDENT/'' ... It's a token we know
 DFA 'suite', state 4: Shift.
 ......
  DFA 'with_stmt', state 4: Direct pop.   <------
 ......
Token NEWLINE/'' ... It's a token we know
 DFA 'file_input', state 0: Shift.
Token ENDMARKER/'' ... It's a token we know
 DFA 'file_input', state 0: Shift.
  DFA 'file_input', state 1: Direct pop.
  ACCEPT.
```

## 有限状态自动机的大致执行流程

其实上面以 `with_stmt` 为例，已经解释的比较详细了。

整个执行过程会维护一个"状态栈"，当需要进入某个状态时，将该状态进行压栈，然后离开某个状态时再出栈。

下面是整个执行过程的核心，在 `parser.c` 的 `PyParser_AddToken` 中被执行。

1. 初始化整个状态栈为空。
2. 每来了一个新的token（终结符）后，在LABEL数组中检索该token的索引。
3. 获得自动机状态栈顶的子状态，并对其引出边进行遍历：
    - **如果该边需要的索引就是此终结符的索引，则接受该token，并改变子状态。**
    - **如果该边需要的索引是一个非终结符，则检测该非终结符的`firstsets`：**
        - **如果包含了该token的索引（代表可以从该token进入此状态），则压栈，并继续处理该token。**
    - **如果该边需要的索引是0（`EMPTY`），则表示该状态可以结束，出栈，回到上个状态继续处理该token。**
    - **如果上述都不成立，则报错（不可接受的token）。**
4. token流结束后，再将所有内容进行出栈并检查是否到了`EMPTY`，如果没有则报错（某个状态未完成）。
5. 当所有元素都出栈后，标记`ACCEPT`，表示该token流已被完全状态机所接受。

## 加速器 `accelerator`

上面的流程固然可以，但是仍然存在两个性能瓶颈：

- 根据token（终结符）在LABEL数组中查询其索引
- 检测边，如果是非终结符，还需要再检测其firstsets中是否包含

前者并没有进行优化处理（总共也只有169项，对该token遍历一遍也很快）。

对于后者，代码中使用了 **加速器 `accel`** 来进行处理。详见 `acceler.c`。

它给每个子状态增加了一个`accel`数组。

``` c
accel[key] = value
```

key对应的是该token终结符的数字（进一步利用s_lower和s_upper进行压缩空间，只保存可能是有效范围内的。）

value对应的是当前子状态遇到该token终结符后应该进行的行为：

- 如果小于`1<<7=128`，则视为"该状态应接受此终结符，并应该跳转到哪个子状态"
- 如果大于等于`1<<7=128`，则表示下一个为非终结符，且会分为三个部分
  - 最低的六位：接受该为非终结符符后应该跳到的状态
  - 第七位：一定是1，表示是非终结符
  - 第八位及以上：表示目标非终结符的数字-256的结果

``` text
accel[ibit] = a->a_arrow | (1 << 7) | ((type - NT_OFFSET) << 8);

低 -----------> 高
0000000 1 xxxxxxx
<-----> | <----->
目标子状态 应压栈的非终结符
```

另外，由于某个子状态并不是针对所有的终结符都能接受并转化的（往往只有一部分），因此无需把s_accel开到169这么大。可以存下来能接受的"第一个token"和"最后一个token"（记为 `s_lower` 和 `s_upper` ），然后只开这个区域的范围内的数组大小；实际计算时减去s_lower即可，从而减少内存空间的开销。

仍然以下面代码为例：

``` python
with a, b:
    pass
```

当我们接受`with`后（进入子状态为1），下一个token为`a`到来时：

算的`a`对应的token是`NAME`，索引是21。由于此时`s_lower=13, s_upper=156`，因此应查s_accel的第8项。

查询 `s->accel[8]` 后得到的x值为`10882 = (10101010000010)_2`。

`x & (1<<7) != 0`，因此表示这是一个非终结符。

该终结符的编号是 `(x >> 8) + NT_OFFSET = (101010)_2 + 256 = 42 + 256 = 298`，即`with_item`。

之后跳转到的子状态为 `x & ((1<<7) - 1) = 2`，即子状态2。

因此，应该执行的操作是：**将 `with_item` 压栈，之后将当前子状态变成2。**

使用加速器后，对于任何一个子状态遇到的的任何一个终结符，我们无需再去依次给每条边查询是否应该接受，如果是非终结符则`firstsets`中是否包含该终结符等，从而大大提升运行效率。

## 未完成代码的下一个token类型的预测

我们的目的是，根据输入的代码，预测下一个token的类型，比如是关键词？identifier？运算符？换行/缩进？……

有两种方式可以做到这一点：**遍历所有的终结符**，或**根据当前自动机的状态进行计算**。

### 遍历所有的终结符

这种想法比较好理解。所有不同的终结符类型是有限的。

但是，需要对所有类型的终结符（53个），和所有的关键字（31个）都拿出来单独计算，计算次数较多（共计84次）。

#### 方式一：直接加到原始序列后面，重复计算

1. 将每个类型的终结符分别加到原始序列后面
2. 从头算整个自动机，看看是否报错；如果不报错则视为有效。

这种方式最简单，但是存在大量的重复计算，得不偿失。

#### 方式二：保存自动机状态

1. 对输入的代码分词完毕后扔进有限状态自动机机内，然后进行运算到当前的子状态。
2. 保存当前自动机的状态。
3. 对所有类型的token进行计算，看看是否出错；如果不出错就认为该token是合法的。

这种方式的难点是，要完全保存当前自动机的状态，并且能否进行恢复。本质上就是要把整个状态栈保存下来。

### 根据当前自动机的状态进行计算

这种方法无需对所有的都进行尝试，只需要根据当前自动机的状态来进行计算所有合法的token。

主要流程如下：

1. 对输入的代码分词完毕后扔进有限状态自动机机内，然后进行运算到当前的子状态。
2. 检测当前子状态的所有输出边，如果token是终结符，则加入到合法token列表中。
3. 如果当前子状态是可被接受的（`s_accepted=true`），则将其弹出，然后回到2重新运算。
4. 如果栈全部被弹出，则也可视为一段完整的程序（即`ENDMARKER`）。

这种方式的优势是，无需保存自动机的状态，也无需对所有终结符都拿出来单独计算，而且能够一次性得到结果。

实现可参见 [predict.c](predict.c) 。

## 可能遇到的问题

### 输入代码的可靠性

使用这个自动机，需要保证整个输入代码是完整有效的，能让整个自动机完整运行到断点处。

如果之前出现了错误，则会在出错的地方就让自动机停下，不会再继续计算后面的内容。

要解决这个方式，可以尝试只将"当前行"喂给自动机而不是之前的全部代码；如果当前行不合法（比如是前一行的断行），则再继续将前一行加入，依次类推即可。

