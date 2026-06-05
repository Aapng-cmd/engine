#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include "animation.h"
#include "scene_loader.h"
#include "textures_path.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char* argv[])
{
    srand(time(NULL));

    int fpsLimit = 0;
    const char* scenePath = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-sync") == 0 && i + 1 < argc) {
            fpsLimit = atoi(argv[i + 1]);
            if (fpsLimit <= 0) {
                fprintf(stderr, "Invalid fps limit. Using unlimited.\n");
                fpsLimit = 0;
            }
            ++i;
        } else if (strcmp(argv[i], "-scene") == 0 && i + 1 < argc) {
            scenePath = argv[++i];
        } else if (argv[i][0] != '-' && scenePath == nullptr) {
            scenePath = argv[i];
        }
    }

    animation& anim = animation::GetRef(argc, argv);
    anim.SetFPS(fpsLimit);

    const std::string defaultScene = defaultSceneFilePath();
    const std::string loadPathStr = scenePath ? std::string(scenePath) : defaultScene;
    if (!loadEditorSceneFile(animation::GetScene(), loadPathStr)) {
        fprintf(stderr, "Warning: could not load scene '%s' (using empty object list from file failure).\n",
                loadPathStr.c_str());
    }

    anim.Run();
    return 0;
}
