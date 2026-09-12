#pragma once

/**
 * Runtime quality scale for weak → strong PCs.
 * Level 2 is the original low-power profile; 10 targets ~32 GB RAM / RTX 4090-class.
 */
namespace engine {

void setPowerLevel(int level);
int powerLevel();

/** 1.0 at level 2; ~0.6 at 1; ~3.2 at 10. Applied to GLUT/GLU tessellation. */
double lodQuality();
int physicsSubsteps();
int maxCollisionSubdiv();
int maxMeshVerts();
int maxMeshTris();
int maxTessSlices();
int maxTessStacks();
int maxFaceSubdiv();
int hypersphereSlices();
int hypersphereStacks();
/** 0 / 4 / 8 — hint for GLUT_MULTISAMPLE. */
int multisampleHint();
int defaultWindowWidth();
int defaultWindowHeight();
const char* powerLabel();

} // namespace engine
