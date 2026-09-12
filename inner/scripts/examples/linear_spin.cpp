/**
 * Example: constant extra acceleration + slow rotation via spin.
 */
#include "object_script_api.h"

struct LinearSpin {
    const ObjectScriptApi* api = nullptr;
    ObjectScriptHost* host = nullptr;
};

extern "C" int object_script_api_version()
{
    return OBJECT_SCRIPT_API_VERSION;
}

extern "C" void* object_script_create(const ObjectScriptApi* api, ObjectScriptHost* host)
{
    auto* s = new LinearSpin;
    s->api = api;
    s->host = host;
    return s;
}

extern "C" void object_script_start(void* /*inst*/) {}

extern "C" void object_script_update(void* inst, double /*dt*/)
{
    auto* s = static_cast<LinearSpin*>(inst);
    if (!s || !s->api || !s->host)
        return;
    ObjectScriptVars* v = s->api->get_vars(s->host);
    if (!v)
        return;

    v->ax = 0.0;
    v->ay = -2.0;
    v->az = 0.0;
    v->spinX = 30.0;
    v->spinZ = 15.0;
}

extern "C" void object_script_on_collision(void* /*inst*/, int /*other*/, double /*nx*/, double /*ny*/, double /*nz*/,
                                           double /*pen*/)
{
}

extern "C" void object_script_destroy(void* inst)
{
    delete static_cast<LinearSpin*>(inst);
}
