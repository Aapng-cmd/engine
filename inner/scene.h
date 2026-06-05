#pragma once

#include "figures.h"
#include "vector.h"
#include <cmath>
#include "templates.h"
#include "textures.h"
#include "textures_path.h"
#include <functional>

/* TEXTURES_PATH() / texturesPath() — absolute shared folder .../driver_test/textures
   (inner dir + "/../textures"). LoadTexID("textures/foo.png") resolves via that. */

struct Scene
{
    std::vector<based*> Objects;
    std::vector<based*> texturedObjects;
    std::vector<std::function<based*()>> objectList;
    std::vector<std::string> texturePaths;

public:
    unsigned int lastDrawnCount = 0;
    Scene(void)
    {
        texturedObjects.push_back(new GroundPlane(LoadTexID("textures/water.png")));
        texturedObjects.push_back(new SkySphere(LoadTexID("textures/mountains.jpg")));

        // KABASIK START
//        Objects.push_back(new kabasik(vec<>(0, 5, 0), vec<>(0, 0, 1), 1, 30, 0, vec<>(5, 0, 5)));
//        Objects.push_back(new kabasik(vec<>(0, 5, 20), vec<>(0, 0, 1), 1, 30, 0, vec<>(5, 0, 5)));
//        Objects.push_back(new kabasik(vec<>(20, 5 * 1.5, 20), vec<>(0, 0, 1), 1.5, 30, 0, vec<>(5, 0, 5)));

        for (int i = 0; i < 0; i++) {
            based* k = new kabasik();
            k->setTexture(LoadTexID("textures/battler.png"));
            Objects.push_back(k);
        }

        // KABASIK END

        // TREES START
        // there is a problem with bounding sphere
        for (int i = 0; i < 0; i++)
            Objects.push_back(new tree());
        // TREES END

        // SHOW START
        for (int i = 0; i < 0; i++)
            Objects.push_back(new snowflake());
        // SNOW END

        // KANAR
        for (int i = 0; i < 0; i++) {
            based* k = new kanar();
            k->setTexture(LoadTexID("textures/Filth.png"));
            Objects.push_back(k);
        }
        // END KANAR

        // SNOWMAN
        for (int i = 0; i < 0; i++)
            Objects.push_back(new snowman());
        //END SNOWMAN
        
        objectList.push_back([]() -> based* { return new kabasik(); });
        texturePaths.push_back("textures/battler.png");
        objectList.push_back([]() -> based* { return new tree(); });
        texturePaths.push_back("textures/Goddess.png");
        objectList.push_back([]() -> based* { return new snowflake(); });
        texturePaths.push_back("textures/eldrich_horror.png");
        objectList.push_back([]() -> based* { return new kanar(); });
        texturePaths.push_back("textures/Filth.png");
        objectList.push_back([]() -> based* { return new snowman(); });
        texturePaths.push_back("textures/poch.png");
    }
    
    void addObject(int i) {
        if (objectList.empty()) return;
        based* k = objectList[i % objectList.size()]();
        k->setTexture(LoadTexID(texturePaths[i % objectList.size()]));
        Objects.push_back(k);
    }

    void Draw(void) {}
    
    vec<> transformPoint(const vec<>& p, const double mat[16]) {
        return vec<>(
            mat[0]*p.x + mat[4]*p.y + mat[8]*p.z + mat[12],
            mat[1]*p.x + mat[5]*p.y + mat[9]*p.z + mat[13],
            mat[2]*p.x + mat[6]*p.y + mat[10]*p.z + mat[14]
        );
    }
    
