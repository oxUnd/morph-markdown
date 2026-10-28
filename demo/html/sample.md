# Morph Markdown HTML

支持中文排版、**粗体**、*斜体*、~~删除线~~、`inline code` 和 [链接](https://example.com)。

Inline formula: $E=mc^2$, $\frac{a+b}{c}$, and $x_1+x_2$.

$$\sum_{i=1}^{n}i=\frac{n(n+1)}{2}$$

$$\begin{pmatrix}a&b\\c&d\end{pmatrix}$$

> A blockquote containing $\sqrt{x^2+y^2}$.

- [x] Markdown and GFM
- [x] Native MathML generated from the MathJax-C AST
- [ ] Host syntax coloring

1. First item
2. Second item

| Content | Formula | Code |
| :--- | :---: | ---: |
| 中文表格 | $\frac{1}{2}$ | `a < b` |
| Matrix | $x_i^2$ | `printf("hello")` |

```c
#include <stdio.h>
int main(void) {
    puts("<HTML> & Markdown");
    return 0;
}
```

Footnote example[^note].

[^note]: Footnotes and backlinks share the morph namespace.

<div id="example" class="custom morph-ready"><a href="#example">Raw HTML anchor</a></div>

---

![External image](https://example.com/image.png "Image title")
