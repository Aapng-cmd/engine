#include "object_factory.h"
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

int expectedExtraCount(const std::string& type)
{
    if (type == "sphere")
        return 1;
    if (type == "box")
        return 3;
    if (type == "cylinder")
        return 2;
    if (type == "torus")
        return 2;
    if (type == "planet")
        return 3;
    if (type == "weirdo")
        return 4;
    if (type == "param_cylinder")
        return 5;
    if (type == "fucked_cylinder")
        return 5;
    if (type == "kabasik")
        return 3;
    if (type == "tree")
        return 1;
    if (type == "snowflake")
        return 1;
    if (type == "kanar")
        return 3;
    if (type == "snowman")
        return 1;
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

    if (type == "planet") {
        if (!need(3, ex, err))
            return nullptr;
        based* inner =
            new Planet(vec<>(0, 0, 0), ex[0], ex[1], ex[2], vec<>(1, 1, 0), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "weirdo") {
        if (!need(4, ex, err))
            return nullptr;
        based* inner = new weirdo(vec<>(0, 0, 0), ex[0], ex[1], ex[2], ex[3], vec<>(1, 1, 0), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "param_cylinder") {
        if (!need(5, ex, err))
            return nullptr;
        based* inner = new cylinder(vec<>(0, 0, 0), ex[0], ex[1], ex[2], ex[3], vec<>(0, 0, 0), ex[4],
                                    vec<>(1, 1, 1), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "fucked_cylinder") {
        if (!need(5, ex, err))
            return nullptr;
        based* inner = new fucked_cylinder(vec<>(0, 0, 0), ex[0], ex[1], ex[2], ex[3], ex[4], vec<>(0, 0, 0), 0.0,
                                           vec<>(1, 1, 1), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "kabasik") {
        if (!need(3, ex, err))
            return nullptr;
        based* inner =
            new kabasik(vec<>(0, 0, 0), vec<>(0.8, 0.8, 0.9), ex[0], ex[1], ex[2], vec<>(0, 0, 0), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "tree") {
        if (!need(1, ex, err))
            return nullptr;
        based* inner = new tree(vec<>(0, 0, 0), vec<>(0, 1, 0), ex[0], 0, 0, tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "snowflake") {
        if (!need(1, ex, err))
            return nullptr;
        based* inner = new snowflake(vec<>(0, 8, 0), ex[0], 0, 0, vec<>(1, 1, 1), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "kanar") {
        if (!need(3, ex, err))
            return nullptr;
        based* inner =
            new kanar(vec<>(0, 0, 0), vec<>(0.9, 0.7, 0.8), ex[0], ex[1], ex[2], vec<>(0, 0, 0), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }
    if (type == "snowman") {
        if (!need(1, ex, err))
            return nullptr;
        based* inner = new snowman(vec<>(0, 0, 0), vec<>(1, 1, 1), ex[0], 0, 0, vec<>(0, 0, 0), tex);
        return wrap(inner, px, py, pz, sx, sy, sz, rx, ry, rz);
    }

    if (err)
        *err = "unknown type: " + type;
    return nullptr;
}
