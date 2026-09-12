#pragma once

#include "figures.h"
#include "fourd_math.h"
#include <string>
#include <vector>

/** 4D figure: tetrahedral cells, hyperplane slice, optional Hollasch projection. */
class FourDWireFigure : public based {
public:
    std::string shapeType;
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;
    double rwx = 0, rwy = 0, rwz = 0;
    double sizeParam = 1.0;
    double kPos = 0.0;
    vec<> color{0.75, 0.75, 0.85};
    std::vector<Vec4> verts4;
    std::vector<Edge4D> edges4;
    std::vector<Tet4> tets;

    FourDWireFigure(const std::string& type, vec<> p, vec<> s, double rx_, double ry_, double rz_, double size,
                    vec<> col, GLuint tex = 0);

    void rebuildGeometry();
    void collectSliceTris(const vec<>& worldPos, double kOffset, double sliceK, std::vector<SliceTri>& out) const;
    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void Draw(double t) override;
    void drawSliced(double sliceK, double kWorld = 0.0, const vec<>* worldPos = nullptr) const;
    void drawProjected(const Camera4DState& cam, double kWorld = 0.0, const vec<>* worldPos = nullptr) const;

    void uniqueLocalVerts(std::vector<Vec4>& out) const;
    Vec4 worldVert(const Vec4& local, const vec<>& worldPos, double kOffset) const;
    /** Move a unique local vert; all tet corners that match it follow. */
    bool moveUniqueVert(int index, const Vec4& local);
    /** Set unique vert so its world xyz matches `xyz` and world k = `sliceK`. */
    bool moveUniqueVertOnSlice(int index, const vec<>& xyz, double sliceK, const vec<>& worldPos, double kOffset);
};

void applyFourDAngles(based* o, double rwx, double rwy, double rwz);
FourDWireFigure* asFourDFigure(based* o);
const FourDWireFigure* asFourDFigure(const based* o);
void applyFourDTets(based* o, const std::vector<Tet4>& tets);
void copyFourDTets(const based* o, std::vector<Tet4>& tets);
void packFourDTets(const std::vector<Tet4>& tets, std::vector<double>& packed);
bool unpackFourDTets(const std::vector<double>& packed, std::vector<Tet4>& tets);
