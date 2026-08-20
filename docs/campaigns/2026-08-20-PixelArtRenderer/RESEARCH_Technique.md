# RESEARCH — the t3ssel8r technique, verified corpus (2026-08-20)

> Produced by a web-research sweep on 2026-08-20. Everything below is confirmed against a cited
> source unless marked **UNVERIFIED** or **[synthesis]**. Sources at the bottom. Sibling docs:
> [RESEARCH_UeApis.md](RESEARCH_UeApis.md) (engine integration, verified against the 5.7.4 fork),
> [RESEARCH_Codebase.md](RESEARCH_Codebase.md) (CkUsf/CkCamera state).

Scope: the "t3ssel8r look" = 3D scene → low-res orthographic render (~640×360 or smaller) →
toon/banded lighting with palette ramps → 1-texel-wide depth/normal outlines computed AT the low
resolution → snap-compensated smooth camera → sharp (box-filter) upscale to native.

Reference pipeline order (confirmed across bababuyyy's Unity 6 URP recreation, KYRIOTA, David
Holland's Godot port, and t3ssel8r's own imgur breakdown):

```
[3D scene, ortho camera, camera pos texel-snapped]
  → render color + depth + normals at LOW res (e.g. 640×360)
  → outline pass at LOW res (guarantees exactly-1-pixel edges)
  → composite outlines over color at LOW res
  → sharp/box-filter upscale to screen, UV-shifted by the snap error
  → UI at native res on top
```

---

## A. Pixel-perfect ortho camera / no pixel creep

### What "pixel creep" precisely is — five distinct causes

1. **Camera sub-texel translation.** The rasterizer's sample grid is fixed to the render target;
   moving the ortho camera by a fraction of a texel re-rasterizes every surface against a shifted
   grid — coverage of each texel changes, so pixels "crawl" along edges while the image mostly
   stays put. Fix: snap the camera to the texel grid. (ProPixelizer docs; arXiv 2603.14587; Holland.)
2. **Object sub-texel motion.** Even with a snapped camera, a slowly moving object re-rasterizes
   fractionally each frame — its own silhouette crawls. Fix: per-object snapping (optional).
   (ProPixelizer `ObjectRenderSnapable`: snap just before render, restore after, so gameplay/physics
   never see the quantization. Cost: "motion becomes clunkier" — artistic per-object choice.)
3. **Snapped-camera stutter.** Snapping alone makes camera motion advance in whole-texel steps.
   Fix: the sub-texel remainder is re-applied as a UV offset on the *upscaled* output.
4. **Perspective projection.** One world-space snap displaces near and far geometry by different
   screen amounts — *no single snap corrects all depths*. This is why the style is
   orthographic-only. ("Pixel creep can only ever be solved for orthographic projections" —
   ProPixelizer; "no single grid snap compensates for all depths" — arXiv 2603.14587.)
5. **Rotation.** Rotating the view rotates the texel grid itself; no translation snap helps.
   Practical answers: (a) lock to fixed isometric angles (t3ssel8r baseline); (b) accept full-frame
   shimmer during orbit, softened by the box-filter upscale (bababuyyy allows free orbit);
   (c) texel splatting's cubemap indexing — "rotation invariance by construction" (arXiv).

### The snap math (confirmed, code)

Texel size in world units (Unity convention: `orthographicSize` = half of view height):

```
texelWorld = 2 * orthographicSize / renderTargetHeightInPixels
```

Snap is performed on the camera position **projected into the camera's own right/up plane**
(view-aligned grid — NOT world axes), leaving view-forward untouched. Verbatim from
`IsometricCameraController.cs` (bababuyyy/unity-isometric-pixel-pipeline):

