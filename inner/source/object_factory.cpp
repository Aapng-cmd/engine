#include "object_factory.h"
#include "collision_mesh.h"
#include "collision_repr.h"
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
    if (t == "tesseract" || t == "hypersphere" || t == "pyramid4d")
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
        return new EditorBox(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1], ex[2],
                             vec<>(0.75, 0.75, 0.75), tex);
    }
    if (t == "solid_cube") {
        if (!need(1, ex, err))
            return nullptr;
        based* inner = new SolidCube(ex[0], ex[0], ex[0], vec<>(0.75, 0.75, 0.75), tex);
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
        return new EditorTorus(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1],
                               vec<>(0.75, 0.75, 0.75), tex);
    }
    if (t == "cone") {
        if (!need(2, ex, err))
            return nullptr;
        based* inner = new SolidCone(ex[0], ex[1], vec<>(0.75, 0.75, 0.75), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (t == "pyramid") {
        if (!need(2, ex, err))
            return nullptr;
        based* inner = new SolidPyramid(ex[0], ex[1], vec<>(0.75, 0.75, 0.75), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (t == "tesseract" || t == "hypersphere" || t == "pyramid4d") {
        if (!need(1, ex, err))
            return nullptr;
        return new FourDWireFigure(t, vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0],
                                   vec<>(0.75, 0.75, 0.85), tex);
    }
    if (err)
        *err = "unknown type: " + type;
    return nullptr;
}
