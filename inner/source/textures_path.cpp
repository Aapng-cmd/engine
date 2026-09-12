#include "textures_path.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

static std::string dirnameOfFile(const std::string& path)
{
    size_t p = path.find_last_of('/');
    if (p == std::string::npos)
        return ".";
    if (p == 0)
        return "/";
    return path.substr(0, p);
}

#if defined(__linux__)
static std::string readExecutablePath()
{
    char buf[PATH_MAX + 1];
    ssize_t n = readlink("/proc/self/exe", buf, PATH_MAX);
    if (n < 0)
        return {};
    buf[n] = '\0';
    return std::string(buf);
}
#endif

static std::string canonicalOrOriginal(const std::string& p)
{
#if defined(__linux__)
    char out[PATH_MAX + 1];
    if (realpath(p.c_str(), out))
        return std::string(out);
#endif
    return p;
}

static bool looksLikeInnerDir(const std::string& p)
{
    if (p.empty())
        return false;
    struct stat st {};
    const std::string mk = p + "/Makefile";
    return stat(mk.c_str(), &st) == 0;
}

static std::string detectInnerRootFromExe(const std::string& exePath)
{
    if (exePath.empty())
        return {};
    std::string path = canonicalOrOriginal(exePath);
    const std::string needle = "/inner/";
    const size_t pos = path.rfind(needle);
    if (pos != std::string::npos)
        return path.substr(0, pos + std::strlen("/inner"));

    std::string dir = dirnameOfFile(path);
    const std::string candidates[] = {
        dir + "/inner",
        dir + "/../inner",
        dir + "/../../inner",
    };
    for (const std::string& c : candidates) {
        const std::string abs = canonicalOrOriginal(c);
        if (looksLikeInnerDir(abs))
            return abs;
    }
    return {};
}

static std::string gInnerOverride;
static std::string gInnerCached;

void setInnerDirectoryOverride(const std::string& absInner)
{
    if (absInner.empty())
        return;
    gInnerOverride = canonicalOrOriginal(absInner);
    gInnerCached = gInnerOverride;
}

std::string innerDirectory()
{
    if (!gInnerOverride.empty())
        return gInnerOverride;
    if (!gInnerCached.empty())
        return gInnerCached;

    if (const char* env = std::getenv("DRIVER_TEST_ROOT")) {
        if (env[0]) {
            const std::string inner = canonicalOrOriginal(std::string(env) + "/inner");
            if (looksLikeInnerDir(inner)) {
                gInnerCached = inner;
                return gInnerCached;
            }
        }
    }

#if defined(__linux__)
    std::string exe = readExecutablePath();
    if (!exe.empty()) {
        const std::string innerRoot = detectInnerRootFromExe(exe);
        if (!innerRoot.empty())
            gInnerCached = innerRoot;
        else
            gInnerCached = canonicalOrOriginal(dirnameOfFile(exe));
    } else
#endif
        gInnerCached = canonicalOrOriginal(".");

    return gInnerCached;
}

std::string texturesPath()
{
    static std::string cached;
    if (!cached.empty())
        return cached;

    const char* env = std::getenv("TEXTURES_PATH");
    if (env && env[0])
        return (cached = canonicalOrOriginal(env));

    std::string inner = innerDirectory();
    std::string joined = inner + "/../textures";
    cached = canonicalOrOriginal(joined);
    return cached;
}

std::string defaultCollisionTestScenePath()
{
    std::string inner = innerDirectory();
    return inner + "/default_collision_test.scene";
}

std::string defaultSceneFilePath()
{
    std::string inner = innerDirectory();
    std::string p = inner + "/default.scene";
#if defined(__linux__)
    char out[PATH_MAX + 1];
    if (realpath(p.c_str(), out))
        return std::string(out);
#endif
    return p;
}
