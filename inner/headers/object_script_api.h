#pragma once

/**
 * Stable C ABI for per-object behaviour plugins (.so).
 * Plugins export object_script_create / update / destroy.
 * Optional (ABI v2): object_script_start, object_script_on_collision.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define OBJECT_SCRIPT_API_VERSION 2

typedef struct ObjectScriptHost ObjectScriptHost;

/** Mutable state exposed to scripts each tick. New fields are appended (v1 plugins keep working). */
typedef struct ObjectScriptVars {
    double px, py, pz;
    double vx, vy, vz;
    /** Extra acceleration (units/s²) applied by the host after update(). */
    double ax, ay, az;
    double rx, ry, rz;
    /** Euler spin speed (degrees/s) added to rx/ry/rz after update(). */
    double spinX, spinY, spinZ;
    /** Angular velocity (rad/s) for physics spin, optional. */
    double angularVx, angularVy, angularVz;
    double time;
    int objectIndex;
    /** ABI v2 */
    double pk, vk;
    double rwx, rwy, rwz;
    int isStatic;
    int is4D;
} ObjectScriptVars;

typedef struct ObjectScriptApi {
    int api_version;
    ObjectScriptVars* (*get_vars)(ObjectScriptHost* host);
    void (*log)(ObjectScriptHost* host, const char* message);
} ObjectScriptApi;

#ifdef __cplusplus
}
#endif
