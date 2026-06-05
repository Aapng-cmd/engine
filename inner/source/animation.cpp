#include "animation.h"
#include "render_settings.h"
#include <cstdio>
#include <random>


double animation::Time = 0.0;
animation animation::Instance(0, nullptr);

int animation::lastMouseX = 0;
int animation::lastMouseY = 0;

bool animation::mouseRotating = false;
bool animation::mousePanning = false;
int animation::lastPanX = 0;
int animation::lastPanY = 0;

int animation::targetFPS = 0;
int animation::lastFrameTimeMs = 0;
double animation::lastRealTime = 0.0;

Scene animation::scene;

Scene& animation::GetScene()
{
    return scene;
}

animation::animation(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(800, 100);
    glutInitWindowSize(Instance.W, Instance.H);
    glutCreateWindow("head");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    float Pos[4] = { 0, 10, 0, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, Pos);

    glEnable(GL_TEXTURE_2D);

    glutDisplayFunc(Display);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutIdleFunc(Idle);
    glutReshapeFunc(Reshape);
}

animation::~animation(void)
{
    Instance.scene.Objects.clear();
}

void animation::Reshape(int W, int H)
{
    glViewport(0, 0, W, H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70, (double)W / H, 2, 100000 + Instance.Z);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        Instance.X + Instance.camX, Instance.Y + Instance.camY, Instance.Z + -Instance.camZ,
        Instance.camX, Instance.camY, -Instance.camZ,
        0, 1, 0
    );
}

