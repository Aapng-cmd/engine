#pragma once

#include "object_script_api.h"
#include "figures.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

/** Per-object bridge between Scene::BodyState and a script plugin. */
struct ObjectScriptBridge {
    double px = 0, py = 0, pz = 0;
    double vx = 0, vy = 0, vz = 0;
    double ax = 0, ay = 0, az = 0;
    double rx = 0, ry = 0, rz = 0;
    double spinX = 0, spinY = 0, spinZ = 0;
    double angularVx = 0, angularVy = 0, angularVz = 0;
    double time = 0;
    int objectIndex = -1;
    double pk = 0, vk = 0;
    double rwx = 0, rwy = 0, rwz = 0;
    bool isStatic = false;
    bool is4D = false;
    based* obj = nullptr;
};

/**
 * Loads .so plugins (dlopen) and runs object_script_update each physics substep.
 */
class ObjectScriptHost {
public:
    ObjectScriptHost();
    ~ObjectScriptHost();

    ObjectScriptHost(const ObjectScriptHost&) = delete;
    ObjectScriptHost& operator=(const ObjectScriptHost&) = delete;

    void clear();
    void resize(size_t objectCount);
    void setScriptPath(size_t index, const std::string& path);
    const std::string& scriptPath(size_t index) const;

    /** (Re)create plugin instances when paths change. */
    void syncInstances();

    /** Optional Start(); no-op if already called or export missing. */
    bool runStart(size_t index, ObjectScriptBridge& io);
    /** Call plugin update; writes bridge fields back. Returns false if no script. */
    bool runUpdate(size_t index, ObjectScriptBridge& io, double dt);
    bool runCollision(size_t index, ObjectScriptBridge& io, int otherIndex, double nx, double ny, double nz,
                      double pen);

    /** Resolve relative script path against repo / inner directory. */
    static std::string resolveScriptPath(const std::string& path);

private:
    struct PluginModule {
        void* handle = nullptr;
        int (*api_version)() = nullptr;
        void* (*create)(const ObjectScriptApi*, ObjectScriptHost*) = nullptr;
        void (*start)(void*) = nullptr;
        void (*update)(void*, double) = nullptr;
        void (*on_collision)(void*, int, double, double, double, double) = nullptr;
        void (*destroy)(void*) = nullptr;
        int refCount = 0;
    };

    struct ObjectInstance {
        std::string path;
        PluginModule* module = nullptr;
        void* instance = nullptr;
        bool started = false;
    };

    PluginModule* loadModule(const std::string& resolvedPath);
    void unloadModule(PluginModule* mod);
    void destroyInstance(ObjectInstance& inst);
    void copyBridgeToVars(const ObjectScriptBridge& b, ObjectScriptVars& vars) const;
    void copyVarsToBridge(const ObjectScriptVars& vars, ObjectScriptBridge& io) const;

    static ObjectScriptVars* getVarsCallback(ObjectScriptHost* host);
    static void logCallback(ObjectScriptHost* host, const char* message);

    ObjectScriptApi api_{};
    std::unordered_map<std::string, PluginModule*> modules_;
    std::vector<ObjectInstance> objects_;
    ObjectScriptBridge* activeBridge_ = nullptr;
    ObjectScriptVars varsScratch_{};
};
