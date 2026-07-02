# TUTORIAL — физико-математико-программное пособие по движку `engine`

Полный разбор исходников проекта: от векторной алгебры до столкновений, текстур, 4D-проекции и автотестов.  
Аудитория: **от начинающего** (что за файл и зачем) **до опытного** (формулы импульсов, эвристики LOD, структура тестов).

---

## Оглавление

1. [Введение и карта репозитория](#1-введение-и-карта-репозитория)
2. [Стек технологий и поток данных](#2-стек-технологий-и-поток-данных)
3. [Глава I. Векторы и базовая математика](#3-глава-i-векторы-и-базовая-математика)
4. [Глава II. Фигуры, рендер и окружение](#4-глава-ii-фигуры-рендер-и-окружение)
5. [Глава III. Текстуры, материалы и отражения](#5-глава-iii-текстуры-материалы-и-отражения)
6. [Глава IV. Коллизии — меши, контакты, разбиения](#6-глава-iv-коллизии--меши-контакты-разбиения)
7. [Глава V. Физика — интеграция, импульсы, группы](#7-глава-v-физика--интеграция-импульсы-группы)
8. [Глава VI. Сцена, загрузчик и главный цикл](#8-глава-vi-сцена-загрузчик-и-главный-цикл)
9. [Глава VII. Четырёхмерная геометрия](#9-глава-vii-четырёхмерная-геометрия)
10. [Глава VIII. Редактор сцен (outer)](#10-глава-viii-редактор-сцен-outer)
11. [Глава IX. Автотесты коллизий](#11-глава-ix-автотесты-коллизий)
12. [Глава X. Референс `4d_logic_for_windows`](#12-глава-x-референс-4d_logic_for_windows)
13. [Формат файла `.scene`](#13-формат-файла-scene)
14. [Сборка, запуск, отладка](#14-сборка-запуск-отладка)
15. [Справочник: каждый `.h` / `.cpp` файл](#15-справочник-каждый-h--cpp-файл)

---

## 1. Введение и карта репозитория

Проект — **3D/4D физический песочник** на OpenGL 1.x: объекты падают, сталкиваются, вращаются; есть редактор с превью и просмотрщик в реальном времени.

```
driver_test/
├── inner/                 # Ядро: scene_viewer, физика, коллизии
│   ├── headers/           # Заголовки (.h)
│   ├── source/            # Реализации (.cpp)
│   ├── tests/collision/   # 6 автотестовых бинарников
│   ├── *.scene            # Сцены
│   └── textures/          # (или ../textures/)
├── outer/                 # Qt-редактор scene_editor
│   ├── headers/
│   └── source/
├── textures/              # Общая папка текстур
└── 4d_logic_for_windows/  # Исходный 4D-референс Jackson Hall (отдельная сборка)
```

**Два исполняемых файла:**

| Программа | Путь | Назначение |
|-----------|------|------------|
| `scene_viewer` | `inner/scene_viewer` | Просмотр + физика + HUD |
| `scene_editor` | `outer/build/scene_editor` | Редактирование `.scene`, превью, сборка viewer |

**Не документируем подробно** (служебные / сторонние):

- `outer/scene_editor_autogen/` — автогенерация Qt MOC
- `inner/stb/stb_image.h` — библиотека загрузки изображений (Sean Barrett)
- `outer/build/`, объектные `.o` — артефакты сборки

---

## 2. Стек технологий и поток данных

### 2.1. Стек

| Слой | Технология |
|------|------------|
| Язык | C++17 (совместимо с C++14+) |
| Графика | OpenGL 1.x fixed pipeline, GLUT, GLU |
| Сборка inner | Makefile |
| Сборка outer | CMake + Qt5 (`QOpenGLWidget`) |
| Физика | Собственная: semi-implicit Euler, 4 substep/кадр |
| Коллизии | Сферы + аналитика (box/cyl/torus) + треугольные меши |
| 4D | `fourd_math`: проекция 4D→3D, K-ось в физике |

### 2.2. Поток данных (от файла до кадра)

```mermaid
flowchart TB
    SCENE[".scene файл"]
    LOADER["scene_loader.cpp / SceneFile.cpp"]
    FACTORY["object_factory.cpp"]
    SCENE_OBJ["Scene: Objects + ObjectPhysics"]
    REBUILD["Scene::rebuildBodies()"]
    BODIES["BodyState[]"]
    STEP["Scene::stepPhysics(dt)"]
    RENDER["Scene::Render(t)"]

    SCENE --> LOADER --> FACTORY --> SCENE_OBJ
    SCENE_OBJ --> REBUILD --> BODIES
    BODIES --> STEP --> BODIES
    BODIES --> RENDER
```

**На каждом кадре:**

1. `animation::Display` обновляет LOD (`render_settings`) и вызывает `Scene::Render(t)`.
2. `Render` интегрирует физику (`stepPhysics`), рисует небо/пол/объекты.
3. Текстуры перепривязываются через `ensureSceneTexturesLoaded()` при первом GL-контексте.

---

## 3. Глава I. Векторы и базовая математика

### 3.1. Файл `inner/headers/vector.h`

Единственный «чистый» математический заголовок. Шаблон `vec<Type, N>` хранит **три** компоненты `x, y, z` (параметр `N` исторический, не используется).

#### Операции

| Метод / оператор | Формула |
|------------------|---------|
| `len2()` | \(\|v\|^2 = x^2 + y^2 + z^2\) |
| `len()` | \(\|v\| = \sqrt{\|v\|^2}\) |
| `dot(v)` | \(v \cdot w = x w_x + y w_y + z w_z\) |
| `operator^` (cross) | \(a \times b = (a_y b_z - a_z b_y,\; a_z b_x - a_x b_z,\; a_x b_y - a_y b_x)\) |
| `operator!` | Нормализация: \(v / \|v\|\), нулевой вектор при \(\|v\|=0\) |
| `operator*` / `/` | Покомпонентное умножение/деление на скаляр |

#### Иллюстрация: скалярное и векторное произведение

```
        z
        |   w
        |  /
        | /
        +-------- y
       /
      /
     x

dot(a,b) = |a| |b| cos(θ)     — проекция a на b
a × b    ⟂ плоскости (a,b)   — нормаль к плоскости
```

#### Вспомогательные функции

- `sign(A)` — знак числа (−1 или +1).
- `vec::Rnd()`, `RndCol()` — случайные точки/цвета (для демо-структур в `templates.h`).

**Зависимости:** только STL + `<cmath>`. Все остальные модули строятся на `vec<>`.

### 3.2. Матрицы 4×4 и frustum culling (`scene.h`)

OpenGL fixed pipeline хранит матрицы в **column-major** порядке (16 `double` в массиве `mat[16]`).

#### Умножение точки на матрицу вида/проекции

`transformPoint(p, mat)`:

\[
\begin{pmatrix} x' \\ y' \\ z' \end{pmatrix}
=
\begin{pmatrix}
m_0 & m_4 & m_8  & m_{12} \\
m_1 & m_5 & m_9  & m_{13} \\
m_2 & m_6 & m_{10} & m_{14}
\end{pmatrix}
\begin{pmatrix} x \\ y \\ z \\ 1 \end{pmatrix}
\]

(В коде индексация column-major: столбец `j` → `mat[j*4 + i]`.)

#### Умножение матриц

`multiplyMatrices(a, b, out)` — классическое \(4\times4\) произведение для построения `viewProj = P · V`.

#### Frustum: 6 плоскостей от проекции

`extractFrustumPlanesFromProj(planes, proj)` вытаскивает левую/правую/верхнюю/нижнюю/ближнюю/дальнюю плоскости из комбинированной матрицы.

`sphereInFrustum(planes, center, radius)` — для каждой плоскости проверяется

\[
n \cdot c + d \ge -r
\]

Если сфера bounding-объекта целиком с одной стороны хотя бы одной плоскости — объект **не рисуется** (early cull).

```
        far plane
           ___
          /   \
   left /     \ right
        |  cam  |
         \     /
          \___/
        near plane

Сфера (center, radius) должна пересекать все 6 полупространств «внутри» пирамиды.
```

Это единственное место в ядре с полноценными матрицами; повороты тел и камеры в остальном коде — **векторы + Rodrigues / glRotated**, без накопления матриц на CPU.

---

## 4. Глава II. Фигуры, рендер и окружение

### 4.1. `inner/headers/figures.h` + `inner/source/figures.cpp`

#### Базовый класс `based`

Абстрактная «рисуемая сущность»:

| Поле | Смысл |
|------|--------|
| `renderAlpha` | Прозрачность + (при >1) сила отражения — см. [главу III](#5-глава-iii-текстуры-материалы-и-отражения) |
| `reflectAmount` | Декомпозиция из alpha |
| `textureID` | OpenGL id текстуры (0 = только цвет) |

**Виртуальные методы:**

- `Draw(double t)` — полная отрисовка с позицией объекта.
- `getBoundingSpheres(...)` — сферы для грубых границ и пикинга.
- `emergency_bounding_sphere_calc_protocol()` — запасной радиус.

#### Примитивы `Solid*`

Локальные примитивы в начале координат (без явного `pos`):

| Класс | Параметры | Отрисовка |
|-------|-----------|-----------|
| `SolidSphere` | `radius` | `gluSphere` / `glutSolidSphere` |
| `SolidCube` | `size` | `glutSolidCube` |
| `SolidCylinder` | `radius`, `height` | `gluCylinder` |
| `SolidCone` | `radius`, `height` | `gluCylinder` (конус) |
| `SolidPyramid` | `base`, `height` | Ручные треугольники |
| `SolidTorus` | `innerR`, `outerR` | `glutSolidTorus` |

`figures.cpp` только **определяет статические** `GLUquadric*` для конуса/цилиндра/сферы.

#### Окружение

**`GroundPlane`** — горизонтальная плоскость Y=0:

- Текстурированный квад с UV 0…1.
- `setReflect(amount, isWater)` — зеркальность для воды.

**`SkySphere`** — большая сфера «изнутри» с текстурой неба.

#### Namespace `physmath` — объёмы (для оценки массы)

\[
V_{\text{sphere}} = \tfrac{4}{3}\pi r^3,\quad
V_{\text{box}} = 2h_x \cdot 2h_y \cdot 2h_z,\quad
V_{\text{cyl}} = \pi r^2 h,\quad
V_{\text{cone}} = \tfrac{1}{3}\pi r^2 h
\]

#### Вспомогательные повороты

`rotateX/Y/Z(angle_rad)` — матрицы Эйлера вокруг одной оси (используются в bounding sphere merge).

`mergeSpheres(a, ra, b, rb)` — минимальная сфера, покрывающая две сферы (центр на линии между центрами).

---

### 4.2. `inner/headers/manual_shapes.h` + `inner/source/manual_shapes.cpp`

**Редакторские примитивы** с явной трансформацией:

| Класс | Поля |
|-------|------|
| `EditorSphere` | `pos`, `scale`, `rx,ry,rz`, `radius`, `color` |
| `EditorBox` | + `dx,dy,dz` (размеры) |
| `EditorCylinder` | + `baseRadius`, `height` |
| `EditorTorus` | + `innerR`, `outerR` |

#### Порядок трансформации при отрисовке

```
Мировая позиция задаётся физикой (Scene::drawObjectRigidBody).
Локально в drawLocal():

  T(pos) · Rz(ry) · Ry(rx) · Rx(rz) · S(sx,sy,sz) · примитив
```

(В коде: `applyRot(rx, ry, rz)` — повороты в градусах через `glRotated`.)

#### Текстурированный куб

`drawBoxUnitCubeTextured()` рисует **6 граней** с `glTexCoord2f` — UV от 0 до 1 на каждой грани. Это эталон того, как текстура «обёрнута» на боксе.

#### Сфера с текстурой

При `textureID != 0` используется **`gluSphere`** (генерирует UV), иначе `glutSolidSphere` (быстрее, без UV).

---

### 4.3. `inner/headers/transform_wrapper.h` + `inner/source/transform_wrapper.cpp`

Обёртка **TRS** (Translation–Rotation–Scale) над дочерним `based*`:

```
Мир: p_world = pos + R·(scale ⊙ p_local)
```

**Функции:**

| Функция | Назначение |
|---------|------------|
| `TransformWrapper::drawLocal` | Применяет TRS, вызывает `child->drawLocal` |
| `getBoundingSpheres` | Сэмплирует 6 осевых точек локального AABB → мировые сферы |
| `setFigureRenderAlpha(o, a)` | Рекурсивно выставляет alpha на обёртке и ребёнке |

Используется для `solid_cube`, `cone`, `pyramid` и составных фигур из редактора.

---

### 4.4. `inner/headers/object_factory.h` + `inner/source/object_factory.cpp`

**Фабрика объектов** из строки типа сцены.

| Строка `type` | Создаётся |
|---------------|-----------|
| `sphere` | `EditorSphere` |
| `cube`, `box` | `EditorBox` |
| `solid_cube` | `SolidCube` в `TransformWrapper` |
| `cylinder`, `torus` | `EditorCylinder`, `EditorTorus` |
| `cone`, `pyramid` | `Solid*` в обёртке |
| `tesseract`, `hypersphere`, `pyramid4d` | `FourDWireFigure` |

**Функции:**

- `createSceneObject(type, px…rz, extra[], tex, err)` — главная точка входа.
- `expectedExtraCount(type)` — сколько чисел в `extra` обязательно.
- `isComplexFigureType` — зарезервировано (сейчас всегда false).
- `shapeUsesTriangleCollision(obj)` — делегирует в `collisionReprForObject`.

---

### 4.5. `inner/headers/render_settings.h`

**LOD тесселяции** для GLUT/GLU (не для физики напрямую):

```cpp
rs::ed_sph_slc, rs::ed_sph_stk   // сфера
rs::ed_tor_s, rs::ed_tor_r       // тор (визуальный)
rs::sky_slc, rs::sky_stk         // небо
```

`setLodFromCameraDistance(dist)`:

\[
q = 1.25 - \mathrm{clamp}(d/120,\ 0,\ 0.9),\quad
\text{segments} = \max(\text{minSeg},\ \text{base} \cdot q)
\]

Чем дальше камера — тем меньше полигонов в **отрисовке** (экономия на слабом GPU).

---

### 4.6. `inner/headers/templates.h`

**Наследие демо:** иерархия `voxel` → `plate` / `line` с рекурсивным `Draw`. Не участвует в основной физике сцены, подключена в `scene.h` для совместимости.

---

## 5. Глава III. Текстуры, материалы и отражения

### 5.1. Загрузка: `textures_path.h` / `textures.cpp`

#### Разрешение путей (`textures_path.cpp`)

| Функция | Поведение |
|---------|-----------|
| `innerDirectory()` | Каталог бинарника `scene_viewer` (`/proc/self/exe` на Linux) |
| `texturesPath()` | `TEXTURES_PATH` или `inner/../textures` |
| `defaultSceneFilePath()` | `inner/default.scene` |

#### Загрузка в OpenGL (`textures.cpp`)

```
LoadTexID("textures/water.png")
    → resolveTextureFile (абсолютный путь)
    → stbi_load (RGB/RGBA)
    → glGenTextures + gluBuild2DMipmaps
    → GLuint id
```

**Важно:** если GL-контекста ещё нет (`glXGetCurrentContext() == nullptr`), функция **возвращает 0 без ошибки**. Поэтому в `Scene::ensureSceneTexturesLoaded()` текстуры **перезагружаются** на первом кадре `Render()`.

### 5.2. Привязка к объектам (`scene.h`, `scene_loader.cpp`)

1. Строки `TEXTURE path` в `.scene` → `editorTexturePaths` + попытка `LoadTexID`.
2. `OBJECT … texIndex` → индекс в списке текстур.
3. `objectTextureIndices[i]` хранит индекс для i-го объекта.
4. При рендере: `Objects[i]->textureID = editorTextureGlIds[texIndex]`.

### 5.3. Материалы: `render_material.h` / `render_material.cpp`

#### Двойная роль `alpha`

Параметр `alpha` в PHYS кодирует **и прозрачность, и отражение**:

| Диапазон `alpha` | Интерпретация |
|------------------|---------------|
| `[0, 1]` | `opacity = alpha`, `reflect = 0` |
| `(1, 2]` | `opacity = 1`, `reflect = alpha - 1` |

`decomposeAlphaReflect(alpha)` возвращает `{opacity, reflect}`.

#### OpenGL fixed pipeline

`initMatteSceneLighting()` — мягкий рассеянный свет без бликов по умолчанию.

`applyFigureMaterial(opacity, reflect, surfaceColor?)`:

- Ambient / diffuse масштабируются цветом поверхности (если нет текстуры).
- Specular: `spec = reflect * 0.85`, shininess `8 + reflect * 88`.

```
Без текстуры:  glColor + material tint
С текстурой:   glColor(1,1,1) + texture MODULATE + material
```

`bindTextureReflective(tex, reflect, waterLike)` — для пола/воды: `GL_REPEAT` vs `CLAMP`, режим `GL_MODULATE`.

### 5.4. Отражения на полу (`GroundPlane` в `figures.h`)

Пол с текстурой `water` в имени файла получает `setReflect(1.0, true)` при загрузке сцены.

Визуально: повышенный specular + отдельная логика отрисовки в `GroundPlane::Draw` (зеркальный компонент через `reflectAmount`).

### 5.5. Цепочка рендера объекта

```mermaid
flowchart LR
    A["decomposeAlphaReflect(alpha)"]
    B["applyFigureMaterial"]
    C["drawObjectRigidBody"]
    D["drawLocal: bind texture"]
    E["glut/glu примитив"]

    A --> B --> C --> D --> E
```

`drawObjectRigidBody` (в `scene.h`) **всегда** рисует текстурированный примитив, а не плоский collision-mesh (последний только в debug-слое).

---

## 6. Глава IV. Коллизии — меши, контакты, разбиения

Это центральная глава: как из примитива получается сетка треугольников, как ищется контакт и почему тесты устроены именно так.

### 6.1. Два представления: `collision_repr.h` / `collision_repr.cpp`

```cpp
enum class CollisionRepr { Sphere, Triangle };
```

`collisionReprForObject(obj)` выбирает режим **без участия пользователя** (эвристика):

| Условие | Результат |
|---------|-----------|
| Сфера сильно вытянута (`scale`) | Triangle |
| Сфера маленькая (`r ≤ 1.25`) + `--O1` | Sphere (быстро) |
| Большая сфера | Triangle |
| Box: тонкая или длинная плита | Triangle |
| Box: маленький компактный | Sphere |
| Тор, пирамида, составные | Triangle |
| `--O1` выключен | Чаще Triangle |

Флаг `--O1` в `main.cpp` включает `collision::gLodO1Enabled` — **оптимизация** для дальних мелких объектов.

### 6.2. Структуры в `collision_mesh.h`

```cpp
struct CollTri {
    vec<> v0, v1, v2;
    vec<> normal() const;   // (e1 × e2) нормализованный
    double area() const;    // 0.5 |e1 × e2|
    vec<> centroid() const; // (v0+v1+v2)/3
};

struct CollisionContact {
    vec<> point, normal;
    double penetration;
};
```

### 6.3. Генерация меша: `collision_mesh.cpp`

#### Общий пайплайн

```
buildObjectCollisionMesh(obj, out, faceSubdiv)
    → локальные треугольники в системе объекта

buildWorldCollisionMesh(obj, center, baseCenter, spinAxis, spinDeg, faceSubdiv, out)
    → поворот + перенос в мир
```

#### Разбиение граней (`faceSubdiv`)

Параметр **`collisionSubdiv`** из PHYS (1…24) управляет плотностью сетки.

**Коробка** `appendBoxTriangles(hx, hy, hz, faceSubdiv)`:

- Каждая из 6 граней — сетка `(faceSubdiv × faceSubdiv)` квадов → `2 × faceSubdiv²` треугольников на грань.

**Сфера** `appendSphereTriangles(r, slices, stacks)`:

```
slices = clamp(faceSubdiv * 3, 8, 72)
stacks = clamp(faceSubdiv * 2, 6, 48)
```

Параметризация (широта/долгота):

\[
\begin{aligned}
y &= r\sin v \\
r_{xy} &= r\cos v \\
x &= r_{xy}\cos u,\quad z = r_{xy}\sin u
\end{aligned}
\]

**Тор** `appendTorusTriangles(tubeR, ringR, sides, rings)`:

```
sides = clamp(6 + faceSubdiv*2, 8, 48)
rings = clamp(8 + faceSubdiv*3, 12, 72)
```

Согласовано с `glutSolidTorus(inner, outer, sides, rings)`:

\[
\begin{aligned}
x &= (R + r\cos v)\cos u \\
y &= (R + r\cos v)\sin u \\
z &= r\sin v
\end{aligned}
\]

где `inner` = радиус трубки, `outer` = радиус кольца (GLUT-конвенция).

**Цилиндр / конус:** `slices = faceSubdiv * 3` (с клампом).

**Пирамида:** тесселяция основания `baseSubdiv = faceSubdiv`.

**TransformWrapper:** рекурсивный вызов `buildObjectCollisionMesh(child, …, faceSubdiv)` — **один subdiv на всю составную фигуру**.

#### LOD без явного subdiv (`lodFaceSubdiv`)

Если `collisionSubdiv == 0`, используется дистанция до камеры:

\[
\text{subdiv} = \mathrm{lerp}(2,\ \lceil \text{faceSize}/0.35 \rceil,\ \mathrm{clamp}(d/40,\ 0,\ 1))
\]

### 6.4. Геометрия контакта

#### Ближайшая точка на треугольнике

`closestPointOnTriangle(p, v0, v1, v2)` — классический алгоритм по регионам Вороного (вершина / ребро / внутрь).

#### Сфера — треугольник

```
penetration = r - |center - p_closest|
normal      = от поверхности к центру сферы
```

`bestSphereTriangleContact` — максимальная глубина по всем треугольникам меша.

#### Тонкие плиты (`bodyIsThinPlate`)

Только **`EditorBox`** и **`Compound`** с одной очень маленькой толщиной по Y:

```
halfExtents.y < 0.15 * max(hx, hz)
```

Исправление бага «улёта вверх»: вытянутые **сферы** больше не считаются плитами.

`sphereThinPlateTopContact` — контакт только с **верхней** гранью плиты в XZ.

#### Mesh на плите

`meshBodyOnThinPlateTop` — ищет самую низкую вершину меша над верхней гранью плиты.

### 6.5. Инициализация тела из меша (`Scene::initBodyFromTriMesh`)

1. Строится collision mesh с заданным `faceSubdiv`.
2. **Центр масс (COM)** — взвешенный по площади треугольников:

\[
\text{COM} = \frac{\sum_i A_i \cdot \text{centroid}_i}{\sum_i A_i}
\]

3. Треугольники переводятся в **локальные координаты относительно COM** → `partsTriLocal`.
4. Масса и инерция:

   - Из `massOverride` в PHYS, если задана.
   - Иначе сумма площадей / объёмные формулы для box.

5. `collisionRepr = Triangle` — дальнейшие контакты идут по `partsTriLocal`.

### 6.6. Детекция в `scene.h` (обзор)

`detectCollision(ia, ib, Contact& c)` — каскад:

```
1. Разные K-слои (4D) → нет контакта
2. Sphere–Sphere (быстрый путь)
3. Box, Cylinder, Torus — аналитика + mesh fallback
4. Triangle mesh — vertex penetration, sphere-triangle, swept sphere
5. Тонкие плиты — отдельные ветки
```

**Импульсное разрешение** `resolveCollision`:

Нормальный импульс (упрощённо):

\[
j_n = -\frac{(1+e)\, v_{rel,n}}{1/m_a + 1/m_b + I_a^{-1}(r_a\times n)^2 + I_b^{-1}(r_b\times n)^2}
\]

Трение: \(j_t = \mathrm{clamp}(-v_{t,rel}/denom,\ -\mu j_n,\ \mu j_n)\), \(\mu \approx 0.24\).

Позиционная коррекция (убирает «залипание»):

\[
\Delta x = n \cdot \max(0,\ pen - slop) \cdot k / (1/m_a + 1/m_b)
\]

### 6.7. Схема: от слайдера subdiv до контакта

```mermaid
flowchart TD
    UI["Слайдер collisionSubdiv в редакторе"]
    PHYS["PHYS … collisionSubdiv"]
    REBUILD["rebuildBodies → initBodyFromTriMesh"]
    MESH["buildObjectCollisionMesh(faceSubdiv)"]
    STORE["partsTriLocal"]
    STEP["stepPhysics → detectCollision"]
    RES["resolveCollision"]

    UI --> PHYS --> REBUILD --> MESH --> STORE --> STEP --> RES
```

Для **группы** объектов редактор синхронизирует один `collisionSubdiv` на все части с общим `groupId`; счётчик полигонов в UI **суммирует** треугольники всех частей.

---

## 7. Глава V. Физика — интеграция, импульсы, группы

Основная логика в **`inner/headers/scene.h`** (header-only физика ~1700 строк).

### 7.1. `BodyState` — состояние тела

Ключевые поля:

| Поле | Смысл |
|------|--------|
| `center` | Положение COM в мире |
| `velocity`, `angularVelocity` | Линейная и угловая скорость |
| `spinAxis`, `spinDeg` | Визуальный/физический поворот (градусы вокруг оси) |
| `invMass`, `invInertia` | 0 для статических тел |
| `isStatic` | Полностью неподвижное тело (коллизии есть) |
| `gravityMode` | 0=выкл, 1=вектор, 2=аттрактор |
| `kPos`, `kVel` | Координата на оси K (4D) |
| `groupId`, `isLeader` | Составные объекты |

### 7.2. Режимы гравитации

| mode | Поведение |
|------|-----------|
| 0 | Нет ускорения; тело **может** двигаться от столкновений |
| 1 | `velocity += gravity * dt` (примитив, по умолчанию `(0,-9.81,0)`) |
| 2 | Притяжение к точке/объекту: \(a = \min(120,\ strength/r^2)\) |

**Орбита:** `orbitOmegaY` вращает тело вокруг `orbitCenter` в плоскости XZ.

### 7.3. `stepPhysics(dt)`

```
substeps = 4
h = dt / substeps
для каждого substep:
  1. Применить силы (гравитация, орбита), пропустить isStatic
  2. Интегрировать center += v*h, обновить spinDeg от angularVelocity
  3. detectCollision + resolveCollision (2 прохода)
  4. Пол, границы арены, calmBodyOnSupport
  5. Синхронизация групп (последователи → лидер)
```

**Статическое тело** (`isStatic=1`):

```cpp
invMass = 0; invInertia = 0;
velocity = 0; // каждый substep
// участвует в detectCollision как неподвижный партнёр
```

### 7.4. Группы объектов

Несколько `OBJECT` с одним `GROUP id`:

1. При `rebuildBodies` вычисляется общий COM группы и суммарная масса на **лидере**.
2. Последователи хранят `localFromCom` относительно лидера.
3. После интеграции лидера позиции последователей обновляются жёстко.

В редакторе `onMergeSelected` назначает новый `groupId`.

### 7.5. Поворот: формула Родрига

`rotateAroundAxis(v, axis, angle_rad)`:

\[
v' = v\cos\theta + (a\times v)\sin\theta + a(a\cdot v)(1-\cos\theta)
\]

Используется при выводе мировых треугольников из `partsTriLocal`.

---

## 8. Глава VI. Сцена, загрузчик и главный цикл

### 8.1. `scene_loader.h` / `scene_loader.cpp`

`bool loadEditorSceneFile(path, Scene& scene)`

Парсит VERSION 1 (см. [§13](#13-формат-файла-scene)):

- `TEXTURE`, `ENV`, `OBJECT`, `PHYS`, `GROUP`
- Создаёт объекты через `createSceneObject`
- Заполняет `scene.objectPhysics`, `objectTextureIndices`
- `scene.setEnvironment(...)` → отложенная загрузка текстур

### 8.2. `animation.h` / `animation.cpp`

Singleton `animation::GetScene()` — глобальная сцена GLUT.

| Callback | Роль |
|----------|------|
| `Display` | `scene.Render(Time)`, HUD (FPS, Drawn, Camera 3D/4D) |
| `Keyboard` | WASD, QE (K в 4D), T переключение 4D камеры, P пауза, `;` debug |
| `Motion` | Мышь: yaw/pitch, панорама |
| `Idle` | `Time += dt`, лимит FPS (`-sync N`) |

Камера:

\[
\begin{aligned}
X &= \cos(\text{pitch})\sin(\text{yaw}) \\
Y &= \sin(\text{pitch}) \\
Z &= \cos(\text{pitch})\cos(\text{yaw})
\end{aligned}
\]

### 8.3. `main.cpp` (inner)

Аргументы:

| Флаг | Эффект |
|------|--------|
| `-scene path` | Загрузить сцену |
| `--collision-test` | `default_collision_test.scene` |
| `--O1` | LOD коллизий |
| `-sync N` | Лимит FPS |
| `--no-info` | Без HUD |

### 8.4. `Scene::Render(t)` (кратко)

1. `ensureSceneTexturesLoaded()`
2. `rebuildBodies()` при изменении объектов
3. `stepPhysics` если не пауза
4. Frustum culling по bounding sphere
5. Для каждого объекта: `applyFigureMaterial` → `drawObjectRigidBody`
6. Пол, небо, debug-слои (зелёные сферы / траектории / collision mesh LOD)

---

## 9. Глава VII. Четырёхмерная геометрия

### 9.1. `fourd_math.h` / `fourd_math.cpp`

#### `Vec4 { x, y, z, k }`

4-я координата обозначена **`k`** (в референсе Jackson Hall — `a`).

#### `Camera4DState`

| Поле | Смысл |
|------|--------|
| `location`, `focus` | Точки в R⁴ |
| `normal` | Единичная нормаль гиперплоскости взгляда |
| `focalDistance` | Расстояние фокуса |

#### Проекция `projectTo3D(cam, v4) → vec3`

1. Луч из `location` через точку `v4`.
2. Пересечение с гиперплоскостью через `focus` с нормалью `n`.
3. Скалярные проекции на базис `(right, up, out)` → 3D точка.

\[
t = \frac{n \cdot (p - location)}{n \cdot (focus - p)}
\]

#### `buildTesseract(s, verts, edges)`

16 вершин \((\pm s, \pm s, \pm s, \pm s)\), рёбра между вершинами с **расстоянием Хэмминга 1**.

#### `syncViewerToCamera4d`

Связывает обычную 3D камеру viewer с `Camera4DState` (направление взгляда → нормаль 4D камеры).

### 9.2. `fourd_figure.h` / `fourd_figure.cpp`

`FourDWireFigure` — каркас 4D фигуры:

- `drawProjected(cam, kWorld, worldPos)` — каждое ребро: 4D → 3D → `GL_LINES`.
- `kPos` / `pk` в PHYS — сдвиг по K при отрисовке и физике.

### 9.3. `fourd_collision.h` / `fourd_collision.cpp`

`hyperSphereSphereContact` — сферы в R⁴:

\[
pen = r_a + r_b - \|c_b - c_a\|_{4D}
\]

В полной симуляции `scene.h` использует упрощённую модель **K-среза** (см. ниже).

### 9.4. K-ось в физике (`scene.h`)

3D тела занимают «слой» по K толщиной `kSliceHalf = 0.25`.

Два тела сталкиваются в 4D только если:

\[
|k_a - k_b| \le k_a^{slice} + k_b^{slice}
\]

и одновременно пересекаются в 3D.

`resolveCollision4D` добавляет импульс по `kVel` аналогично 1D столкновению.

---

## 10. Глава VIII. Редактор сцен (outer)

### 10.1. `main.cpp`

`QApplication` → `MainWindow` → `exec()`.

### 10.2. `ProjectRoot.h` / `ProjectRoot.cpp`

`resolveDriverTestRoot()` — ищет корень репозитория (env `DRIVER_TEST_ROOT`, walk-up от `outer/build`, cwd).

### 10.3. `SceneFile.h` / `SceneFile.cpp`

Зеркало inner-загрузчика для Qt:

- `SceneObject`, `SceneData`, `SceneEnvironment`
- `loadSceneFile` / `saveSceneFile`
- `clampSceneTextureIndices`, `remapSceneTextureIndicesByPath`

### 10.4. `MainWindow.h` / `MainWindow.cpp`

Главное окно редактора:

| UI блок | Функция |
|---------|---------|
| Список объектов | `refreshObjectList`, merge → `groupId` |
| Transform | позиция, масштаб, поворот |
| Physics | гравитация, трение, restitution, **Static body**, subdiv |
| Shape params | `extra[]` per type |
| Текстуры | scan `textures/`, combo `texIndex` |
| Build | `make -C inner clean && make`, лог в `QPlainTextEdit` |
| Custom figures | кнопки из каталога |

**`collisionPolyCountForObject`** — строит меш через `buildObjectCollisionMesh` и показывает число треугольников (для группы — сумма).

**`pushUiToObject`** — при `groupId >= 0` синхронизирует `collisionSubdiv` и `isStatic` на все части.

### 10.5. `PreviewWidget.h` / `PreviewWidget.cpp`

OpenGL превью внутри Qt:

- Орбитальная камера, пикинг лучом через `gluUnProject`
- Отрисовка collision mesh для выделенного объекта
- `T` — 4D камера; `Q`/`E` — сдвиг `m_cam4dK`

### 10.6. `CustomFigures.h` / `CustomFigures.cpp`

Пресеты в `inner/custom_figures.catalog`:

```
PRESET name
OBJECT …
PHYS 0 …
```

---

## 11. Глава IX. Автотесты коллизий

Каталог: `inner/tests/collision/`. Сборка: `make test` запускает **6 бинарников подряд**.

### 11.1. `Makefile`

Линкует **ядро** (`collision_mesh`, `scene_loader`, shapes, …) с каждым `*_suite.cpp`. Флаги: `-Wall -O2`, `-lGL -lGLU -lglut`.

### 11.2. `run_suite.cpp` → `collision_suite`

**Задача:** объекты падают на **статическую плиту** (`solid_cube` масштабом 30×1×30, `isStatic`).

~37 кейсов: 9 форм × 4 высоты сброса + длинный куб.

| Проверка | Порог |
|----------|-------|
| Не провалились | `y >= 0.5` всегда |
| Высота покоя | в ожидаемом диапазоне у верха плиты (~2.5) |
| Успокоение | `|vy| ≤ 0.25`, `|v| ≤ 0.5` в конце |

**Почему так:** ловит tunneling, «дрожание» на плите, неверный COM.

### 11.3. `pair_suite.cpp`

Два тела летят навстречу **без гравитации** (`restitution=0`).

| Проверка | Смысл |
|----------|-------|
| `minDist ≤ dist ≤ maxDist` | Разошлись, но не слишком далеко |
| `maxPen ≤ 0.5` | Нет глубокого внедрения |
| `maxAttract ≤ 12` | Нет «присасывания» при разлёте |

Пары: cube+cube, torus+long_cube, sphere+sphere, вытянутые фигуры.

### 11.4. `stress_suite.cpp`

**3600 шагов** (60 с при 60 Hz) на файлах `stress_test.scene`, `default_collision_test.scene` + мульти-объекты.

| Проверка | Порог |
|----------|-------|
| Конечные координаты | не NaN |
| Скорость | `|v| ≤ 25` (файлы) / 40 (multi) |
| Высота | `y ≤ 80` |

### 11.5. `fourd_suite.cpp`

Юнит-тесты **без полной Scene**:

- касание/разделение 4D сфер
- `buildTesseract` → 16 вершин, 32 ребра
- `projectTo3D`, `syncViewerToCamera4d`

### 11.6. `k_axis_suite.cpp`

| Тест | Ожидание |
|------|----------|
| Разный K | 3D сферы **не** сталкиваются |
| Одинаковый K | 4D тела сталкиваются |
| Удар по K | передаётся `kVel` |

### 11.7. `impulse_suite.cpp`

| Тест | Физика |
|------|--------|
| `head_on_swap` | Две сферы ±4 m/s, e=0.9 → обмен скоростями |
| `glancing_spin` | Частичный удар → `|ω| ≥ 0.15` |
| `floor_bounce` | Отскок от статической плиты |
| `gravity_off_hover` | mode 0: нет дрейфа без сил |
| `gravity_off_collision` | столкновение без гравитации сохраняет импульс |

**Почему отдельный suite:** регрессии «улёта вверх» и «заморозки» при выключенной гравитации ловятся только динамикой импульсов, не статикой высоты.

---

## 12. Глава X. Референс `4d_logic_for_windows`

Отдельный учебный движок Jackson Hall (2020). **Не линкуется** с `inner`, но математика перенесена в `fourd_math`.

### Цепочка проекции

```
point4d ──Camera4D──► point3d ──Camera3D──► point2d ──×ORTHO_ZOOM──► экран
```

### Ключевые файлы

| Файл | Содержание |
|------|------------|
| `utils.h/cpp` | `point2d/3d/4d`, `spatialVector`, скалярная проекция |
| `Camera3D` | R³→R² через пересечение луча с плоскостью |
| `Camera4D` | R⁴→R³, оси Q/E (in/out), U/O (phi) |
| `Object4D` | Рёбра 4D → проекция → `edge2d::draw` |
| `graphics.cpp` | GLUT main, пресеты: куб, тессеракт, 600-cell |
| `Scene.cpp` | Списки `objects3d`, `objects4d`, `toggleActiveCamera` |

### Сферические углы 4D

\[
\begin{aligned}
x &= \sin\phi\sin\theta_{az}\sin\theta_{pol} \\
y &= \sin\phi\sin\theta_{az}\cos\theta_{pol} \\
z &= \sin\phi\cos\theta_{az} \\
a &= \cos\phi
\end{aligned}
\]

---

## 13. Формат файла `.scene`

```
VERSION 1
ENV GROUND textures/water.png 200 200
ENV SKY textures/mountains.jpg 1000
TEXTURE textures/Filth.png
…
OBJECT <type> px py pz sx sy sz rx ry rz texIndex [extra…]
…
PHYS <index> vx vy vz ox oy oz omegaY gravityMode useFriction gx gy gz
       friction restitution collide alpha mass pk vk
       gravTargetX gravTargetY gravTargetZ gravStrength gravTargetObject
       collisionSubdiv isStatic
GROUP <index> groupId
```

### `extra` по типам

| type | extra |
|------|-------|
| sphere | radius |
| cube/box | dx dy dz |
| solid_cube, 4D | size |
| cylinder | radius, height |
| torus | innerR, outerR |
| cone/pyramid | base, height |

---

## 14. Сборка, запуск, отладка

```bash
# Viewer
cd inner && make scene_viewer
./scene_viewer -scene default.scene

# Редактор
cd outer/build && cmake .. && make && ./scene_editor

# Все тесты
cd inner/tests/collision && make test
```

**Отладка в viewer:**

| Клавиша | Слой |
|---------|------|
| `;` | 0 → bounding spheres → COM/velocity trail |
| `P` | Пауза физики |
| `T` | 3D / 4D камера |

---

## 15. Справочник: каждый `.h` / `.cpp` файл

Краткая карта **всех** исходников (кроме autogen и stb).

### Inner — headers

| Файл | Назначение |
|------|------------|
| `vector.h` | 3D вектор, dot/cross/normalize |
| `figures.h` | `based`, Solid*, GroundPlane, SkySphere, physmath |
| `manual_shapes.h` | Editor* примитивы с TRS |
| `transform_wrapper.h` | TRS обёртка над child |
| `object_factory.h` | createSceneObject API |
| `collision_mesh.h` | CollTri, генерация мешей, геотесты |
| `collision_repr.h` | Sphere vs Triangle выбор |
| `render_material.h` | alpha/reflect, GL material |
| `render_settings.h` | LOD сегментов GLUT |
| `textures.h` | LoadTexID |
| `textures_path.h` | пути к textures/ |
| `animation.h` | GLUT app shell |
| `scene.h` | **Scene**, физика, рендер, коллизии |
| `scene_loader.h` | loadEditorSceneFile |
| `fourd_math.h` | Vec4, Camera4D, проекция |
| `fourd_figure.h` | FourDWireFigure |
| `fourd_collision.h` | 4D sphere contact |
| `templates.h` | demo voxel/plate/line |

### Inner — sources

| Файл | Назначение |
|------|------------|
| `main.cpp` | CLI, запуск viewer |
| `animation.cpp` | GLUT callbacks, HUD, камера |
| `figures.cpp` | GLUquadric static defs |
| `manual_shapes.cpp` | отрисовка Editor* |
| `transform_wrapper.cpp` | TRS + bounding spheres |
| `object_factory.cpp` | таблица type→объект |
| `collision_mesh.cpp` | тесселяция + contact tests |
| `collision_repr.cpp` | эвристики repr |
| `render_material.cpp` | GL lighting/material |
| `textures.cpp` | stb → GL texture |
| `textures_path.cpp` | resolve paths |
| `scene_loader.cpp` | парсер .scene |
| `fourd_math.cpp` | проекция, тессеракт |
| `fourd_figure.cpp` | wireframe 4D draw |
| `fourd_collision.cpp` | hyperSphere tests |

### Inner — tests

| Файл | Назначение |
|------|------------|
| `Makefile` | сборка 6 suites |
| `run_suite.cpp` | drop tests |
| `pair_suite.cpp` | pairwise separation |
| `stress_suite.cpp` | long-run stability |
| `fourd_suite.cpp` | 4D math unit tests |
| `k_axis_suite.cpp` | K-axis physics |
| `impulse_suite.cpp` | импульсы, bounce, no-grav |

### Outer — headers / sources

| Файл | Назначение |
|------|------------|
| `ProjectRoot.h/cpp` | корень репозитория |
| `SceneFile.h/cpp` | I/O .scene для Qt |
| `CustomFigures.h/cpp` | каталог пресетов |
| `PreviewWidget.h/cpp` | GL превью |
| `MainWindow.h/cpp` | UI редактора |
| `main.cpp` | точка входа Qt |

### 4d_logic_for_windows

| Файл | Назначение |
|------|------------|
| `utils.h/cpp` | точки, векторы, углы |
| `Camera.h/cpp` | база камеры |
| `Camera3D.h/cpp` | 3D камера |
| `Camera4D.h/cpp` | 4D камера |
| `Object.h/cpp` | абстрактный объект |
| `Object3D.h/cpp` | 3D рёбра |
| `Object4D.h/cpp` | 4D→3D→2D рёбра |
| `Scene.h/cpp` | сцена, списки объектов |
| `graphics.h/cpp` | GLUT main, ввод |

---

## Эпилог: с чего начать читать код

| Уровень | Маршрут |
|---------|---------|
| Новичок | `vector.h` → `figures.h` → `manual_shapes.cpp` → `main.cpp` |
| Графика | `textures.cpp` → `render_material.cpp` → `Scene::Render` |
| Коллизии | `collision_mesh.cpp` → `collision_repr.cpp` → `Scene::detectCollision` |
| Физика | `Scene::stepPhysics` → `resolveCollision` → `impulse_suite.cpp` |
| 4D | `fourd_math.cpp` → `fourd_figure.cpp` → `k_axis_suite.cpp` |
| Редактор | `SceneFile.cpp` → `MainWindow.cpp` → `PreviewWidget.cpp` |

Движок намеренно использует **OpenGL 1.x fixed pipeline** и **явные формулы** вместо сторонних библиотек — чтобы по исходникам можно было пройти полный путь от `.scene` до столкновения торов с плитой и увидеть результат в `scene_viewer`.

---

*Версия документа соответствует коммиту с `isStatic`, отложенными текстурами и subdiv-коллизиями для всех примитивов.*