    // очень интересно, надо разобраться TODO
    // оказывается - решение уравнений для понимания граней усечённой пирамиды (то, как видит камера)
    void extractFrustumPlanesFromProj(double planes[6][4], const double proj[16]) {
        // Extract rows of the projection matrix (column‑major storage)
        double row0[4] = { proj[0], proj[4], proj[8], proj[12] };
        double row1[4] = { proj[1], proj[5], proj[9], proj[13] };
        double row2[4] = { proj[2], proj[6], proj[10], proj[14] };
        double row3[4] = { proj[3], proj[7], proj[11], proj[15] };

        // Right plane: row3 - row0
        planes[0][0] = row3[0] - row0[0];
        planes[0][1] = row3[1] - row0[1];
        planes[0][2] = row3[2] - row0[2];
        planes[0][3] = row3[3] - row0[3];
        // Left plane: row3 + row0
        planes[1][0] = row3[0] + row0[0];
        planes[1][1] = row3[1] + row0[1];
        planes[1][2] = row3[2] + row0[2];
        planes[1][3] = row3[3] + row0[3];
        // Bottom plane: row3 + row1
        planes[2][0] = row3[0] + row1[0];
        planes[2][1] = row3[1] + row1[1];
        planes[2][2] = row3[2] + row1[2];
        planes[2][3] = row3[3] + row1[3];
        // Top plane: row3 - row1
        planes[3][0] = row3[0] - row1[0];
        planes[3][1] = row3[1] - row1[1];
        planes[3][2] = row3[2] - row1[2];
        planes[3][3] = row3[3] - row1[3];
        // Near plane: row3 + row2
        planes[4][0] = row3[0] + row2[0];
        planes[4][1] = row3[1] + row2[1];
        planes[4][2] = row3[2] + row2[2];
        planes[4][3] = row3[3] + row2[3];
        // Far plane: row3 - row2
        planes[5][0] = row3[0] - row2[0];
        planes[5][1] = row3[1] - row2[1];
        planes[5][2] = row3[2] - row2[2];
        planes[5][3] = row3[3] - row2[3];

        // Normalize the plane equations
        for (int i = 0; i < 6; i++) {
            double len = sqrt(planes[i][0]*planes[i][0] + planes[i][1]*planes[i][1] + planes[i][2]*planes[i][2]);
            if (len != 0.0) {
                planes[i][0] /= len;
                planes[i][1] /= len;
                planes[i][2] /= len;
                planes[i][3] /= len;
            }
        }
    }
    
    void multiplyMatrices(const double a[16], const double b[16], double out[16]) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                out[j*4 + i] = 0.0;
                for (int k = 0; k < 4; ++k) {
                    out[j*4 + i] += a[k*4 + i] * b[j*4 + k];
                }
            }
        }
    }

    // а здесь смотрим, видит ли камера описывающую сферу
    // если видит, то отрисовываем
    bool sphereInFrustum(double planes[6][4], const vec<>& center, double radius) {
        for (int i = 0; i < 6; i++) {
            double dist = planes[i][0]*center.x + planes[i][1]*center.y + planes[i][2]*center.z + planes[i][3];
            if (dist + radius < -1e-6) return false;
        }
        return true;
    }
    
    void Render(double t = (double)clock() / CLOCKS_PER_SEC) {
        glPushMatrix();

        double proj[16], viewMat[16];
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetDoublev(GL_MODELVIEW_MATRIX, viewMat);
        

        double planes[6][4];
        extractFrustumPlanesFromProj(planes, proj);

        // Draw textured objects (ground & sky) without culling
        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        int drawnCount = 0;   // for debugging
        for (auto obj : texturedObjects) {
            vec<> center;
            double radius;
            obj->getBoundingSphere(center, radius, t);

            vec<> eyeCenter = transformPoint(center, viewMat);

            if (sphereInFrustum(planes, eyeCenter, radius)) {
                glPushMatrix();
                obj->Draw(t);
                glPopMatrix();
                drawnCount++;
            }
        }
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

        for (auto el : Objects) {
            vec<> center;
            double radius;
            el->getBoundingSphere(center, radius, t);
            // std::cout << center << std::endl;
            // Draw bounding sphere (debug)
//            glPushMatrix();
//            glTranslated(center.x, center.y, center.z);
//            // Save current attributes to avoid interference
//            glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
//            glDisable(GL_LIGHTING);
//            glDisable(GL_TEXTURE_2D);
//            glColor3f(1, 1, 0); // yellow wireframe, change color if desired
//            glutWireSphere(radius, 10, 10); // use enough segments for smoothness
//            glPopAttrib();
//            glPopMatrix();

            // Transform center to eye space
            vec<> eyeCenter = transformPoint(center, viewMat);

            if (sphereInFrustum(planes, eyeCenter, radius)) {
                glPushMatrix();
                el->Draw(t);
                glPopMatrix();
                drawnCount++;
            }
        }

        // std::cout << "drawnCount = " << drawnCount << std::endl;
        lastDrawnCount = drawnCount;

        glPopMatrix();
    }
};
