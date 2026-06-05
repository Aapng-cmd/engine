#pragma once

#include <GL/glut.h>
#include <ctime>
#include <iostream>
#include "scene.h"
#include "vector.h"
#include "figures.h"

class animation
{
private:
    static double Time;
    double pausedTime = 0.0;
    static animation Instance;
    int W = 800, H = 600;
    double angle = 10.0, speed = 10.0;
    double PI = 3.14;
    bool isPaused = false;

    static int lastMouseX, lastMouseY;
    static bool mouseRotating;

    static bool mousePanning;
    static int lastPanX, lastPanY;
    
    static int targetFPS;
    static int lastFrameTimeMs;
    static double lastRealTime;

    static Scene scene;

    animation(int argc, char* argv[]);
    ~animation(void);

    static void Reshape(int W, int H);
    static void Display(void);
    static void Keyboard(unsigned char Key, int X, int Y);
    static void Mouse(int Button, int State, int X, int Y);
    static void Motion(int X, int Y);
    static void Idle(void);

public:
    double Z = 0, Y = 0.34202, X = 0.939693;
    double camZ = 0, camY = 53, camX = 147;

    static animation& GetRef(int argc, char* argv[]) { return Instance; }
    /** Editor / scene loader: access the live scene (objects + environment). */
    static Scene& GetScene();
    void Run();
    static void SetFPS(int fps);
};
