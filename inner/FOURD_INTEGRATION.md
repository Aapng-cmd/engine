# 4D в движке

Исходники референса: `4d_logic_for_windows/` (форк [4D-Graphics-Engine](https://github.com/jacksonthall22/4D-Graphics-Engine)).

**Редактор сейчас не показывает 4D:** нет кнопок тессеракта, вкладки «4D Transform», слайсера K. Ядро по-прежнему загружает и считает старые `.scene` с `tesseract` / `hypersphere` / `pyramid4d` / `16cell`.

## Linux (отдельный референс)

```bash
cd 4d_logic_for_windows
mkdir -p build && cd build
cmake ..
make
./graphics
```

Клавиша `t` — переключение 3D/4D камеры. WASD, Space/Shift, Q/E — движение в 4D.

## Что уже в `inner`

| Модуль | Роль |
|--------|------|
| `fourd_math` | `Vec4`, Camera4D, проекция Hollasch, tet-срез `k = const` |
| `fourd_figure` | Каркас политопа, `drawSliced` / `drawProjected` |
| `fourd_collision` | Гиперсферы |
| `scene.h` | `bodiesShareKSlice`, `detectCollision4D`, импульс по `kVel` |

3D-тела живут на **одной плоскости K** (нулевая толщина): разные `kPos` не сталкиваются и не притягиваются. 4D-тело имеет толщину `hyperRadius` вдоль K.

## Viewer

- `T` — камера 3D / 4D (для старых сцен).
- `Q` / `E` при 4D-камере — сдвиг по K.

Редактор эти режимы больше не предлагает.
