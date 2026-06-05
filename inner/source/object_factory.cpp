#include "object_factory.h"
#include "collision_mesh.h"
#include "collision_repr.h"
#include "figures.h"
#include "manual_shapes.h"
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
    if (type == "sphere")
        return 1;
    if (type == "box")
        return 3;
    if (type == "cube")
        return 1;
    if (type == "cylinder")
        return 2;
    if (type == "cone")
        return 2;
    if (type == "pyramid")
        return 2;
    if (type == "torus")
        return 2;
    return -1;
}

based* createSceneObject(const std::string& type, double px, double py, double pz, double sx, double sy, double sz,
                         double rx, double ry, double rz, const std::vector<double>& ex, GLuint tex, std::string* err)
{
    if (type == "sphere") {
        if (!need(1, ex, err))
            return nullptr;
        return new EditorSphere(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], vec<>(0.75, 0.75, 0.75),
                                tex);
    }
    if (type == "box") {
        if (!need(3, ex, err))
            return nullptr;
        return new EditorBox(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1], ex[2],
                             vec<>(0.75, 0.75, 0.75), tex);
    }
    if (type == "cube") {
        if (!need(1, ex, err))
            return nullptr;
        based* inner = new SolidCube(ex[0], ex[0], ex[0], vec<>(0.75, 0.75, 0.75), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "cylinder") {
        if (!need(2, ex, err))
            return nullptr;
        return new EditorCylinder(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1],
                                  vec<>(0.75, 0.75, 0.75), tex);
    }
    if (type == "torus") {
        if (!need(2, ex, err))
            return nullptr;
        return new EditorTorus(vec<>(px, py, pz), vec<>(sx, sy, sz), rx, ry, rz, ex[0], ex[1],
                               vec<>(0.75, 0.75, 0.75), tex);
    }
    if (type == "cone") {
        if (!need(2, ex, err))
            return nullptr;
        based* inner = new SolidCone(ex[0], ex[1], vec<>(0.75, 0.75, 0.75), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "pyramid") {
        if (!need(2, ex, err))
            return nullptr;
        based* inner = new SolidPyramid(ex[0], ex[1], vec<>(0.75, 0.75, 0.75), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (err)
        *err = "unknown type: " + type;
    return nullptr;
}
