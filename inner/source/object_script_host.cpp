#include "object_script_host.h"
#include "textures_path.h"

#include <dlfcn.h>
#include <iostream>

ObjectScriptHost::ObjectScriptHost()
{
    api_.api_version = OBJECT_SCRIPT_API_VERSION;
    api_.get_vars = &ObjectScriptHost::getVarsCallback;
    api_.log = &ObjectScriptHost::logCallback;
}

ObjectScriptHost::~ObjectScriptHost()
{
    clear();
}

void ObjectScriptHost::clear()
{
    for (ObjectInstance& inst : objects_) {
        destroyInstance(inst);
    }
    objects_.clear();
    for (auto& kv : modules_) {
        PluginModule* mod = kv.second;
        if (mod && mod->handle)
            dlclose(mod->handle);
        delete mod;
    }
    modules_.clear();
    activeBridge_ = nullptr;
}

void ObjectScriptHost::resize(size_t objectCount)
{
    if (objects_.size() == objectCount)
        return;
    while (objects_.size() > objectCount) {
        ObjectInstance& back = objects_.back();
        destroyInstance(back);
        objects_.pop_back();
    }
    objects_.resize(objectCount);
}

void ObjectScriptHost::setScriptPath(size_t index, const std::string& path)
{
    if (index >= objects_.size())
        objects_.resize(index + 1);
    if (objects_[index].path != path) {
        destroyInstance(objects_[index]);
        objects_[index].path = path;
    }
}

const std::string& ObjectScriptHost::scriptPath(size_t index) const
{
    static const std::string kEmpty;
    if (index >= objects_.size())
        return kEmpty;
    return objects_[index].path;
}

std::string ObjectScriptHost::resolveScriptPath(const std::string& path)
{
    if (path.empty())
        return {};
    if (!path.empty() && path[0] == '/')
        return path;
    const std::string inner = innerDirectory();
    if (!inner.empty())
        return inner + "/" + path;
    return path;
}

void ObjectScriptHost::copyBridgeToVars(const ObjectScriptBridge& b, ObjectScriptVars& vars) const
{
    vars.px = b.px;
    vars.py = b.py;
    vars.pz = b.pz;
    vars.vx = b.vx;
    vars.vy = b.vy;
    vars.vz = b.vz;
    vars.ax = b.ax;
    vars.ay = b.ay;
    vars.az = b.az;
    vars.rx = b.rx;
    vars.ry = b.ry;
    vars.rz = b.rz;
    vars.spinX = b.spinX;
    vars.spinY = b.spinY;
    vars.spinZ = b.spinZ;
    vars.angularVx = b.angularVx;
    vars.angularVy = b.angularVy;
    vars.angularVz = b.angularVz;
    vars.time = b.time;
    vars.objectIndex = b.objectIndex;
    vars.pk = b.pk;
    vars.vk = b.vk;
    vars.rwx = b.rwx;
    vars.rwy = b.rwy;
    vars.rwz = b.rwz;
    vars.isStatic = b.isStatic ? 1 : 0;
    vars.is4D = b.is4D ? 1 : 0;
}

void ObjectScriptHost::copyVarsToBridge(const ObjectScriptVars& vars, ObjectScriptBridge& io) const
{
    io.px = vars.px;
    io.py = vars.py;
    io.pz = vars.pz;
    io.vx = vars.vx;
    io.vy = vars.vy;
    io.vz = vars.vz;
    io.ax = vars.ax;
    io.ay = vars.ay;
    io.az = vars.az;
    io.rx = vars.rx;
    io.ry = vars.ry;
    io.rz = vars.rz;
    io.spinX = vars.spinX;
    io.spinY = vars.spinY;
    io.spinZ = vars.spinZ;
    io.angularVx = vars.angularVx;
    io.angularVy = vars.angularVy;
    io.angularVz = vars.angularVz;
    io.pk = vars.pk;
    io.vk = vars.vk;
    io.rwx = vars.rwx;
    io.rwy = vars.rwy;
    io.rwz = vars.rwz;
}