```csharp
float texelSize = _cam.orthographicSize * 2f / pixelRenderHeight;

// Convert to local space of the camera's visual perspective.
Vector3 localPos = _cam.transform.InverseTransformPoint(_truePosition);

// Snap XY to texel grid.
float snappedX = Mathf.Round(localPos.x / texelSize) * texelSize;
float snappedY = Mathf.Round(localPos.y / texelSize) * texelSize;

// Calculate rounding error.
Vector2 snapError = new Vector2(localPos.x - snappedX, localPos.y - snappedY);

// Convert error to UV space.
float uvOffsetX = snapError.x / (_cam.orthographicSize * 2f * _cam.aspect);
float uvOffsetY = snapError.y / (_cam.orthographicSize * 2f);

// Apply snapped position to pivot.
Vector3 snappedPos = new Vector3(snappedX, snappedY, localPos.z);
transform.position = _cam.transform.TransformPoint(snappedPos);

// Pass global UV offset to the upscale shader.
Shader.SetGlobalVector("_PixelPanOffset", new Vector4(uvOffsetX, uvOffsetY, 0, 0));
```

Note `uvOffset = snapError_world / viewSize_world` ≡ `snapError_in_texels / renderTargetResolution`.
In the upscale shader the compensation is one line applied **before** the sharp-filter UV math:
`uv += _PixelPanOffset.xy;`

### Render margin

So the shifted sampling window never reads outside the rendered image, the low-res target is
rendered slightly larger than what is displayed. Confirmed concrete instance (2D, identical
mechanism): voithos/godot-smooth-pixel-camera-demo renders the SubViewport at **target + 1-pixel
border** (322×182 for a 320×180 canvas) and nudges the displayed texture by the snap delta. The
snap error is bounded by ±½ texel per axis, so a 1-texel margin suffices; 1–2 texels per side for
safety with outline kernels. **[synthesis of bound; margin practice confirmed by voithos + Unity
2D Pixel Perfect padding]**. bababuyyy skips the margin and relies on clamp sampling — visible as
a 1-px edge smear worst-case; the margin is the correct fix.

### Zoom (ortho width change)

- Continuous zoom (bababuyyy): ortho size lerps; `texelSize` recomputed per frame so the snap grid
  rescales. During zoom the world-to-texel mapping changes every frame — full-frame shimmer is
  inherent and accepted; the box-filter upscale masks it. Stability returns once zoom settles.
- Quantized zoom **[standard 2D practice]**: only zoom levels where texel size stays an integer
  multiple of a screen pixel. Between integer steps, zoom by scaling the upscale quad, re-bind
  ortho size at the next integer point — **UNVERIFIED as t3ssel8r's specific method**.

### The modern alternative: texel splatting (Dylan Ebert, 2026)

Eliminates snap/offset machinery for translation *and* rotation by decoupling texels from screen:

- Scene rasterized **into cubemaps from a fixed probe origin** (G-buffer per texel: albedo,
  octahedral normal, Chebyshev radial depth, object ID; 4-MRT, 6 faces × 3 probes at 384²).
- Per-texel world reconstruction (paper Eq. 1): `p = o + (d / ‖r‖∞) · r`. Code (splat.ts):
  ```wgsl
  let chebyshev = near + radial * (far - near);
  let rawDir = texelDir(face, cu, cv);          // dominant axis = ±1
  let maxComp = max(abs(rawDir.x), max(abs(rawDir.y), abs(rawDir.z)));
  let worldPos = origin + rawDir * (chebyshev / maxComp);
  ```
- Each visible texel becomes a **world-space quad** rendered through the real camera. Translation
  stability from snapping the *probe origin* to a world grid (`GRID_STEP = 1.0` world unit).
- Three probes: eye (at camera, fills disocclusions, +0.001·w depth priority), grid (snapped,
  stable), previous (old cell). Grid-cell crossings trigger a **4×4 Bayer-dither crossfade**.
- Anti-z-fighting: per-texel 4-bit edge mask (compute pass, relative radial-depth discontinuity
  threshold 0.002); at surviving edges half-size expands with grazing angle
  (`hsEdge = halfTexel * 1.15 + 0.0005 * tanTheta`), plus a Knuth-hash per-texel depth nudge.
- Cost: <4 ms on RTX 4090, 40 fps iPhone 15 at 384²/face; shading camera-independent, cacheable.
  Limitation: fixed origin can't see occluded geometry — the eye probe fills gaps but shimmers.

