# Rendering Ideas for Planetary Simulation

## General Rendering Strategy
- Support rendering planets at vastly different distances and detail levels.
- Use different rendering techniques based on camera distance to the body.

---

## Near-Field Rendering (High Detail)
**Technique:** Triangle-based rasterisation  
**Use Case:** Current planet under focus

- Full geometry with terrain mesh (e.g., elevation data, tessellation)
- Atmosphere effects using procedural shaders or scattering
- High-precision depth and lighting
- Dynamic LOD based on camera altitude
- Supports landing/close flyovers

---

## Mid-Range Rendering (Medium Detail)
**Technique:** Ray-marched SDF (Signed Distance Fields)  
**Use Case:** Nearby planets or moons

- Render shape and large terrain features using SDF
- Approximate shading and curvature
- Allows smooth transition from triangle-based to distant representation
- Can provide silhouette shadows

---

## Far-Field Rendering (Low Detail)
**Technique:** Ray-traced spheres  
**Use Case:** Distant celestial bodies

- Simple colour/texture sphere representation
- Ignore terrain, detail shadows, etc.
- Focus on accurate position and lighting
- Useful for rendering dozens/hundreds of distant bodies efficiently

---

## Shadows
- Ray-traced shadows for SDF and sphere objects
- Consider penumbra soft shadows for realism
- Shadow transitions based on distance

---

## Atmosphere Rendering
- Use high-precision projection matrix near planet
- Split projection matrices for local/near space rendering
- Procedural atmosphere shader highly sensitive to depth precision
- Possibly a dedicated framebuffer for atmosphere only

---

## 🧠 Miscellaneous Ideas
- Cascaded projection matrices for nearby, mid, and far objects
- Surface gravity is derived for display, not simulation
- Simulation runs in km for physics; SU or m for rendering
- Time step simulation using velocity-verlet for better stability
