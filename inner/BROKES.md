1. malloc(): unaligned tcache chunk detected in scene_viewer
2. cone boundings are wrong
3. cone is broken completely
4. "opacity=2" does not work as it should (it should reflect light)
5. reflecting light sometimes breaks

Примечание: в редакторе Opacity — слайдер **0.00–1.00** (непрозрачность). Значения PHYS `alpha` в диапазоне 1–2 по-прежнему пишутся в файл как «отражение», но UI их больше не выставляет.
