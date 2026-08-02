#include "CkParticlesEditor/Generator/CkParticles_TextureGenerator.h"

#include "CkParticlesEditor_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------
// Procedural VFX texture baker: plain math sampled per-pixel into UTexture2D assets — deterministic, art-free and
// resolution-independent stand-ins for the textures a VFX artist would paint. The master materials combine them.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor::TexGenLocal
{
    static const TCHAR* TexDir = TEXT("/CkFoundation/CkParticles/Textures");

    // ---- Hash / value-noise / FBM (tileable over a period so octaves stay seamless) -------------------------------
    static auto Hash(int32 InX, int32 InY) -> float
    {
        uint32 n = uint32(InX) * 374761393u + uint32(InY) * 668265263u;
        n = (n ^ (n >> 13)) * 1274126177u;
        n =  n ^ (n >> 16);
        return float(n & 0x00FFFFFFu) / 16777216.0f; // [0,1)
    }

    static auto Wrap(int32 InV, int32 InPeriod) -> int32
    { return ((InV % InPeriod) + InPeriod) % InPeriod; }

    // Smoothed value noise, tileable on [0,Period).
    static auto ValueNoise(float InX, float InY, int32 InPeriod) -> float
    {
        const int32 Xi = FMath::FloorToInt(InX);
        const int32 Yi = FMath::FloorToInt(InY);
        const float Xf = InX - Xi;
        const float Yf = InY - Yi;
        const float U  = Xf * Xf * (3.0f - 2.0f * Xf);
        const float V  = Yf * Yf * (3.0f - 2.0f * Yf);

        const float A = Hash(Wrap(Xi, InPeriod),     Wrap(Yi, InPeriod));
        const float B = Hash(Wrap(Xi + 1, InPeriod), Wrap(Yi, InPeriod));
        const float C = Hash(Wrap(Xi, InPeriod),     Wrap(Yi + 1, InPeriod));
        const float D = Hash(Wrap(Xi + 1, InPeriod), Wrap(Yi + 1, InPeriod));

        return FMath::Lerp(FMath::Lerp(A, B, U), FMath::Lerp(C, D, U), V);
    }

    // Fractal sum. InTiles = base period (the texture wraps if you sample with x in [0,InTiles)).
    static auto Fbm(float InX, float InY, int32 InOctaves, int32 InTiles) -> float
    {
        float Sum = 0.0f, Amp = 0.5f, Freq = 1.0f, Norm = 0.0f;
        int32 Period = InTiles;
        for (int32 i = 0; i < InOctaves; ++i)
        {
            Sum  += Amp * ValueNoise(InX * Freq, InY * Freq, Period);
            Norm += Amp;
            Amp  *= 0.5f;
            Freq *= 2.0f;
            Period *= 2;
        }
        return Sum / Norm;
    }

    // Cellular (Worley) F1 distance, tileable on [0,Period). Returns ~0 at cell centers, larger at borders.
    static auto Voronoi(float InX, float InY, int32 InPeriod) -> float
    {
        const int32 Xi = FMath::FloorToInt(InX);
        const int32 Yi = FMath::FloorToInt(InY);
        float MinD = 10.0f;
        for (int32 dy = -1; dy <= 1; ++dy)
        for (int32 dx = -1; dx <= 1; ++dx)
        {
            const int32 Cx = Xi + dx;
            const int32 Cy = Yi + dy;
            const float Jx = Hash(Wrap(Cx, InPeriod), Wrap(Cy, InPeriod));
            const float Jy = Hash(Wrap(Cx, InPeriod) + 17, Wrap(Cy, InPeriod) + 31);
            const float Px = Cx + Jx;
            const float Py = Cy + Jy;
            MinD = FMath::Min(MinD, FMath::Sqrt((InX - Px) * (InX - Px) + (InY - Py) * (InY - Py)));
        }
        return MinD;
    }

    static auto Saturate(float InV) -> float { return FMath::Clamp(InV, 0.0f, 1.0f); }
    static auto Smooth(float InE0, float InE1, float InX) -> float { return FMath::SmoothStep(InE0, InE1, InX); }

    // A measured profile, sampled verbatim. Where a paint's falloff fits no closed form, the honest bake is the
    // measurement itself: the anchors below each texture ARE the numbers read off the corpus PNG, interpolated
    // linearly and clamped to the end values outside the sampled range. Px_LightStrip established the idiom
    // with a hand-rolled if-chain; this is the same thing with the anchors kept legible.
    struct FCk_ProfileAnchor
    {
        float X;
        float Value;
    };

    static auto Sample_Profile(float InX, const FCk_ProfileAnchor* InAnchors, int32 InNum) -> float
    {
        if (InX <= InAnchors[0].X)
        { return InAnchors[0].Value; }

        for (int32 Index = 1; Index < InNum; ++Index)
        {
            if (InX > InAnchors[Index].X)
            { continue; }

            const auto& Lo = InAnchors[Index - 1];
            const auto& Hi = InAnchors[Index];
            return FMath::Lerp(Lo.Value, Hi.Value, (InX - Lo.X) / FMath::Max(Hi.X - Lo.X, 1.0e-6f));
        }
        return InAnchors[InNum - 1].Value;
    }

    // IEC 61966-2-1 decode. A paint measured off a corpus PNG is measured in ENCODED bytes; a per-pixel function
    // must hand back LINEAR colour, because an sRGB bake re-encodes on quantization and would otherwise apply the
    // curve twice.
    static auto SrgbToLinear(float InEncoded) -> float
    {
        return InEncoded <= 0.04045f
            ? InEncoded / 12.92f
            : FMath::Pow((InEncoded + 0.055f) / 1.055f, 2.4f);
    }

    // Frame layout of a 2x2 flipbook sheet: four frames, row-major, each a quarter of the texture. The returned
    // UV is frame-LOCAL so one paint function serves all four frames and evolves them by the frame index.
    struct FSheetSample
    {
        int32 Frame;
        float U;
        float V;
    };

    static auto Sample_Sheet2x2(float U, float V) -> FSheetSample
    {
        const int32 Fx = U < 0.5f ? 0 : 1;
        const int32 Fy = V < 0.5f ? 0 : 1;
        return FSheetSample{Fy * 2 + Fx, U * 2.0f - Fx, V * 2.0f - Fy};
    }

    // ---- Per-pixel look functions. (u,v) in [0,1]; return packed FLinearColor (channels documented per texture). ---

    static auto Px_Glow(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f; // 0 center, 1 at mid-edge
        const float Body = FMath::Pow(Saturate(1.0f - R), 2.4f);
        const float Hot  = FMath::Pow(Saturate(1.0f - R * 1.5f), 7.0f);
        const float I = Saturate(Body * 0.85f + Hot);
        return FLinearColor(I, I, I, I);
    }

    static auto Px_Flare(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);
        const float Spikes = FMath::Pow(FMath::Abs(FMath::Cos(Ang * 3.0f)), 36.0f); // 6 lobes
        const float Beam   = Spikes * FMath::Pow(Saturate(1.0f - R), 1.6f) * 1.6f;
        const float Core   = FMath::Pow(Saturate(1.0f - R), 5.0f);
        const float I = Saturate(Beam + Core);
        return FLinearColor(I, I, I, I);
    }

    // Density in RGB, erosion mask in A — the SoftSmoke material dissolves along the mask over life.
    static auto Px_Smoke(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Mask    = Saturate(1.0f - Smooth(0.30f, 1.05f, R));
        const float Density = Fbm(U * 3.5f, V * 3.5f, 6, 4);
        const float Detail  = Fbm(U * 8.0f + 11.0f, V * 8.0f + 7.0f, 5, 8);
        const float N       = Saturate(Density * 0.8f + Detail * 0.35f);
        const float Smoke   = Saturate(N * 1.5f - 0.30f) * Mask;
        const float Erode   = Fbm(U * 5.0f + 31.0f, V * 5.0f + 23.0f, 4, 6);
        return FLinearColor(Smoke, Smoke, Smoke, Erode);
    }

    static auto Px_Electric(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float N  = Fbm(U * 5.0f, V * 5.0f, 5, 8);
        const float Ridged = FMath::Pow(1.0f - FMath::Abs(N * 2.0f - 1.0f), 5.0f);
        const float Fil  = Ridged * FMath::Pow(Saturate(1.0f - R), 1.4f) * 1.9f;
        const float Core = FMath::Pow(Saturate(1.0f - R), 7.0f);
        const float I = Saturate(Fil + Core);
        return FLinearColor(I, I, I, I);
    }

    // For the velocity-aligned sprite renderer: a thin bright streak along U with noise breakup.
    static auto Px_Streak(float U, float V) -> FLinearColor
    {
        const float Yd  = FMath::Abs(V - 0.5f) * 2.0f;
        const float Core = FMath::Pow(Saturate(1.0f - Yd), 9.0f);
        const float Len  = FMath::Pow(Saturate(1.0f - FMath::Abs(U - 0.5f) * 2.0f), 0.6f);
        const float Jit  = 0.65f + 0.35f * Fbm(U * 12.0f, V * 3.0f, 4, 12);
        const float I = Saturate(Core * Len * Jit * 1.5f);
        return FLinearColor(I, I, I, I);
    }

    // Slash carrier: bright leading head near V=0, wispy body, fading tail. The SweepErode material pans it
    // along V over a sweep mesh, so the head appears to travel along the arc.
    static auto Px_SweepStreak(float U, float V) -> FLinearColor
    {
        const float Hx   = FMath::Pow(Saturate(1.0f - FMath::Abs(U - 0.5f) * 1.9f), 1.1f);
        const float Head = FMath::Pow(Saturate(1.0f - FMath::Abs(V - 0.10f) / 0.14f), 2.0f);
        const float Body = Smooth(0.02f, 0.18f, V) * (1.0f - Smooth(0.40f, 0.85f, V));
        const float Wisp = 0.55f + 0.45f * Fbm(U * 5.0f + 3.0f, V * 8.0f + 17.0f, 5, 5);
        const float I = Saturate((Head * 1.25f + Body * 0.55f * Wisp) * Hx);
        return FLinearColor(I, I, I, I);
    }

    // Dissolve-threshold and UV-distortion source, tuned to T_VFX_Noise_02's measured spectrum — 70.7% of AC
    // energy at 1-8 cyc, 22.6% at 8-16, 6.3% at 16-32, so the cell/grain content must sit at 13 and 40 per
    // tile — under the measured percentile stretch (median 0.353, p1 0.059, rms contrast 0.148, skewed bright).
    // The per-component centering constants are frozen statistics of the fixed noise lattices, so the mix
    // stays a pure per-pixel function.
    static auto Px_TileNoise(float U, float V) -> FLinearColor
    {
        constexpr float SourceMedian      = 0.3529f;
        constexpr float SourceRmsContrast = 0.1476f;
        constexpr float SourceSkew        = 0.08f; // p99 sits 2.7 sigma above the median, p1 only 2.0 below

        const float Billow = Fbm(U * 5.0f, V * 5.0f, 2, 5);
        const float Cells  = Saturate(Voronoi(U * 13.0f + 3.0f, V * 13.0f + 3.0f, 13) * 0.85f);
        const float Grain  = Fbm(U * 40.0f + 5.0f, V * 40.0f + 17.0f, 1, 40);

        const float Mix = 3.9617f * (Billow - 0.4257f)
                        + 3.6941f * (Cells  - 0.3676f)
                        + 1.6485f * (Grain  - 0.4896f); // unit-variance blend weighted onto the measured bands
        const float Skewed = Mix + SourceSkew * (Mix * Mix - 1.0f);
        const float I = Saturate(Skewed * SourceRmsContrast + SourceMedian);
        return FLinearColor(I, I, I, I);
    }

    // ---- NS_BasicAttack stand-ins ---------------------------------------------------------------------------------
    //
    // The Vefects slash paints are NOT imported (maintainer decision, recipe Cookbook/NS_BasicAttack.md §6.5).
    // Each function below is parameterized from characteristics MEASURED off the corpus PNGs — per-axis intensity
    // profiles, streak counts, radial falloff exponents — quoted in recipe §7. No pixel is ever copied.
    //
    // All five are sampled through the crescent's UV convention where relevant: u runs ALONG the arc, v ACROSS the
    // band from the outer edge (v=0) to the inner one (v=1).

    // T_VFX_Slash_01 stand-in. Measured: content spans u as a cos bell covering 73% of the axis (FWHM 0.418);
    // across v a bright rim peaking at v~0.07, a body at 0.63 of the rim in the central columns falling to
    // ~0.36-0.46 toward the bell's edges (the paint's body fades along u faster than its rim), 5-6 shallow
    // motion-line ridges at 3-13% of peak, and a cut-off whose steepest fall (|dI/dv| 2.26) sits at v~0.63.
    // A body left flat at ~0.42 of the rim reads as a SEPARATE thin arc over a dark plateau instead of the
    // source's single merged swoosh, and overshoots the 4-8 cyc v-band energy by 56%.
    static auto Px_SlashArc01(float U, float V) -> FLinearColor
    {
        constexpr float PeakGain      = 1.08f;  // lands the measured 0.004 saturated fraction
        constexpr float ToneGamma     = 1.6f;   // compresses midtones onto the measured p50/p75/mean/coverage
        constexpr float BodyLevel     = 0.68f;  // post-gamma body/rim 0.627 in the central columns (measured 0.631)
        constexpr float RimAmp        = 0.30f;
        constexpr float BodyBellExtra = 0.8f;   // extra u-falloff on the body only — the measured per-column
                                                // body/rim ratio drops 0.63 -> 0.36-0.46 toward the bell edges
        constexpr float LineHalfWidthV = 0.024f;
        constexpr float MotionLineV[]   = { 0.235f, 0.337f, 0.431f, 0.489f, 0.577f }; // measured ridge positions
        constexpr float MotionLineAmp[] = { 0.080f, 0.045f, 0.065f, 0.035f, 0.055f }; // 5-11% of peak post-gamma
        constexpr int32 NumMotionLines  = 5;

        const float Bell     = FMath::Pow(FMath::Cos(HALF_PI * Saturate(FMath::Abs(U - 0.5f) / 0.43f)), 1.1f);
        const float BodyBell = FMath::Pow(Bell, BodyBellExtra);
        const float Cut      = 1.0f - Smooth(0.585f, 0.765f, V);

        float Lines = 0.0f;
        for (int32 Index = 0; Index < NumMotionLines; ++Index)
        { Lines += MotionLineAmp[Index] * Saturate(1.0f - FMath::Abs(V - MotionLineV[Index]) / LineHalfWidthV); }

        const float Body = (BodyLevel + Lines) * Cut * BodyBell;
        const float Rim  = RimAmp * FMath::Pow(Saturate(1.0f - FMath::Abs(V - 0.075f) / 0.15f), 1.5f);
        const float I = FMath::Pow(Saturate(Bell * (Body + Rim) * PeakGain), ToneGamma);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Slash_02 stand-in. Measured: the paint is SEPARABLE and piecewise LINEAR — a triangle bell along u
    // (a linear ramp's constant small u-gradient is what keeps the structure-tensor anisotropy at the measured
    // 26) times a sawtooth across v: attack from 0.38 of peak at the v=0 edge up to full at v=0.018, then a
    // linear decay to zero by v~0.135. Peak intensity 0.867. A cos bell + smoothstep reads far too isotropic.
    static auto Px_SlashArc02(float U, float V) -> FLinearColor
    {
        constexpr float BellHalfWidth = 0.37f;
        constexpr float AttackEndV    = 0.018f; // the measured v-peak of the line
        constexpr float EdgeStartFrac = 0.38f;  // intensity at the v=0 border relative to peak
        constexpr float DecayEndV     = 0.135f;
        constexpr float Peak          = 0.867f; // the measured maximum
        const float Bell = Saturate(1.0f - FMath::Abs(U - 0.5f) / BellHalfWidth);
        const float P = V < AttackEndV
            ? EdgeStartFrac + (1.0f - EdgeStartFrac) * (V / AttackEndV)
            : Saturate(1.0f - (V - AttackEndV) / (DecayEndV - AttackEndV));
        const float I = Saturate(Peak * Bell * P);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Wind_03 stand-in. Measured: the band is SOLID — no in-band pixel drops below 2% — so there is no
    // streak floor and no breakup to zero. A generalized-Gaussian envelope at v=0.215 (sigma 0.0836, falloff
    // exponent 2.5 reproduces both the 54 px 10-90 shoulder and the fast tail), carved by sparse paint dabs
    // and textured by a fine 20-cycle 2D noise that carries the measured 8-16/16-32 cyc band energy and pulls
    // the structure tensor down to the measured 4.5 (a bare envelope reads 30+).
    static auto Px_WindBandAt(float U, float V) -> float
    {
        constexpr float BandCenterV    = 0.215f;  // the measured v-peak
        constexpr float BandSigma      = 0.0836f; // from the measured FWHM 0.20
        constexpr float BandFalloffExp = 2.5f;
        constexpr float DabDepth       = 0.45f;
        constexpr float TexAmp         = 0.72f;
        const float Band = FMath::Exp(-0.5f * FMath::Pow(FMath::Abs(V - BandCenterV) / BandSigma, BandFalloffExp));
        const float Dab  = Smooth(0.55f, 0.85f, Fbm(U * 7.0f + 13.0f, V * 7.0f + 5.0f, 2, 7));
        const float Tex  = 1.0f + TexAmp * (Fbm(U * 20.0f + 29.0f, V * 20.0f + 17.0f, 2, 20) - 0.5f);
        return Saturate(Band * (1.0f - DabDepth * Dab) * Tex);
    }

    static auto Px_WindBand(float U, float V) -> FLinearColor
    {
        const float I = Px_WindBandAt(U, V);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Wind_02 stand-in. Measured against the Wind_03 paint above rather than re-derived, because the two
    // source PNGs are THE SAME IMAGE: rolling Wind_03 down by 141 of its 512 rows reproduces Wind_02 to a mean
    // absolute 1.3e-06, i.e. to quantization. Their global statistics are identical to four decimals and their
    // histogram intersection is 1.000. So this is not a new paint — it is the same paint whose band sits at
    // v 0.4902 instead of 0.2148, which matters because both textures address WRAP and the mesh UV decides
    // where along the carrier the band lands.
    static auto Px_WindBandMid(float U, float V) -> FLinearColor
    {
        constexpr float ShiftV = 141.0f / 512.0f; // the measured roll, in v
        const float I = Px_WindBandAt(U, FMath::Frac(V - ShiftV + 1.0f));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Part_01 stand-in. Measured: perfectly radially symmetric (u and v profiles identical), falloff fitting
    // pow(1 - r, 2.2) to within a bin across all ten radial rings, r normalized to the half-width.
    static auto Px_SoftParticle(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float I  = FMath::Pow(Saturate(1.0f - R), 2.2f);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Part_04 stand-in — the velocity-aligned spark streak, hence narrow in u (width) and long in v (length),
    // matching the source's width x length sprite sizing. Measured: a hard-clipped WHITE capsule (interior is
    // noise-free, 8.1% of all pixels saturated) behind a soft glow. The wall into the white core is a near-step
    // at 0.52 of the 0.156 half-width whose position wobbles slowly along the length — the local step carries
    // the high-band energy while the wobble smears the mean-profile rise to the measured 29 px. The capsule's
    // v-ends are sharp (they hold the v-gradient energy that puts the anisotropy at the measured 5.2); the
    // glow's ends stay soft and reach past the nominal half-width.
    static auto Px_SparkStreak(float U, float V) -> FLinearColor
    {
        constexpr float HalfWidthU = 0.156f; // measured
        constexpr float WobbleAmp  = 0.30f;
        constexpr float WallStart  = 0.52f;  // white plateau extent, fraction of the half-width
        constexpr float WallEnd    = 0.63f;
        constexpr float GlowAmp    = 0.40f;  // glow intensity where it meets the wall
        constexpr float GlowReach  = 1.22f;
        const float Wobble = Fbm(V * 3.0f + 9.0f, 3.0f, 1, 3);
        const float H = HalfWidthU * (1.0f + WobbleAmp * (Wobble - 0.5f));
        const float X = FMath::Abs(U - 0.5f) / H;
        const float Wall = 1.0f - Smooth(WallStart, WallEnd, X);
        const float Glow = GlowAmp * (1.0f - Smooth(WallStart, GlowReach, X));
        const float WallCap = Smooth(0.09f, 0.145f, V) * (1.0f - Smooth(0.75f, 0.775f, V));
        const float GlowCap = Smooth(0.06f, 0.13f, V)  * (1.0f - Smooth(0.60f, 0.84f, V));
        const float I = Saturate(FMath::Max(Wall * WallCap, Glow * GlowCap));
        return FLinearColor(I, I, I, I);
    }

    // ---- Colour LUT + flipbook sheet stand-ins ---------------------------------------------------------------
    //
    // T_VFX_LUT_Rainbow_01 transcription. A colour ramp is functional CONFIG, not painted art, so its stop VALUES
    // are transcribed from the measurement rather than re-derived from a formula: ten stops reproduce all 512
    // source texels to within 4.4/255, where dropping any single one costs 8-30. The source ramp is piecewise
    // linear in sRGB-ENCODED space (the shape a painted gradient has), so the stops are stored encoded and
    // interpolated there, then decoded once.
    struct FLutStop
    {
        float Pos;
        float R;
        float G;
        float B;
    };

    static auto Get_RainbowLutStops() -> TArrayView<const FLutStop>
    {
        static const FLutStop Stops[] =
        {
            { 0.0000f, 0.3137f, 0.1255f, 0.5294f }, // violet — the ramp wraps back to this at Pos 1
            { 0.1624f, 1.0000f, 0.0000f, 0.0000f }, // red
            { 0.2348f, 1.0000f, 0.4118f, 0.0000f }, // orange
            { 0.3444f, 1.0000f, 0.7922f, 0.0000f }, // amber
            { 0.3992f, 0.9922f, 0.9412f, 0.0000f }, // yellow
            { 0.4384f, 0.9098f, 0.9961f, 0.0039f }, // chartreuse
            { 0.4932f, 0.6157f, 0.9255f, 0.2745f }, // green
            { 0.6184f, 0.1882f, 0.8431f, 0.6745f }, // spring green
            { 0.8102f, 0.2000f, 0.3137f, 0.8471f }, // blue
            { 1.0000f, 0.3137f, 0.1255f, 0.5294f },
        };
        return MakeArrayView(Stops);
    }

    // The family's default gradient map, standing in for the source pack's T_VFX_WhitePixel: a ramp that is the
    // same colour everywhere samples to one, which is what makes the gradient chain inert wherever a look does
    // not deliberately drive it.
    static auto Px_LutWhite(float /*U*/, float /*V*/) -> FLinearColor
    {
        return FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static auto Px_LutRainbow(float U, float /*V*/) -> FLinearColor
    {
        const auto Stops = Get_RainbowLutStops();

        auto Index = 0;
        while (Index + 2 < Stops.Num() && U > Stops[Index + 1].Pos)
        { ++Index; }

        const auto& Lo = Stops[Index];
        const auto& Hi = Stops[Index + 1];
        const float T  = Saturate((U - Lo.Pos) / FMath::Max(Hi.Pos - Lo.Pos, 1.0e-6f));

        return FLinearColor(
            SrgbToLinear(FMath::Lerp(Lo.R, Hi.R, T)),
            SrgbToLinear(FMath::Lerp(Lo.G, Hi.G, T)),
            SrgbToLinear(FMath::Lerp(Lo.B, Hi.B, T)),
            1.0f);
    }

    // T_VFX_Wind_01 stand-in — a 2x2 flipbook of one wind puff. Measured off the source sheet: peak 0.4706 and
    // never saturating; per-frame centroid (0.50, 0.51), coverage above 0.02 of 0.226-0.239, mean 0.038-0.040;
    // the annulus-mean radial profile is LINEAR from 0.43 at the centre to zero at r 0.575 (a cone fits it to
    // 0.012, where any power curve fits worse); 12-bin angular anisotropy 2.5-6.6 over r 0.25-0.65 with almost
    // none near the centre — so the irregularity lives in the SILHOUETTE radius, not in the interior intensity;
    // frame-to-frame correlation 0.79-0.88, i.e. one puff slowly evolving rather than four separate paints.
    // A radially symmetric cone reads as a hard disc and drops the anisotropy to 1.
    static auto Px_WindSheet(float U, float V) -> FLinearColor
    {
        constexpr float Peak        = 0.4706f; // the measured maximum; the source never reaches white
        constexpr float EdgeRadius  = 0.60f;
        constexpr float RaggedAmp   = 1.20f;   // silhouette wobble — lands the measured 2.5-6.6 anisotropy band
        constexpr float RaggedTiles = 3.0f;
        constexpr float GrainDepth  = 0.10f;   // interior breakup, from the 0.43 centre mean under a 0.4706 peak
        constexpr float GrainTiles  = 8.0f;
        constexpr float FrameStride = 29.0f;   // noise-domain step per frame — lands the 0.79-0.88 correlation

        const auto  Sheet = Sample_Sheet2x2(U, V);
        const float Dx    = Sheet.U - 0.5f;
        const float Dy    = Sheet.V - 0.5f;
        const float R     = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang   = FMath::Atan2(Dy, Dx);
        const float Off   = Sheet.Frame * FrameStride;

        // Sampled on the unit circle so the wobble is continuous across the seam at +-PI.
        const float Ragged = Fbm(FMath::Cos(Ang) * RaggedTiles + Off + 3.0f,
                                 FMath::Sin(Ang) * RaggedTiles + Off + 11.0f, 3, int32(RaggedTiles) * 2);
        const float Edge   = EdgeRadius * (1.0f + RaggedAmp * (Ragged - 0.5f));
        const float Grain  = 1.0f - GrainDepth * Fbm(Sheet.U * GrainTiles + Off + 5.0f,
                                                     Sheet.V * GrainTiles + Off + 17.0f, 3, int32(GrainTiles));

        const float I = Peak * Saturate(1.0f - R / FMath::Max(Edge, 1.0e-4f)) * Grain;
        return FLinearColor(I, I, I, I);
    }

    // ---- The Vefects hit/impact paints. Every constant below is MEASURED off the corpus PNG (recipes
    // NS_FireBall_Hit.md §7 / NS_Gunshot_Hit.md §7); none is copied art. r is the centre distance doubled, so
    // r = 1 at the mid-edge — the same convention every function above uses.

    // T_VFX_Part_02 stand-in. Radially symmetric to machine precision (12-bin anisotropy 1.00 at every radius),
    // but its falloff is NOT a power curve: it holds 0.99/0.96/0.91 across the first three annuli and reaches
    // exactly zero at r = 1. A cosine bell fits to 0.0034 where the best pow(1-r,k) costs 0.063 — 11x worse,
    // because no power curve has a flat shoulder. Peak saturates over 0.25% of the texture.
    static auto Px_SoftParticleBright(float U, float V) -> FLinearColor
    {
        constexpr float BellExp = 1.14f; // measured
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float I  = FMath::Pow(FMath::Cos(HALF_PI * Saturate(R)), BellExp);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Part_03 stand-in. Also radially symmetric, and EXPONENTIAL rather than power or Gaussian: successive
    // annulus ratios are a constant 0.521/0.524/0.517/0.508, which is the signature. A Gaussian costs 4.6x the
    // residual. Content hard-terminates at r 0.79, past which the source sits on its quantization floor.
    static auto Px_SoftParticleFine(float U, float V) -> FLinearColor
    {
        constexpr float Amplitude   = 0.750f; // measured fit A
        constexpr float DecayLength = 0.135f; // measured fit L 0.128, opened to 0.135 to land the annulus profile
        constexpr float HardCut     = 0.79f;
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float I  = R > HardCut ? 0.0f : Amplitude * FMath::Exp(-R / DecayLength);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Ring_01 stand-in. NOT the SDF Ring bake: measured against it, the source's interior is EXACTLY empty
    // (max 0.000000 over every pixel inside r < 0.6, where the SDF ring paints a cubic inner glow across 38% of
    // the texture), it never saturates, and its circumference is a hand-painted 9.5:1 bright-to-dim with four to
    // five nodes (dominant harmonic 4) against the SDF ring's flat 1.04:1.
    static auto Px_RingUneven(float U, float V) -> FLinearColor
    {
        constexpr float RingRadius = 0.7698f; // measured raw 0.3849, doubled into r
        constexpr float RingSigma  = 0.0398f; // from the measured 24 px FWHM, in r
        constexpr float Peak       = 0.898f;  // the source never saturates
        constexpr float NodePhase  = 0.7330f; // 42 degrees — one of the measured node angles
        constexpr float NodeFloor  = 0.28f;   // lands the measured bright-to-dim band
        constexpr float NodeExp    = 0.7f;
        constexpr float BandHalfWidth = 0.10f; // the source ring is empty outside this, not merely faint

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        // Hard-windowed: a bare Gaussian leaves a tail everywhere, and the source measures EXACTLY zero over
        // every pixel inside r < 0.6.
        const float Inside = FMath::Abs(R - RingRadius) <= BandHalfWidth ? 1.0f : 0.0f;
        const float Band   = FMath::Exp(-0.5f * FMath::Square((R - RingRadius) / RingSigma)) * Inside;
        const float Node   = FMath::Pow(0.5f + 0.5f * FMath::Cos(4.0f * (Ang - NodePhase)), NodeExp);
        const float I      = Peak * Band * (NodeFloor + (1.0f - NodeFloor) * Node);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Ring_02 stand-in — the lens-flare ring. A far wider, SATURATING annulus than Ring_01: 51.5% of its
    // band sits at or above 0.99 and 9.2% of the whole texture is at white. Its outer glow carries seven
    // irregular spokes (the flare streaks) which a clean annulus reads wrong without.
    static auto Px_RingFlare(float U, float V) -> FLinearColor
    {
        constexpr float RingRadius = 0.5364f; // measured, normalized
        constexpr float RingSigma  = 0.0890f; // measured, normalized
        constexpr float Overdrive  = 1.22f;   // saturates the measured 51.5% of the band
        constexpr float SpokeAmp   = 0.21f;   // measured outer-glow maximum
        constexpr float SpokeTiles = 7.0f;    // seven measured spokes
        constexpr float BandHalfWidth = 0.20f; // outside this the source is black, not faint

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float Inside = FMath::Abs(R - RingRadius) <= BandHalfWidth ? 1.0f : 0.0f;
        const float Band  = FMath::Exp(-0.5f * FMath::Square((R - RingRadius) / RingSigma)) * Inside;
        // Sampled on the unit circle so the spoke pattern is continuous across the seam at +-PI.
        const float Spoke = Fbm(FMath::Cos(Ang) * SpokeTiles + 5.0f, FMath::Sin(Ang) * SpokeTiles + 19.0f, 2, 14);
        const float Glow  = SpokeAmp * Spoke * Saturate(1.0f - Smooth(0.66f, 0.95f, R)) * Smooth(0.60f, 0.70f, R);
        const float I     = Saturate(Band * Overdrive + Glow);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Star_01 stand-in. NOT the Flare bake: the source is a FOUR-lobe star on the cardinal axes (dominant
    // angular harmonic 4, lobes at 0 / +-90 / 180, u-profile and v-profile identical to 0.0001) where Flare is a
    // six-lobe pow(|cos 3t|, 36). Its core is LINEAR in radius, not a power curve, and its lobes TAPER — 90
    // degrees wide at r 0.3, 20 at 0.5, 2 at 0.7 — where Flare's angular width is fixed at every radius.
    static auto Px_StarFour(float U, float V) -> FLinearColor
    {
        constexpr float DiscA      = 1.1399f; // measured linear core: I = DiscA - DiscB * r
        constexpr float DiscB      = 1.7881f;
        constexpr float DiscCut    = 0.31f;   // where the linear fit ends
        constexpr float DiscEnd    = 0.50f;   // the measured 54 px shoulder to zero
        constexpr float LobeA      = 1.0811f; // measured lobe envelope: LobeA - LobeB * r
        constexpr float LobeB      = 0.9333f;
        // ln k of the angular exponent, fitted through the measured FWHM taper (11.1 at r 0.5, 1136 at r 0.7).
        constexpr float TaperSlope = 23.1f;
        constexpr float TaperBias  = -9.14f;

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float Disc = Saturate(DiscA - DiscB * R) * (1.0f - Smooth(DiscCut, DiscEnd, R));

        const float K    = FMath::Clamp(FMath::Exp(TaperSlope * R + TaperBias), 0.05f, 2000.0f);
        const float Lobe = Saturate(LobeA - LobeB * R) * FMath::Pow(FMath::Abs(FMath::Cos(2.0f * Ang)), K);

        const float I = Saturate(FMath::Max(Disc, Lobe));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Star_02 stand-in. Also a four-lobe cardinal star (dominant angular harmonic 4, u-profile and
    // v-profile identical to 0.0002) and also NOT the StarFour bake: measured against it, Star_01 holds
    // 0.98/0.89/0.79 across its first three annuli where this paint has already fallen to 0.75/0.28/0.14, and
    // its rays reach r 0.85 against Star_01's 0.6. What it has instead is a SATURATED core out to r 0.08 — the
    // measured 0.43% of the texture at white — behind rays whose brightness barely decays (0.78 at r 0.18 down
    // to 0.58 at 0.70) before a hard cliff between 0.75 and 0.88. Angular lobe FWHM tapers 26/12/8/4/2 degrees
    // over r 0.15..0.55, which is the exponent fit below. Fit residual: mean absolute 0.0072, correlation 0.979.
    static auto Px_StarFourTight(float U, float V) -> FLinearColor
    {
        // Measured along the 45-degree diagonal, where only the isotropic core survives.
        static constexpr FCk_ProfileAnchor Core[] =
        {
            { 0.00f, 1.000f }, { 0.06f, 1.000f }, { 0.08f, 0.808f }, { 0.10f, 0.596f },
            { 0.12f, 0.224f }, { 0.15f, 0.071f }, { 0.18f, 0.012f }, { 0.22f, 0.000f },
        };
        // Measured along the cardinal axes, where the ray is the whole signal.
        static constexpr FCk_ProfileAnchor Ray[] =
        {
            { 0.10f, 0.988f }, { 0.15f, 0.820f }, { 0.18f, 0.780f }, { 0.26f, 0.753f },
            { 0.35f, 0.718f }, { 0.50f, 0.663f }, { 0.70f, 0.584f }, { 0.80f, 0.353f },
            { 0.85f, 0.067f }, { 0.90f, 0.000f },
        };
        constexpr float TaperSlope = 11.5f; // ln k of the angular exponent, fitted through the measured FWHM taper
        constexpr float TaperBias  = 0.0f;

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float K    = FMath::Clamp(FMath::Exp(TaperSlope * R + TaperBias), 0.05f, 2000.0f);
        const float Lobe = Sample_Profile(R, Ray, UE_ARRAY_COUNT(Ray))
                         * FMath::Pow(FMath::Abs(FMath::Cos(2.0f * Ang)), K);

        const float I = Saturate(FMath::Max(Sample_Profile(R, Core, UE_ARRAY_COUNT(Core)), Lobe));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Star_03 stand-in — the lens-flare streak star. Four cardinal lobes again, but the only paint in the
    // pack whose HORIZONTAL and VERTICAL rays have different reach: measured at the 0.05 level the +-X pair runs
    // to r 0.805 and the +-Y pair stops dead at 0.414, which is what puts its u-profile 0.138 away from its
    // v-profile where every other star in the pack matches to 0.0002. So it is modelled as two independent
    // two-lobe terms rather than one four-lobe mask. Its core is soft (0.44 by r 0.08 against Star_02's
    // saturated 1.0) and it barely saturates at all (0.05% of the texture). Lobe FWHM tapers 21/9/4/2/1 degrees
    // over r 0.15..0.55. Fit residual: mean absolute 0.0038, correlation 0.987.
    static auto Px_StarFourSplit(float U, float V) -> FLinearColor
    {
        static constexpr FCk_ProfileAnchor Core[] =
        {
            { 0.00f, 1.000f }, { 0.04f, 0.922f }, { 0.06f, 0.733f }, { 0.08f, 0.443f },
            { 0.10f, 0.290f }, { 0.15f, 0.192f }, { 0.22f, 0.071f }, { 0.30f, 0.004f },
            { 0.35f, 0.000f },
        };
        static constexpr FCk_ProfileAnchor RayX[] =
        {
            { 0.00f, 1.000f }, { 0.08f, 0.847f }, { 0.12f, 0.694f }, { 0.18f, 0.549f },
            { 0.26f, 0.506f }, { 0.40f, 0.471f }, { 0.50f, 0.408f }, { 0.70f, 0.329f },
            { 0.80f, 0.063f }, { 0.85f, 0.000f },
        };
        static constexpr FCk_ProfileAnchor RayY[] =
        {
            { 0.00f, 1.000f }, { 0.08f, 0.792f }, { 0.12f, 0.714f }, { 0.18f, 0.651f },
            { 0.26f, 0.553f }, { 0.35f, 0.416f }, { 0.40f, 0.122f }, { 0.45f, 0.000f },
        };
        constexpr float TaperSlope = 18.0f;
        constexpr float TaperBias  = 0.3f;

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float K     = FMath::Clamp(FMath::Exp(TaperSlope * R + TaperBias), 0.05f, 2000.0f);
        const float Horiz = Sample_Profile(R, RayX, UE_ARRAY_COUNT(RayX))
                          * FMath::Pow(FMath::Abs(FMath::Cos(Ang)), K);
        const float Vert  = Sample_Profile(R, RayY, UE_ARRAY_COUNT(RayY))
                          * FMath::Pow(FMath::Abs(FMath::Sin(Ang)), K);

        const float I = Saturate(FMath::Max(Sample_Profile(R, Core, UE_ARRAY_COUNT(Core)),
                                            FMath::Max(Horiz, Vert)));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Arrow_01 stand-in — the chevron the Buff and Debuff loops fire as their arrow streams, and the only
    // POLYGONAL paint in the cookbook. Measured: horizontally symmetric to a correlation of 0.99999996, no
    // vertical symmetry at all, lit only over v 0.207..0.559, and its half-intensity contour traces a quadrilateral
    // whose corners fall on the four points below (outer apex, outer corner, arm tip, inner apex; the right arm is
    // the mirror). Regressing intensity against the SIGNED DISTANCE to that quad fits a two-term exponential to an
    // RMSE of 0.0047 over the whole 0..0.21 range, which is what the falloff constants are.
    //
    // No existing bake was a candidate: every mask in the library is radial, streaked or noise, and none of them
    // has a straight edge, let alone four.
    static auto Px_ArrowChevron(float U, float V) -> FLinearColor
    {
        // The measured half-intensity polygon on the LEFT arm, in uv.
        static const FVector2f Quad[] =
        {
            FVector2f(0.5000f, 0.2666f), // outer apex, on the centreline
            FVector2f(0.2637f, 0.4189f), // outer corner where the arm's leading edge turns
            FVector2f(0.3242f, 0.5010f), // arm tip
            FVector2f(0.5000f, 0.3914f), // inner apex — the notch between the two arms
        };

        constexpr float NearAmp   = 0.4222f;
        constexpr float NearDecay = 49.0f;
        constexpr float FarAmp    = 0.0650f;
        constexpr float FarDecay  = 12.5f;

        // Mirror into the left half; the paint is symmetric about u = 0.5 to six decimal places.
        const FVector2f P(0.5f - FMath::Abs(U - 0.5f), V);

        auto  MinDistSq = TNumericLimits<float>::Max();
        auto  Sign      = 1.0f;
        const auto Count = static_cast<int32>(UE_ARRAY_COUNT(Quad));

        for (auto I = 0; I < Count; ++I)
        {
            const auto J = (I + Count - 1) % Count;

            const auto E = Quad[J] - Quad[I];
            const auto W = P - Quad[I];
            const auto T = FMath::Clamp(FVector2f::DotProduct(W, E)
                                      / FMath::Max(FVector2f::DotProduct(E, E), 1.0e-9f), 0.0f, 1.0f);
            const auto B = W - E * T;

            MinDistSq = FMath::Min(MinDistSq, FVector2f::DotProduct(B, B));

            // Winding test: an odd number of edge crossings puts the point inside.
            const auto C1 = P.Y >= Quad[I].Y;
            const auto C2 = P.Y <  Quad[J].Y;
            const auto C3 = E.X * W.Y > E.Y * W.X;

            if ((C1 && C2 && C3) || (NOT C1 && NOT C2 && NOT C3))
            { Sign = -Sign; }
        }

        const auto D = Sign * FMath::Sqrt(MinDistSq);
        const auto I = Saturate(NearAmp * FMath::Exp(-NearDecay * D) + FarAmp * FMath::Exp(-FarDecay * D));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_LightStrip_01 stand-in. NOT the Streak bake, which is a lobe centred at v = 0.5: this is a vertical
    // strip that RAMPS along v (centroid v 0.735, linear from 0.03 at v 0.25 to 0.99 at v 0.90, then collapsing
    // to nothing by v 0.98) with a flat-topped cross-section, where Streak's along-u term is a cusp with no
    // plateau. Measured correlation between the two is 0.02 — they are different texture classes.
    static auto Px_LightStrip(float U, float V) -> FLinearColor
    {
        constexpr float Peak      = 0.7569f; // the source never saturates
        constexpr float FlatHalfU = 0.085f;  // measured >=0.99 plateau half-width
        constexpr float EdgeHalfU = 0.235f;  // measured 0.1-level half-width

        // The measured v anchors, verbatim; the strip dies between v 0.90 and 0.98.
        float Ramp;
        if (V <= 0.25f)      { Ramp = FMath::Lerp(0.0f,   0.033f, Saturate(V / 0.25f)); }
        else if (V <= 0.50f) { Ramp = FMath::Lerp(0.033f, 0.281f, (V - 0.25f) / 0.25f); }
        else if (V <= 0.70f) { Ramp = FMath::Lerp(0.281f, 0.672f, (V - 0.50f) / 0.20f); }
        else if (V <= 0.90f) { Ramp = FMath::Lerp(0.672f, 0.989f, (V - 0.70f) / 0.20f); }
        else                 { Ramp = FMath::Lerp(0.989f, 0.015f, Saturate((V - 0.90f) / 0.08f)); }

        const float X    = FMath::Abs(U - 0.5f);
        const float Prof = 1.0f - Smooth(FlatHalfU, EdgeHalfU, X);
        const float I    = Saturate(Peak * Ramp * Prof);
        return FLinearColor(I, I, I, I);
    }

    // The two smoke paints share a form and NOT their constants: Cloud_04 has a hard 0.596 ceiling and never
    // saturates, Cloud_05 reaches white over 1.4% of itself and carries 3.5x its 8-16 cyc energy. Both hold a
    // real silhouette (zero fraction 0.62 / 0.58) that the generic Smoke bake does not — Smoke lights 84% of its
    // square against these paints' 36-40%, which is why neither reuses it.
    static auto Px_CloudPuff(float U, float V, float InPeak, float InGain, float InBias, float InGrainTiles,
                             float InEdgeRadius) -> float
    {
        constexpr float RaggedAmp = 0.45f;

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float Ragged = Fbm(FMath::Cos(Ang) * 3.0f + 7.0f, FMath::Sin(Ang) * 3.0f + 23.0f, 3, 6);
        const float Edge   = InEdgeRadius * (1.0f + RaggedAmp * (Ragged - 0.5f));
        const float Body   = Saturate(1.0f - R / FMath::Max(Edge, 1.0e-4f));

        const float Cloud = Fbm(U * InGrainTiles + 31.0f, V * InGrainTiles + 13.0f, 4, int32(InGrainTiles));
        return InPeak * Saturate(Body * (InBias + InGain * Cloud));
    }

    // T_VFX_Cloud_04 stand-in — the darker of the smoke pair, ceilinged at 0.596 with 1.9% of its energy above
    // 8 cyc.
    static auto Px_Cloud04(float U, float V) -> FLinearColor
    {
        const float I = Px_CloudPuff(U, V, 0.596f, 2.6f, -0.45f, 3.0f, 0.80f);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Cloud_05 stand-in — the brighter one: it saturates, and it carries 3.5x Cloud_04's fine-band energy.
    static auto Px_Cloud05(float U, float V) -> FLinearColor
    {
        const float I = Px_CloudPuff(U, V, 1.0f, 2.4f, -0.12f, 6.0f, 0.76f);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Noise_04 stand-in. NOT TileNoise: measured against Noise_02 (the paint TileNoise WAS measured off) it
    // is a different family — histogram intersection 0.378, 3x the correlation length, and 98.6% of its energy in
    // the 1-8 cyc band against Noise_02's 70.7%. It is a coarse cloud CLIPPED at white over 18% of itself, which
    // is what makes it left-skewed where every other noise in the pack is right-skewed.
    static auto Px_TileNoiseCoarse(float U, float V) -> FLinearColor
    {
        constexpr float Tiles = 5.0f;  // measured 49 px correlation half-decay
        constexpr float Gain  = 1.50f; // lands the measured 0.251 rms and 18% clipped fraction
        constexpr float Bias  = 0.33f; // lands the measured 0.72 median
        const float N = Fbm(U * Tiles, V * Tiles, 2, int32(Tiles));
        const float I = Saturate(0.5f + (N - 0.5f) * Gain + Bias);
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Noise_07 stand-in. Also not TileNoise: band-limited with essentially NOTHING above 16 cyc (a 660x
    // deficit against TileNoise's deliberate grain octave) and sitting on a hard 0.0196 black floor it never
    // crosses — a dissolve driven by it can never fully clear, which a floor-less stand-in would get wrong.
    static auto Px_TileNoiseBandLimited(float U, float V) -> FLinearColor
    {
        constexpr float Tiles = 3.0f;    // measured 28 px correlation half-decay
        constexpr float Gain  = 0.95f;   // lands the measured 0.159 rms
        constexpr float Bias  = 0.06f;   // lands the measured 0.502 median
        constexpr float Floor = 0.0196f; // the measured minimum: this paint is never black
        const float N = Fbm(U * Tiles, V * Tiles, 2, int32(Tiles));
        const float I = FMath::Max(Floor, Saturate(0.5f + (N - 0.5f) * Gain + Bias));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Impact_01 stand-in — a FIVE-fold star at 36 + 72k degrees over a HOLLOW core. Measured as two
    // Gaussians rather than one radial law, because no single law fits: the core peaks at r 0.15 (not at the
    // centre) and dies by 0.375, while the spike envelope peaks at 0.353 and reaches out to 0.86. The
    // inter-spike floor is 0.001 of the peak — these are thin rays, not a soft star.
    static auto Px_ImpactStar(float U, float V) -> FLinearColor
    {
        // The measured 0.3673 is the ISOTROPIC component (the angular MINIMUM); the annulus means the whole
        // shape has to hit are higher, so the core amplitude is fitted against those instead.
        constexpr float CoreA   = 0.50f;
        constexpr float CoreR   = 0.118f;
        constexpr float CoreS   = 0.050f;
        constexpr float SpikeA  = 0.9887f; // measured spike envelope Gaussian
        constexpr float SpikeR  = 0.353f;
        constexpr float SpikeS  = 0.180f;
        // The rays TAPER: measured anisotropy is only 2.43 at r 0.15 and 917 by r 0.55, so the angular exponent
        // has to grow with radius. These two land the measured annulus profile to within 0.03.
        constexpr float TaperSlope = 16.0f;
        constexpr float TaperBias  = -2.9f;
        constexpr float SpikePh    = 0.6283f; // 36 degrees

        const float Dx  = U - 0.5f, Dy = V - 0.5f;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float Core  = CoreA  * FMath::Exp(-0.5f * FMath::Square((R - CoreR)  / CoreS));
        const float Env   = SpikeA * FMath::Exp(-0.5f * FMath::Square((R - SpikeR) / SpikeS));
        const float K     = FMath::Clamp(FMath::Exp(TaperSlope * R + TaperBias), 0.05f, 2000.0f);
        const float Rays  = FMath::Pow(FMath::Abs(FMath::Cos(2.5f * (Ang - SpikePh))), K);

        const float I = Saturate(FMath::Max(Core, Env * Rays));
        return FLinearColor(I, I, I, I);
    }

    // T_VFX_Impact_02 stand-in — a 2x2 flipbook of ONE evolving impact burst, not four unrelated paints: every
    // frame is a vertical two-lobe burst (dominant angular harmonic 2 at +-90 in all four), the radial law
    // correlates 0.94-0.99 across frames, and the frame-ordered quantities move monotonically — centroid v
    // 0.608 -> 0.571 -> 0.535 -> 0.420, coverage 0.136 -> 0.108. It never saturates (peak 0.71-0.88), and it
    // evolves FASTER than the wind sheet (frame correlation 0.51-0.78 against that paint's 0.79-0.88).
    static auto Px_ImpactSheet(float U, float V) -> FLinearColor
    {
        constexpr float FrameStride = 47.0f;  // wider than the wind sheet's, for the measured faster evolution
        constexpr float LobeK       = 2.2f;   // the measured harmonic-2 vertical pair
        constexpr float GrainDepth  = 0.22f;

        const auto  Sheet = Sample_Sheet2x2(U, V);
        const float Off   = Sheet.Frame * FrameStride;
        const float F     = Sheet.Frame / 3.0f;

        // Measured per-frame constants, interpolated across the four frames in source order.
        const float Peak     = FMath::Lerp(0.7490f, 0.7098f, F);
        const float Centroid = FMath::Lerp(0.6078f, 0.4195f, F);
        const float Sigma    = FMath::Lerp(0.310f,  0.240f,  F); // tracks the measured radius of gyration

        const float Dx  = Sheet.U - 0.5f;
        const float Dy  = Sheet.V - Centroid;
        const float R   = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f;
        const float Ang = FMath::Atan2(Dy, Dx);

        const float Body  = FMath::Exp(-0.5f * FMath::Square(R / Sigma));
        const float Lobes = FMath::Pow(FMath::Abs(FMath::Sin(Ang)), LobeK);
        const float Grain = 1.0f - GrainDepth * Fbm(Sheet.U * 6.0f + Off + 3.0f, Sheet.V * 6.0f + Off + 29.0f, 3, 6);

        const float I = Saturate(Peak * Body * Lobes * Grain);
        return FLinearColor(I, I, I, I);
    }

    static auto Px_Ring(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float Raw = FMath::Sqrt(Dx * Dx + Dy * Dy); // [0, ~0.707]
        const float Radius = 0.40f;
        const float D = FMath::Abs(Raw - Radius);
        const float Ring = 1.0f - Smooth(0.006f, 0.030f, D);
        const float Inner = FMath::Pow(Saturate(1.0f - Raw / Radius), 3.0f) * 0.12f;
        const float I = Saturate(Ring + Inner);
        return FLinearColor(I, I, I, I);
    }

    // ---- Bake one texture asset from a per-pixel function -----------------------------------------------------------
    using FPxFn = FLinearColor (*)(float, float);

    // What a texture IS, as opposed to what it looks like. Dimensions, colour space and compression are properties
    // of the kind, so a new texture only ever has to write its per-pixel paint — and two textures of one kind
    // cannot drift apart in format.
    enum class ECk_VfxTextureKind : uint8
    {
        Mask,      // 512x512 linear greyscale — Particle Color tints it; uncompressed so SDF edges stay crisp
        MaskSheet, // the same, laid out as a 2x2 flipbook of 256x256 frames (see Sample_Sheet2x2)
        ColorLut,  // 512x2 sRGB colour ramp read along u; a gradient map, not a mask, so it carries real colour
    };

    struct FCk_VfxTextureFormat
    {
        int32                      Width;
        int32                      Height;
        bool                       Srgb;
        TextureCompressionSettings Compression;
    };

    static auto Get_TextureFormat(ECk_VfxTextureKind InKind) -> FCk_VfxTextureFormat
    {
        // 512 is the source pack's uniform size — below it the 16-32 cyc content cannot survive.
        constexpr int32 MaskSize = 512;

        switch (InKind)
        {
            // TC_Default because a LUT is real colour, and sRGB because the source ramp is authored and sampled
            // encoded; TC_VectorDisplacementmap here would band the ramp.
            case ECk_VfxTextureKind::ColorLut:
                return FCk_VfxTextureFormat{512, 2, true, TC_Default};
            case ECk_VfxTextureKind::MaskSheet:
            case ECk_VfxTextureKind::Mask:
            default:
                return FCk_VfxTextureFormat{MaskSize, MaskSize, false, TC_VectorDisplacementmap};
        }
    }

    struct FCk_VfxTextureSpec
    {
        const TCHAR*       Name;
        ECk_VfxTextureKind Kind;
        FPxFn              Fn;
    };

    static auto Bake(const TCHAR* InName, const FCk_VfxTextureFormat& InFormat, FPxFn InFn) -> bool
    {
        const FString PkgPath = FString::Printf(TEXT("%s/%s"), TexDir, InName);

        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, *PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr) { Package = CreatePackage(*PkgPath); }
        if (Package == nullptr) { return false; }

        if (auto* Old = StaticFindObject(UTexture2D::StaticClass(), Package, InName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
        }

        auto* Texture = NewObject<UTexture2D>(Package, InName, RF_Public | RF_Standalone);
        if (Texture == nullptr) { return false; }

        // FColor is laid out B,G,R,A in memory, matching TSF_BGRA8. Per-pixel functions always hand back LINEAR
        // colour, so the encode happens exactly once here and only for an sRGB texture.
        const auto EncodeSrgb = InFormat.Srgb;

        TArray<FColor> Pixels;
        Pixels.SetNumUninitialized(InFormat.Width * InFormat.Height);
        for (int32 Y = 0; Y < InFormat.Height; ++Y)
        for (int32 X = 0; X < InFormat.Width;  ++X)
        {
            const float U = (X + 0.5f) / InFormat.Width;
            const float V = (Y + 0.5f) / InFormat.Height;
            Pixels[Y * InFormat.Width + X] = InFn(U, V).ToFColor(EncodeSrgb);
        }

        constexpr auto Slices = 1;
        constexpr auto Mips   = 1;

        Texture->PreEditChange(nullptr);
        Texture->Source.Init(InFormat.Width, InFormat.Height, Slices, Mips, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
        Texture->SRGB                = InFormat.Srgb;
        Texture->CompressionSettings = InFormat.Compression;
        Texture->MipGenSettings      = TMGS_FromTextureGroup;
        Texture->LODGroup            = TEXTUREGROUP_Effects;
        Texture->PostEditChange();
        Texture->UpdateResource();

        Texture->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(Texture);

        const FString FileName = FPackageName::LongPackageNameToFilename(PkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, Texture, *FileName, SaveArgs);
    }
}

namespace ck::particles_editor
{
    auto Generate_AllVfxTextures() -> void
    {
        using namespace TexGenLocal;

        static const FCk_VfxTextureSpec Specs[] =
        {
            { TEXT("T_CkParticles_Glow"),         ECk_VfxTextureKind::Mask,      &Px_Glow         },
            { TEXT("T_CkParticles_Flare"),        ECk_VfxTextureKind::Mask,      &Px_Flare        },
            { TEXT("T_CkParticles_Smoke"),        ECk_VfxTextureKind::Mask,      &Px_Smoke        },
            { TEXT("T_CkParticles_Electric"),     ECk_VfxTextureKind::Mask,      &Px_Electric     },
            { TEXT("T_CkParticles_Streak"),       ECk_VfxTextureKind::Mask,      &Px_Streak       },
            { TEXT("T_CkParticles_Ring"),         ECk_VfxTextureKind::Mask,      &Px_Ring         },
            { TEXT("T_CkParticles_SweepStreak"),  ECk_VfxTextureKind::Mask,      &Px_SweepStreak  },
            { TEXT("T_CkParticles_TileNoise"),    ECk_VfxTextureKind::Mask,      &Px_TileNoise    },
            { TEXT("T_CkParticles_SlashArc01"),   ECk_VfxTextureKind::Mask,      &Px_SlashArc01   },
            { TEXT("T_CkParticles_SlashArc02"),   ECk_VfxTextureKind::Mask,      &Px_SlashArc02   },
            { TEXT("T_CkParticles_WindBand"),     ECk_VfxTextureKind::Mask,      &Px_WindBand     },
            { TEXT("T_CkParticles_SoftParticle"), ECk_VfxTextureKind::Mask,      &Px_SoftParticle },
            { TEXT("T_CkParticles_SparkStreak"),  ECk_VfxTextureKind::Mask,      &Px_SparkStreak  },
            { TEXT("T_CkParticles_WindSheet"),    ECk_VfxTextureKind::MaskSheet, &Px_WindSheet    },
            { TEXT("T_CkParticles_LutWhite"),     ECk_VfxTextureKind::ColorLut,  &Px_LutWhite     },
            { TEXT("T_CkParticles_LutRainbow"),   ECk_VfxTextureKind::ColorLut,  &Px_LutRainbow   },
            // The Vefects hit/impact paints (recipes NS_FireBall_Hit.md §7, NS_Gunshot_Hit.md §7). Each was
            // measured against the nearest existing stand-in first; every one of them failed that comparison on a
            // structural statistic, which is why each has its own bake rather than a reuse.
            { TEXT("T_CkParticles_SoftParticleBright"), ECk_VfxTextureKind::Mask, &Px_SoftParticleBright },
            { TEXT("T_CkParticles_SoftParticleFine"),   ECk_VfxTextureKind::Mask, &Px_SoftParticleFine   },
            { TEXT("T_CkParticles_RingUneven"),         ECk_VfxTextureKind::Mask, &Px_RingUneven         },
            { TEXT("T_CkParticles_RingFlare"),          ECk_VfxTextureKind::Mask, &Px_RingFlare          },
            { TEXT("T_CkParticles_StarFour"),           ECk_VfxTextureKind::Mask, &Px_StarFour           },
            { TEXT("T_CkParticles_LightStrip"),         ECk_VfxTextureKind::Mask, &Px_LightStrip         },
            { TEXT("T_CkParticles_Cloud04"),            ECk_VfxTextureKind::Mask, &Px_Cloud04            },
            { TEXT("T_CkParticles_Cloud05"),            ECk_VfxTextureKind::Mask, &Px_Cloud05            },
            { TEXT("T_CkParticles_TileNoiseCoarse"),    ECk_VfxTextureKind::Mask, &Px_TileNoiseCoarse    },
            { TEXT("T_CkParticles_TileNoiseBanded"),    ECk_VfxTextureKind::Mask, &Px_TileNoiseBandLimited },
            { TEXT("T_CkParticles_ImpactStar"),         ECk_VfxTextureKind::Mask, &Px_ImpactStar         },
            { TEXT("T_CkParticles_ImpactSheet"),        ECk_VfxTextureKind::MaskSheet, &Px_ImpactSheet   },
            // The Vefects arrow/bomb paints (recipes NS_Arrow_Cast.md §7, NS_Bomb_Spawn.md §7). Both stars were
            // measured against the existing StarFour bake and rejected on their radial law; the wind band was
            // measured against WindBand and turned out to be the SAME source image rolled 141 rows in v, so it
            // reuses that paint function at the measured offset rather than inventing a second one.
            { TEXT("T_CkParticles_StarFourTight"),      ECk_VfxTextureKind::Mask, &Px_StarFourTight      },
            { TEXT("T_CkParticles_StarFourSplit"),      ECk_VfxTextureKind::Mask, &Px_StarFourSplit      },
            { TEXT("T_CkParticles_WindBandMid"),        ECk_VfxTextureKind::Mask, &Px_WindBandMid        },
            // The Vefects Loop paints (recipes NS_BuffLoop.md §7, NS_DebuffLoop.md §7). Only ONE new bake: the
            // arrow chevron. Every other texture those four ports need is a paint an earlier batch already
            // measured and baked, reached through a look that already binds it.
            { TEXT("T_CkParticles_ArrowChevron"),       ECk_VfxTextureKind::Mask, &Px_ArrowChevron       },
        };

        auto Ok = 0;
        for (const auto& Spec : Specs)
        {
            if (Bake(Spec.Name, Get_TextureFormat(Spec.Kind), Spec.Fn))
            { ++Ok; }
            else
            { Log(TEXT("Failed to bake VFX texture: {}"), FString(Spec.Name)); }
        }

        Log(TEXT("Generated {}/{} CkParticles VFX textures under {}."),
            FString::FromInt(Ok), FString::FromInt(static_cast<int32>(UE_ARRAY_COUNT(Specs))), FString(TexDir));
    }
}
