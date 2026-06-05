# 4D-движок (интеграция)

Исходники: `4d_logic_for_windows/` (форк [4D-Graphics-Engine](https://github.com/jacksonthall22/4D-Graphics-Engine)).

## Linux

```bash
cd 4d_logic_for_windows
mkdir -p build && cd build
cmake ..
make
./graphics
```

Клавиша `t` — переключение 3D/4D камеры. WASD, Space/Shift, Q/E — движение в 4D.

## В scene_viewer

- Заголовок `inner/headers/fourd_collision.h` — API гиперколлизий (расширяется).
- Полное слияние рендера и физики 4D в `Scene` — следующий этап.

## Коллизии 4D (план)

1. Проекция 4D→3D для отображения (уже в `Camera4D`).
2. Гиперсферы / выпуклые оболочки 4D для broadphase.
3. Столкновение с гиперплоскостями (срез w = const).
4. Связка с `BodyState` и шагом `stepPhysics`.
