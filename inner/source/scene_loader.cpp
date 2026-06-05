#include "scene_loader.h"
#include "manual_shapes.h"
#include "object_factory.h"
#include "textures.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static std::string trim(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a])))
        ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
        --b;
    return s.substr(a, b - a);
}

static bool readLine(std::ifstream& in, std::string& out)
{
    while (std::getline(in, out)) {
        std::string t = trim(out);
        if (t.empty() || t[0] == '#')
            continue;
        out = t;
        return true;
    }
    return false;
}

bool loadEditorSceneFile(Scene& scene, const std::string& path)
{
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open scene file: " << path << std::endl;
        return false;
    }

    for (based* p : scene.Objects)
        delete p;
    scene.Objects.clear();
    scene.objectPhysics.clear();
    scene.physicsInitialized = false;

    std::string line;
    if (!readLine(in, line)) {
        std::cerr << "Empty scene file: " << path << std::endl;
        return false;
    }
    {
        std::istringstream iss(line);
        std::string tag;
        int ver = 0;
        iss >> tag >> ver;
        if (tag != "VERSION" || ver != 1) {
            std::cerr << "Expected 'VERSION 1', got: " << line << std::endl;
            return false;
        }
    }

    std::vector<GLuint> texIds;
    std::vector<std::string> texPaths;

    struct PhysMeta {
        double vx = 0, vy = 0, vz = 0;
        double ox = 0, oy = 0, oz = 0;
        double omegaY = 0;
        int useGravity = 0;
        int useFriction = 0;
        double gx = 0, gy = -9.81, gz = 0;
        double friction = 0.0;
        double restitution = 0.74;
    };
    std::vector<PhysMeta> physByIndex;
    std::vector<int> groupByIndex;

    while (readLine(in, line)) {
        std::istringstream iss(line);
        std::string kw;
        iss >> kw;
        if (kw == "PHYS") {
            int idx = -1;
            PhysMeta m;
            if (!(iss >> idx >> m.vx >> m.vy >> m.vz >> m.ox >> m.oy >> m.oz >> m.omegaY) || idx < 0) {
                std::cerr << "Bad PHYS line: " << line << std::endl;
                return false;
            }
            if (!(iss >> m.useGravity >> m.useFriction >> m.gx >> m.gy >> m.gz >> m.friction >> m.restitution)) {
                // old format compatibility: keep defaults for optional fields
                iss.clear();
            }
            if (idx >= static_cast<int>(physByIndex.size()))
                physByIndex.resize(static_cast<size_t>(idx + 1));
            physByIndex[static_cast<size_t>(idx)] = m;
            continue;
        }
        if (kw == "GROUP") {
            int idx = -1, gid = -1;
            if (!(iss >> idx >> gid) || idx < 0) {
                std::cerr << "Bad GROUP line: " << line << std::endl;
                return false;
            }
            if (idx >= static_cast<int>(groupByIndex.size()))
                groupByIndex.resize(static_cast<size_t>(idx + 1), -1);
            groupByIndex[static_cast<size_t>(idx)] = gid;
            continue;
        }
        if (kw == "TEXTURE") {
            std::string rest;
            std::getline(iss, rest);
            std::string pathTex = trim(rest);
            if (pathTex.empty()) {
                std::cerr << "TEXTURE with empty path\n";
                return false;
            }
            GLuint id = LoadTexID(pathTex);
            texIds.push_back(id);
            texPaths.push_back(pathTex);
            continue;
        }
        if (kw != "OBJECT") {
            std::cerr << "Unknown keyword: " << kw << std::endl;
            return false;
        }

        std::string type;
        iss >> type;
        double px, py, pz, sx, sy, sz, rx, ry, rz;
        int texIdx;
        if (!(iss >> px >> py >> pz >> sx >> sy >> sz >> rx >> ry >> rz >> texIdx)) {
            std::cerr << "Bad OBJECT line (missing transform): " << line << std::endl;
            return false;
        }

        GLuint gltex = 0;
        if (texIdx >= 0) {
            if (texIdx >= static_cast<int>(texIds.size())) {
                std::cerr << "Texture index out of range: " << texIdx << " (have " << texIds.size() << ")\n";
                return false;
            }
            gltex = texIds[static_cast<size_t>(texIdx)];
        }

        std::vector<double> extras;
        double v;
        while (iss >> v)
            extras.push_back(v);

        std::string err;
        based* obj = createSceneObject(type, px, py, pz, sx, sy, sz, rx, ry, rz, extras, gltex, &err);
        if (!obj) {
            std::cerr << "Object factory: " << err << " (line: " << line << ")\n";
            return false;
        }
        Scene::ObjectPhysics p;
        const int oi = static_cast<int>(scene.Objects.size());
        if (oi < static_cast<int>(physByIndex.size())) {
            const PhysMeta& m = physByIndex[static_cast<size_t>(oi)];
            p.velocity = vec<>(m.vx, m.vy, m.vz);
            p.orbitCenter = vec<>(m.ox, m.oy, m.oz);
            p.orbitOmegaY = m.omegaY;
            p.useGravity = m.useGravity;
            p.useFriction = m.useFriction;
            p.gravity = vec<>(m.gx, m.gy, m.gz);
            p.groundFriction = m.friction;
            p.restitution = m.restitution;
        }
        if (oi < static_cast<int>(groupByIndex.size()))
            p.groupId = groupByIndex[static_cast<size_t>(oi)];
        scene.addLoadedObject(obj, p);
    }

    for (size_t i = 0; i < scene.objectPhysics.size(); ++i) {
        Scene::ObjectPhysics& p = scene.objectPhysics[i];
        if (i < physByIndex.size()) {
            const PhysMeta& m = physByIndex[i];
            p.velocity = vec<>(m.vx, m.vy, m.vz);
            p.orbitCenter = vec<>(m.ox, m.oy, m.oz);
            p.orbitOmegaY = m.omegaY;
            p.useGravity = m.useGravity;
            p.useFriction = m.useFriction;
            p.gravity = vec<>(m.gx, m.gy, m.gz);
            p.groundFriction = m.friction;
            p.restitution = m.restitution;
        }
        if (i < groupByIndex.size())
            p.groupId = groupByIndex[i];
    }

    return true;
}
