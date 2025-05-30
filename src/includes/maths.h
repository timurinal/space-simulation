#ifndef MATHS_H
#define MATHS_H

// --- Central definition ---
constexpr double SU_IN_KM = 100.0; // 1 SU = 1 km
constexpr double KM_IN_M = 1000.0;

constexpr double GravitationalConstant = 6.67430e-20;

// --- Derived constants ---
constexpr double SU_IN_M = SU_IN_KM * KM_IN_M;
constexpr double M_IN_SU = 1.0 / SU_IN_M;
constexpr double KM_IN_SU = 1.0 / SU_IN_KM;

// --- Conversion functions ---
inline double kmToSu(double d) { return d * KM_IN_SU; }
inline double suToKm(double d) { return d * SU_IN_KM; }

inline glm::dvec3 kmToSu(glm::dvec3 d) { return d * KM_IN_SU; }
inline glm::dvec3 suToKm(glm::dvec3 d) { return d * SU_IN_KM; }

inline double mToKm(double d) { return d / KM_IN_M; }
inline double kmToM(double d) { return d * KM_IN_M; }

inline glm::dvec3 mToKm(glm::dvec3 d) { return d / KM_IN_M; }
inline glm::dvec3 kmToM(glm::dvec3 d) { return d * KM_IN_M; }

inline double mToSu(double d) { return d * M_IN_SU; }
inline double suToM(double d) { return d * SU_IN_M; }

// Returns the surface gravity of a body in m/s^2
inline double deriveSurfaceGravity(double mass, double radius) {
    return ((GravitationalConstant * mass) / (radius * radius)) * 1000.0;
}

#endif // MATHS_H