Verdict for us: a fundamentally different renderer (out of scope as the primary path), but the
outline decision rule and OKLab posterization below are directly reusable.

---

## B. Sharp-bilinear / pixel-art AA upscale

### t3ssel8r's box-filter upscaler (the one to implement)

Model: the ideal result is the average of the source over each screen pixel's footprint — a
pixel-sized box filter. A bilinear tap is a box reconstruction, so the average of a texel-aligned
box comes from **one bilinear sample at a shifted UV**. Footprint estimated from derivatives, box
clamped to ≤1 texel, `tex2Dgrad` keeps mip selection stable. Verbatim (Unity Discussions
transcription of the video shader; the forum's `.xw` in the last line is a transcription bug —
`.xy` is the correct texel-size pair):

```hlsl
fixed4 frag (v2f data) : SV_Target
{
    // box size in texels covered by this screen pixel (clamped to <= 1 texel)
    float2 boxSize = clamp(fwidth(data.uv) * _MainTex_TexelSize.zw, 1e-5, 1);
    // scale uv to texel space, offset to the box's lower corner
    float2 tx = data.uv * _MainTex_TexelSize.zw - 0.5 * boxSize;
    // fraction of the texel boundary crossed by the box -> blend amount
    float2 txOffset = smoothstep(1 - boxSize, 1, frac(tx));
    // single bilinear tap at the offset position does the box average
    float2 uv = (floor(tx) + 0.5 + txOffset) * _MainTex_TexelSize.xy;
    return tex2Dgrad(_MainTex, uv, ddx(data.uv), ddy(data.uv));
}
```

Requirements: **bilinear** sampler; valid mips if minified. Properties: when texels ≥ pixels the
box clamps to 1 and hardware filtering takes over (correct minification); works for low-res
textures on 3D surfaces too (derivatives capture the local UV→screen mapping); weakest under
strong rotation/perspective distortion (box is axis-aligned in texel space). bababuyyy's
`SharpUpscale.shader` is this exact shader with `uv += _PixelPanOffset.xy` prepended.

### The simpler "AA point-sample" family (equivalent for pure upscale)

Skeleton: snap UV to texel center, re-add the fraction compressed into a ±(fwidth/2) window around
the texel seam — point sampling everywhere except a 1-screen-pixel AA band at texel boundaries,
resolved by the hardware bilinear unit.

d7samurai (HLSL, one line):
```hlsl
// p.tex is uv * texture_size
float2 pix = floor(p.tex) + min(frac(p.tex) / fwidth(p.tex), 1) - 0.5;
color = tex.Sample(bilinearSampler, pix / texture_size);
```

iq (clamp form):
```glsl
vec2 uv_iq( vec2 uv, ivec2 texture_size ) {
    vec2 pixel = uv * texture_size;
    vec2 seam = floor(pixel + 0.5);
    vec2 dudv = fwidth(pixel);
    pixel = seam + clamp( (pixel - seam) / dudv, -0.5, 0.5);
    return pixel / texture_size;
}
```

Klems "fat pixel" smoothstep variant:
```glsl
vec2 uv_klems( vec2 uv, ivec2 texture_size ) {
    vec2 pixels = uv * texture_size + 0.5;
    vec2 fl = floor(pixels);
    vec2 fr = fract(pixels);
    vec2 aa = fwidth(pixels) * 0.75;
    fr = smoothstep( vec2(0.5) - aa, vec2(0.5) + aa, fr);
    return (fl + fr - 0.5) / texture_size;
}
```

