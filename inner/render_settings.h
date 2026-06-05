#pragma once

/**
 * Fixed-function GL (GLU/GLUT) tessellation. Lower slice/stack counts = fewer triangles.
 * Tune here for weak GPUs; one place drives scene figures + editor preview meshes.
 */
namespace rs {

inline constexpr int sph_hi_slc = 24;
inline constexpr int sph_hi_stk = 20;
inline constexpr int sph_med_slc = 18;
inline constexpr int sph_med_stk = 16;
inline constexpr int sph_lo_slc = 14;
inline constexpr int sph_lo_stk = 12;
inline constexpr int sph_tiny_slc = 8;
inline constexpr int sph_tiny_stk = 8;

inline constexpr int tor_sides = 18;
inline constexpr int tor_rings = 22;

inline constexpr int sky_slc = 32;
inline constexpr int sky_stk = 28;

inline constexpr int cone_seg = 10;

inline constexpr int ed_sph_slc = 22;
inline constexpr int ed_sph_stk = 18;
inline constexpr int ed_tor_s = 18;
inline constexpr int ed_tor_r = 22;
inline constexpr int ed_cyl_slc = 18;

/** Caps snowflake sphere stacks/slices (constructor allows large `shades`). */
inline constexpr int snowflake_max_seg = 24;

} // namespace rs
