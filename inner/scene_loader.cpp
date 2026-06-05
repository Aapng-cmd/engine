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

    while (readLine(in, line)) {
        std::istringstream iss(line);
        std::string kw;
        iss >> kw;
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
        scene.Objects.push_back(obj);
    }

    return true;
}
