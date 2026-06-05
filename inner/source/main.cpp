#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include "animation.h"
#include "collision_mesh.h"
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
    static std::string collisionTestPath;
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
        } else if (strcmp(argv[i], "--O1") == 0 || strcmp(argv[i], "-O1") == 0) {
            collision::gLodO1Enabled = true;
            fprintf(stderr, "Collision LOD --O1 enabled (distance-based triangle density).\n");
        } else if (strcmp(argv[i], "--collision-test") == 0) {
            collisionTestPath = defaultCollisionTestScenePath();
            scenePath = collisionTestPath.c_str();
        } else if (strcmp(argv[i], "--no-info") == 0) {
            animation::GetScene().showHud = false;
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
