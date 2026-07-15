#include "CkParticlesEditor/Generator/CkParticles_TextureGenerator.h"

#include "CkParticlesEditor_Log.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------
// Procedural VFX texture baker. Everything here is plain math sampled per-pixel into a UTexture2D asset — the textures
// a VFX artist would paint (soft glow, star flare, smoke, electric crackle, spark streak, SDF ring), generated in code
// so they stay deterministic, art-free, and resolution-independent. The master materials combine these channels.
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

    // ---- Per-pixel look functions. (u,v) in [0,1]; return packed FLinearColor (channels documented per texture). ---

    // Soft glow (grayscale): wide soft body + a brighter hotspot core. RGB == A so it tints cleanly with ParticleColor.
    static auto Px_Glow(float U, float V) -> FLinearColor
    {
        const float Dx = U - 0.5f, Dy = V - 0.5f;
        const float R  = FMath::Sqrt(Dx * Dx + Dy * Dy) * 2.0f; // 0 center, 1 at mid-edge
        const float Body = FMath::Pow(Saturate(1.0f - R), 2.4f);
        const float Hot  = FMath::Pow(Saturate(1.0f - R * 1.5f), 7.0f);
        const float I = Saturate(Body * 0.85f + Hot);
        return FLinearColor(I, I, I, I);
    }

    // Star flare: 6 sharp spikes + bright core. R/A = intensity.
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

    // Smoke puff: grayscale wispy density in RGB (radial-masked FBM + finer detail), erosion mask in A for
    // dissolve-over-life in the material.
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

    // Electric/energy ball: ridged FBM filaments + hot core, radial-masked. R/A = intensity.
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

    // Spark streak: thin bright horizontal gradient with noise breakup (for velocity-stretched sparks / ribbons).
    static auto Px_Streak(float U, float V) -> FLinearColor
    {
        const float Yd  = FMath::Abs(V - 0.5f) * 2.0f;
        const float Core = FMath::Pow(Saturate(1.0f - Yd), 9.0f);
        const float Len  = FMath::Pow(Saturate(1.0f - FMath::Abs(U - 0.5f) * 2.0f), 0.6f);
        const float Jit  = 0.65f + 0.35f * Fbm(U * 12.0f, V * 3.0f, 4, 12);
        const float I = Saturate(Core * Len * Jit * 1.5f);
        return FLinearColor(I, I, I, I);
    }

    // Sweep streak (grayscale): the anime-slash carrier texture — bright leading head near V=0, wispy turbulent
    // body, fading tail (modeled on marketplace T_VFX_Slash_01). Panned along V over a sweep mesh by the
    // SweepErode master material, the head appears to travel along the arc. RGB == A.
    static auto Px_SweepStreak(float U, float V) -> FLinearColor
    {
        const float Hx   = FMath::Pow(Saturate(1.0f - FMath::Abs(U - 0.5f) * 1.9f), 1.1f);
        const float Head = FMath::Pow(Saturate(1.0f - FMath::Abs(V - 0.10f) / 0.14f), 2.0f);
        const float Body = Smooth(0.02f, 0.18f, V) * (1.0f - Smooth(0.40f, 0.85f, V));
        const float Wisp = 0.55f + 0.45f * Fbm(U * 5.0f + 3.0f, V * 8.0f + 17.0f, 5, 5);
        const float I = Saturate((Head * 1.25f + Body * 0.55f * Wisp) * Hx);
        return FLinearColor(I, I, I, I);
    }

    // Tileable soft noise (grayscale): smooth billow/cell blend for dissolve thresholds and UV distortion
    // (modeled on marketplace T_VFX_Noise_02). Wraps seamlessly. RGB == A.
    static auto Px_TileNoise(float U, float V) -> FLinearColor
    {
        const float Billow = Fbm(U * 5.0f, V * 5.0f, 4, 5);
        const float Cells  = Saturate(Voronoi(U * 5.0f, V * 5.0f, 5) * 0.85f);
        const float I = Saturate((0.60f * Billow + 0.40f * Cells) * 1.20f - 0.06f);
        return FLinearColor(I, I, I, I);
    }

    // SDF magic ring (grayscale): crisp anti-aliased ring + faint inner glow. RGB == A.
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

    static auto Bake(const TCHAR* InName, int32 InSize, FPxFn InFn) -> bool
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

        // FColor is laid out B,G,R,A in memory, matching TSF_BGRA8. ToFColor(false) quantizes the linear value with no
        // sRGB encode, which is what we want since the texture is linear data (SRGB = false).
        TArray<FColor> Pixels;
        Pixels.SetNumUninitialized(InSize * InSize);
        for (int32 Y = 0; Y < InSize; ++Y)
        for (int32 X = 0; X < InSize; ++X)
        {
            const float U = (X + 0.5f) / InSize;
            const float V = (Y + 0.5f) / InSize;
            Pixels[Y * InSize + X] = InFn(U, V).ToFColor(/*bSRGB*/ false);
        }

        Texture->PreEditChange(nullptr);
        Texture->Source.Init(InSize, InSize, /*Slices*/ 1, /*Mips*/ 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
        Texture->SRGB               = false;
        Texture->CompressionSettings = TC_VectorDisplacementmap; // uncompressed RGBA8 — crisp SDF edges, linear data
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

        constexpr int32 Size = 256;

        int32 Ok = 0, Total = 0;
        const auto BakeOne = [&](const TCHAR* InName, FPxFn InFn)
        {
            ++Total;
            if (Bake(InName, Size, InFn)) { ++Ok; }
            else { Log(TEXT("Failed to bake VFX texture: {}"), FString(InName)); }
        };

        BakeOne(TEXT("T_CkParticles_Glow"),        &Px_Glow);
        BakeOne(TEXT("T_CkParticles_Flare"),       &Px_Flare);
        BakeOne(TEXT("T_CkParticles_Smoke"),       &Px_Smoke);
        BakeOne(TEXT("T_CkParticles_Electric"),    &Px_Electric);
        BakeOne(TEXT("T_CkParticles_Streak"),      &Px_Streak);
        BakeOne(TEXT("T_CkParticles_Ring"),        &Px_Ring);
        BakeOne(TEXT("T_CkParticles_SweepStreak"), &Px_SweepStreak);
        BakeOne(TEXT("T_CkParticles_TileNoise"),   &Px_TileNoise);

        Log(TEXT("Generated {}/{} CkParticles VFX textures under {}."),
            FString::FromInt(Ok), FString::FromInt(Total), FString(TexDir));
    }
}