CptPotato (Godot; `textureGrad` for mip correctness — closest to t3ssel8r's):
```glsl
vec4 texturePointSmooth(sampler2D smp, vec2 uv, vec2 pixel_size)
{
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);
    vec2 lxy = sqrt(ddx * ddx + ddy * ddy);
    vec2 uv_pixels = uv / pixel_size;
    vec2 uv_pixels_floor = round(uv_pixels) - vec2(0.5);
    vec2 uv_dxy_pixels = uv_pixels - uv_pixels_floor;
    uv_dxy_pixels = clamp((uv_dxy_pixels - vec2(0.5)) * pixel_size / lxy + vec2(0.5), 0.0, 1.0);
    uv = uv_pixels_floor * pixel_size;
    return textureGrad(smp, uv + uv_dxy_pixels * pixel_size, ddx, ddy);
}
```

Fixed-scale 2D "fat pixel" (zoom-parameterized):
```glsl
vec2 uv_fat_pixel( vec2 uv, ivec2 texture_size ) {
    vec2 pixel = uv * texture_size;
    vec2 fat_pixel = floor(pixel) + 0.5;
    fat_pixel += 1 - clamp((1.0 - fract(pixel)) * texels_per_pixel, 0, 1);
    return fat_pixel / texture_size;
}
// CPU: texels_per_pixel = (low_res_height / screen_height) / camera_zoom
```

Implementation notes (d7samurai + community): premultiplied alpha for transparency; 1-texel
transparent border (also in the normal map if AA-ing normal-mapped sprites); atlases clamp UVs to
±0.5 texel inside tile borders. **Difference between families**: the box filter computes the true
area average (correct when a pixel spans >1 texel boundary per axis, and during minification);
snap-and-blend is exact only when at most one seam crosses the pixel — for the final full-screen
upscale they are visually identical. **[assessment]**

---

## C. Outline shader

Universal placement (all recreations agree): **edge detection runs at the LOW resolution, before
upscale** — that guarantees exactly-1-pixel lines. (bababuyyy README: "computing outlines at low
resolution guarantees exactly 1-pixel edges"; t3ssel8r imgur: "the main challenge … is to get
every edge pixel exactly perfect, or else the pixel art effect will be lost".)

t3ssel8r's own description (imgur, verbatim): "we need to **highlight the outward-facing edges,
and darken the outlines** of objects a bit … we first tell the GPU to compute a **depth pass and
normal pass** … the shader code can determine whether each pixel lies on an edge or not."
Community observation of early videos: "edge highlights that only appeared on **convex** edges."
Holland: kernel = up/down/left/right texels (4-tap plus, not Roberts cross); "convex edges are
isolated by taking the cross product of neighbouring texels, graded against arbitrary
heuristics"; outlines integrate with the lighting model ("when lines are lighter or darker"); a
perfect outline for all shapes/angles at this resolution may not exist.

### C.1 Canonical baseline (three.js `RenderPixelatedPass`) — verbatim

Depth edge (outer silhouette; 4-tap plus; **one-sided** clamp so only the nearer surface draws):

```glsl
float depthEdgeIndicator(float depth, vec3 normal) {
    float diff = 0.0;
    diff += clamp(getDepth(1, 0) - depth, 0.0, 1.0);
    diff += clamp(getDepth(-1, 0) - depth, 0.0, 1.0);
    diff += clamp(getDepth(0, 1) - depth, 0.0, 1.0);
    diff += clamp(getDepth(0, -1) - depth, 0.0, 1.0);
    return floor(smoothstep(0.01, 0.02, diff) * 2.) / 2.;
}
```

Normal edge (inner crease) with the two standard de-duplication tricks:

```glsl
float neighborNormalEdgeIndicator(int x, int y, float depth, vec3 normal) {
    float depthDiff = getDepth(x, y) - depth;
    vec3 neighborNormal = getNormal(x, y);

    // Edge pixels should yield to faces who's normals are closer to the bias normal.
    vec3 normalEdgeBias = vec3(1., 1., 1.);
    float normalDiff = dot(normal - neighborNormal, normalEdgeBias);
    float normalIndicator = clamp(smoothstep(-.01, .01, normalDiff), 0.0, 1.0);

    // Only the shallower pixel should detect the normal edge.
    float depthIndicator = clamp(sign(depthDiff * .25 + .0025), 0.0, 1.0);

    return (1.0 - dot(normal, neighborNormal)) * depthIndicator * normalIndicator;
}
float normalEdgeIndicator(float depth, vec3 normal) {
    float indicator = 0.0;
    indicator += neighborNormalEdgeIndicator(0, -1, depth, normal);
    indicator += neighborNormalEdgeIndicator(0,  1, depth, normal);
    indicator += neighborNormalEdgeIndicator(-1, 0, depth, normal);
    indicator += neighborNormalEdgeIndicator(1,  0, depth, normal);
    return step(0.1, indicator);
}
// composite: depth edge DARKENS, normal edge BRIGHTENS
float Strength = dei > 0.0 ? (1.0 - depthEdgeStrength * dei)
                           : (1.0 + normalEdgeStrength * nei);
gl_FragColor = texel * Strength;
```

The `normalEdgeBias` dot is the **de-doubling**: of the two pixels straddling a crease, only the
one whose normal aligns with the bias fires → 1-px, not 2-px, crease. The `depthIndicator` makes
only the *nearer* side of a crease-with-depth-step draw. KYRIOTA variant: multiply for darkening,
`pow(texel, 1 - normalStrength*nei)` for brightening.

### C.2 Production variant with grazing-angle fix (bababuyyy `OutlineShader.shader`)

3×3 view-space reconstruction (raw depth → inverse projection), then:

**Silhouette with view-angle-scaled threshold** (kills false outlines on floors/slopes at grazing
angles — the standard staircase fix):

```hlsl
// vn[4] = center view-space normal; facing = how side-on the surface is
float facing = 1.0 - vn[4].z;
float t01 = saturate((facing - _AngleZCutoff) / (1.0 - _AngleZCutoff));
float z_thresh = _ZDeltaCutoff * (t01 * _AngleZScale + 1.0);

// 8-neighbor depth discontinuity (reversed-Z: larger = closer)
if ((vp[idx].z - vp[4].z) > z_thresh) has_line = true;
// line color = darkened color of the CLOSEST neighbor
result = closest_color * _LineDarken;
```

**Crease via opposed-pair contrast** (instead of summing `1-dot(n,ni)`, which false-fires on
curvature):

```hlsl
float d0 = 1.0 - dot(vn[4], vn[cardinals[0]]);  // up
float d1 = 1.0 - dot(vn[4], vn[cardinals[1]]);  // left
float d2 = 1.0 - dot(vn[4], vn[cardinals[2]]);  // right
float d3 = 1.0 - dot(vn[4], vn[cardinals[3]]);  // down
float normalDifference = max(abs(d0 - d3), abs(d1 - d2));
normalDifference = smoothstep(_NormalSmoothLow, _NormalSmoothHigh, normalDifference);
// suppress creases adjacent to silhouettes:
float crease_weight = saturate(normalDifference - invDepthDifference);
// brighten: result = center_color * (1.0 + _CreaseBrighten);
```

Output: RGBA mask (RGB = edge color, A = intensity) composited over low-res color.

### C.3 The lighting-modulated edge (t3ssel8r's signature)

Decision rule as implemented end-to-end in texel-splatting `splat.ts` (verbatim):

```wgsl
const OUTLINE_NORMAL_THRESH: f32 = 0.7;
fn detectEdge(...) -> i32 {   // 0 none, 1 darken, 2 highlight
    // 4-tap plus kernel over the texel's neighbors
    // (a) object-ID discontinuity → silhouette:
    if (nEid != 0u && centerEid != 0u && nEid != centerEid) {
        if (centerRadial <= nRadial) { return 1; }   // nearer side darkens
        continue;
    }
    // (b) normal crease:
    if (dot(centerNormal, nNormal) < OUTLINE_NORMAL_THRESH) {
        if (dot(centerNormal, viewDir) > dot(nNormal, viewDir)) { return 2; } // more view-facing side = convex → highlight
        return 1;                                                            // else darken
    }
}
// application: shift EXACTLY ONE lighting band in OKLab lightness, pre-posterize:
let bandSize = 1.0 / 32.0;
var lab = toOKLab(color);
lab.x += select(+1.0, -1.0, edge == 1) * bandSize;   // highlight vs darken
color = fromOKLab(lab);
```

Convex/concave decision is **geometric** (which side of the crease faces the viewer more); the
edge applies as a **±1-band lightness shift before quantization** — lit edges pick the
next-brighter ramp color, shadowed regions the next-darker, i.e. highlight/shadow colors come from
the *palette*. Whether t3ssel8r additionally gates the highlight on `dot(edgeDir, lightDir)`
(sun-facing convex only) is **UNVERIFIED** — the ±1-band formulation reproduces the screenshots
and is the recommended reimplementation target. **[assessment]**

Kernel choice: every t3ssel8r-lineage implementation surveyed uses the **4-tap plus** for both
depth and normals, optionally widened to 8 neighbors for silhouette only (bababuyyy). Roberts
cross appears in generic outline literature, not in this style. **[survey conclusion]**

---

## D. Quantized / toon lighting

### The banding math

t3ssel8r (imgur, verbatim): "a fairly standard toon shader: each material has an adjustable
**highlight, midtone, and shadow color**, and some settings for **how smoothly to blend** between
the different colors." I.e. **palette-driven ramps per material**, not a global LUT.

Concrete banding implementation (bababuyyy `ToonLit.shader`, core):

```hlsl
float diffuseAmount = dot(normalWS, mainLight.direction) + _Wrap;   // wrap lighting
diffuseAmount *= _Steepness;

float cloudLight = GetCloudNoise(input.positionWS);
diffuseAmount = min(diffuseAmount, cloudLight);                     // cloud shadows clamp light

// optional Bayer 4x4 dither at band boundaries:
diffuseAmount = clamp(diffuseAmount + (bayerVal - 0.5) * _DitherStrength * 0.25, 0, 1);

// quantize into _Cuts bands, with a controllable smooth transition zone:
float cutsInv = 1.0 / float(_Cuts);
float diffuseStepped = saturate(diffuseAmount + GLSLMod(1.0 - diffuseAmount, cutsInv));
if (_ThresholdGradientSize > 0.0) {
    float nearestK = floor(diffuseAmount / cutsInv + 0.5);
    float threshold = nearestK * cutsInv;
    float halfWidth = 0.5 * cutsInv * _ThresholdGradientSize;
    float blend = smoothstep(max(0, threshold - halfWidth), min(1, threshold + halfWidth), diffuseAmount);
    diffuseStepped = saturate(lerp(threshold, min(threshold + cutsInv, 1.0), blend));
}

float lit = diffuseStepped * shadow;
// palette mode — t3ssel8r-style ramp instead of albedo*light:
finalColor = albedo * lerp(_ShadowColor.rgb, _HighlightColor.rgb, lit);
```

### Why the bands stay texel-stable

1. Lighting is evaluated **once per output texel** because the whole scene is shaded at the low
   resolution — no higher-frequency signal to alias down. **[structural, all pipelines surveyed]**
2. Under ortho + camera texel-snap, every world point's view-space position moves in exact texel
   multiples — band boundaries don't crawl on translation.
3. Band-boundary flicker from *light* movement (slow sun): smooth the directional attenuation, add
   normal noise on flat surfaces against shadow popping (Holland); ordered Bayer dithering at band
   boundaries is the other tool (bababuyyy; texel splatting uses world-space hash dither ±0.03 L).
4. Hard shadow maps flicker at low res; smooth shadows look wrong — "a balancing act" (Holland).

### Texel splatting's shading (reusable pieces)

Shading evaluated in cubemap texture space, one compute invocation per texel; boundaries align to
the texel grid by construction and cache across frames. Lighting: sun N·L + BVH shadow ray per
texel, ≤64 point lights with `(1 - d/r)²` falloff each with its own shadow ray, ambient, emissive.
Then the ±1-band edge shift (C.3), then **OKLab posterization** (verbatim `oklab.ts`):

```wgsl
fn posterize(color: vec3f) -> vec3f {
    var lab = toOKLab(color);
    let L = clamp(lab.x, 0.0, 1.0);
    lab.x = floor(L * 32.0 + 0.5) / 32.0;      // 32 lightness bands
    lab.z += (lab.x - 0.5) * 0.05;              // slight warm/cool shift with lightness
    return max(fromOKLab(lab), vec3f(0.0));
}
```

---

## E. Content-side techniques (material/content work, not renderer work)

- **Grass**: billboard quads, GPU-instanced (~35k in bababuyyy's demo). The defining trick,
  t3ssel8r's words: "each grass sprite is **shaded with the same toon coloring as the terrain it
  sits on** … the boundaries between colors get broken up by a grassy pattern." Mechanically
  (bababuyyy `GrassBlade.shader`): sprite carries only alpha/shape; albedo from the same
  world-space noise system as the terrain; lighting normal = **terrain normal passed per instance**
  (fallback `(0,1,0)`); shadow map sampled at the **instance origin**, so the whole tuft flips
  bands as one unit, in step with the ground. Grass is **excluded from the depth/normal prepasses**
  so the outline pass doesn't outline every blade. Fixed-axis camera-facing (world-up locked).
- **Wind**: vertex shader, world space at instance position — two scrolling noise samples along
  ±diverged wind directions, multiplied, thresholded, remapped to ±1, then 2D rotation of the
  quad's local XY by `windSample * swayAngle/2`; a "fake perspective" term shears sprite UVs.
- **Cloud shadows**: global scrolling noise sampled by **projecting the shaded point along the
  light direction onto a fixed cloud plane** (`t = (cloudY - wp.y)/lightDir.y`), two diverged
  scrolling samples multiplied, contrast-shaped, applied as `diffuse = min(diffuse, cloud)` so
  clouds only darken. Direction kept fixed (not sun-following) to avoid stretching at low angles.
  NOT a light function (see RESEARCH_UeApis — light functions in ortho are broken/unverified).
- **Water** (t3ssel8r imgur): "standard mix of refraction, reflection, texture displacement … The
  most important detail … is to draw a **single pixel highlight on the border between the water
  and air**, using the same pixel art edge detector." Holland: depth-fade shore, wave-line texture,
  planar reflections (SSR unsatisfactory) requiring an oblique-near-plane ortho projection, engine
  patch so opaque outlines survive into the refraction pass.
- **God rays**: t3ssel8r (via Holland): "sample the light-space depth map based on world-space
  coordinates of a series of parallel planes aligned with the light direction." Holland rewrote as
  a single **full-screen raymarch post pass** testing scene depth + sampling the directional shadow
  map (~1.7 ms, GTX 1060, 640×360). Needs shadow-map access from a custom pass — engine-coupled.
- **Fireflies / point lights**: dynamic point light entering the banded ramp + an emissive 1-texel
  sprite. The Fab plugin's "stencil point-light" implementation is **UNVERIFIED** — nearest
  documented analogue is a Custom-Stencil-marked emitter splatted as quantized discs in a PP pass.
- **Day/night**: palette swap + light intensity from sun geometry through a curve.

Renderer-vs-content split: the renderer owns low-res targets + snap/offset camera + depth/normal
inputs + outline/composite/upscale + band-quantization convention. The coupling to respect: content
that must NOT be outlined needs an opt-out from the edge detector's inputs; content that wants the
waterline highlight must be in them.

---

## F. Known failure modes catalog

| Failure | Cause | Fix (source) |
|---|---|---|
| Temporal shimmer on sub-texel camera motion | re-rasterization against shifted sample grid | texel-grid camera snap + UV shift-back + margin (§A); per-object snap for movers (ProPixelizer) |
| Stutter after snapping | whole-texel camera steps | sub-texel remainder as UV offset on the upscale |
| Edge smear at screen border when offset applied | sampling outside rendered area | render margin: +1–2 texels per side (voithos; Ocias) |
| Creep never fully fixable | perspective projection | ortho only; or texel splatting |
| Full-frame shimmer on camera rotation | texel grid rotates with view | fixed iso angles; accept during orbit; or cubemap indexing |
| Staircase/false outlines on slopes & grazing floors | depth delta across 1 texel grows ~tan(angle) | depth threshold scaled by view-facing (bababuyyy; KYRIOTA) |
| Double (2-px) crease lines | both sides of a crease fire | normal-edge bias dot + shallower-pixel-only (`sign(depthDiff*0.25+0.0025)`) (three.js) |
| Curved surfaces spuriously detected as creases | summed `1-dot(n,ni)` accumulates on curvature | opposed-pair contrast `max(|d_up-d_down|, |d_left-d_right|)` (bababuyyy) |
| Outline flicker from slow sun / band crawl | band threshold crossings ripple across flat faces | smooth directional attenuation; normal noise; Bayer dither at band edges (Holland; bababuyyy) |
| Per-blade outline noise on foliage | grass in depth/normal inputs | exclude foliage from edge-detector inputs |
| Moiré / sparkle on high-frequency detail | point sampling under minification | box filter clamps box to 1 texel, hands off to hardware trilinear via `tex2Dgrad`; keep mips |
| Screen-space effects fight the low-res buffer | SSAO/bloom sample sub-texel gradients | SSAO off (bababuyyy: artifacts with instanced grass); pixel-locked effects before upscale, atmospheric glow after **[synthesis]** |
| UI/text unreadable | UI inside the low-res target | UI at native res after upscale (all implementations) |
| Zoom breaks snap | texel size is a function of ortho size | recompute texelSize per frame + re-snap (continuous, accepts shimmer); or integer-ratio zoom levels only |
| Refresh-rate ghosting (high-Hz) | snap phase desyncs from render | snap in the same phase that renders; velocity-based smoothing on the high-res layer (notkey.studio) |
| Disocclusion shimmer (texel splatting only) | fixed probe can't see behind occluders | eye probe fills gaps; Bayer crossfade on cell changes |

---

## Sources

**Primary**
- Dylan Ebert, *Texel Splatting* — https://dylanebert.com/texel-splatting/ ; repo
  https://github.com/dylanebert/texel-splatting (mined: `src/splat.ts`, `src/lighting.ts`,
  `src/oklab.ts`, `src/post.ts`, `src/cubemap.ts`); paper arXiv:2603.14587.
- t3ssel8r, *Creating a Pixel Art Scene in Realtime 3D* (own breakdown, captions verbatim) —
  https://imgur.com/gallery/qwhbHQq
- t3ssel8r, *Crafting a Better Shader for Pixel Art Upscaling* —
  https://www.youtube.com/watch?v=d6tp43wZqps ; Patreon notes 403-walled (recovered via ports).

**Recreations with code (mined verbatim)**
- bababuyyy/unity-isometric-pixel-pipeline (Unity 6 URP; snap + pan offset, outline, toon, grass,
  cloud shadows, sharp upscale) — https://github.com/bababuyyy/unity-isometric-pixel-pipeline
- three.js `RenderPixelatedPass` — canonical depth-darken/normal-brighten edge shader.
- KYRIOTA, *Unity Pixelated Art Style In URP* — https://kyriota.com/2022/08/02/UnityPixelatedArtStyleInURP/
- David Holland, *3D Pixel Art Rendering* (Godot) — https://www.davidhol.land/articles/3d-pixel-art-rendering/ ;
  repo https://git.sr.ht/~denovodavid/3d-pixel-art-in-godot
- voithos/godot-smooth-pixel-camera-demo (margin + snap-delta offset).

**Filtering math**
- t3ssel8r box-filter transcription — https://discussions.unity.com/t/pixel-art-filter-to-fix-creep-and-jitter/1610967
- d7samurai pixel-art AA gist; Joren Joestar *Pixel Art Filtering* survey; CptPotato *Smooth Pixel
  Filtering*; Themaister *Pseudo-bandlimited pixel art filtering*.

**Camera / creep**
- ProPixelizer docs; Alex Ocias *Unity Pixel Art Camera*; notkey.studio Godot tutorial; Godot
  proposal #6389; Unity Discussions "Recreating t3ssel8r's 3D pixel art".

**UNVERIFIED / unfetchable**: t3ssel8r Patreon post bodies; light-direction gating of his edge
highlights; his exact zoom implementation; the Fab "stencil fireflies" implementation.
