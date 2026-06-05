#ifndef graphics_h
#define graphics_h

#include <cstdlib>
/*
So, I am on linux, and the program was built for windows, so i need to rewrite the logic via smth like
```
#include <X11/Xlib.h>
char keys_return[32];
XQueryKeymap(display, keys_return);
```
*/

//#include <winuser.h>
//#include "winuser.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#ifndef VK_SPACE
#define VK_SPACE ' '
#define VK_SHIFT 0xFFE1
#define VK_CONTROL 0xFFE3
#endif
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


/** Const Variables */
const int DEFAULT_WINDOW_WIDTH = 1400;
const int DEFAULT_WINDOW_HEIGHT = 700;

const double DEFAULT_LINE_COLOR_RGB[3] = {0.0, 0.0, 0.0};
const double DEFAULT_BACKGROUND_COLOR_RGBA[4] = {0.9, 0.9, 0.86, 1.0};

const double ORTHO_ZOOM = 700;

/// Keybinds
/* Movement */
#ifdef _WIN32
const int UP_KEY = VK_SPACE;
const int DOWN_KEY = VK_SHIFT;
#else
const int UP_KEY = ' ';
const int DOWN_KEY = 0xFFE1;
#endif
const char RIGHT_KEY = 0x44;        // D
const char LEFT_KEY = 0x41;         // A
const char FORWARD_KEY = 0x57;      // W
const char BACK_KEY = 0x53;         // S
const char OUT_KEY = 0x51;          // Q
const char IN_KEY = 0x45;           // E

/* Rotation */
const char ROT_RIGHT_KEY = 0x4C;    // L
const char ROT_LEFT_KEY = 0x4A;     // J
const char ROT_UP_KEY = 0x49;       // I
const char ROT_DOWN_KEY = 0x4B;     // K
const char ROT_OUT_KEY = 0x4F;      // O
const char ROT_IN_KEY = 0x55;       // U

/* Other */
//const char TOGGLE_ACTIVE_CAMERA_KEY = 0x54;     // T
const char TOGGLE_ACTIVE_CAMERA_KEY = 't';
#ifdef _WIN32
const int TOGGLE_MOVEMENT_MODE = VK_CONTROL;
#else
const int TOGGLE_MOVEMENT_MODE = 0xFFE3;
#endif


// Program initialization NOT OpenGL/GLUT dependent,
// as we haven't created a GLUT window yet
void initScene();

// Initialize OpenGL Graphics
void initGL();

// Callback functions for GLUT

// Draw the window - this is where all the GL actions are
void display();

// Trap and process alphanumeric keyboard events
void kbd(unsigned char key, int x, int y);

// Trap and process special keyboard events
void kbdS(int key, int x, int y);

// Handle "mouse cursor moved" events
void cursor(int x, int y);

// Calls itself after a specified time
void timer(int dummy);

// Handle mouse button pressed and released events
void mouse(int button, int state, int x, int y);

#endif /* graphics_h */