ObjectScriptHost::PluginModule* ObjectScriptHost::loadModule(const std::string& resolvedPath)
{
    if (resolvedPath.empty())
        return nullptr;
    auto it = modules_.find(resolvedPath);
    if (it != modules_.end()) {
        ++it->second->refCount;
        return it->second;
    }

    void* handle = dlopen(resolvedPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cerr << "ObjectScript: dlopen failed: " << dlerror() << " (" << resolvedPath << ")\n";
        return nullptr;
    }

    auto* mod = new PluginModule;
    mod->handle = handle;
    mod->api_version = reinterpret_cast<int (*)()>(dlsym(handle, "object_script_api_version"));
    mod->create = reinterpret_cast<void* (*)(const ObjectScriptApi*, ObjectScriptHost*)>(
        dlsym(handle, "object_script_create"));
    mod->start = reinterpret_cast<void (*)(void*)>(dlsym(handle, "object_script_start"));
    mod->update = reinterpret_cast<void (*)(void*, double)>(dlsym(handle, "object_script_update"));
    mod->on_collision = reinterpret_cast<void (*)(void*, int, double, double, double, double)>(
        dlsym(handle, "object_script_on_collision"));
    mod->destroy = reinterpret_cast<void (*)(void*)>(dlsym(handle, "object_script_destroy"));

    if (!mod->create || !mod->update || !mod->destroy) {
        std::cerr << "ObjectScript: missing exports in " << resolvedPath << "\n";
        dlclose(handle);
        delete mod;
        return nullptr;
    }
    if (mod->api_version) {
        const int ver = mod->api_version();
        if (ver != 1 && ver != 2) {
            std::cerr << "ObjectScript: unsupported API version " << ver << " in " << resolvedPath << "\n";
            dlclose(handle);
            delete mod;
            return nullptr;
        }
    }

    mod->refCount = 1;
    modules_[resolvedPath] = mod;
    return mod;
}

void ObjectScriptHost::unloadModule(PluginModule* mod)
{
    if (!mod)
        return;
    --mod->refCount;
    if (mod->refCount > 0)
        return;
    for (auto it = modules_.begin(); it != modules_.end(); ++it) {
        if (it->second == mod) {
            if (mod->handle)
                dlclose(mod->handle);
            delete mod;
            modules_.erase(it);
            break;
        }
    }
}

void ObjectScriptHost::destroyInstance(ObjectInstance& inst)
{
    if (inst.instance && inst.module && inst.module->destroy)
        inst.module->destroy(inst.instance);
    inst.instance = nullptr;
    inst.started = false;
    if (inst.module) {
        unloadModule(inst.module);
        inst.module = nullptr;
    }
}

void ObjectScriptHost::syncInstances()
{
    for (size_t i = 0; i < objects_.size(); ++i) {
        ObjectInstance& inst = objects_[i];
        if (inst.path.empty()) {
            destroyInstance(inst);
            continue;
        }
        const std::string resolved = resolveScriptPath(inst.path);
        if (inst.instance && inst.module) {
            auto found = modules_.find(resolved);
            if (found != modules_.end() && found->second == inst.module)
                continue;
        }
        destroyInstance(inst);
        PluginModule* mod = loadModule(resolved);
        if (!mod)
            continue;
        inst.module = mod;
        inst.instance = mod->create(&api_, reinterpret_cast<ObjectScriptHost*>(this));
        inst.started = false;
    }
}

ObjectScriptVars* ObjectScriptHost::getVarsCallback(ObjectScriptHost* host)
{
    if (!host || !host->activeBridge_)
        return nullptr;
    return &host->varsScratch_;
}

void ObjectScriptHost::logCallback(ObjectScriptHost* /*host*/, const char* message)
{
    if (message)
        std::cerr << "[ObjectScript] " << message << "\n";
}

bool ObjectScriptHost::runStart(size_t index, ObjectScriptBridge& io)
{
    if (index >= objects_.size() || objects_[index].path.empty())
        return false;
    ObjectInstance& inst = objects_[index];
    if (!inst.instance || !inst.module || inst.started)
        return false;
    inst.started = true;
    if (!inst.module->start)
        return true;
    copyBridgeToVars(io, varsScratch_);
    activeBridge_ = &io;
    inst.module->start(inst.instance);
    copyVarsToBridge(varsScratch_, io);
    activeBridge_ = nullptr;
    return true;
}

bool ObjectScriptHost::runUpdate(size_t index, ObjectScriptBridge& io, double dt)
{
    if (index >= objects_.size() || objects_[index].path.empty())
        return false;
    ObjectInstance& inst = objects_[index];
    if (!inst.instance || !inst.module || !inst.module->update)
        return false;

    if (!inst.started)
        runStart(index, io);

    copyBridgeToVars(io, varsScratch_);
    activeBridge_ = &io;
    inst.module->update(inst.instance, dt);
    copyVarsToBridge(varsScratch_, io);
    activeBridge_ = nullptr;
    return true;
}

bool ObjectScriptHost::runCollision(size_t index, ObjectScriptBridge& io, int otherIndex, double nx, double ny,
                                    double nz, double pen)
{
    if (index >= objects_.size() || objects_[index].path.empty())
        return false;
    ObjectInstance& inst = objects_[index];
    if (!inst.instance || !inst.module || !inst.module->on_collision)
        return false;
    copyBridgeToVars(io, varsScratch_);
    activeBridge_ = &io;
    inst.module->on_collision(inst.instance, otherIndex, nx, ny, nz, pen);
    copyVarsToBridge(varsScratch_, io);
    activeBridge_ = nullptr;
    return true;
}
