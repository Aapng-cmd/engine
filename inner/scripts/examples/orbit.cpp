/**
 * Example object script: circular motion in XZ, bobbing on Y.
 * Build: make -C inner/scripts
 */
#include "object_script_api.h"

#include <cmath>

struct OrbitScript {
    const ObjectScriptApi* api = nullptr;
    ObjectScriptHost* host = nullptr;
    double radius = 4.0;
    double height = 3.0;
    double speed = 1.2;
};

extern "C" int object_script_api_version()
{
    return OBJECT_SCRIPT_API_VERSION;
}

extern "C" void* object_script_create(const ObjectScriptApi* api, ObjectScriptHost* host)
{
    auto* s = new OrbitScript;
    s->api = api;
    s->host = host;
    if (api && api->log)
        api->log(host, "orbit script loaded");
    return s;
}

extern "C" void object_script_start(void* /*inst*/) {}

extern "C" void object_script_update(void* inst, double dt)
{
    auto* s = static_cast<OrbitScript*>(inst);
    if (!s || !s->api || !s->host)
        return;
    ObjectScriptVars* v = s->api->get_vars(s->host);
    if (!v)
        return;

    const double t = v->time + dt;
    v->px = std::cos(t * s->speed) * s->radius;
    v->pz = std::sin(t * s->speed) * s->radius;
    v->py = s->height + std::sin(t * s->speed * 2.0) * 0.35;
    v->vx = -std::sin(t * s->speed) * s->radius * s->speed;
    v->vz = std::cos(t * s->speed) * s->radius * s->speed;
    v->vy = std::cos(t * s->speed * 2.0) * 0.35 * s->speed * 2.0;
    v->ax = v->ay = v->az = 0.0;
    v->spinY = 45.0;
    v->time = t;
}

extern "C" void object_script_on_collision(void* /*inst*/, int /*other*/, double /*nx*/, double /*ny*/, double /*nz*/,
                                           double /*pen*/)
{
}

extern "C" void object_script_destroy(void* inst)
{
    delete static_cast<OrbitScript*>(inst);
}
