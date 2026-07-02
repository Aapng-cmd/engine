#pragma once

#include <GL/glut.h>
#include "vector.h"
#include <vector>

class voxel
{
    vec<> C, Color;
    double l;
    int rnd;
    std::vector<voxel*> children;

public:
    voxel(vec<> C = vec<>(0, 0, 0), vec<> Color = vec<>(1, 1, 1), int rnd = 0, double l = 0.1) : C(C), Color(Color), rnd(rnd), l(l) {}

    void AddChild(voxel* p)
    {
        children.push_back(p);
    }

    void Draw(double t)
    {
        glPushMatrix();

        if (rnd)
            glColor3d((sin(t) + 1) / 2 * 255, (cos(t) + 1) / 2 * 255, (int)(log10(t)) % 255);
        else
            glColor3d(Color.x, Color.y, Color.z);

        glTranslated(C.x, C.y, C.z);

        glutSolidCube(l);

        for (auto child : children)
            child->Draw(t);

        glTranslated(-C.x, -C.y, -C.z);

        glPopMatrix();
    }
};

struct plate
{
    vec<> C, Color, Follow;
    std::vector<voxel*> children;

public:
    plate(vec<> C = vec<>(0, 0, 0), double w = 1, double h = 1, vec<> Color = vec<>(1, 1, 0), vec<> Follow = vec<>(1, 0, 1)) : C(C), Color(Color), Follow(Follow)
    {
        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                children.push_back(new voxel(vec<>(i * 0.1 * Follow.x, j * 0.1 * Follow.y, Follow.z), vec<>(1, 1, 0)));
            }
        }
    }

    void AddChild(voxel* p)
    {
        children.push_back(p);
    }

    void Draw(double t)
    {
        glPushMatrix();
        glColor3d(Color.x, Color.y, Color.z);

        glTranslated(C.x, C.y, C.z);

        for (auto child : children)
            child->Draw(t);

        glTranslated(-C.x, -C.y, -C.z);

        glPopMatrix();
    }
};

struct line
{
    vec<> C, Color, Follow;
    std::vector<voxel*> children;

public:
    line(double length = 10, vec<> C = vec<>(0, 0, 0), vec<> Follow = vec<>(1, 1, 1), double R = 1, int rnd = 0, vec<> Color = vec<>(1, 1, 1)) : C(C), Follow(Follow), Color(Color)
    {
        for (int i = 0; i < abs(length); i++)
        {
            children.push_back(new voxel(vec<>(i * 0.1 * R * Follow.x, i * 0.1 * R * Follow.y, i * 0.1 * R * Follow.z), Color, rnd));
        }
    }

    void AddChild(voxel* p)
    {
        children.push_back(p);
    }

    void Draw(double t)
    {
        glPushMatrix();

        glTranslated(C.x, C.y, C.z);

        for (auto child : children)
            child->Draw(t);

        glTranslated(-C.x, -C.y, -C.z);

        glPopMatrix();
    }
};
