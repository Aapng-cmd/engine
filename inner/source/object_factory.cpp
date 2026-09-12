#include "object_factory.h"
#include "collision_mesh.h"
#include "collision_repr.h"
#include "editable_mesh.h"
#include "figures.h"
#include "manual_shapes.h"
#include "fourd_figure.h"
#include "fourd_math.h"
#include "transform_wrapper.h"

#include <sstream>

static based* wrap(based* inner, double px, double py, double pz, double sx, double sy, double sz, double rx, double ry,
                   double rz)
{
    return new TransformWrapper(inner, vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz);
}

/** Тайлинг считаем по мировому размеру: у большой плиты иначе видно один растянутый тексель. */
static based* withTexRepeat(based* obj, const vec<>& rep)
{
    if (obj)
        obj->texRepeat = rep;
    return obj;
}

static bool need(size_t n, const std::vector<double>& ex, std::string* err)
{
    if (ex.size() < n) {
        if (err) {
            std::ostringstream o;
            o << "expected " << n << " extra values, got " << ex.size();
            *err = o.str();
        }
        return false;
    }
    return true;
}

/** box — устаревший алиас для cube (EditorBox). */
static std::string resolveSceneType(const std::string& type, const std::vector<double>& ex)
{
    if (type == "box")
        return "cube";
    if (type == "cube" && ex.size() == 1)
        return "solid_cube";
    return type;
}

bool shapeUsesTriangleCollision(const based* obj)
{
    return usesTriangleCollision(collisionReprForObject(obj));
}

bool isComplexFigureType(const std::string& type)
{
    (void)type;
    return false;
}

int expectedExtraCount(const std::string& type)
{
    if (type == "box")
        return 3;
    if (type == "cube")
        return 3;
    if (type == "solid_cube")
        return 1;
    const std::string t = resolveSceneType(type, {});
    if (t == "sphere")
        return 1;
    if (t == "cube")
        return 3;
    if (t == "solid_cube")
        return 1;
    if (t == "cylinder")
        return 2;
    if (t == "cone")
        return 2;
    if (t == "pyramid")
        return 2;
    if (t == "torus")
        return 2;
    if (t == "tesseract" || t == "hypersphere" || t == "pyramid4d" || t == "16cell")
        return 1;
    if (t == "mesh")
        return 0;
    if (t == "camera")
        return 1;
    return -1;
}

based* createSceneObject(const std::string& type, double px, double py, double pz, double sx, double sy, double sz,
                         double rx, double ry, double rz, const std::vector<double>& ex, GLuint tex, std::string* err)
{
    const std::string t = resolveSceneType(type, ex);
    if (t == "sphere") {
        if (!need(1, ex, err))
            return nullptr;
        return new EditorSphere(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], vec<>(0.75, 0.75, 0.75),
                                tex);
    }
    if (t == "cube") {
        if (!need(3, ex, err))
            return nullptr;
        auto* box = new EditorBox(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1], ex[2],
                                  vec<>(0.75, 0.75, 0.75), tex);
        box->texRepeat = autoTexRepeat(ex[0] * sx, ex[1] * sy, ex[2] * sz);
        return box;
    }
    if (t == "solid_cube") {
        if (!need(1, ex, err))
            return nullptr;
        based* inner = new SolidCube(ex[0], ex[0], ex[0], vec<>(0.75, 0.75, 0.75), tex);
        withTexRepeat(inner, autoTexRepeat(ex[0] * sx, ex[0] * sy, ex[0] * sz));
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (t == "cylinder") {
        if (!need(2, ex, err))
            return nullptr;
        return new EditorCylinder(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1],
                                  vec<>(0.75, 0.75, 0.75), tex);
    }
    if (t == "torus") {
        if (!need(2, ex, err))
            return nullptr;
        /* Тор оставляем с одним оборотом текстуры: тайлинг по кольцу смазывает картинку. */
        return new EditorTorus(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1],
                               vec<>(0.75, 0.75, 0.75), tex);
    }
    if (t == "cone") {
        if (!need(2, ex, err))
            return nullptr;
        based* inner = new SolidCone(ex[0], ex[1], vec<>(0.75, 0.75, 0.75), tex);
        withTexRepeat(inner, autoTexRepeat(2.0 * M_PI * std::abs(ex[0]) * std::max(std::abs(sx), std::abs(sz)),
                                           std::abs(ex[1]) * std::abs(sy), 1.0));
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (t == "pyramid") {
        if (!need(2, ex, err))
            return nullptr;
        based* inner = new SolidPyramid(ex[0], ex[1], vec<>(0.75, 0.75, 0.75), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (t == "tesseract" || t == "hypersphere" || t == "pyramid4d" || t == "16cell") {
        if (!need(1, ex, err))
            return nullptr;
        return new FourDWireFigure(t, vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0],
                                   vec<>(0.75, 0.75, 0.85), tex);
    }
    if (t == "mesh") {
        auto* mesh = new EditableMesh(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, tex);
        mesh->fillUnitCube();
        return mesh;
    }
    if (t == "camera") {
        struct SceneCameraMarker : public based {
            vec<> pos;
            explicit SceneCameraMarker(vec<> p) : pos(p) {}
            void Draw(double) override {}
            void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double) override
            {
                out.push_back({pos, 0.2});
            }
        };
        return new SceneCameraMarker(vec<>(px, py, pz));
    }
    if (err)
        *err = "unknown type: " + type;
    return nullptr;
}
