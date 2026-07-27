# Streaming Markdown on Kitty

Model text arrives in chunks. This paragraph has **bold**, *emphasis*, `inline code`, [a link](https://example.com), and inline math: $e^{i\pi}+1=0$ and display math follows.

Emergency wrap check: 12345678901234567890123456789012345678901234567890123456789012345678901234567890 should wrap instead of clipping. Long inline code stays unbreakable: `abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789`.

$$\frac{-b\pm\sqrt{b^2-4ac}}{2a}$$

## Lists and tasks

1. Ordered item one
2. Ordered item two with nested bullets
   - nested bullet A
   - nested bullet B

## Marker styles

- level 1 uses disc marker
  - level 2 uses circle marker
    - level 3 uses square marker
      - level 4 cycles marker styles

- [x] parse CommonMark/GFM task lists
- [ ] migrate renderer into production UI
- [ ] preserve streaming updates on partial blocks
- [ ] a long task item that wraps to a second line so checkbox alignment can be checked against the first line of text

> A block quote can arrive while the model is still generating. It should stay readable and not collapse the layout.

```c
struct morph_md_kitty *renderer = morph_md_kitty_create(&options);
morph_md_kitty_append(renderer, chunk, length, 0);
morph_md_kitty_render(renderer);
```

---

## Dynamic table

| feature | status |
|:---|:---:|
| CommonMark blocks | ok |
| GFM tasklist | ok |
| mathjax-c bitmap | ok |
| dynamic table growth | ok |
| inline formula $a^2+b^2=c^2$ | rendered in cell |
| tall aligned formula | $\begin{aligned}\nabla f(x^*)+A^T\lambda+B^T\mu&=0 \\ Ax^*-b&=0 \\ \mu_i(Bx^*-c)_i&=0\end{aligned}$ |
| valid image ![generated](file:///tmp/morph-markdown-generated.png) | image placeholder in terminal |
| invalid image ![missing](file:///tmp/morph-markdown-missing.png) | error placeholder in terminal |
| code `cell.value()` and [link](https://example.com) | mixed inline |
| tab text | key	value	with tabs |
| long digits | 12345678901234567890123456789012345678901234567890123456789012345678901234567890 |
| long cell | this is a deliberately long table value that should wrap inside the cell while the table can still scroll horizontally |

HTML sample: <span>treated as text by policy</span>

Valid image:

![generated demo](file:///tmp/morph-markdown-generated.png "generated")

Invalid image:

![missing demo](file:///tmp/morph-markdown-missing.png "missing")

# 中文排版 HetiLike

这是一段用于检查中文阅读排版的长段落。今天发布 MorphMarkdown v1.0，它支持 Android/iOS/Kitty、MathJax-C、GFM table 和 task list。中文与 English、数字 12345、`inlineCode()`、[链接](https://example.com) 混排时，需要保持舒适的行高、合理的中西文间距，以及不会挤在一起的视觉节奏。

## 二级标题：中文标题间距

中文段落通常比英文更依赖稳定的行高和段落节奏。HetiLike 主题会使用阅读型标题层级，并让表格、列表、引用都跟正文网格靠齐。

### 三级标题：列表与任务

- 第一层无序列表使用实心圆，适合普通条目。
  - 第二层列表使用空心圆，用于补充说明。
    - 第三层列表使用方块，层级需要清楚但不能太重。
- 中英混排 item：MorphMarkdown 渲染中文、English 和 2026 年的数字。

1. 有序列表第一项，文本比较长时应该自然换行。
2. 有序列表第二项，包含嵌套项目。
   - 嵌套项目 A：中文与 Android 混排。
   - 嵌套项目 B：中文与 iOS 混排。

- [x] 支持中文 HetiLike 主题
- [ ] 检查中文 task list 的 checkbox 与文字基线
- [ ] 这是一条很长的中文任务，用于验证多行换行后，复选框仍然对齐第一行文字而不是挤到中间

> 引用块也需要中文阅读节奏。它不应该像代码块一样紧凑，而是应该保留足够呼吸感，并且在流式输出时保持布局稳定。

```text
中文代码块不参与中西文 spacing：MorphMarkdown	HetiLike	中文排版
```

## 中文表格

| 场景 | 效果 |
|:---|:---|
| 中文长文本 | 这是一段故意写得比较长的中文表格内容，用来验证单元格换行、行高、padding 和横向滚动是否仍然稳定。 |
| 表格内公式 | 勾股定理 $a^2+b^2=c^2$ 应该可以在中文单元格中渲染。 |
| 表格内高公式 | 求根公式 $\frac{-b\pm\sqrt{b^2-4ac}}{2a}$ 只应该撑高当前行，后面的中文仍然保持稳定换行。 |
| 表格内三行公式 | $\begin{aligned}\nabla f(x^*)+A^T\lambda+B^T\mu&=0 \\ Ax^*-b&=0 \\ \mu_i(Bx^*-c)_i&=0\end{aligned}$ |
| 表格内有效图片 | ![generated](file:///tmp/morph-markdown-generated.png) |
| 表格内无效图片 | ![missing](file:///tmp/morph-markdown-missing.png) |
| 中英混排 | MorphMarkdown SDK 在 Android、iOS 和 Kitty 上复用相同的 Markdown 能力。 |

---

#### 四级标题

收尾段落：中文排版不只是换字体，还包括字号、行高、段落间距、列表缩进、表格 padding、代码字号和混排 spacing 的整体组合。