void animation::Display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    const double camDist = std::sqrt(Instance.camX * Instance.camX + Instance.camY * Instance.camY +
                                     Instance.camZ * Instance.camZ);
    rs::setLodFromCameraDistance(camDist);
    Instance.scene.physicsCameraPos =
        vec<>(Instance.X + Instance.camX, Instance.Y + Instance.camY, Instance.Z - Instance.camZ);
    Instance.scene.Render(Time);
    
    // FPS
    glDisable(GL_DEPTH_TEST);           // text should always be on top
    glDisable(GL_LIGHTING);             // no lighting for text

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int winWidth = glutGet(GLUT_WINDOW_WIDTH);
    int winHeight = glutGet(GLUT_WINDOW_HEIGHT);
    gluOrtho2D(0, winWidth, 0, winHeight);  // set origin at bottom‑left

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // --- FPS calculation (static locals) ---
    static double fpsTimer = 0.0;
    static int frameCount = 0;
    static double currentFPS = 0.0;

    frameCount++;
    double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    if (now - fpsTimer >= 1.0) {
        currentFPS = frameCount / (now - fpsTimer);
        frameCount = 0;
        fpsTimer = now;
    }

    // --- Draw FPS string ---
    char fpsStr[32];
    sprintf(fpsStr, "FPS: %.1f", currentFPS);

    glColor3f(1.0f, 1.0f, 0.0f);        // yellow text
    glRasterPos2i(10, winHeight - 20);   // top‑left, 10 pixels from edges

    for (char* c = fpsStr; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    // --- NEW: Draw drawn objects count ---
    char drawStr[64];
    sprintf(drawStr, "Drawn: %d", Instance.scene.lastDrawnCount);
    glRasterPos2i(10, winHeight - 40);   // 20 pixels below FPS
    for (char* c = drawStr; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    if (Instance.scene.showBoundingSpheres) {
        glColor3f(0.2f, 1.0f, 0.4f);
        glRasterPos2i(10, winHeight - 60);
        const char* hint = collision::gLodO1Enabled
                                ? "Bounds (;): triangles/spheres, --O1 LOD on"
                                : "Bounds (;): green mesh or sphere parts, yellow=COM";
        for (const char* c = hint; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // --- Restore previous state ---
    glPopMatrix();                       // restore modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();                       // restore projection
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);               // re‑enable lighting (if needed for next frame)
    glEnable(GL_DEPTH_TEST);

    // END
    glRasterPos2d(-1, 1);
    glFinish();
    glutSwapBuffers();
}

void animation::Keyboard(unsigned char Key, int X, int Y)
{
    if (Key == 27)
        exit(0);
    if (Key == 'w')
    {
        Instance.camX -= Instance.speed * Instance.X;
        Instance.camZ += Instance.speed * Instance.Z;
        Instance.camY -= Instance.speed * Instance.Y;
    }
    if (Key == 's')
    {
        Instance.camX += Instance.speed * Instance.X;
        Instance.camZ -= Instance.speed * Instance.Z;
        Instance.camY += Instance.speed * Instance.Y;
    }
    if (Key == 'a')
    {
        Instance.camX -= Instance.speed * Instance.Z;
        Instance.camZ -= Instance.speed * Instance.X;
    }
    if (Key == 'd')
    {
        Instance.camX += Instance.speed * Instance.Z;
        Instance.camZ += Instance.speed * Instance.X;
    }
    if (Key == 'q')
    {
        double angle = acos(Instance.X) * sign(asin(Instance.Z) * acos(Instance.X)) - Instance.angle / 180 * Instance.PI;
        Instance.X = cos(angle);
        Instance.Z = sin(angle);
    }
    if (Key == 'e')
    {
        double angle = acos(Instance.X) * sign(asin(Instance.Z) * acos(Instance.X)) + Instance.angle / 180 * Instance.PI;
        Instance.X = cos(angle);
        Instance.Z = sin(angle);
    }
    if (Key == ' ')
    {
        Instance.camX += Instance.speed * (-Instance.X * Instance.Y);
        Instance.camY += Instance.speed * (Instance.X*Instance.X + Instance.Z*Instance.Z);
        Instance.camZ += Instance.speed * (-Instance.Y * Instance.Z);
    }
    if (Key == '2')
    {
        Instance.camX -= Instance.speed * (-Instance.X * Instance.Y);
        Instance.camY -= Instance.speed * (Instance.X*Instance.X + Instance.Z*Instance.Z);
        Instance.camZ -= Instance.speed * (-Instance.Y * Instance.Z);
    }
    if (Key == '5')
    {
        double yaw = atan2(Instance.X, Instance.Z);
        double pitch = asin(Instance.Y);
        pitch -= Instance.angle * 3.14159 / 180.0;
        if (pitch < -89.9 * Instance.PI / 180) pitch += 2 * Instance.angle * 3.14159 / 180.0;
        if (pitch >  89.9 * Instance.PI / 180) pitch -= 2 * Instance.angle * 3.14159 / 180.0;
        Instance.X = cos(pitch) * sin(yaw);
        Instance.Y = sin(pitch);
        Instance.Z = cos(pitch) * cos(yaw);
    }
    if (Key == '0')
    {
        double yaw = atan2(Instance.X, Instance.Z);
        double pitch = asin(Instance.Y);
        pitch += Instance.angle * 3.14159 / 180.0;
        if (pitch < -89.9 * Instance.PI / 180) pitch += 2 * Instance.angle * 3.14159 / 180.0;
        if (pitch >  89.9 * Instance.PI / 180) pitch -= 2 * Instance.angle * 3.14159 / 180.0;
//        std::cout << "PITCH " << pitch << std::endl;
        Instance.X = cos(pitch) * sin(yaw);
        Instance.Y = sin(pitch);
        Instance.Z = cos(pitch) * cos(yaw);
    }
    if (Key == 'p')
    {
        Instance.isPaused = !Instance.isPaused;
        if (Instance.isPaused)
            Instance.pausedTime = (double)clock() / CLOCKS_PER_SEC;
    }
    if (Key == ';')
    {
        Instance.scene.showBoundingSpheres = !Instance.scene.showBoundingSpheres;
        std::printf("Bounding spheres debug: %s\n",
                    Instance.scene.showBoundingSpheres ? "ON" : "OFF");
    }
    if (Key == 'o')
    {
        Instance.scene.addObject(rand());
    }

//    std::cout << "(" << Instance.X << ", " << Instance.Y << ", " << Instance.Z << " | "
//              << Instance.camX << ", " << Instance.camY << ", " << Instance.camZ << ")" << std::endl;
    Reshape(Instance.W, Instance.H);
}

void animation::Mouse(int Button, int State, int X, int Y)
{
    if (Button == GLUT_LEFT_BUTTON)
    {
        if (State == GLUT_DOWN)
        {
            mouseRotating = true;
            lastMouseX = X;
            lastMouseY = Y;
        }
        else
        {
            mouseRotating = false;
        }
    }
    else if (Button == GLUT_RIGHT_BUTTON)
    {
        if (State == GLUT_DOWN)
        {
            mousePanning = true;
            lastPanX = X;
            lastPanY = Y;
        }
        else
        {
            mousePanning = false;
        }
    }
}

void animation::Motion(int X, int Y)
{
    if (mouseRotating)
    {
        int dx = X - lastMouseX;
        int dy = Y - lastMouseY;
        if (dx != 0 || dy != 0)
        {
            double sensitivity = 0.5;
            double deltaYaw   = dx * sensitivity * Instance.PI / 180.0;
            double deltaPitch = dy * sensitivity * Instance.PI / 180.0;

            double yaw = atan2(Instance.X, Instance.Z);
            yaw -= deltaYaw;

            double pitch = asin(Instance.Y);
            pitch += deltaPitch;

            Instance.X = cos(pitch) * sin(yaw);
            Instance.Y = sin(pitch);
            Instance.Z = cos(pitch) * cos(yaw);

            lastMouseX = X;
            lastMouseY = Y;

            Reshape(Instance.W, Instance.H);
        }
    }
    else if (mousePanning) // две клавиши не регает
    {
        int dx = X - lastPanX;
        int dy = Y - lastPanY;

        if (dx != 0 || dy != 0)
        {
            double panSensitivity = 0.5;
            vec<> right(Instance.Z, 0, -Instance.X);
            right = !right; // нормализация
            vec<> up(0, 1, 0);
            
            // Перемещаем камеру
            Instance.camX += right.x * dx * panSensitivity;
            Instance.camZ += right.z * dx * panSensitivity;
            Instance.camY += dy * panSensitivity;

            lastPanX = X;
            lastPanY = Y;

            Reshape(Instance.W, Instance.H);
        }
    }
}

void animation::Idle(void)
{
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    double now = nowMs / 1000.0;

    if (lastRealTime == 0.0) {
        lastRealTime = now;
    }
    double delta = now - lastRealTime;
    if (Instance.isPaused) delta = 0.0;
    Time += delta;
    lastRealTime = now;

    if (targetFPS > 0) {
        int targetMs = lastFrameTimeMs + static_cast<int>(1000.0 / targetFPS);
        if (nowMs < targetMs) {
            // Busy‑wait until it's time to draw the next frame
            while (glutGet(GLUT_ELAPSED_TIME) < targetMs) {}
        }
        lastFrameTimeMs = glutGet(GLUT_ELAPSED_TIME);
    }

    glutPostRedisplay();
}

void animation::Run()
{
    glutMainLoop();
}

void animation::SetFPS(int fps)
{
    targetFPS = fps;
}
