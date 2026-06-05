#pragma once

/**
 * Fixed-function GL (GLU/GLUT) tessellation. Lower slice/stack counts = fewer triangles.
 * Runtime LOD can scale these values based on camera distance.
 */
#include <algorithm>

namespace rs {

inline constexpr int base_sph_hi_slc = 24;
inline constexpr int base_sph_hi_stk = 20;
inline constexpr int base_sph_med_slc = 18;
inline constexpr int base_sph_med_stk = 16;
inline constexpr int base_sph_lo_slc = 14;
inline constexpr int base_sph_lo_stk = 12;
inline constexpr int base_sph_tiny_slc = 8;
inline constexpr int base_sph_tiny_stk = 8;
inline constexpr int base_tor_sides = 18;
inline constexpr int base_tor_rings = 22;
inline constexpr int base_sky_slc = 32;
inline constexpr int base_sky_stk = 28;
inline constexpr int base_cone_seg = 10;
inline constexpr int base_ed_sph_slc = 22;
inline constexpr int base_ed_sph_stk = 18;
inline constexpr int base_ed_tor_s = 18;
inline constexpr int base_ed_tor_r = 22;
inline constexpr int base_ed_cyl_slc = 18;
inline constexpr int snowflake_max_seg = 24;

inline int sph_hi_slc = base_sph_hi_slc;
inline int sph_hi_stk = base_sph_hi_stk;
inline int sph_med_slc = base_sph_med_slc;
inline int sph_med_stk = base_sph_med_stk;
inline int sph_lo_slc = base_sph_lo_slc;
inline int sph_lo_stk = base_sph_lo_stk;
inline int sph_tiny_slc = base_sph_tiny_slc;
inline int sph_tiny_stk = base_sph_tiny_stk;
inline int tor_sides = base_tor_sides;
inline int tor_rings = base_tor_rings;
inline int sky_slc = base_sky_slc;
inline int sky_stk = base_sky_stk;
inline int cone_seg = base_cone_seg;
inline int ed_sph_slc = base_ed_sph_slc;
inline int ed_sph_stk = base_ed_sph_stk;
inline int ed_tor_s = base_ed_tor_s;
inline int ed_tor_r = base_ed_tor_r;
inline int ed_cyl_slc = base_ed_cyl_slc;

inline int scaleSeg(int base, double q, int minSeg = 6)
{
    return std::max(minSeg, static_cast<int>(base * q));
}

inline void setLodQuality(double q)
{
    q = std::clamp(q, 0.35, 1.25);
    sph_hi_slc = scaleSeg(base_sph_hi_slc, q);
    sph_hi_stk = scaleSeg(base_sph_hi_stk, q);
    sph_med_slc = scaleSeg(base_sph_med_slc, q);
    sph_med_stk = scaleSeg(base_sph_med_stk, q);
    sph_lo_slc = scaleSeg(base_sph_lo_slc, q, 5);
    sph_lo_stk = scaleSeg(base_sph_lo_stk, q, 5);
    sph_tiny_slc = scaleSeg(base_sph_tiny_slc, q, 4);
    sph_tiny_stk = scaleSeg(base_sph_tiny_stk, q, 4);
    tor_sides = scaleSeg(base_tor_sides, q);
    tor_rings = scaleSeg(base_tor_rings, q);
    sky_slc = scaleSeg(base_sky_slc, q);
    sky_stk = scaleSeg(base_sky_stk, q);
    cone_seg = scaleSeg(base_cone_seg, q, 6);
    ed_sph_slc = scaleSeg(base_ed_sph_slc, q);
    ed_sph_stk = scaleSeg(base_ed_sph_stk, q);
    ed_tor_s = scaleSeg(base_ed_tor_s, q);
    ed_tor_r = scaleSeg(base_ed_tor_r, q);
    ed_cyl_slc = scaleSeg(base_ed_cyl_slc, q);
}

inline void setLodFromCameraDistance(double distance)
{
    const double q = 1.25 - std::clamp(distance / 120.0, 0.0, 0.9);
    setLodQuality(q);
}

} // namespace rs
