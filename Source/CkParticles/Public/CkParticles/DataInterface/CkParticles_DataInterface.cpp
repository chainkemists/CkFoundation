#include "CkParticles/DataInterface/CkParticles_DataInterface.h"

#include "NiagaraCompileHashVisitor.h"
#include "NiagaraTypeRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_DataInterface)

#define LOCTEXT_NAMESPACE "UCkParticles_DataInterface"

// --------------------------------------------------------------------------------------------------------------------

namespace NDICkParticlesLocal
{
    static const FName NAME_ExecuteStage(TEXT("ExecuteStage"));

    static const TCHAR* TemplateShaderFile = TEXT("/CkParticles/CkParticles_DataInterfaceTemplate.ush");

    static const TCHAR* DependentShaderFiles[] =
    {
        TEXT("/CkParticles/CkParticles_DataInterfaceTemplate.ush"),
        TEXT("/CkParticles/CkParticles_Behaviors.ush"),
        TEXT("/CkParticles/Common.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Gravity.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Swirl.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Explosion.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Fire.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Fireworks.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Galaxy.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Beam.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Slash.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Nova.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_MuzzleFlash.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_ImpactBurst.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_Tracer.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_SmokePlume.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_SparksBurst.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_GroundRing.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_LightningStrike.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_AuraSwirl.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_LightningRange.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_GunshotProjectile.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_ArrowProjectile.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_FireBurst.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_FireBallHit.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_GunshotHit.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_ArrowCast.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_ArrowHit.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_BombSpawn.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_PickupLoop.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_HealLoop.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_BuffLoop.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_DebuffLoop.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_PickupCast.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_HealCast.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_DebuffCast.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_GunshotCast.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_FireBallCast.ush"),
        TEXT("/CkParticles/Behaviors/Behavior_LightningCast.ush"),
    };
}

// --------------------------------------------------------------------------------------------------------------------
// The DI is stateless, so this proxy carries no data — it exists only because a GPU-capable DI requires one.
struct FNiagaraDataInterfaceProxyCkParticles : public FNiagaraDataInterfaceProxy
{
    virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override { check(false); }
    virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }
};

// --------------------------------------------------------------------------------------------------------------------
// CPU mirror of /CkParticles/CkParticles_Behaviors.ush — same BehaviorId, same math, or the two diverge.
namespace NDICkParticlesLocal
{
    // The public result type IS the mirror's output — one definition, so a field added here cannot drift
    // out of sync with what callers and tests observe.
    using FStageIO = FCk_Particles_StageResult;

    // Mirrors CkParticles_Rand / CkParticles_RandDir (Common.ush); 24-bit normalization keeps it bit-identical.
    static auto Rand(int32 InSeed, int32 InSalt) -> float
    {
        uint32 n = uint32(InSeed) * 747796405u + uint32(InSalt) * 2891336453u + 1u;
        n ^= n >> 16;
        n *= 2246822519u;
        n ^= n >> 13;
        n *= 3266489917u;
        n ^= n >> 16;
        return float(n & 0x00FFFFFFu) / 16777216.0f;
    }

    static auto RandDir(int32 InSeed) -> FVector3f
    {
        const float z = 2.0f * Rand(InSeed, 1) - 1.0f;
        const float t = 6.28318530718f * Rand(InSeed, 2);
        const float r = FMath::Sqrt(FMath::Clamp(1.0f - z * z, 0.0f, 1.0f));
        return FVector3f(r * FMath::Cos(t), r * FMath::Sin(t), z);
    }

    // Mirrors CkParticles_CylinderPoint (Common.ush) — uniform through the volume, so the radius takes a square root.
    static auto CylinderPoint(int32 InSeed, float InRadius, float InHeight, FVector3f InOffset) -> FVector3f
    {
        const float Angle = Rand(InSeed, 9) * 6.28318530718f;
        const float R     = InRadius * FMath::Sqrt(Rand(InSeed, 7));
        const float Z     = (Rand(InSeed, 10) - 0.5f) * InHeight;

        return FVector3f(R * FMath::Cos(Angle), R * FMath::Sin(Angle), Z) + InOffset;
    }

    // Mirrors CkParticles_HsvToRgb (Common.ush) — the inverse half of the source's Random Hue/Saturation/Value mode.
    static auto HsvToRgb(float InH, float InS, float InV) -> FVector3f
    {
        const float Hue    = FMath::Frac(InH) * 6.0f;
        const float Sector = FMath::FloorToFloat(Hue);
        const float F      = Hue - Sector;

        const float P = InV * (1.0f - InS);
        const float Q = InV * (1.0f - InS * F);
        const float T = InV * (1.0f - InS * (1.0f - F));

        if (Sector < 1.0f) { return FVector3f(InV, T, P); }
        if (Sector < 2.0f) { return FVector3f(Q, InV, P); }
        if (Sector < 3.0f) { return FVector3f(P, InV, T); }
        if (Sector < 4.0f) { return FVector3f(P, Q, InV); }
        if (Sector < 5.0f) { return FVector3f(T, P, InV); }
        return FVector3f(InV, P, Q);
    }

    // Mirrors CkParticles_SpawnPhase / CkParticles_IsBurstSpawn (Common.ush). Read the .ush header for why the
    // epsilon is a float-precision floor rather than a tolerance.
    static auto SpawnPhase(float InEmitterAge, float InAge, float InLoopDuration) -> float
    {
        const auto P = FMath::Fmod(InEmitterAge - InAge, InLoopDuration);
        return P < 0.0f ? P + InLoopDuration : P;
    }

    static auto IsBurstSpawn(float InSpawnPhase, float InLoopDuration) -> bool
    {
        constexpr auto Eps = 0.001f;
        return InSpawnPhase < Eps || InSpawnPhase > InLoopDuration - Eps;
    }

    // ---- Deterministic curl noise. Mirrors Common.ush's CkParticles_Noise*/CurlNoise/CurlPath -------------------
    // Read the .ush's header comment for the design (24-bit lattice hash, central differences at a fixed eps,
    // stateless fixed-step advection). Every constant below is the same literal on the GPU side.

    static auto NoiseHash3(int32 InX, int32 InY, int32 InZ, uint32 InSalt) -> float
    {
        uint32 n = uint32(InX) * 747796405u + uint32(InY) * 2654435761u + uint32(InZ) * 805459861u
                 + InSalt * 2891336453u + 1u;
        n ^= n >> 16;
        n *= 2246822519u;
        n ^= n >> 13;
        n *= 3266489917u;
        n ^= n >> 16;
        return float(n & 0x00FFFFFFu) / 16777216.0f;
    }

    static auto NoiseWrap(int32 InV, int32 InPeriod) -> int32
    {
        return ((InV % InPeriod) + InPeriod) % InPeriod;
    }

    static auto ValueNoise3(FVector3f InP, int32 InPeriod, uint32 InSalt) -> float
    {
        const auto Cix = FMath::FloorToInt(InP.X);
        const auto Ciy = FMath::FloorToInt(InP.Y);
        const auto Ciz = FMath::FloorToInt(InP.Z);

        const auto Cfx = InP.X - float(Cix);
        const auto Cfy = InP.Y - float(Ciy);
        const auto Cfz = InP.Z - float(Ciz);

        const auto Wx = Cfx * Cfx * (3.0f - 2.0f * Cfx);
        const auto Wy = Cfy * Cfy * (3.0f - 2.0f * Cfy);
        const auto Wz = Cfz * Cfz * (3.0f - 2.0f * Cfz);

        const auto X0 = NoiseWrap(Cix,     InPeriod);
        const auto Y0 = NoiseWrap(Ciy,     InPeriod);
        const auto Z0 = NoiseWrap(Ciz,     InPeriod);
        const auto X1 = NoiseWrap(Cix + 1, InPeriod);
        const auto Y1 = NoiseWrap(Ciy + 1, InPeriod);
        const auto Z1 = NoiseWrap(Ciz + 1, InPeriod);

        const auto N000 = NoiseHash3(X0, Y0, Z0, InSalt);
        const auto N100 = NoiseHash3(X1, Y0, Z0, InSalt);
        const auto N010 = NoiseHash3(X0, Y1, Z0, InSalt);
        const auto N110 = NoiseHash3(X1, Y1, Z0, InSalt);
        const auto N001 = NoiseHash3(X0, Y0, Z1, InSalt);
        const auto N101 = NoiseHash3(X1, Y0, Z1, InSalt);
        const auto N011 = NoiseHash3(X0, Y1, Z1, InSalt);
        const auto N111 = NoiseHash3(X1, Y1, Z1, InSalt);

        const auto Near = FMath::Lerp(FMath::Lerp(N000, N100, Wx), FMath::Lerp(N010, N110, Wx), Wy);
        const auto Far  = FMath::Lerp(FMath::Lerp(N001, N101, Wx), FMath::Lerp(N011, N111, Wx), Wy);
        return FMath::Lerp(Near, Far, Wz);
    }

    static auto NoiseFbm3(FVector3f InP, uint32 InSalt) -> float
    {
        auto Sum = 0.0f, Amp = 0.5f, Freq = 1.0f, Norm = 0.0f;
        auto Period = 16;

        for (auto i = 0; i < 3; ++i)
        {
            Sum  += Amp * ValueNoise3(InP * Freq, Period, InSalt);
            Norm += Amp;
            Amp  *= 0.5f;
            Freq *= 2.0f;
            Period *= 2;
        }
        return Sum / Norm;
    }

    // These two are the pair a behavior actually calls (behavior 32's two sparkle clouds are the first). The
    // lattice helpers above stay internal.
    auto CurlNoise(FVector3f InP, float InFreq, uint32 InSeed) -> FVector3f
    {
        const auto Q = InP * InFreq;
        constexpr auto E = 0.01f;

        const auto Dx = FVector3f(E, 0.0f, 0.0f);
        const auto Dy = FVector3f(0.0f, E, 0.0f);
        const auto Dz = FVector3f(0.0f, 0.0f, E);

        const auto PsiX_Yp = NoiseFbm3(Q + Dy, InSeed);
        const auto PsiX_Yn = NoiseFbm3(Q - Dy, InSeed);
        const auto PsiX_Zp = NoiseFbm3(Q + Dz, InSeed);
        const auto PsiX_Zn = NoiseFbm3(Q - Dz, InSeed);

        const auto PsiY_Xp = NoiseFbm3(Q + Dx, InSeed + 1u);
        const auto PsiY_Xn = NoiseFbm3(Q - Dx, InSeed + 1u);
        const auto PsiY_Zp = NoiseFbm3(Q + Dz, InSeed + 1u);
        const auto PsiY_Zn = NoiseFbm3(Q - Dz, InSeed + 1u);

        const auto PsiZ_Xp = NoiseFbm3(Q + Dx, InSeed + 2u);
        const auto PsiZ_Xn = NoiseFbm3(Q - Dx, InSeed + 2u);
        const auto PsiZ_Yp = NoiseFbm3(Q + Dy, InSeed + 2u);
        const auto PsiZ_Yn = NoiseFbm3(Q - Dy, InSeed + 2u);

        const auto Inv = 1.0f / (2.0f * E);
        return FVector3f(
            ((PsiZ_Yp - PsiZ_Yn) - (PsiY_Zp - PsiY_Zn)) * Inv,
            ((PsiX_Zp - PsiX_Zn) - (PsiZ_Xp - PsiZ_Xn)) * Inv,
            ((PsiY_Xp - PsiY_Xn) - (PsiX_Yp - PsiX_Yn)) * Inv);
    }

    auto CurlPath(FVector3f InSpawnPosition, float InAge, float InFreq, float InStrength, uint32 InSeed)
        -> FVector3f
    {
        const auto Dt = InAge / 16.0f;

        auto P = InSpawnPosition;
        for (auto i = 0; i < 16; ++i)
        { P += CurlNoise(P, InFreq, InSeed) * InStrength * Dt; }
        return P;
    }

    // HLSL intrinsic mirrors: keeps the C++ textually parallel to the .ush so review is a line-by-line diff.
    static auto Saturate(float InX) -> float { return FMath::Clamp(InX, 0.0f, 1.0f); }
    static auto Frac(float InX) -> float { return FMath::Frac(InX); }
    static auto Step(float InEdge, float InX) -> float { return InX >= InEdge ? 1.0f : 0.0f; }
    static auto SmoothStep(float InE0, float InE1, float InX) -> float
    {
        const float S = FMath::Clamp((InX - InE0) / (InE1 - InE0), 0.0f, 1.0f);
        return S * S * (3.0f - 2.0f * S);
    }

    // Component-wise on purpose: engine quat helpers could reorder the float math and break GPU/CPU lockstep.
    static auto QuatFromAxisAngle(FVector3f InAxis, float InAngle) -> FQuat4f
    {
        const float H = 0.5f * InAngle;
        const float S = FMath::Sin(H);
        return FQuat4f(InAxis.X * S, InAxis.Y * S, InAxis.Z * S, FMath::Cos(H));
    }

    static auto QuatMul(FQuat4f InA, FQuat4f InB) -> FQuat4f
    {
        return FQuat4f(
            InA.W * InB.X + InA.X * InB.W + InA.Y * InB.Z - InA.Z * InB.Y,
            InA.W * InB.Y - InA.X * InB.Z + InA.Y * InB.W + InA.Z * InB.X,
            InA.W * InB.Z + InA.X * InB.Y - InA.Y * InB.X + InA.Z * InB.W,
            InA.W * InB.W - InA.X * InB.X - InA.Y * InB.Y - InA.Z * InB.Z);
    }

    // ---- Shared source-curve transcription. Mirrors Common.ush's CkParticles_KeyN / _IntN / _QuatFromZTo -----------

    static auto Key2(float InT, float InT0, float InV0, float InT1, float InV1) -> float
    {
        return FMath::Lerp(InV0, InV1, Saturate((InT - InT0) / FMath::Max(InT1 - InT0, 1.0e-6f)));
    }

    static auto Key3(float InT, float InT0, float InV0, float InT1, float InV1, float InT2, float InV2) -> float
    {
        return InT <= InT1 ? Key2(InT, InT0, InV0, InT1, InV1)
                           : Key2(InT, InT1, InV1, InT2, InV2);
    }

    static auto Key4(float InT, float InT0, float InV0, float InT1, float InV1, float InT2, float InV2,
                     float InT3, float InV3) -> float
    {
        return InT <= InT1 ? Key2(InT, InT0, InV0, InT1, InV1)
                           : Key3(InT, InT1, InV1, InT2, InV2, InT3, InV3);
    }

    static auto Key5(float InT, float InT0, float InV0, float InT1, float InV1, float InT2, float InV2,
                     float InT3, float InV3, float InT4, float InV4) -> float
    {
        return InT <= InT1 ? Key2(InT, InT0, InV0, InT1, InV1)
                           : Key4(InT, InT1, InV1, InT2, InV2, InT3, InV3, InT4, InV4);
    }

    static auto Int2(float InS, float InV0, float InV1) -> float
    {
        const auto A = Saturate(InS);
        return A * (InV0 + Key2(A, 0.0f, InV0, 1.0f, InV1)) * 0.5f;
    }

    static auto Int3(float InS, float InK, float InV0, float InV1, float InV2) -> float
    {
        const auto T = Saturate(InS);
        const auto A = FMath::Min(T, InK);
        auto Sum = A * (InV0 + Key2(A, 0.0f, InV0, InK, InV1)) * 0.5f;

        if (T > InK)
        {
            const auto B = T - InK;
            Sum += B * (InV1 + Key2(T, InK, InV1, 1.0f, InV2)) * 0.5f;
        }
        return Sum;
    }

    static auto QuatFromZTo(FVector3f InDir) -> FQuat4f
    {
        const auto Up = FVector3f(0.0f, 0.0f, 1.0f);
        const auto D  = FMath::Clamp(FVector3f::DotProduct(Up, InDir), -1.0f, 1.0f);

        if (D < -0.999999f)
        { return FQuat4f(1.0f, 0.0f, 0.0f, 0.0f); }

        const auto Axis = FVector3f::CrossProduct(Up, InDir);
        const auto Len  = Axis.Size();

        if (Len < 1.0e-6f)
        { return FQuat4f(0.0f, 0.0f, 0.0f, 1.0f); }

        return QuatFromAxisAngle(Axis / Len, FMath::Acos(D));
    }

    // ---- Behavior 7 (Slash) helpers. Mirrors of Behavior_Slash.ush's CkParticles_Slash_* functions ----------------

    static auto Slash_Key2(float InT, float InT0, float InV0, float InT1, float InV1) -> float
    {
        return FMath::Lerp(InV0, InV1, Saturate((InT - InT0) / FMath::Max(InT1 - InT0, 1.0e-6f)));
    }

    static auto Slash_Key3(float InT, float InT0, float InV0, float InT1, float InV1, float InT2, float InV2) -> float
    {
        return InT <= InT1
            ? Slash_Key2(InT, InT0, InV0, InT1, InV1)
            : Slash_Key2(InT, InT1, InV1, InT2, InV2);
    }

    static auto Slash_VelScaleIntegral(float InS, float InC1, float InC2) -> float
    {
        constexpr auto Knee = 0.2f;
        const auto A = FMath::Min(InS, Knee);
        auto Sum = A * (1.0f + Slash_Key2(A, 0.0f, 1.0f, Knee, InC1)) * 0.5f;

        if (InS > Knee)
        {
            const auto B = InS - Knee;
            Sum += B * (InC1 + Slash_Key2(InS, Knee, InC1, 1.0f, InC2)) * 0.5f;
        }
        return Sum;
    }

    static auto ExecuteStage_CPU(
        int32 InBehaviorId, float InDeltaTime, float InAge, float InLifetime,
        FVector3f InPosition, FVector3f InVelocity, int32 InSeed, float InEmitterAge) -> FStageIO
    {
        FStageIO Out;
        Out.Position = InPosition;
        Out.Velocity = InVelocity;

        const float NormalizedAge = InLifetime > 0.0f ? FMath::Clamp(InAge / InLifetime, 0.0f, 1.0f) : 0.0f;

        const float SizeBase = FMath::Lerp(18.0f, 8.0f, NormalizedAge) * FMath::Lerp(0.65f, 1.35f, Rand(InSeed, 7));
        Out.Size = FVector2f(SizeBase, SizeBase);

        switch (InBehaviorId)
        {
            case 2: // Explosion — per-Seed radial burst, ease-out, slight gravity sag.
            {
                const FVector3f Dir  = RandDir(InSeed);
                const float     MaxR = FMath::Lerp(120.0f, 300.0f, Rand(InSeed, 3));
                const float     Ease = 1.0f - FMath::Pow(1.0f - NormalizedAge, 2.0f);
                const FVector3f Sag(0.0f, 0.0f, -150.0f * NormalizedAge * NormalizedAge);
                Out.Position = Dir * (MaxR * Ease) + Sag;
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 2.2f, 0.8f, 1.0f), FLinearColor(1.0f, 0.08f, 0.0f, 1.0f), NormalizedAge);
                break;
            }
            case 3: // Fire — per-Seed rising column that tapers inward, sine flicker, white-hot -> dark red.
            {
                const float Ang   = Rand(InSeed, 1) * 6.28318530718f;
                const float BaseR = FMath::Lerp(8.0f, 45.0f, Rand(InSeed, 2));
                const float Taper = 1.0f - NormalizedAge;
                const float Flick = FMath::Sin(InAge * 9.0f + float(InSeed)) * 8.0f * Taper;
                const float Rise  = FMath::Lerp(180.0f, 280.0f, Rand(InSeed, 3)) * NormalizedAge;
                Out.Position = FVector3f(FMath::Cos(Ang) * BaseR * Taper + Flick, FMath::Sin(Ang) * BaseR * Taper, Rise);
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 1.8f, 0.4f, 1.0f), FLinearColor(0.5f, 0.03f, 0.0f, 1.0f), NormalizedAge);
                break;
            }
            case 4: // Fireworks — per-Seed burst sphere, strong decel, gravity arc, per-particle rainbow + twinkle.
            {
                const FVector3f Dir   = RandDir(InSeed);
                const float     MaxR  = FMath::Lerp(180.0f, 280.0f, Rand(InSeed, 4));
                const float     Ease  = 1.0f - FMath::Pow(1.0f - NormalizedAge, 3.0f);
                const FVector3f Grav(0.0f, 0.0f, -260.0f * NormalizedAge * NormalizedAge);
                const float     Twink = 0.5f + 0.5f * FMath::Sin(InAge * 28.0f + float(InSeed) * 1.7f);
                const float     H     = Rand(InSeed, 5);
                const FVector3f Hue(
                    0.6f + 0.4f * FMath::Cos(6.28318530718f * (H + 0.0f)),
                    0.6f + 0.4f * FMath::Cos(6.28318530718f * (H + 0.33f)),
                    0.6f + 0.4f * FMath::Cos(6.28318530718f * (H + 0.67f)));
                const float Amp = Twink * (1.0f - NormalizedAge) * 2.5f;
                Out.Position = Dir * (MaxR * Ease) + Grav;
                Out.Color    = FLinearColor(Hue.X * Amp, Hue.Y * Amp, Hue.Z * Amp, 1.0f);
                break;
            }
            case 5: // Galaxy — per-Seed 3-arm rotating spiral disk winding outward, blue core -> warm rim.
            {
                const int32 Arm     = int32(Rand(InSeed, 1) * 3.0f);
                const float ArmBase = float(Arm) * 2.09439510239f;
                const float Radius  = FMath::Lerp(40.0f, 260.0f, NormalizedAge);
                const float Spin    = InAge * 0.6f;
                const float Angle   = ArmBase + Radius * 0.012f + Spin;
                const float ZJit    = (Rand(InSeed, 2) - 0.5f) * 30.0f;
                Out.Position = FVector3f(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), ZJit);
                Out.Color    = FMath::Lerp(FLinearColor(0.7f, 0.8f, 2.0f, 1.0f), FLinearColor(1.6f, 1.2f, 0.9f, 1.0f), NormalizedAge);
                break;
            }
            case 1: // Swirl — self-driving Age-based vortex helix (mirrors Behavior_Swirl.ush). Tint blue->magenta.
            {
                constexpr float AngularSpeed = 3.0f;   // radians / second
                constexpr float RadialSpeed  = 120.0f; // units / second outward
                constexpr float RiseSpeed    = 80.0f;  // units / second upward
                const float Angle  = AngularSpeed * InAge;
                const float Radius = RadialSpeed * InAge;
                const float S = FMath::Sin(Angle);
                const float C = FMath::Cos(Angle);
                Out.Position = FVector3f(Radius * C, Radius * S, RiseSpeed * InAge);
                Out.Velocity = FVector3f(RadialSpeed * C - Radius * AngularSpeed * S,
                                         RadialSpeed * S + Radius * AngularSpeed * C,
                                         RiseSpeed);
                Out.Color    = FMath::Lerp(FLinearColor(0.1f, 0.4f, 1.0f, 1.0f), FLinearColor(1.0f, 0.2f, 0.6f, 1.0f), NormalizedAge);
                break;
            }
            case 6: // Beam — Tube carrier body + converging core stream down +X. Mirrors Behavior_Beam.ush.
            {
                const float k = Rand(InSeed, 3);

                if (k < 0.18f)
                {
                    const float Pulse = 0.5f + 0.5f * FMath::Sin(InAge * 9.0f + 6.28318530718f * Rand(InSeed, 4));
                    Out.Position  = FVector3f::ZeroVector;
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 1;
                    Out.Scale     = FVector3f(4.5f, FMath::Lerp(0.55f, 0.72f, Pulse), FMath::Lerp(0.55f, 0.72f, Pulse));
                    Out.Dynamic   = FVector4f(0.22f, 0.35f, -InAge * 1.6f, 1.5f);
                    Out.Color     = FLinearColor(0.4f, 1.2f, 3.2f, 0.8f);
                    break;
                }

                const float Along  = FMath::Lerp(0.0f, 450.0f, NormalizedAge);
                const float Spread = FMath::Lerp(28.0f, 3.0f, NormalizedAge);
                const float A      = Rand(InSeed, 1) * 6.28318530718f;
                const float R      = Spread * Rand(InSeed, 2);
                Out.Position = FVector3f(Along, FMath::Cos(A) * R, FMath::Sin(A) * R);
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 3.0f, 2.2f, 1.0f), FLinearColor(0.2f, 0.5f, 2.0f, 1.0f), NormalizedAge);
                break;
            }
            case 7: // Slash — Vefects NS_BasicAttack. Mirrors Behavior_Slash.ush; recipe Cookbook/NS_BasicAttack.md.
            {
                constexpr auto NumLayers    = 19;
                constexpr auto LayerSlash01 = 0;
                constexpr auto LayerSlash02 = 1;
                constexpr auto LayerSlash03 = 2;
                constexpr auto LayerWind    = 3;
                constexpr auto FirstSpark   = 4;

                constexpr auto LifeSlash  = 0.3f;
                constexpr auto LifeWind   = 0.5f;
                constexpr auto SparkDelay = 0.06f;

                constexpr auto SparkRingRadius = 209.769f;
                constexpr auto SparkRingAngle  = -0.11344640138f; // -6.5 degrees

                const auto HideLayer = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;

                if (Layer < FirstSpark)
                {
                    const auto Life = Layer == LayerWind ? LifeWind : LifeSlash;
                    if (InAge > Life)
                    {
                        HideLayer();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    Out.Orientation = QuatFromAxisAngle(FVector3f(-1.0f, 0.0f, 0.0f), 1.57079632679f);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    if (Layer == LayerSlash01 || Layer == LayerSlash02)
                    {
                        Out.VisTag = Layer == LayerSlash01 ? 5 : 6;
                        Out.Scale  = FVector3f(1.0f, 1.0f, 1.0f);
                        Out.Color  = FLinearColor(1.0f,
                            Slash_Key2(t, 0.0f, 0.7835f, 1.0f, 0.4564f),
                            Slash_Key2(t, 0.0f, 0.0976f, 1.0f, 0.0018f),
                            1.0f);

                        Out.Dynamic = Layer == LayerSlash01
                            ? FVector4f(Slash_Key2(t, 0.4f, -0.1f, 1.0f, -1.0f),
                                        0.5f,
                                        Slash_Key2(t, 0.0f, -1.0f, 1.0f, 0.4f),
                                        0.0f)
                            : FVector4f(1.0f,
                                        0.0f,
                                        Slash_Key2(t, 0.0f, -1.0f, 1.0f, 0.7f),
                                        Slash_Key2(t, 0.4f, 0.0f, 1.0f, -0.5f));
                        break;
                    }

                    if (Layer == LayerSlash03)
                    {
                        Out.VisTag  = 7;
                        Out.Scale   = FVector3f(1.0f, 1.0f, 1.0f);
                        Out.Color   = FLinearColor(0.3813f, 0.1946f, 0.0006f, 0.2f);
                        Out.Dynamic = FVector4f(1.0f,
                            0.0f,
                            Slash_Key2(t, 0.0f, -1.0f, 1.0f, 0.6f),
                            Slash_Key2(t, 0.4f, 0.0f, 1.0f, -0.4f));
                        break;
                    }

                    const auto WindRamp  = Saturate(t / 0.880f);
                    const auto WindAlpha = t <= 0.157f ? Slash_Key2(t, 0.0f,   0.0f, 0.157f, 1.0f)
                                         : t <= 0.316f ? 1.0f
                                                       : Slash_Key2(t, 0.316f, 1.0f, 1.0f,   0.0f);

                    Out.VisTag  = 8;
                    Out.Scale   = FVector3f(1.1f, 1.1f, 1.1f);
                    Out.Color   = FLinearColor(
                        FMath::Lerp(1.0f, 0.2623f, WindRamp),
                        FMath::Lerp(1.0f, 0.1384f, WindRamp),
                        FMath::Lerp(1.0f, 0.0887f, WindRamp),
                        WindAlpha * 0.2f);
                    Out.Dynamic = FVector4f(-0.27f,
                        0.0f,
                        Slash_Key2(t, 0.0f, -0.375f, 0.8f, 0.375f),
                        Slash_Key2(t, 0.4f, 0.0f, 1.0f, -0.4f));
                    break;
                }

                const auto SparkLife = FMath::Lerp(0.2f, 0.4f, Rand(InSeed, 1));
                const auto td        = InAge - SparkDelay;

                if (td < 0.0f || td > SparkLife)
                {
                    HideLayer();
                    break;
                }

                const auto s = Saturate(td / SparkLife);

                const auto V0 = FVector3f(
                    FMath::Lerp(-1500.0f, -3500.0f, Rand(InSeed, 2)),
                    FMath::Lerp( -500.0f,   500.0f, Rand(InSeed, 3)),
                    FMath::Lerp( 1500.0f,  3000.0f, Rand(InSeed, 4)));

                const auto ScaleXy = Slash_Key3(s, 0.0f, 1.0f, 0.2f, 0.35f, 1.0f,  0.05f);
                const auto ScaleZ  = Slash_Key3(s, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, -0.25f);

                const auto IntXy = Slash_VelScaleIntegral(s, 0.35f,  0.05f) * SparkLife;
                const auto IntZ  = Slash_VelScaleIntegral(s, 0.25f, -0.25f) * SparkLife;

                const auto Spawn = FVector3f(
                    SparkRingRadius * FMath::Cos(SparkRingAngle),
                    0.0f,
                    SparkRingRadius * FMath::Sin(SparkRingAngle));

                Out.Position = Spawn + FVector3f(V0.X * IntXy, V0.Y * IntXy, V0.Z * IntZ);
                Out.Velocity = FVector3f(V0.X * ScaleXy, V0.Y * ScaleXy, V0.Z * ScaleZ);
                Out.VisTag   = 9;

                const auto Width  = FMath::Lerp(10.0f, 30.0f, Rand(InSeed, 5));
                const auto Length = FMath::Lerp(50.0f, 70.0f, Rand(InSeed, 6));
                const auto Grow   = Slash_Key3(s, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                Out.Size = FVector2f(Width * Grow, Length * Grow * Slash_Key2(s, 0.0f, 1.0f, 1.0f, 0.6f));

                Out.Color = FLinearColor(1.0f,
                    Slash_Key3(s, 0.0f, 0.9473f, 0.4576f, 0.6939f, 1.0f, 0.4508f),
                    Slash_Key3(s, 0.0f, 0.6654f, 0.4576f, 0.1470f, 1.0f, 0.0409f),
                    1.0f);
                break;
            }
            case 8: // Nova — Shell dome + Disc shockwave + rim sparks. Mirrors Behavior_Nova.ush.
            {
                const float k = Rand(InSeed, 3);

                if (k < 0.14f)
                {
                    Out.Position  = FVector3f::ZeroVector;
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 2;
                    Out.Scale     = FVector3f(1.0f, 1.0f, 1.0f) * FMath::Lerp(0.15f, 2.2f, NormalizedAge);
                    Out.Dynamic   = FVector4f(NormalizedAge * 0.9f, 0.2f, 0.0f, 2.5f * (1.0f - NormalizedAge));
                    const float Fade = 1.0f - NormalizedAge;
                    Out.Color = FLinearColor(1.5f * Fade, 0.9f * Fade, 0.35f * Fade, Fade);
                    break;
                }
                if (k < 0.32f)
                {
                    Out.Position  = FVector3f(0.0f, 0.0f, 2.0f);
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 3;
                    Out.Scale     = FVector3f(1.0f, 1.0f, 1.0f) * FMath::Lerp(0.1f, 3.8f, NormalizedAge);
                    Out.Dynamic   = FVector4f(NormalizedAge, 0.25f, -NormalizedAge * 0.8f, 1.8f * (1.0f - NormalizedAge));
                    const float Fade = 1.0f - NormalizedAge;
                    Out.Color = FLinearColor(2.6f * Fade, 1.1f * Fade, 0.2f * Fade, Fade);
                    break;
                }

                const float A      = Rand(InSeed, 1) * 6.28318530718f;
                const float Rad    = FMath::Lerp(10.0f, 380.0f, NormalizedAge);
                const float Jitter = (Rand(InSeed, 2) - 0.5f) * 16.0f;
                Out.Position = FVector3f(FMath::Cos(A) * (Rad + Jitter), FMath::Sin(A) * (Rad + Jitter), 0.0f);
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 2.4f, 1.0f, 1.0f), FLinearColor(1.2f, 0.3f, 0.0f, 1.0f), NormalizedAge) * (1.0f - NormalizedAge);
                break;
            }
            case 9: // MuzzleFlash — core flash + wide flecks + drag-damped sparks, cycled; barrel = +X. Mirrors Behavior_MuzzleFlash.ush.
            {
                const float Cycle = 0.35f;
                const float t = Frac(InAge / Cycle + Rand(InSeed, 9)) * Cycle;
                const float m = t / Cycle;
                const float k = Rand(InSeed, 0);

                const FVector3f Hot = FMath::Lerp(FVector3f(100.0f, 18.9f, 0.54f), FVector3f(10.0f, 8.0f, 1.0f), m);

                if (k < 0.12f)
                {
                    const float Vis  = Saturate(1.0f - t / 0.08f);
                    const float Grow = FMath::Lerp(0.5f, 1.3f, Rand(InSeed, 1)) * (0.6f + 0.8f * Saturate(t / 0.03f));
                    Out.Position = FVector3f(18.0f, 0.0f, 0.0f);
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Size     = FVector2f(55.0f, 40.0f) * Grow;
                    Out.Color    = FLinearColor(Hot.X * Vis, Hot.Y * Vis, Hot.Z * Vis, Vis);
                }
                else if (k < 0.4f)
                {
                    const float u  = Rand(InSeed, 2);
                    const float az = 6.28318530718f * Rand(InSeed, 3);
                    const float th = 0.7605f * FMath::Sqrt(u);
                    const FVector3f d(FMath::Cos(th), FMath::Sin(th) * FMath::Cos(az), FMath::Sin(th) * FMath::Sin(az));
                    const float s   = FMath::Lerp(50.0f, 100.0f, Rand(InSeed, 4));
                    const float Vis = Saturate(1.0f - t / 0.25f);
                    Out.Position = d * s * t;
                    Out.Velocity = d * s;
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(2.2f, 7.0f);
                    Out.Color    = FLinearColor(Hot.X * Vis, Hot.Y * Vis, Hot.Z * Vis, Vis);
                }
                else
                {
                    const float u  = Rand(InSeed, 5);
                    const float az = 6.28318530718f * Rand(InSeed, 6);
                    const float th = 0.5236f * FMath::Sqrt(u);
                    const FVector3f d(FMath::Cos(th), FMath::Sin(th) * FMath::Cos(az), FMath::Sin(th) * FMath::Sin(az));
                    const float s   = FMath::Lerp(250.0f, 500.0f, Rand(InSeed, 7));
                    const float Vis = Saturate(1.0f - t / 0.20f);
                    Out.Position = d * s * (1.0f - FMath::Exp(-t));
                    Out.Velocity = d * s * FMath::Exp(-t);
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(1.6f, 9.0f) * Saturate(1.0f - t / 0.18f);
                    Out.Color    = FLinearColor(37.0f, 5.58f, 0.5f, 1.0f) * Vis;
                }
                break;
            }
            case 10: // ImpactBurst — flash pop + gravity sparks + smoke puff (burst template, real Age). Mirrors Behavior_ImpactBurst.ush.
            {
                const float t = InAge;
                const float m = Saturate(t / 1.2f);
                const float k = Rand(InSeed, 0);

                if (k < 0.08f)
                {
                    const float s   = Saturate(t / 0.12f);
                    const float Pop = s < 0.38f ? s / 0.38f : 1.0f - (s - 0.38f) / 0.62f;
                    const float Vis = Saturate(1.0f - t / 0.12f);
                    Out.Position = FVector3f(0.0f, 0.0f, 6.0f);
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Size     = FVector2f(90.0f, 90.0f) * Pop;
                    Out.Color    = FLinearColor(35.0f, 0.645f, 0.0f, 1.0f) * Vis;
                }
                else if (k < 0.55f)
                {
                    const float u  = Rand(InSeed, 1);
                    const float az = 6.28318530718f * Rand(InSeed, 2);
                    const float th = 0.7854f * FMath::Sqrt(u);
                    const FVector3f d(FMath::Sin(th) * FMath::Cos(az), FMath::Sin(th) * FMath::Sin(az), FMath::Cos(th));
                    const float s   = FMath::Lerp(150.0f, 500.0f, Rand(InSeed, 3));
                    const float Vis = Saturate(1.0f - t / 0.53f);
                    Out.Position = d * s * (1.0f - FMath::Exp(-t)) + FVector3f(0.0f, 0.0f, -490.0f * t * t);
                    Out.Velocity = d * s * FMath::Exp(-t) + FVector3f(0.0f, 0.0f, -980.0f * t);
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(1.6f, 11.0f);
                    Out.Color    = FLinearColor(35.0f, 2.6f, 0.2f, 1.0f) * Vis;
                }
                else
                {
                    const float u  = Rand(InSeed, 4);
                    const float az = 6.28318530718f * Rand(InSeed, 5);
                    const float th = 0.8552f * FMath::Sqrt(u);
                    const FVector3f d(FMath::Sin(th) * FMath::Cos(az), FMath::Sin(th) * FMath::Sin(az), FMath::Cos(th));
                    const float s   = FMath::Lerp(100.0f, 300.0f, Rand(InSeed, 6));
                    const float kd  = (1.0f - FMath::Exp(-0.25f * t)) / 0.25f;
                    const float Vis = Saturate(t / 0.08f) * Saturate(1.0f - m);
                    const float g   = FMath::Lerp(0.5f, 0.12f, m);
                    Out.Position = d * s * kd * 0.25f + FVector3f(0.0f, 0.0f, 8.0f);
                    Out.Velocity = d * s * FMath::Exp(-0.25f * t) * 0.25f;
                    Out.VisTag   = 2;
                    Out.Rotation = (Rand(InSeed, 7) - 0.5f) * 360.0f + (Rand(InSeed, 8) - 0.5f) * 80.0f * t;
                    Out.Dynamic  = FVector4f(m * 0.85f, 0.0f, 0.0f, 0.0f);
                    Out.Size     = FVector2f(1.0f, 1.0f) * FMath::Lerp(40.0f, 95.0f, m);
                    Out.Color    = FLinearColor(g, g, g * 1.05f, 0.6f) * Vis;
                }
                break;
            }
            case 11: // Tracer — backward-drifting trail quads + backward spark spray; forward = +X. Mirrors Behavior_Tracer.ush.
            {
                const float k = Rand(InSeed, 0);

                if (k < 0.25f)
                {
                    const FVector3f Jitter(0.0f, (Rand(InSeed, 1) - 0.5f) * 4.0f, (Rand(InSeed, 2) - 0.5f) * 4.0f);
                    Out.Position = FVector3f(-800.0f * InAge, 0.0f, 0.0f) + Jitter;
                    Out.Velocity = FVector3f(-800.0f, 0.0f, 0.0f);
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(7.0f, 34.0f) * (1.0f - NormalizedAge);
                    Out.Color    = FLinearColor(37.0f, 13.85f, 0.5f, 1.0f) * Saturate((1.0f - NormalizedAge) / 0.82f);
                }
                else
                {
                    const float u  = Rand(InSeed, 3);
                    const float az = 6.28318530718f * Rand(InSeed, 4);
                    const float th = 0.4189f * FMath::Sqrt(u);
                    const FVector3f d(-FMath::Cos(th), FMath::Sin(th) * FMath::Cos(az), FMath::Sin(th) * FMath::Sin(az));
                    const float s = FMath::Lerp(150.0f, 500.0f, Rand(InSeed, 5));
                    Out.Position = d * s * (1.0f - FMath::Exp(-InAge)) + FVector3f(0.0f, 0.0f, -59.03f * InAge * InAge);
                    Out.Velocity = d * s * FMath::Exp(-InAge) + FVector3f(0.0f, 0.0f, -118.07f * InAge);
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(1.2f, 8.0f);
                    Out.Color    = FLinearColor(37.0f, 10.83f, 0.5f, 1.0f) * Saturate(1.0f - NormalizedAge / 0.44f);
                }
                break;
            }
            case 12: // SmokePlume — buoyant rising puffs, grow ~7x, grey ramp + ember birth pop. Mirrors Behavior_SmokePlume.ush.
            {
                const float t = InAge;

                const float r  = 50.0f * FMath::Sqrt(Rand(InSeed, 1));
                const float th = 6.28318530718f * Rand(InSeed, 2);
                const float vx = FMath::Lerp(-75.0f, 75.0f, Rand(InSeed, 3)) * 0.35f;
                const float vy = FMath::Lerp(-25.0f, 25.0f, Rand(InSeed, 4)) * 0.35f;
                const float vz = FMath::Lerp(100.0f, 200.0f, Rand(InSeed, 5)) * 0.6f;

                const float WobX = 8.0f * FMath::Sin(1.3f * t + 6.28318530718f * Rand(InSeed, 6));
                const float WobY = 8.0f * FMath::Cos(1.1f * t + 6.28318530718f * Rand(InSeed, 7));

                Out.Position = FVector3f(r * FMath::Cos(th) + vx * t + WobX, r * FMath::Sin(th) + vy * t + WobY, 20.0f + vz * t + 6.0f * t * t);
                Out.Velocity = FVector3f(vx, vy, vz + 12.0f * t);

                const float Base = FMath::Lerp(12.5f, 17.5f, Rand(InSeed, 8));
                Out.Size = FVector2f(1.0f, 1.0f) * Base * (10.0f + 5.0f * Saturate(NormalizedAge / 0.1f) + 35.0f * NormalizedAge) * 0.35f;

                const float g1 = FMath::Lerp(0.3f, 0.164f, Saturate(2.0f * NormalizedAge));
                const float g  = FMath::Lerp(g1, 0.43f, Saturate(2.0f * NormalizedAge - 1.0f));
                const float A  = 0.57f * SmoothStep(0.0f, 0.122f, NormalizedAge) * (1.0f - SmoothStep(0.122f, 1.0f, NormalizedAge));
                const FVector3f Ember = FVector3f(1.0f, 0.55f, 0.26f) * 8.0f * Saturate(1.0f - NormalizedAge / 0.15f);

                Out.Color = FLinearColor((g + Ember.X) * A, (g + Ember.Y) * A, (1.1f * g + Ember.Z) * A, A);

                Out.VisTag   = 2;
                Out.Rotation = (Rand(InSeed, 10) - 0.5f) * 360.0f + (Rand(InSeed, 11) - 0.5f) * 60.0f * t;
                Out.Dynamic  = FVector4f(Saturate(NormalizedAge - 0.35f) * 1.3f, 0.0f, 0.0f, 0.0f);
                break;
            }
            case 13: // SparksBurst — hemisphere spark streaks with heavy gravity + rare flash pop (burst template, real Age). Mirrors Behavior_SparksBurst.ush.
            {
                const float t = InAge;
                const float k = Rand(InSeed, 0);

                if (k < 0.985f)
                {
                    const float z  = Rand(InSeed, 1);
                    const float a  = 6.28318530718f * Rand(InSeed, 2);
                    const float rr = FMath::Sqrt(Saturate(1.0f - z * z));
                    const FVector3f d(rr * FMath::Cos(a), rr * FMath::Sin(a), z);

                    const float Arc = t / FMath::Lerp(0.5f, 0.7f, Rand(InSeed, 3));

                    Out.Position = d * 2.0f + d * 160.0f * t + FVector3f(0.0f, 0.0f, -1000.0f * t * t);
                    Out.Velocity = d * 160.0f + FVector3f(0.0f, 0.0f, -2000.0f * t);
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(2.0f, 15.0f * FMath::Lerp(1.0f, 0.3f, Saturate(Arc)));

                    const FVector3f c = FMath::Lerp(FVector3f(2.0f, 6.0f, 15.0f), FVector3f(1.0f, 3.0f, 7.0f), Saturate((Arc - 0.1f) / 0.4f));
                    const float a2 = Saturate(1.5f * (1.0f - Saturate((Arc - 0.7f) / 0.3f)));
                    Out.Color = FLinearColor(c.X * a2, c.Y * a2, c.Z * a2, a2);
                }
                else
                {
                    const float s   = FMath::Lerp(10.0f, 20.0f, Rand(InSeed, 4)) * FMath::Lerp(0.1f, 3.0f, Saturate(t / 0.03f));
                    const float Vis = Saturate(1.0f - t / 0.1f);
                    Out.Position = FVector3f(0.0f, 0.0f, 4.0f);
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Size     = FVector2f(s, s);
                    Out.Color    = FLinearColor(2.0f, 10.0f, 20.0f, 1.0f) * (1.5f * Vis);
                }
                break;
            }
            case 14: // GroundRing — Disc carrier ring + spark fountain from the ring radius (burst template, real Age). Mirrors Behavior_GroundRing.ush.
            {
                const float t = InAge;
                const float m = Saturate(t / 1.0f);
                const float k = Rand(InSeed, 0);

                const float a = 6.28318530718f * Rand(InSeed, 1);
                const FVector3f RadDir(FMath::Cos(a), FMath::Sin(a), 0.0f);

                if (k < 0.06f)
                {
                    const float Vis = Saturate(1.0f - t / 0.79f);
                    Out.Position  = FVector3f(0.0f, 0.0f, 3.0f);
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 3;
                    Out.Scale     = FVector3f(1.0f, 1.0f, 1.0f) * (1.78f * (t / 0.83f));
                    Out.Dynamic   = FVector4f(Saturate(t / 0.79f), 0.3f, -0.5f * t, 1.6f);
                    Out.Color     = FLinearColor(12.5f, 0.8f, 0.0f, 1.0f) * Vis;
                }
                else
                {
                    const float u  = Rand(InSeed, 2);
                    const float az = 6.28318530718f * Rand(InSeed, 3);
                    const float th = 0.925f * FMath::Sqrt(u);
                    const FVector3f d(FMath::Sin(th) * FMath::Cos(az), FMath::Sin(th) * FMath::Sin(az), FMath::Cos(th));
                    const float s  = FMath::Lerp(300.0f, 1100.0f, Rand(InSeed, 4));
                    const float kd = (1.0f - FMath::Exp(-0.75f * t)) / 0.75f;

                    Out.Position = RadDir * 178.0f + d * s * kd + FVector3f(0.0f, 0.0f, -490.0f * t * t);
                    Out.Velocity = d * s * FMath::Exp(-0.75f * t) + FVector3f(0.0f, 0.0f, -980.0f * t);
                    Out.VisTag   = 1;

                    const float w = FMath::Lerp(5.0f, 10.0f, Rand(InSeed, 5));
                    Out.Size = FVector2f(w * 0.35f, w * 1.8f) * Saturate(1.0f - m * 0.6f);

                    const float Hdr = FMath::Lerp(10.0f, 20.0f, Rand(InSeed, 6)) * 0.5f;
                    const float Vis = m < 0.52f ? 1.0f : Saturate(1.0f - (m - 0.52f) / 0.26f);
                    Out.Color = FLinearColor(Hdr, Hdr, Hdr, 1.0f) * Vis;
                }
                break;
            }
            case 15: // LightningStrike — strobing jagged bolt column + crackle shell + base dust ring, cycled. Mirrors Behavior_LightningStrike.ush.
            {
                const float Cycle = 0.6f;
                const float t = Frac(InAge / Cycle + Rand(InSeed, 9)) * Cycle;
                const float m = t / Cycle;
                const float k = Rand(InSeed, 0);

                if (k < 0.25f)
                {
                    const float zf   = Rand(InSeed, 1);
                    const float Band = FMath::Floor(zf * 6.0f);
                    const float Jag  = (Rand(InSeed, 4 + int32(Band)) - 0.5f) * (60.0f - 30.0f * zf);
                    const float Flick = Step(0.35f, Frac(InAge * 16.0f + Rand(InSeed, 2)));
                    const float Vis   = Flick * Saturate(1.0f - m);
                    Out.Position = FVector3f(Jag, (Rand(InSeed, 5) - 0.5f) * 18.0f, zf * 380.0f);
                    Out.Velocity = FVector3f(0.0f, 0.0f, 900.0f); // renderer-facing only: velocity-aligns the segment vertically
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(10.0f, 58.0f);
                    Out.Color    = FLinearColor(0.7f, 135.0f, 150.0f, 1.0f) * (0.15f * Vis);
                }
                else if (k < 0.7f)
                {
                    const float z  = 2.0f * Rand(InSeed, 1) - 1.0f;
                    const float a  = 6.28318530718f * Rand(InSeed, 2);
                    const float rr = FMath::Sqrt(Saturate(1.0f - z * z));
                    const FVector3f d(rr * FMath::Cos(a), rr * FMath::Sin(a), FMath::Abs(z));
                    const FVector3f Jit(
                        FMath::Sin(InAge * 31.0f + 6.28318530718f * Rand(InSeed, 3)),
                        FMath::Cos(InAge * 27.0f + 6.28318530718f * Rand(InSeed, 4)),
                        FMath::Sin(InAge * 23.0f + 6.28318530718f * Rand(InSeed, 5)));
                    Out.Position = FVector3f(0.0f, 0.0f, 161.0f) + d * FMath::Lerp(47.0f, 95.0f, Rand(InSeed, 6)) + Jit * 9.0f;
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Size     = FVector2f(12.0f, 12.0f);
                    Out.Color    = FLinearColor(0.14f, 27.0f, 30.0f, 1.0f) * (0.3f * Saturate(1.0f - m));
                }
                else
                {
                    const float a = 6.28318530718f * Rand(InSeed, 1);
                    const FVector3f d(FMath::Cos(a), FMath::Sin(a), 0.0f);
                    const float Vis = 0.35f * Saturate(1.0f - m);
                    Out.Position = d * (100.0f + 27.7f * t) + FVector3f(0.0f, 0.0f, 20.0f);
                    Out.Velocity = d * 27.7f;
                    Out.VisTag   = 2;
                    Out.Rotation = (Rand(InSeed, 3) - 0.5f) * 360.0f;
                    Out.Dynamic  = FVector4f(m * 0.8f, 0.0f, 0.0f, 0.0f);
                    Out.Size     = FVector2f(1.0f, 1.0f) * FMath::Lerp(120.0f, 180.0f, Rand(InSeed, 2));
                    Out.Color    = FLinearColor(0.35f, 0.4f, 0.45f, 1.0f) * Vis;
                }
                break;
            }
            case 16: // AuraSwirl — orbiting torus flame ring + breathing fresnel dome. Mirrors Behavior_AuraSwirl.ush.
            {
                if (Rand(InSeed, 10) < 0.10f)
                {
                    const float Breathe = 1.15f + 0.1f * FMath::Sin(InAge * 2.0f);
                    Out.Position  = FVector3f(0.0f, 0.0f, 70.0f);
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 2;
                    Out.Scale     = FVector3f(Breathe, Breathe, Breathe * 1.25f);
                    Out.Dynamic   = FVector4f(0.5f + 0.2f * FMath::Sin(InAge * 0.9f), 0.4f, -InAge * 0.35f, 1.2f);
                    Out.Color     = FLinearColor(1.0f, 0.35f, 0.08f, 0.5f);
                    break;
                }

                const float ArcLen = FMath::Lerp(0.36f, 0.71f, Rand(InSeed, 1));
                const float t = Frac(InAge / ArcLen + Rand(InSeed, 9)) * ArcLen;
                const float m = t / ArcLen;

                const float th = 6.28318530718f * Rand(InSeed, 2) + 4.25f * InAge;
                const float r  = 68.0f + 16.0f * (2.0f * Rand(InSeed, 3) - 1.0f);
                const float vz = FMath::Lerp(65.0f, 110.0f, Rand(InSeed, 4));
                const float kd = (1.0f - FMath::Exp(-0.25f * t)) / 0.25f;

                const float JitPhase = 6.28318530718f * Rand(InSeed, 6);
                const float Jit      = 17.4f * FMath::Sin(50.0f * m + JitPhase);

                Out.Position = FVector3f(r * FMath::Cos(th) + Jit, r * FMath::Sin(th) + Jit * 0.5f,
                                         16.0f * (2.0f * Rand(InSeed, 5) - 1.0f) + vz * kd);
                Out.Velocity = FVector3f(-r * FMath::Sin(th), r * FMath::Cos(th), 0.0f) * 4.25f * 0.2f + FVector3f(0.0f, 0.0f, vz * FMath::Exp(-0.25f * t));

                const float Bright = FMath::Lerp(0.69f, 20.0f, Rand(InSeed, 7) * Rand(InSeed, 7)) * 0.4f;
                const FVector3f Ramp = FMath::Lerp(FVector3f(1.0f, 0.6f, 0.2f), FVector3f(1.0f, 0.15f, 0.02f), m);
                const float A = FMath::Pow(Saturate(1.0f - m / 0.78f), 2.0f);

                Out.Color = FLinearColor(Ramp.X * Bright * A, Ramp.Y * Bright * A, Ramp.Z * Bright * A, A);
                Out.Size  = FVector2f(1.0f, 1.0f) * FMath::Lerp(22.6f, 57.3f, Rand(InSeed, 8)) * (1.0f + m) * 0.55f;
                break;
            }
            case 17: // LightningRange — Vefects NS_Lightning_Range range ring. Mirrors Behavior_LightningRange.ush.
            {
                constexpr auto ColorSplit  = 0.0630213f;
                constexpr auto AlphaT0     = 0.206364f;
                constexpr auto AlphaT1     = 0.693235f;
                constexpr auto AlphaT2     = 0.95397f;
                constexpr auto DissolveEnd = 0.4f;

                const float T = NormalizedAge;

                Out.Position = FVector3f::ZeroVector;
                Out.Velocity = FVector3f::ZeroVector;

                const float ColorT = Saturate(T / ColorSplit);
                const float R = FMath::Lerp(1.0f,      0.266356f, ColorT);
                const float G = FMath::Lerp(0.89627f,  0.102242f, ColorT);
                const float B = FMath::Lerp(0.520996f, 1.0f,      ColorT);

                float Alpha;
                if (T <= AlphaT0)
                { Alpha = 0.15f; }
                else if (T <= AlphaT1)
                { Alpha = FMath::Lerp(0.15f, 0.2f, (T - AlphaT0) / (AlphaT1 - AlphaT0)); }
                else if (T <= AlphaT2)
                { Alpha = FMath::Lerp(0.2f, 1.0f, (T - AlphaT1) / (AlphaT2 - AlphaT1)); }
                else
                { Alpha = 1.0f; }

                Out.Color       = FLinearColor(R, G, B, Alpha);
                Out.Size        = FVector2f(700.0f, 700.0f);
                Out.Dynamic     = FVector4f(FMath::Lerp(-1.0f, 0.2f, Saturate(T / DissolveEnd)), 0.0f, 0.0f, -0.5f);
                Out.Orientation     = FQuat4f::Identity;
                Out.Rotation        = 0.0f;
                Out.VisTag          = 4;
                Out.SpriteAlignment = FVector3f(0.0f, 1.0f, 0.0f);
                Out.SpriteFacing    = FVector3f(0.0f, 0.0f, 1.0f);
                break;
            }
            case 18: // GunshotProjectile — Vefects NS_Gunshot_Projectile. Mirrors Behavior_GunshotProjectile.ush.
            {
                constexpr auto NumLayers   = 3;
                constexpr auto LayerGlow   = 0;
                constexpr auto LayerBright = 1;

                constexpr auto Life  = 10.0f;
                constexpr auto Drift = 0.01f;

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f(Drift, 0.0f, 0.0f);

                if (InAge > Life)
                {
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Color    = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size     = FVector2f(0.0f, 0.0f);
                    Out.Scale    = FVector3f(0.0f, 0.0f, 0.0f);
                    break;
                }

                if (Layer == LayerGlow)
                {
                    Out.Position = FVector3f(-52.048f, 0.0f, 0.0f);
                    Out.Color    = FLinearColor(1.0f, 0.194618f, 0.021219f, 0.5f);
                    Out.Size     = FVector2f(80.0f, 300.0f);
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 10;
                    break;
                }

                Out.Dynamic = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
                Out.VisTag  = 11;

                if (Layer == LayerBright)
                {
                    Out.Position = FVector3f(-77.6262f, 0.0f, 0.0f);
                    Out.Color    = FLinearColor(1.0f, 0.552046f, 0.147f, 1.0f);
                    Out.Size     = FVector2f(20.0f, 200.0f);
                    break;
                }

                Out.Position = FVector3f(-194.751f, 0.0f, 0.0f);
                Out.Color    = FLinearColor(0.1f, 0.0171441f, 0.00865005f, 0.2f);
                Out.Size     = FVector2f(35.0f, 500.0f);
                break;
            }
            case 19: // ArrowProjectile — Vefects NS_Arrow_Projectile. Mirrors Behavior_ArrowProjectile.ush.
            {
                constexpr auto NumLayers   = 3;
                constexpr auto LayerGlow   = 0;
                constexpr auto LayerBright = 1;

                constexpr auto Life  = 10.0f;
                constexpr auto Drift = 0.01f;

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                if (InAge > Life)
                {
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Color    = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size     = FVector2f(0.0f, 0.0f);
                    Out.Scale    = FVector3f(0.0f, 0.0f, 0.0f);
                    break;
                }

                if (Layer == LayerGlow)
                {
                    Out.Position = FVector3f(-5.0f, 0.0f, 0.0f);
                    Out.Velocity = FVector3f::ZeroVector;
                    Out.Color    = FLinearColor(1.0f, 0.775394f, 0.257f, 0.3f);
                    Out.Size     = FVector2f(120.0f, 120.0f);
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 0;
                    break;
                }

                Out.Velocity = FVector3f(Drift, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
                Out.VisTag   = 11;

                if (Layer == LayerBright)
                {
                    Out.Position = FVector3f(-20.1693f, 0.0f, 0.0f);
                    Out.Color    = FLinearColor(1.0f, 0.558341f, 0.102242f, 0.5f);
                    Out.Size     = FVector2f(20.0f, 50.0f);
                    break;
                }

                Out.Position = FVector3f(-66.0904f, 0.0f, 0.0f);
                Out.Color    = FLinearColor(0.06f, 0.0470123f, 0.0270472f, 0.2f);
                Out.Size     = FVector2f(35.0f, 150.0f);
                break;
            }
            case 20: // FireBurst — Vefects NS_Fire. Mirrors Behavior_FireBurst.ush; recipe Cookbook/NS_Fire.md.
            {
                constexpr auto NumLayers    = 10;
                constexpr auto LayerGlow01  = 0;
                constexpr auto FirstFlame   = 2;
                constexpr auto FirstSparkle = 5;

                constexpr auto Delay       = 0.05f;
                constexpr auto LifeGlow01  = 0.25f;
                constexpr auto LifeGlow02  = 0.2f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto HemiDir = [](int32 InS) -> FVector3f
                {
                    const auto D = RandDir(InS);
                    return FVector3f(D.X, D.Y, FMath::Abs(D.Z));
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;

                if (Layer < FirstFlame)
                {
                    const auto Life = Layer == LayerGlow01 ? LifeGlow01 : LifeGlow02;
                    const auto td   = InAge - Delay;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(td / Life);
                    const auto Alpha = 0.5f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f);
                    const auto Grow  = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.VisTag = 12;

                    if (Layer == LayerGlow01)
                    {
                        Out.Color   = FLinearColor(1.0f, 0.0908417f, 0.043735f, Alpha);
                        Out.Size    = FVector2f(500.0f * Grow, 500.0f * Grow);
                        Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                        break;
                    }

                    Out.Color   = FLinearColor(1.0f, 0.496933f, 0.043735f, Alpha);
                    Out.Size    = FVector2f(400.0f * Grow, 400.0f * Grow);
                    Out.Dynamic = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
                    break;
                }

                if (Layer < FirstSparkle)
                {
                    const auto Life = FMath::Lerp(0.2f, 0.7f, Rand(InSeed, 1));
                    const auto td   = InAge - Delay;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = HemiDir(InSeed);

                    const auto Spawn = Dir * 20.0f;
                    const auto V0    = Dir * FMath::Lerp(100.0f, 250.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int2(t, 1.0f, 0.2f) * Life);
                    Out.Velocity = V0 * Key2(t, 0.0f, 1.0f, 1.0f, 0.2f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0796861f, 5.0f,      0.368246f, 3.0f,       0.738907f, 0.250158f),
                        Key3(t, 0.0796861f, 3.43343f,  0.368246f, 0.67227f,   0.738907f, 0.00749903f),
                        Key3(t, 0.0796861f, 0.115767f, 0.368246f, 0.0841786f, 0.738907f, 0.00749903f),
                        Key3(t, 0.0f,       0.0f,      0.303049f, 1.0f,       0.992454f, 0.0f));

                    const auto Base = FMath::Lerp(50.0f, 200.0f, Rand(InSeed, 3));
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f + FMath::Lerp(-30.0f, 30.0f, Rand(InSeed, 5)) * td;

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 5.0f, 0.0f, 0.0f);
                    Out.VisTag  = 13;
                    break;
                }

                const auto LoopIndex   = InSeed / NumLayers;
                const auto NumThisLoop = 3 + static_cast<int32>(Rand(LoopIndex, 11) * 3.0f);
                const auto Slot        = Layer - FirstSparkle;

                if (Slot >= NumThisLoop)
                {
                    Hide();
                    break;
                }

                const auto SparkleLife = FMath::Lerp(0.3f, 1.0f, Rand(InSeed, 1));

                if (InAge > SparkleLife)
                {
                    Hide();
                    break;
                }

                const auto t   = Saturate(InAge / SparkleLife);
                const auto Dir = HemiDir(InSeed);

                const auto Spawn = Dir * (2.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                const auto V0    = Dir * FMath::Lerp(300.0f, 1000.0f, Rand(InSeed, 2));

                Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.35f, 0.05f) * SparkleLife);
                Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.35f, 1.0f, 0.05f);

                Out.Color = FLinearColor(
                    1.0f,
                    Key3(t, 0.0f, 0.703584f, 0.327196f, 0.288367f,  0.779958f, 0.0419999f),
                    Key3(t, 0.0f, 0.031f,    0.327196f, 0.0369999f, 0.779958f, 0.0642406f),
                    1.0f);

                const auto Width  = FMath::Lerp(15.0f, 30.0f, Rand(InSeed, 8));
                const auto Length = FMath::Lerp(40.0f, 50.0f, Rand(InSeed, 9));
                const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                const auto Taper  = Key2(t, 0.1f, 1.0f, 1.0f, 0.4f);
                Out.Size = FVector2f(Width * Grow, Length * Grow * Taper);

                Out.Dynamic = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
                Out.VisTag  = 14;
                break;
            }
            case 21: // FireBallHit — Vefects NS_FireBall_Hit. Mirrors Behavior_FireBallHit.ush; recipe Cookbook/NS_FireBall_Hit.md.
            {
                constexpr auto NumLayers      = 47;
                constexpr auto LayerRainbow   = 0;
                constexpr auto LayerRing      = 1;
                constexpr auto LayerGlow01    = 9;
                constexpr auto LayerStar      = 15;
                constexpr auto LayerGlow02    = 16;
                constexpr auto LayerSecond    = 17;
                constexpr auto FirstFlame     = 21;
                constexpr auto FirstSmoke     = 26;
                constexpr auto FirstFlare     = 31;
                constexpr auto FirstFlash     = 33;
                constexpr auto LayerFirstGlow = 37;
                constexpr auto LayerFlareImp  = 38;
                constexpr auto FirstSpike02   = 44;

                constexpr auto DelayEarly = 0.04f;
                constexpr auto DelayLate  = 0.05f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto Spike = [&](FVector3f InScaleMin, FVector3f InScaleMax) -> void
                {
                    const auto Life = FMath::Lerp(0.1f, 0.15f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        return;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto V0 = Dir * 10.0f;
                    Out.Position = Dir * 1.0f + V0 * td;
                    Out.Velocity = V0;

                    Out.Orientation = QuatFromZTo(Dir);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(InScaleMin.X, InScaleMax.X, Rand(InSeed, 3)),
                        FMath::Lerp(InScaleMin.Y, InScaleMax.Y, Rand(InSeed, 4)),
                        FMath::Lerp(InScaleMin.Z, InScaleMax.Z, Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.5f, 1.0f, 0.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.4f, 1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 0.2f, 1.0f));

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.118322f, 1.0f,      0.515545f, 1.0f,      0.671295f, 0.391573f, 0.843948f, 0.009134f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.693872f, 0.515545f, 0.040915f, 0.671295f, 0.003677f, 0.843948f, 0.004025f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.147027f, 0.515545f, 0.045186f, 0.671295f, 0.022174f, 0.843948f, 0.006995f),
                        1.0f);

                    Out.VisTag = 26;
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == LayerRainbow)
                {
                    if (InAge > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / 0.2f);
                    const auto Grow  = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);
                    const auto Alpha = 0.2f * Key3(t, 0.0f, 0.0f, 0.328403f, 1.0f, 1.0f, 0.0f);

                    Out.Color    = FLinearColor(0.913099f * 0.5f, 0.913099f * 0.5f, 0.913099f * 0.5f, Alpha);
                    Out.Size     = FVector2f(250.0f * Grow, 250.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 20;
                    break;
                }

                if (Layer == LayerRing)
                {
                    if (InAge > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.25f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.118322f, 1.0f,      0.295804f, 1.0f,      0.542107f, 0.391573f, 0.843948f, 0.009134f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.693872f, 0.295804f, 0.040915f, 0.542107f, 0.003677f, 0.843948f, 0.004025f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.147027f, 0.295804f, 0.045186f, 0.542107f, 0.022174f, 0.843948f, 0.006995f),
                        0.5f);
                    Out.Size     = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 19;
                    break;
                }

                if (Layer < LayerGlow01)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (20.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(200.0f, 1200.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(1.0f, 0.563224f, 0.224f, Key2(t, 0.613341f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(10.0f, 20.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 16;
                    break;
                }

                if (Layer == LayerGlow01)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0881376f, 1.0f,      0.377905f, 1.0f,       0.9345f, 0.391573f),
                        Key3(t, 0.0881376f, 0.693872f, 0.377905f, 0.0409152f, 0.9345f, 0.00367651f),
                        Key3(t, 0.0881376f, 0.147027f, 0.377905f, 0.0451862f, 0.9345f, 0.0221739f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.6f);
                    Out.Size    = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 15;
                    break;
                }

                if (Layer < LayerStar)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.5f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (10.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(800.0f, 2000.0f, Rand(InSeed, 2));

                    const auto VelScale = Key3(t, 0.0f, 1.0f, 0.2f, 0.2f, 1.0f, 0.0f);
                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.2f, 0.0f) * Life);
                    Out.Velocity = V0 * VelScale;

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f, 0.088138f, 1.0f,      0.615756f, 1.0f,       0.98038f, 0.391573f),
                        Key4(t, 0.0f, 1.0f, 0.088138f, 0.671611f, 0.615756f, 0.025f,     0.98038f, 0.003677f),
                        Key4(t, 0.0f, 1.0f, 0.088138f, 0.085f,    0.615756f, 0.0293413f, 0.98038f, 0.022174f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.6f);

                    const auto Width  = FMath::Lerp(25.0f, 40.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(70.0f, 60.0f, Rand(InSeed, 9));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.3f, 0.35f, 1.0f, 0.2f);

                    const auto SpeedFactor = FMath::Lerp(1.0f, 2.0f, Saturate(Out.Velocity.Size() / 1000.0f));

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper * SpeedFactor);
                    Out.VisTag = 18;
                    break;
                }

                if (Layer == LayerStar)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f, 0.115907f, 1.0f,      0.636281f, 1.0f,      0.9345f, 0.391573f),
                        Key4(t, 0.0f, 1.0f, 0.115907f, 0.693872f, 0.636281f, 0.040915f, 0.9345f, 0.003677f),
                        Key4(t, 0.0f, 1.0f, 0.115907f, 0.147027f, 0.636281f, 0.045186f, 0.9345f, 0.022174f),
                        1.0f);
                    Out.Size     = FVector2f(20.0f * Grow, 20.0f * Grow);
                    Out.Rotation = 0.1f;
                    Out.VisTag   = 21;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.214911f, 0.693872f, 0.817386f, 0.0409152f),
                        Key2(t, 0.214911f, 0.147027f, 0.817386f, 0.0451862f),
                        Key2(t, 0.406882f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(100.0f * Grow, 100.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 15;
                    break;
                }

                if (Layer == LayerSecond)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t       = Saturate(InAge / 0.1f);
                    const auto Uniform = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);
                    const auto ScaleX  = Key2(t, 0.0f, 1.0f, 1.0f, 0.4f);
                    const auto ScaleY  = Key2(t, 0.0f, 1.0f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.618171f, 2.0f,      0.963477f, 3.0f),
                        Key2(t, 0.618171f, 0.683829f, 0.963477f, 2.25883f),
                        Key2(t, 0.618171f, 0.218923f, 0.963477f, 0.328385f),
                        Key2(t, 0.0f, 0.0f, 0.246302f, 1.0f));
                    Out.Size    = FVector2f(170.0f * Uniform * ScaleX, 170.0f * Uniform * ScaleY);
                    Out.Dynamic = FVector4f(3.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 17;
                    break;
                }

                if (Layer < FirstFlame)
                {
                    Spike(FVector3f(0.2f, 0.2f, 0.4f), FVector3f(0.2f, 0.2f, 0.7f));
                    break;
                }

                if (Layer < FirstSmoke)
                {
                    const auto Life = FMath::Lerp(0.2f, 0.7f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(InAge / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * 20.0f;
                    const auto V0    = Dir * FMath::Lerp(50.0f, 300.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int2(t, 1.0f, 0.2f) * Life);
                    Out.Velocity = V0 * Key2(t, 0.0f, 1.0f, 1.0f, 0.2f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0796861f, 5.0f,      0.368246f, 3.0f,       0.738907f, 0.250158f),
                        Key3(t, 0.0796861f, 3.43343f,  0.368246f, 0.67227f,   0.738907f, 0.00749903f),
                        Key3(t, 0.0796861f, 0.115767f, 0.368246f, 0.0841786f, 0.738907f, 0.00749903f),
                        Key3(t, 0.0f,       0.0f,      0.303049f, 1.0f,       0.992454f, 0.0f));

                    const auto Base = FMath::Lerp(50.0f, 100.0f, Rand(InSeed, 3));
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f
                                 + FMath::Lerp(-30.0f, 30.0f, Rand(InSeed, 5)) * InAge;

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 5.0f, 0.0f, 0.0f);
                    Out.VisTag  = 24;
                    break;
                }

                if (Layer < FirstFlare)
                {
                    const auto Life = FMath::Lerp(0.7f, 1.3f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayEarly;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto Raw    = RandDir(InSeed);
                    const auto Planar = FVector2f(Raw.X, Raw.Y);
                    const auto Len    = Planar.Size();
                    const auto Dir    = Len > 0.000001f
                        ? FVector3f(Planar.X / Len, Planar.Y / Len, 0.0f)
                        : FVector3f(1.0f, 0.0f, 0.0f);

                    const auto Spawn = Dir * 20.0f;
                    const auto V0    = Dir * FMath::Lerp(50.0f, 200.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.2f, 0.1f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.2f, 1.0f, 0.1f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.0603682f, 1.0f,      0.22457f, 1.0f,      0.363417f, 0.391573f, 0.527618f, 0.0f),
                        Key5(t, 0.0f, 1.0f, 0.0603682f, 0.693872f, 0.22457f, 0.040915f, 0.363417f, 0.003677f, 0.527618f, 0.0f),
                        Key5(t, 0.0f, 1.0f, 0.0603682f, 0.147027f, 0.22457f, 0.045186f, 0.363417f, 0.022174f, 0.527618f, 0.0f),
                        Key2(t, 0.126773f, 1.0f, 0.514337f, 0.35f) * 0.3f);

                    const auto Base = FMath::Lerp(100.0f, 200.0f, Rand(InSeed, 3));
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f
                                 + FMath::Lerp(-30.0f, 30.0f, Rand(InSeed, 5)) * td;

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f,
                                            Key2(t, 0.0f, -1.0f, 0.25f, 1.0f));
                    Out.VisTag  = 25;
                    break;
                }

                if (Layer < FirstFlash)
                {
                    if (InAge > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 2.0f, 0.142469f, 2.0f,      0.47208f, 1.0f,      0.9345f, 0.391573f),
                        Key4(t, 0.0f, 2.0f, 0.142469f, 1.38774f,  0.47208f, 0.040915f, 0.9345f, 0.003677f),
                        Key4(t, 0.0f, 2.0f, 0.142469f, 0.294054f, 0.47208f, 0.045186f, 0.9345f, 0.022174f),
                        Key3(t, 0.0f, 0.0f, 0.214911f, 1.0f, 1.0f, 0.0f) * 0.6f);
                    Out.Size     = FVector2f(120.0f * Grow, 120.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 22;
                    break;
                }

                if (Layer < LayerFirstGlow)
                {
                    const auto td = InAge - DelayEarly;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0374283f, 0.715694f, 0.16058f, 1.0f,      0.312708f, 1.0f,      0.659221f, 0.913099f,  0.96227f, 0.0137021f),
                        Key5(t, 0.0374283f, 0.89627f,  0.16058f, 0.752942f, 0.312708f, 0.341915f, 0.659221f, 0.0241576f, 0.96227f, 0.00182116f),
                        Key5(t, 0.0374283f, 1.0f,      0.16058f, 0.109462f, 0.312708f, 0.109462f, 0.659221f, 0.0241576f, 0.96227f, 0.00802319f),
                        1.0f);
                    Out.Size    = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 17;
                    break;
                }

                if (Layer == LayerFirstGlow)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t       = Saturate(td / 0.2f);
                    const auto Uniform = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto ScaleX  = Key2(t, 0.0f, 1.0f, 1.0f, 0.4f);
                    const auto ScaleY  = Key2(t, 0.0f, 1.0f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f,      0.111078f, 1.0f,      0.322366f, 1.0f,      0.96227f, 0.913099f),
                        Key4(t, 0.0f, 0.938686f, 0.111078f, 0.752942f, 0.322366f, 0.341915f, 0.96227f, 0.0241576f),
                        Key4(t, 0.0f, 0.791298f, 0.111078f, 0.109462f, 0.322366f, 0.109462f, 0.96227f, 0.0241576f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.6f);
                    Out.Size    = FVector2f(1000.0f * Uniform * ScaleX, 1000.0f * Uniform * ScaleY);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 15;
                    break;
                }

                if (Layer == LayerFlareImp)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.05f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.131603f, 1.0f,      0.601268f, 1.0f,      0.828252f, 0.391573f, 0.997283f, 0.009134f),
                        Key5(t, 0.0f, 1.0f, 0.131603f, 0.693872f, 0.601268f, 0.040915f, 0.828252f, 0.003677f, 0.997283f, 0.004025f),
                        Key5(t, 0.0f, 1.0f, 0.131603f, 0.147027f, 0.601268f, 0.045186f, 0.828252f, 0.022174f, 0.997283f, 0.006995f),
                        1.0f);
                    Out.Size    = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.5f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 23;
                    break;
                }

                if (Layer < FirstSpike02)
                {
                    if (InAge > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 0.2f);

                    Out.Orientation = QuatFromZTo(RandDir(InSeed));
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(0.3f, 0.7f, Rand(InSeed, 3)),
                        FMath::Lerp(0.3f, 0.7f, Rand(InSeed, 4)),
                        FMath::Lerp(1.5f, 3.0f, Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.5f, 0.2f, 1.0f,  1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 1.0f, 1.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.75f, 1.0f, 1.0f));

                    Out.Color  = FLinearColor(1.0f, 0.366253f, 0.184475f,
                                              Key3(t, 0.0f, 0.0f, 0.274072f, 1.0f, 1.0f, 0.0f) * 0.3f);
                    Out.VisTag = 27;
                    break;
                }

                Spike(FVector3f(0.3f, 0.3f, 0.6f), FVector3f(0.3f, 0.3f, 1.0f));
                break;
            }
            case 22: // GunshotHit — Vefects NS_Gunshot_Hit. Mirrors Behavior_GunshotHit.ush; recipe Cookbook/NS_Gunshot_Hit.md.
            {
                constexpr auto NumLayers     = 40;
                constexpr auto LayerGlow01   = 0;
                constexpr auto LayerGlow02   = 1;
                constexpr auto FirstSpark02  = 7;
                constexpr auto FirstGlow04   = 12;
                constexpr auto FirstGlow05   = 17;
                constexpr auto LayerStar     = 20;
                constexpr auto LayerFlare    = 27;
                constexpr auto FirstSpark01  = 33;

                constexpr auto DelayEarly = 0.04f;
                constexpr auto DelayLate  = 0.05f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto CurveS = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        Key5(InT, 0.0f, 0.715694f, 0.094174f, 1.0f,      0.21974f, 1.0f,      0.440688f, 0.854993f, 0.707516f, 0.01033f),
                        Key5(InT, 0.0f, 0.89627f,  0.094174f, 0.752942f, 0.21974f, 0.341915f, 0.440688f, 0.135633f, 0.707516f, 0.007499f),
                        Key5(InT, 0.0f, 1.0f,      0.094174f, 0.109462f, 0.21974f, 0.109462f, 0.440688f, 0.051269f, 0.707516f, 0.006049f));
                };

                const auto CurveA = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        1.0f,
                        Key3(InT, 0.0f, 1.0f, 0.46725f, 0.693872f, 1.0f, 0.450786f),
                        Key3(InT, 0.0f, 1.0f, 0.46725f, 0.147027f, 1.0f, 0.040915f));
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == LayerGlow01)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 1.0f, 0.415334f, 0.693872f, 0.810142f, 0.0409152f),
                        Key3(t, 0.0f, 1.0f, 0.415334f, 0.147027f, 0.810142f, 0.0451862f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.3f);
                    Out.Size    = FVector2f(700.0f * Grow, 700.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 28;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color  = FLinearColor(1.0f, 0.850349f, 0.329f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size   = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.VisTag = 28;
                    break;
                }

                if (Layer < FirstSpark02)
                {
                    const auto td = InAge - DelayEarly;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto Rgb = CurveS(Saturate(td / 0.05f));

                    Out.Color  = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 4.0f);
                    Out.Size   = FVector2f(150.0f, 150.0f);
                    Out.VisTag = 29;
                    break;
                }

                if (Layer < FirstGlow04)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (0.1f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(1300.0f, 2000.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.2f, 0.05f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.2f, 1.0f, 0.05f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f,      0.297012f, 1.0f,       0.671295f, 1.0f,       0.910353f, 0.381326f),
                        Key4(t, 0.0f, 0.913099f, 0.297012f, 0.493097f,  0.671295f, 0.24027f,   0.910353f, 0.042927f),
                        Key4(t, 0.0f, 0.584079f, 0.297012f, 0.0409999f, 0.671295f, 0.0839999f, 0.910353f, 0.0366073f),
                        Key2(t, 0.243888f, 1.0f, 1.0f, 0.0f));

                    const auto Width  = FMath::Lerp(20.0f, 25.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(130.0f, 150.0f, Rand(InSeed, 9));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.3f, 0.35f, 1.0f, 0.2f);
                    Out.Size = FVector2f(Width * Grow, Length * Grow * Taper);

                    Out.VisTag = 30;
                    break;
                }

                if (Layer < FirstGlow05)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.349312f,  0.414126f, 0.494059f, 0.926049f, 0.450786f),
                        Key3(t, 0.0f, 0.0970486f, 0.414126f, 0.124008f, 0.926049f, 0.0409152f),
                        Key2(t, 0.0f, 1.0f, 0.992454f, 0.0f) * 0.2f);
                    Out.Size   = FVector2f(800.0f * Grow, 800.0f * Grow);
                    Out.VisTag = 28;
                    break;
                }

                if (Layer < LayerStar)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(3.0f, 1.49184f, 0.509197f, Key2(t, 0.312708f, 1.0f, 0.992454f, 0.0f) * 0.8f);
                    Out.Size    = FVector2f(250.0f * Grow, 250.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 31;
                    break;
                }

                if (Layer == LayerStar)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Rgb  = CurveA(t);

                    Out.Color   = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 1.0f);
                    Out.Size    = FVector2f(20.0f * Grow, 20.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 32;
                    break;
                }

                if (Layer < LayerFlare)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / 0.2f);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (10.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * 10.0f;

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 0.2f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 3.57847f, 0.094174f, 3.0f,      0.315122f, 1.0f,      0.447932f, 0.854993f, 0.606097f, 0.01033f),
                        Key5(t, 0.0f, 4.48135f, 0.094174f, 2.25883f,  0.315122f, 0.341915f, 0.447932f, 0.135633f, 0.606097f, 0.007499f),
                        Key5(t, 0.0f, 5.0f,     0.094174f, 0.328386f, 0.315122f, 0.109462f, 0.447932f, 0.051269f, 0.606097f, 0.006049f),
                        1.0f);

                    const auto Width  = FMath::Lerp(60.0f, 80.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(110.0f, 160.0f, Rand(InSeed, 9));
                    Out.Size = FVector2f(Width * Key3(t, 0.0f, 0.5f, 0.2f, 1.0f, 1.0f, 0.4f), Length);

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 33;
                    break;
                }

                if (Layer == LayerFlare)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.05f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Rgb  = CurveS(t);

                    Out.Color   = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 4.0f);
                    Out.Size    = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.5f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 34;
                    break;
                }

                if (Layer < FirstSpark01)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / 0.1f);
                    const auto Dir = RandDir(InSeed);

                    const auto V0 = Dir * 10.0f;
                    Out.Position = Dir * 20.0f + V0 * td;
                    Out.Velocity = V0;

                    Out.Orientation = QuatFromZTo(Dir);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(0.1f, 0.05f, Rand(InSeed, 3)),
                        FMath::Lerp(0.1f, 0.2f,  Rand(InSeed, 4)),
                        FMath::Lerp(0.2f, 0.5f,  Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.0f, 0.2f, 1.5f, 1.0f, 0.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 1.5f, 1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 0.2f, 1.5f));

                    const auto Rgb = CurveS(t);
                    Out.Color  = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 4.0f);
                    Out.VisTag = 35;
                    break;
                }

                const auto SparkLife = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));
                const auto SparkTd   = InAge - DelayLate;

                if (SparkTd < 0.0f || SparkTd > SparkLife)
                {
                    Hide();
                    break;
                }

                const auto s   = Saturate(SparkTd / SparkLife);
                const auto Dir = RandDir(InSeed);

                const auto Spawn = Dir * (0.1f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                const auto V0    = Dir * FMath::Lerp(800.0f, 1700.0f, Rand(InSeed, 2));

                Out.Position = Spawn + V0 * (Int3(s, 0.1f, 1.0f, 0.15f, 0.0f) * SparkLife);
                Out.Velocity = V0 * Key3(s, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                Out.Color = FLinearColor(
                    Key2(s, 0.562632f, 1.0f,      0.997283f, 0.112f),
                    Key2(s, 0.562632f, 0.603828f, 0.997283f, 0.0676287f),
                    Key2(s, 0.562632f, 0.296138f, 0.997283f, 0.0331675f),
                    Key2(s, 0.562632f, 1.0f, 1.0f, 0.0f) * 0.15f);

                const auto Base = FMath::Lerp(6.0f, 10.0f, Rand(InSeed, 8));
                const auto Grow = Key3(s, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                Out.Size = FVector2f(Base * Grow, Base * Grow);

                Out.Dynamic = FVector4f(3.0f, 0.0f, 0.0f, 0.0f);
                Out.VisTag  = 36;
                break;
            }
            case 23: // ArrowCast — Vefects NS_Arrow_Cast. Mirrors Behavior_ArrowCast.ush; recipe Cookbook/NS_Arrow_Cast.md.
            {
                constexpr auto NumLayers    = 42;
                constexpr auto LayerGlow01  = 0;
                constexpr auto LayerGlow02  = 1;
                constexpr auto LayerRainbow = 7;
                constexpr auto LayerRing    = 13;
                constexpr auto FirstGlow05  = 19;
                constexpr auto LayerFlareIm = 22;
                constexpr auto FirstStrip   = 28;
                constexpr auto LayerStar01  = 33;
                constexpr auto LayerStar02  = 34;
                constexpr auto LayerWindMsh = 35;

                constexpr auto DelayEarly  = 0.04f;
                constexpr auto DelayLate   = 0.05f;
                constexpr auto DelayStar02 = 0.1f;

                constexpr auto Tau = 6.28318530718f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto CurveA = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        1.0f,
                        Key3(InT, 0.0f, 1.0f, 0.46725f, 0.693872f, 1.0f, 0.450786f),
                        Key3(InT, 0.0f, 1.0f, 0.46725f, 0.147027f, 1.0f, 0.040915f));
                };

                const auto WindAlpha = [](float InT) -> float
                {
                    return Key4(InT, 0.0f, 0.0f, 0.240266f, 1.0f, 0.676124f, 1.0f, 1.0f, 0.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == LayerGlow01)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 1.0f, 0.447932f, 0.693872f, 1.0f, 0.450786f),
                        Key3(t, 0.0f, 1.0f, 0.447932f, 0.147027f, 1.0f, 0.0409152f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.3f);
                    Out.Size    = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 37;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color  = FLinearColor(1.0f, 0.899192f, 0.548f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size   = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.VisTag = 37;
                    break;
                }

                if (Layer < LayerRainbow)
                {
                    const auto td = InAge - DelayEarly;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.05f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.450786f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0409152f),
                        1.0f);
                    Out.Size   = FVector2f(150.0f, 150.0f);
                    Out.VisTag = 38;
                    break;
                }

                if (Layer == LayerRainbow)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.5f, 0.5f, 0.5f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(200.0f * Grow, 200.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 39;
                    break;
                }

                if (Layer < LayerRing)
                {
                    const auto Life = FMath::Lerp(0.2f, 0.4f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (0.1f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(1300.0f, 2000.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.35f, 0.05f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.35f, 1.0f, 0.05f);

                    const auto Tint = CurveA(t);
                    Out.Color = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);

                    const auto Width  = FMath::Lerp(35.0f, 50.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(80.0f, 90.0f, Rand(InSeed, 9));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key2(t, 0.0f, 1.0f, 1.0f, 0.6f);

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper);
                    Out.VisTag = 40;
                    break;
                }

                if (Layer == LayerRing)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Tint = CurveA(t);

                    Out.Color    = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size     = FVector2f(60.0f * Grow, 60.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, -0.325f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 41;
                    break;
                }

                if (Layer < FirstGlow05)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.315122f, 0.664854f, 1.0f, 0.450786f),
                        Key2(t, 0.315122f, 0.254571f, 1.0f, 0.0409152f),
                        Key2(t, 0.0f, 1.0f, 0.992454f, 0.0f) * 0.3f);
                    Out.Size   = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    Out.VisTag = 37;
                    break;
                }

                if (Layer < LayerFlareIm)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.315122f, 0.637597f, 1.0f, 0.450786f),
                        Key2(t, 0.315122f, 0.152926f, 1.0f, 0.0409152f),
                        Key2(t, 0.312708f, 1.0f, 0.992454f, 0.0f));
                    Out.Size   = FVector2f(50.0f * Grow, 50.0f * Grow);
                    Out.VisTag = 42;
                    break;
                }

                if (Layer == LayerFlareIm)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.05f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Tint = CurveA(t);

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size    = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.5f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 43;
                    break;
                }

                if (Layer < FirstStrip)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / 0.1f);
                    const auto Dir = RandDir(InSeed);

                    const auto V0 = Dir * 10.0f;
                    Out.Position = Dir * 20.0f + V0 * td;
                    Out.Velocity = V0;

                    Out.Orientation = QuatFromZTo(Dir);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(0.1f, 0.3f, Rand(InSeed, 3)),
                        FMath::Lerp(0.1f, 0.3f, Rand(InSeed, 4)),
                        FMath::Lerp(0.1f, 0.3f, Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.5f, 1.0f, 0.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.4f, 1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 0.2f, 1.0f));

                    const auto Tint = CurveA(t);
                    Out.Color  = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.VisTag = 44;
                    break;
                }

                if (Layer < LayerStar01)
                {
                    const auto Life = FMath::Lerp(0.1f, 0.2f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    Out.Orientation = QuatFromZTo(RandDir(InSeed));
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(0.5f, 1.5f, Rand(InSeed, 3)),
                        FMath::Lerp(0.5f, 1.5f, Rand(InSeed, 4)),
                        FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.5f, 0.2f, 1.0f,  1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 1.0f, 1.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.75f, 1.0f, 1.0f));

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.913099f, 0.319952f, 0.693872f, 1.0f, 0.450786f),
                        Key3(t, 0.0f, 0.715694f, 0.319952f, 0.147027f, 1.0f, 0.040915f),
                        Key2(t, 0.327196f, 1.0f, 1.0f, 0.0f) * 0.4f);
                    Out.VisTag = 45;
                    break;
                }

                if (Layer == LayerStar01)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Tint = CurveA(t);

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size    = FVector2f(20.0f * Grow, 20.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 46;
                    break;
                }

                if (Layer == LayerStar02)
                {
                    const auto td = InAge - DelayStar02;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Tint = CurveA(t);

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size    = FVector2f(70.0f * Grow, 70.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, -0.125f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 47;
                    break;
                }

                if (Layer == LayerWindMsh)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(-200.0f, 0.0f, 0.0f);
                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Spin = QuatFromAxisAngle(FVector3f(1.0f, 0.0f, 0.0f), Tau * 0.3f * td);
                    const auto Lay  = QuatFromAxisAngle(FVector3f(0.0f, 1.0f, 0.0f), Tau * 0.25f);
                    Out.Orientation = QuatMul(Spin, Lay);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Uniform = 0.3f;
                    Out.Scale = FVector3f(
                        Uniform * Key2(t, 0.0f, 1.5f, 0.2f, 2.0f),
                        Uniform * Key2(t, 0.0f, 1.5f, 0.2f, 2.0f),
                        Uniform * 5.0f * Key4(t, 0.0f, 0.5f, 0.4f, 3.0f, 0.7f, 4.75f, 1.0f, 5.0f));

                    Out.Color   = FLinearColor(0.0451862f, 0.0413334f, 0.0363297f, WindAlpha(t) * 0.5f);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, -0.2f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 48;
                    break;
                }

                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(
                        FMath::Lerp(-100.0f, -700.0f, Rand(InSeed, 2)),
                        FMath::Lerp( -20.0f,   20.0f, Rand(InSeed, 3)),
                        FMath::Lerp( -20.0f,   20.0f, Rand(InSeed, 5)));

                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Base = FMath::Lerp(130.0f, 230.0f, Rand(InSeed, 8));
                    const auto Grow = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Color    = FLinearColor(0.5f, 0.447431f, 0.375f, WindAlpha(t) * 0.3f);

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 49;
                }
                break;
            }
            case 24: // ArrowHit — Vefects NS_Arrow_Hit. Mirrors Behavior_ArrowHit.ush; recipe Cookbook/NS_Arrow_Hit.md.
            {
                constexpr auto NumLayers    = 34;
                constexpr auto LayerGlow01  = 0;
                constexpr auto LayerGlow02  = 1;
                constexpr auto LayerRainbow = 7;
                constexpr auto LayerRing01  = 13;
                constexpr auto LayerGlow05  = 19;
                constexpr auto LayerFlareIm = 20;
                constexpr auto FirstStrip   = 26;
                constexpr auto LayerStar01  = 31;
                constexpr auto LayerStar02  = 32;

                constexpr auto DelayEarly  = 0.04f;
                constexpr auto DelayLate   = 0.05f;
                constexpr auto DelayStar02 = 0.1f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto CurveA = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        1.0f,
                        Key3(InT, 0.0f, 1.0f, 0.46725f, 0.693872f, 1.0f, 0.450786f),
                        Key3(InT, 0.0f, 1.0f, 0.46725f, 0.147027f, 1.0f, 0.040915f));
                };

                const auto Ring = [&](float InT) -> void
                {
                    const auto Grow = Key3(InT, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Tint = CurveA(InT);

                    Out.Color    = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size     = FVector2f(60.0f * Grow, 60.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(InT, 0.0f, -0.325f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == LayerGlow01)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.947307f, 0.447932f, 0.693872f, 1.0f, 0.450786f),
                        Key3(t, 0.0f, 0.665387f, 0.447932f, 0.147027f, 1.0f, 0.0409152f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.3f);
                    Out.Size    = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 50;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0f, 0.318547f, 0.0965892f, 1.0f, 1.0f, 1.0f),
                        Key3(t, 0.0f, 0.955974f, 0.0965892f, 1.0f, 1.0f, 0.863157f),
                        Key3(t, 0.0f, 1.0f,      0.0965892f, 1.0f, 1.0f, 0.38643f),
                        Key2(t, 0.143676f, 1.0f, 1.0f, 0.0f));
                    Out.Size   = FVector2f(200.0f * Grow, 200.0f * Grow);
                    Out.VisTag = 50;
                    break;
                }

                if (Layer < LayerRainbow)
                {
                    const auto td = InAge - DelayEarly;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.05f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.450786f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0409152f),
                        1.0f);
                    Out.Size   = FVector2f(150.0f, 150.0f);
                    Out.VisTag = 51;
                    break;
                }

                if (Layer == LayerRainbow)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.5f, 0.5f, 0.5f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(200.0f * Grow, 200.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 52;
                    break;
                }

                if (Layer < LayerRing01)
                {
                    const auto Life = FMath::Lerp(0.2f, 0.4f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (0.1f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(1300.0f, 2000.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.35f, 0.05f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.35f, 1.0f, 0.05f);

                    const auto Tint = CurveA(t);
                    Out.Color = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);

                    const auto Width  = FMath::Lerp(35.0f, 50.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(80.0f, 90.0f, Rand(InSeed, 9));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key2(t, 0.0f, 1.0f, 1.0f, 0.6f);

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper);
                    Out.VisTag = 53;
                    break;
                }

                if (Layer == LayerRing01)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    Ring(Saturate(td / 0.5f));
                    Out.VisTag = 54;
                    break;
                }

                if (Layer < LayerGlow05)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.315122f, 0.664854f, 1.0f, 0.450786f),
                        Key2(t, 0.315122f, 0.254571f, 1.0f, 0.0409152f),
                        Key2(t, 0.0f, 1.0f, 0.992454f, 0.0f) * 0.2f);
                    Out.Size   = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    Out.VisTag = 50;
                    break;
                }

                if (Layer == LayerGlow05)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.07f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.07f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.315122f, 0.80876f, 1.0f, 0.450786f),
                        Key2(t, 0.315122f, 0.553f,   1.0f, 0.0409152f),
                        Key2(t, 0.312708f, 1.0f, 0.992454f, 0.0f));
                    Out.Size   = FVector2f(50.0f * Grow, 50.0f * Grow);
                    Out.VisTag = 55;
                    break;
                }

                if (Layer == LayerFlareIm)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.05f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Tint = CurveA(t);

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size    = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.5f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 56;
                    break;
                }

                if (Layer < FirstStrip)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / 0.1f);
                    const auto Dir = RandDir(InSeed);

                    const auto V0 = Dir * 10.0f;
                    Out.Position = Dir * 20.0f + V0 * td;
                    Out.Velocity = V0;

                    Out.Orientation = QuatFromZTo(Dir);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(0.1f, 0.05f, Rand(InSeed, 3)),
                        FMath::Lerp(0.1f, 0.2f,  Rand(InSeed, 4)),
                        FMath::Lerp(0.2f, 0.5f,  Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.0f, 0.2f, 1.5f, 1.0f, 0.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 1.5f, 1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 0.2f, 1.5f));

                    const auto Tint = CurveA(t);
                    Out.Color  = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.VisTag = 57;
                    break;
                }

                if (Layer < LayerStar01)
                {
                    const auto Life = FMath::Lerp(0.1f, 0.2f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    Out.Orientation = QuatFromZTo(RandDir(InSeed));
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(
                        FMath::Lerp(0.5f, 1.0f, Rand(InSeed, 3)),
                        FMath::Lerp(0.5f, 1.0f, Rand(InSeed, 4)),
                        FMath::Lerp(1.5f, 2.0f, Rand(InSeed, 5)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.5f, 0.2f, 1.0f,  1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 1.0f, 1.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.75f, 1.0f, 1.0f));

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.913099f, 0.319952f, 0.693872f, 1.0f, 0.450786f),
                        Key3(t, 0.0f, 0.715694f, 0.319952f, 0.147027f, 1.0f, 0.040915f),
                        Key2(t, 0.327196f, 1.0f, 1.0f, 0.0f) * 0.15f);
                    Out.VisTag = 58;
                    break;
                }

                if (Layer == LayerStar01)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Tint = CurveA(t);

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size    = FVector2f(20.0f * Grow, 20.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 59;
                    break;
                }

                if (Layer == LayerStar02)
                {
                    const auto td = InAge - DelayStar02;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Tint = CurveA(t);

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, 1.0f);
                    Out.Size    = FVector2f(70.0f * Grow, 70.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, -0.125f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 60;
                    break;
                }

                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    Ring(Saturate(td / 0.5f));
                    Out.SpriteAlignment = FVector3f(0.0f, -1.0f, 0.0f);
                    Out.SpriteFacing    = FVector3f(0.0f,  0.0f, 1.0f);
                    Out.VisTag          = 61;
                }
                break;
            }
            case 25: // BombSpawn — Vefects NS_Bomb_Spawn. Mirrors Behavior_BombSpawn.ush; recipe Cookbook/NS_Bomb_Spawn.md.
            {
                constexpr auto NumLayers     = 28;
                constexpr auto LayerGlow01   = 0;
                constexpr auto LayerGlow02   = 1;
                constexpr auto LayerRainbow  = 5;
                constexpr auto LayerRing     = 16;
                constexpr auto LayerFlash01  = 17;
                constexpr auto LayerFlash02  = 18;
                constexpr auto LayerBomb     = 22;
                constexpr auto LayerFlare04  = 23;
                constexpr auto LayerFlare02  = 26;

                constexpr auto DelayMid  = 0.05f;
                constexpr auto DelayLate = 0.1f;

                constexpr auto Tau = 6.28318530718f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto FadeAlpha = [](float InT) -> float
                {
                    return Key2(InT, 0.0f, 1.0f, 1.0f, 0.0f);
                };

                const auto Flare = [&](float InT, FVector3f InColor, float InInitialAlpha, FVector2f InSize,
                                       float InWidth, float InDissolve) -> void
                {
                    const auto Uniform = Key3(InT, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);
                    const auto Length  = Key2(InT, 0.0f, 1.0f, 0.9f, 0.0f);

                    Out.Color   = FLinearColor(InColor.X, InColor.Y, InColor.Z,
                                               InInitialAlpha * Key2(InT, 0.225777f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(InSize.X * Uniform * Length, InSize.Y * Uniform * InWidth);
                    Out.Dynamic = FVector4f(InDissolve, 0.0f, 0.0f, 0.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f::ZeroVector;
                Out.Position = FVector3f::ZeroVector;
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == LayerGlow01)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(0.0368895f, 0.184475f, 1.0f, 0.5f * FadeAlpha(t));
                    Out.Size    = FVector2f(500.0f * Grow, 500.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 62;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color  = FLinearColor(0.0193824f, 0.0451862f, 0.913099f, FadeAlpha(t));
                    Out.Size   = FVector2f(250.0f * Grow, 250.0f * Grow);
                    Out.VisTag = 62;
                    break;
                }

                if (Layer < LayerRainbow)
                {
                    const auto td = InAge - DelayMid;

                    if (td < 0.0f || td > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(0.0466651f, 0.491021f, 1.0f, FadeAlpha(t));
                    Out.Size    = FVector2f(200.0f * Grow, 200.0f * Grow);
                    Out.Dynamic = FVector4f(2.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 62;
                    break;
                }

                if (Layer == LayerRainbow)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.913099f * 0.5f, 0.913099f * 0.5f, 0.913099f * 0.5f,
                                                0.15f * FadeAlpha(t));
                    Out.Size     = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 63;
                    break;
                }

                if (Layer < LayerRing)
                {
                    const auto Life = FMath::Lerp(0.5f, 1.0f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayMid;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (0.5f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(350.0f, 500.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.615756f, 0.296138f, 0.936915f, 0.0f),
                        Key2(t, 0.615756f, 0.571125f, 0.936915f, 0.0896671f),
                        1.0f,
                        Key2(t, 0.613341f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(7.0f, 10.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 64;
                    break;
                }

                if (Layer == LayerRing)
                {
                    const auto td = InAge - DelayMid;

                    if (td < 0.0f || td > 0.75f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.75f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 0.7f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0f, 0.191202f, 0.405675f, 0.191202f, 1.0f, 0.79f),
                        Key3(t, 0.0f, 0.318547f, 0.405675f, 0.318547f, 1.0f, 0.823064f),
                        1.0f,
                        1.0f);
                    Out.Size     = FVector2f(170.0f * Grow, 170.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = 65;
                    break;
                }

                if (Layer == LayerFlash01 || Layer == LayerFlash02)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);
                    const auto Fade = FadeAlpha(t);

                    const auto Tint = Layer == LayerFlash01
                        ? FVector3f(0.102242f, 0.658375f, 1.0f)
                        : FVector3f(0.102242f, 1.0f,      0.838799f);
                    const auto Alpha = Layer == LayerFlash01 ? 0.2f : 0.3f;
                    const auto Base  = Layer == LayerFlash01 ? 30.0f : 50.0f;

                    Out.Color   = FLinearColor(Tint.X, Tint.Y, Tint.Z, Alpha * Fade);
                    Out.Size    = FVector2f(Base * Grow, Base * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 66;
                    break;
                }

                if (Layer < LayerBomb)
                {
                    const auto td = InAge - DelayMid;

                    if (td < 0.0f || td > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(0.0368895f, 0.184475f, 1.0f, 0.5f);
                    Out.Size    = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.Dynamic = FVector4f(2.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = 62;
                    break;
                }

                if (Layer == LayerBomb)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge);
                    const auto Flash = Key2(t, 0.15f, 5.0f, 0.255961f, 0.25f);

                    const auto SpinTurns = 1.0f * Int3(t, 0.5f, 5.0f, 0.0f, 0.0f);
                    const auto InitTurns = FMath::Lerp(-1.0f, 1.0f, Rand(InSeed, 3));
                    Out.Orientation = QuatFromAxisAngle(FVector3f(0.0f, 0.0f, 1.0f),
                                                        Tau * (InitTurns + SpinTurns));
                    Out.MeshIndex = 0;
                    Out.Size      = FVector2f(0.0f, 0.0f);

                    const auto Pop = Key2(t, 0.0f, 0.0f, 0.2f, 1.0f);
                    Out.Scale = FVector3f(0.45f * Pop, 0.45f * Pop, 0.45f * Pop);

                    Out.Color  = FLinearColor(Flash, Flash, Flash, 1.0f);
                    Out.VisTag = 67;
                    break;
                }

                if (Layer == LayerFlare04)
                {
                    if (InAge > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 0.3f);

                    Flare(t, FVector3f(0.508881f, 0.679543f, 1.0f), 1.0f, FVector2f(400.0f, 50.0f),
                          Key3(t, 0.0f, 0.3f, 0.2f, 1.0f, 0.9f, 0.0999999f), 1.0f);
                    Out.VisTag = 68;
                    break;
                }

                if (Layer < LayerFlare02)
                {
                    if (InAge > 0.7f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 0.7f);

                    Flare(t, FVector3f(0.00402472f, 0.0295568f, 0.130136f), 1.0f, FVector2f(1400.0f, 200.0f),
                          Key2(t, 0.0f, 0.3f, 0.2f, 1.0f), 0.5f);
                    Out.VisTag = 69;
                    break;
                }

                if (Layer == LayerFlare02)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 0.5f);

                    Flare(t, FVector3f(0.238f, 0.502791f, 1.0f), 1.0f, FVector2f(500.0f, 100.0f),
                          Key3(t, 0.0f, 0.3f, 0.2f, 1.0f, 0.9f, 0.2f), 1.0f);
                    Out.VisTag = 70;
                    break;
                }

                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 0.5f);

                    Flare(t, FVector3f(0.00182116f, 0.0561285f, 1.0f), 0.5f, FVector2f(500.0f, 100.0f),
                          Key2(t, 0.0f, 0.3f, 0.2f, 1.0f), 1.0f);
                    Out.VisTag = 62;
                }
                break;
            }
            case 26: // PickupLoop — Vefects NS_PickupLoop. Mirrors Behavior_PickupLoop.ush; recipe Cookbook/NS_PickupLoop.md.
            {
                // Cumulative spawn-rate shares, source emitter order. A rate-only source has no per-loop burst to
                // slice, so the layer is a weighted DRAW rather than a modulus.
                constexpr auto RateTotal   = 27.5f;
                constexpr auto CumGlow01   =  2.0f;
                constexpr auto CumGlow02   =  4.0f;
                constexpr auto CumGlow03   =  8.0f;
                constexpr auto CumGlow04   = 12.0f;
                constexpr auto CumSparkles = 17.0f;
                constexpr auto CumRing     = 17.5f;
                constexpr auto CumStar01   = 19.5f;
                constexpr auto CumStar02   = 21.5f;

                constexpr auto VisPart01   = 71;
                constexpr auto VisPart01Br = 72;
                constexpr auto VisPart02   = 73;
                constexpr auto VisRing03   = 74;
                constexpr auto VisStar01   = 75;
                constexpr auto VisStar02   = 76;

                constexpr auto FlareBaseHue = 0.05499683f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto Glow = [&Out](float InT, FVector3f InRgb, float InAlpha, float InSize, int32 InVisTag) -> void
                {
                    Out.Color   = FLinearColor(InRgb.X, InRgb.Y, InRgb.Z,
                        Key4(InT, 0.0f, 0.0f, 0.304256f, 1.0f, 0.67371f, 1.0f, 1.0f, 0.0f) * InAlpha);
                    Out.Size    = FVector2f(InSize, InSize);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = InVisTag;
                };

                const auto R = Rand(InSeed, 0) * RateTotal;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (R < CumGlow01)
                {
                    if (InAge > 2.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 2.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.237851f, 0.708376f,  0.492605f, 0.527115f, 0.763055f, 0.708376f),
                        Key3(t, 0.237851f, 0.0466651f, 0.492605f, 0.109462f, 0.763055f, 0.0466651f),
                        Key4(t, 0.0f, 0.0f, 0.231814f, 1.0f, 0.767884f, 1.0f, 1.0f, 0.0f) * 0.5f);
                    Out.Size    = FVector2f(220.0f, 220.0f);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (R < CumGlow02)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key5(t, 0.108663f, 0.838799f, 0.237851f, 0.854993f, 0.492605f, 0.637597f, 0.763055f, 0.854993f, 0.924842f, 0.838799f),
                        Key5(t, 0.108663f, 0.296138f, 0.237851f, 0.376262f, 0.492605f, 0.296138f, 0.763055f, 0.376262f, 0.924842f, 0.296138f),
                        Key4(t, 0.0f, 0.0f, 0.171446f, 1.0f, 0.835497f, 1.0f, 1.0f, 0.0f) * 0.5f);
                    Out.Size    = FVector2f(220.0f, 220.0f);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (R < CumGlow03)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    Glow(Saturate(InAge), FVector3f(1.0f, 0.266356f, 0.184475f), 1.0f, 350.0f, VisPart01);
                    break;
                }

                if (R < CumGlow04)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    Glow(Saturate(InAge), FVector3f(1.0f, 0.947307f, 0.520996f), 0.3f, 100.0f, VisPart02);
                    break;
                }

                if (R < CumSparkles)
                {
                    const auto Life = FMath::Lerp(0.6f, 1.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(InAge / Life);
                    const auto Dir = RandDir(InSeed);

                    // Add Velocity from Point is DISABLED in the source, so these motes never move.
                    Out.Position = Dir * (70.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));

                    Out.Color = FLinearColor(
                        Key2(t, 0.615756f, 1.0f,      0.936915f, 0.0f),
                        Key2(t, 0.615756f, 0.508881f, 0.936915f, 0.0896671f),
                        Key2(t, 0.615756f, 0.191202f, 0.936915f, 1.0f),
                        Key2(t, 0.613341f, 1.0f,      1.0f,      0.0f));

                    const auto Base = FMath::Lerp(7.0f, 10.0f, Rand(InSeed, 8));
                    const auto Grow = Key4(t, 0.0f, 0.0f, 0.1f, 1.0f, 0.2f, 0.6f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (R < CumRing)
                {
                    if (InAge > 4.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 4.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.947307f, 0.281316f, 0.693872f, 0.889828f, 0.0612461f),
                        Key3(t, 0.0f, 0.665387f, 0.281316f, 0.147027f, 0.889828f, 0.00856812f),
                        Key4(t, 0.0f, 0.0f, 0.261998f, 1.0f, 0.723212f, 1.0f, 1.0f, 0.0f) * 0.25f);
                    Out.Size = FVector2f(160.0f, 160.0f);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f
                                 + FMath::Lerp(-20.0f, 20.0f, Rand(InSeed, 5)) * InAge;

                    Out.Dynamic = FVector4f(Key3(t, 0.0f, -1.0f, 0.5f, 0.5f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisRing03;
                    break;
                }

                if (R < CumStar01)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.141262f, 0.947307f, 0.429822f, 0.693872f, 0.84757f, 0.450786f),
                        Key3(t, 0.141262f, 0.665387f, 0.429822f, 0.147027f, 0.84757f, 0.0409152f),
                        Key4(t, 0.0f, 0.0f, 0.161787f, 1.0f, 0.839119f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(40.0f, 40.0f);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisStar01;
                    break;
                }

                if (R < CumStar02)
                {
                    if (InAge > 0.8f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.8f);
                    const auto Grow = Key3(t, 0.0f, 0.4f, 0.5f, 1.0f, 1.0f, 0.4f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.141262f, 0.947307f, 0.429822f, 0.693872f, 0.84757f, 0.450786f),
                        Key3(t, 0.141262f, 0.665387f, 0.429822f, 0.147027f, 0.84757f, 0.040915f),
                        Key4(t, 0.0f, 0.0f, 0.161787f, 1.0f, 0.839119f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(80.0f * Grow, 80.0f * Grow);
                    Out.Dynamic = FVector4f(0.745454f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisStar02;
                    break;
                }

                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(InAge / Life);
                    const auto Dir = RandDir(InSeed);

                    Out.Position = Dir * (100.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));

                    const auto Hue = FlareBaseHue + FMath::Lerp(0.1f, -0.1f, Rand(InSeed, 6));
                    const auto Sat = FMath::Lerp(0.35f, 0.5f, Rand(InSeed, 11));
                    const auto Rgb = HsvToRgb(Hue, Sat, 1.0f);

                    const auto Alpha = FMath::Lerp(0.1f, 0.2f, Rand(InSeed, 12))
                                     * Key3(t, 0.0f, 0.0f, 0.3f, 0.125f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Alpha);

                    const auto Base = FMath::Lerp(50.0f, 200.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.8f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(8.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart02;
                }
                break;
            }
            case 27: // HealLoop — Vefects NS_HealLoop. Mirrors Behavior_HealLoop.ush; recipe Cookbook/NS_HealLoop.md.
            {
                constexpr auto RateTotal   = 34.5f;
                constexpr auto CumRainbow  =  0.5f;
                constexpr auto CumSparkles = 10.5f;
                constexpr auto CumStretch  = 20.5f;
                constexpr auto CumStar01   = 21.5f;
                constexpr auto CumStar02   = 22.5f;
                constexpr auto CumGlow01   = 24.5f;
                constexpr auto CumGlow02   = 26.5f;
                constexpr auto CumFlares   = 32.5f;

                constexpr auto VisRainbow  = 77;
                constexpr auto VisPart01Br = 78;
                constexpr auto VisPart04   = 79;
                constexpr auto VisStar02   = 80;
                constexpr auto VisStar01   = 81;
                constexpr auto VisPart01   = 82;
                constexpr auto VisPart02   = 83;

                constexpr auto FlareBaseHue = 0.32084478f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                // The shared "heal" ramp that both sparkle streams carry, keyed identically.
                const auto SparkleRamp = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        Key4(InT, 0.0f, 0.715694f, 0.079686f, 1.0f,      0.290975f, 0.109462f, 0.713553f, 0.024158f),
                        Key4(InT, 0.0f, 0.89627f,  0.079686f, 0.854993f, 0.290975f, 1.0f,      0.713553f, 0.913099f),
                        Key4(InT, 0.0f, 1.0f,      0.079686f, 0.376262f, 0.290975f, 0.617207f, 0.713553f, 0.141263f));
                };

                const auto Star = [&Out, InSeed](float InT, float InLife, float InSizeMin, float InSizeMax,
                                                 float InScaleAlpha, int32 InVisTag) -> void
                {
                    const auto Spawn = CylinderPoint(InSeed, 30.0f, 60.0f, FVector3f(0.0f, 0.0f, -30.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, 500.0f);

                    Out.Position = Spawn + V0 * (Int3(InT, 0.2f, 1.0f, 0.25f, 0.0f) * InLife);
                    Out.Velocity = V0 * Key3(InT, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key4(InT, 0.0f, 0.715694f, 0.0796861f, 1.0f,      0.234229f, 0.921582f, 0.80652f, 0.109462f),
                        Key4(InT, 0.0f, 0.89627f,  0.0796861f, 0.854993f, 0.234229f, 0.723055f, 0.80652f, 1.0f),
                        Key4(InT, 0.0f, 1.0f,      0.0796861f, 0.376262f, 0.234229f, 0.462077f, 0.80652f, 0.617207f),
                        Key2(InT, 0.0808934f, 1.0f, 1.0f, 0.0f) * InScaleAlpha);

                    const auto Base = FMath::Lerp(InSizeMin, InSizeMax, Rand(InSeed, 8));
                    const auto Grow = Key3(InT, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = InVisTag;
                };

                const auto Glow = [&Out](float InT, float InSize, float InScaleAlpha, float InDissolve, int32 InVisTag) -> void
                {
                    Out.Color = FLinearColor(
                        0.296138f,
                        1.0f,
                        Key3(InT, 0.169031f, 0.737911f, 0.470872f, 0.47932f, 0.817386f, 0.737911f),
                        Key4(InT, 0.0f, 0.0f, 0.237851f, 1.0f, 0.712345f, 1.0f, 1.0f, 0.0f) * InScaleAlpha);
                    Out.Size    = FVector2f(InSize, InSize);
                    Out.Dynamic = FVector4f(InDissolve, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = InVisTag;
                };

                const auto R = Rand(InSeed, 0) * RateTotal;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (R < CumRainbow)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge);
                    const auto Grow  = Key2(t, 0.0f, 0.8f, 1.0f, 1.0f);
                    const auto Alpha = 0.1f * Key3(t, 0.0f, 0.0f, 0.269242f, 1.0f, 1.0f, 0.0f);

                    Out.Color    = FLinearColor(0.913099f * 0.5f, 0.913099f * 0.5f, 0.913099f * 0.5f, Alpha);
                    Out.Size     = FVector2f(450.0f * Grow, 450.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRainbow;
                    break;
                }

                if (R < CumSparkles)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 130.0f, FVector3f(0.0f, 0.0f, 0.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(1000.0f, 2000.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.1f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = SparkleRamp(t);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z,
                        Key2(t, 0.080893f, 1.0f, 1.0f, 0.0f) * 0.15f);

                    const auto Base = FMath::Lerp(6.0f, 10.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = 90.0f;
                    Out.Dynamic  = FVector4f(3.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (R < CumStretch)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 120.0f, FVector3f(0.0f, 0.0f, 0.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(1000.0f, 1600.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = SparkleRamp(t);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z,
                        Key2(t, 0.080893f, 1.0f, 1.0f, 0.0f) * 0.15f);

                    // The source's LENGTH range is inverted (min 70 > max 60); copied verbatim.
                    const auto Width  = FMath::Lerp(25.0f, 40.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(70.0f, 60.0f, Rand(InSeed, 3));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.3f, 0.25f, 1.0f, 0.2f);

                    const auto SpeedFactor = FMath::Lerp(1.0f, 2.0f, Saturate(Out.Velocity.Size() / 1000.0f));

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper * SpeedFactor);
                    Out.VisTag = VisPart04;
                    break;
                }

                if (R < CumStar02)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    if (R < CumStar01)
                    { Star(t, Life, 40.0f, 50.0f, 1.0f, VisStar02); }
                    else
                    { Star(t, Life, 30.0f, 40.0f, 0.7f, VisStar01); }
                    break;
                }

                if (R < CumGlow01)
                {
                    if (InAge > 2.0f)
                    {
                        Hide();
                        break;
                    }

                    Glow(Saturate(InAge / 2.0f), 550.0f, 0.03f, 3.0f, VisPart01);
                    break;
                }

                if (R < CumGlow02)
                {
                    if (InAge > 2.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 2.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.169031f, 0.0609999f, 0.318744f, 1.0f,      0.470872f, 0.077f,     0.62783f, 1.0f,      0.817386f, 0.062f),
                        Key5(t, 0.169031f, 1.0f,       0.318744f, 0.78728f,  0.470872f, 1.0f,       0.62783f, 0.790767f, 0.817386f, 1.0f),
                        Key5(t, 0.169031f, 0.650356f,  0.318744f, 0.085f,    0.470872f, 0.317213f,  0.62783f, 0.1f,      0.817386f, 0.650728f),
                        Key4(t, 0.0f, 0.0f, 0.237851f, 1.0f, 0.712345f, 1.0f, 1.0f, 0.0f) * 0.7f);
                    Out.Size   = FVector2f(250.0f, 250.0f);
                    Out.VisTag = VisPart01;
                    break;
                }

                if (R < CumFlares)
                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    Out.Position = CylinderPoint(InSeed, 80.0f, 100.0f, FVector3f(0.0f, 0.0f, -10.0f));

                    // The source's Hue Shift Range DESCENDS (0.2 -> -0.2); lerping in that order is the range as authored.
                    const auto Hue = FlareBaseHue + FMath::Lerp(0.2f, -0.2f, Rand(InSeed, 6));
                    const auto Sat = FMath::Lerp(0.35f, 0.5f, Rand(InSeed, 11));
                    const auto Rgb = HsvToRgb(Hue, Sat, 1.0f);

                    const auto Alpha = FMath::Lerp(0.07f, 0.1f, Rand(InSeed, 12))
                                     * Key3(t, 0.0f, 0.0f, 0.3f, 0.125f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Alpha);

                    const auto Base = FMath::Lerp(50.0f, 200.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.8f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(8.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart02;
                    break;
                }

                {
                    if (InAge > 2.0f)
                    {
                        Hide();
                        break;
                    }

                    Glow(Saturate(InAge / 2.0f), 220.0f, 0.07f, 0.0f, VisPart02);
                }
                break;
            }
            case 28: // BuffLoop — Vefects NS_BuffLoop. Mirrors Behavior_BuffLoop.ush; recipe Cookbook/NS_BuffLoop.md.
            {
                constexpr auto RateTotal   = 48.0f;
                constexpr auto CumGlow01   =  2.0f;
                constexpr auto CumRainbow  =  3.0f;
                constexpr auto CumArrow    =  6.0f;
                constexpr auto CumStars    =  8.0f;
                constexpr auto CumStretch  = 18.0f;
                constexpr auto CumGlow02   = 22.0f;
                constexpr auto CumSparkles = 32.0f;
                constexpr auto CumFlares   = 38.0f;

                constexpr auto VisPart01   = 84;
                constexpr auto VisRainbow  = 85;
                constexpr auto VisArrows   = 86;
                constexpr auto VisStar01   = 87;
                constexpr auto VisPart04   = 88;
                constexpr auto VisPart01Br = 89;
                constexpr auto VisPart02   = 90;

                constexpr auto FlareBaseHue = 0.99887927f;

                // Sparkles_Spiral's Vortex Force, verbatim; the lever arm is floored because the axis itself has no
                // tangential direction.
                constexpr auto VortexForce    = 15881.6f;
                constexpr auto VortexFalloff  = 100.0f;
                constexpr auto VortexMinLever = 10.0f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto WarmRamp = [](float InT, float InK1, float InK2, float InK3) -> FVector3f
                {
                    return FVector3f(
                        Key5(InT, 0.0f, 1.0f,      InK1, 1.0f,       InK2, 1.0f,       InK3, 0.672443f,  0.947781f, 0.223228f),
                        Key5(InT, 0.0f, 0.913099f, InK1, 0.501026f,  InK2, 0.0773835f, InK3, 0.021219f,  0.947781f, 0.0f),
                        Key5(InT, 0.0f, 0.584079f, InK1, 0.0559999f, InK2, 0.025f,     InK3, 0.0168074f, 0.947781f, 0.116971f));
                };

                const auto SegIntUS = [](float InA, float InB, float InSa, float InSb) -> float
                {
                    const auto M  = (InSb - InSa) / FMath::Max(InB - InA, 1.0e-6f);
                    const auto C  = InSa - M * InA;
                    const auto Hi = M * InB * InB * InB / 3.0f + C * InB * InB * 0.5f;
                    const auto Lo = M * InA * InA * InA / 3.0f + C * InA * InA * 0.5f;
                    return Hi - Lo;
                };

                // Integral over [0, U] of u * S(u) for the 3-key clamped curve (0, V0) -> (K, V1) -> (1, V2).
                const auto SwirlIntegral = [&SegIntUS](float InU, float InK, float InV0, float InV1, float InV2) -> float
                {
                    const auto T = Saturate(InU);
                    const auto A = FMath::Min(T, InK);

                    auto Sum = SegIntUS(0.0f, A, InV0, Key2(A, 0.0f, InV0, InK, InV1));

                    if (T > InK)
                    { Sum += SegIntUS(InK, T, InV1, Key2(T, InK, InV1, 1.0f, InV2)); }

                    return Sum;
                };

                const auto R = Rand(InSeed, 0) * RateTotal;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (R < CumGlow01)
                {
                    if (InAge > 2.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 2.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.738388f, 0.457591f, 0.166f, 1.0f, 0.735614f),
                        Key3(t, 0.0f, 0.057f,    0.457591f, 0.166f, 1.0f, 0.0469999f),
                        Key4(t, 0.0f, 0.0f, 0.258376f, 1.0f, 0.680954f, 1.0f, 1.0f, 0.0f) * 0.5f);
                    Out.Size    = FVector2f(500.0f, 500.0f);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (R < CumRainbow)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.3f, 0.875f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.913099f * 0.5f, 0.913099f * 0.5f, 0.913099f * 0.5f,
                                                0.2f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRainbow;
                    break;
                }

                if (R < CumArrow)
                {
                    if (InAge > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 1.5f);

                    const auto Spawn = CylinderPoint(InSeed, 120.0f, 150.0f, FVector3f(0.0f, 0.0f, 30.0f))
                                     + FVector3f(0.0f, 0.0f, -119.316f);
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(350.0f, 500.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.3f, 0.05f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.3f, 1.0f, 0.05f);

                    const auto Rgb = WarmRamp(t, 0.0784787f, 0.283731f, 0.625415f);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Key2(t, 0.0869303f, 1.0f, 1.0f, 0.0f));

                    const auto Grow = Key3(t, 0.0f, 0.4f, 0.1f, 1.0f, 1.0f, 0.4f);
                    Out.Size   = FVector2f(80.0f * Grow, 130.0f * Grow);
                    Out.VisTag = VisArrows;
                    break;
                }

                if (R < CumStars)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 120.0f, FVector3f(0.0f, 0.0f, 30.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(500.0f, 1300.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.2f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.2f, 1.0f, 0.0f);

                    const auto Rgb = WarmRamp(t, 0.182618f, 0.383139f, 0.68034f);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Key2(t, 0.08693f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(40.0f, 70.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisStar01;
                    break;
                }

                if (R < CumStretch)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 120.0f, 150.0f, FVector3f(0.0f, 0.0f, 30.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(1000.0f, 2000.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.1f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = WarmRamp(t, 0.078479f, 0.283731f, 0.625415f);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Key2(t, 0.08693f, 1.0f, 1.0f, 0.0f));

                    const auto Width  = FMath::Lerp(35.0f, 50.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(130.0f, 140.0f, Rand(InSeed, 3));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.1f, 0.2f, 1.0f, 0.15f);

                    const auto SpeedFactor = FMath::Lerp(1.0f, 1.7f, Saturate(Out.Velocity.Size() / 1000.0f));

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper * SpeedFactor);
                    Out.VisTag = VisPart04;
                    break;
                }

                if (R < CumGlow02)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.737001f,  0.457591f, 0.155f, 1.0f, 0.734227f),
                        Key3(t, 0.0f, 0.0519999f, 0.457591f, 0.155f, 1.0f, 0.0419999f),
                        Key4(t, 0.0f, 0.0f, 0.258376f, 1.0f, 0.680954f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(300.0f, 300.0f);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (R < CumSparkles)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 120.0f, 150.0f, FVector3f(0.0f, 0.0f, 30.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(1000.0f, 2000.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.1f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = WarmRamp(t, 0.078479f, 0.283731f, 0.625415f);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Key2(t, 0.08693f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(10.0f, 15.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(3.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01Br;
                    break;
                }

                if (R < CumFlares)
                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    Out.Position = CylinderPoint(InSeed, 110.0f, 130.0f, FVector3f(0.0f, 0.0f, 30.0f));

                    const auto Hue = FlareBaseHue + FMath::Lerp(0.5f, 0.8f, Rand(InSeed, 6));
                    const auto Sat = FMath::Lerp(0.2f, 0.2f, Rand(InSeed, 11));
                    const auto Rgb = HsvToRgb(Hue, Sat, 1.0f);

                    const auto Alpha = FMath::Lerp(0.13f, 0.13f, Rand(InSeed, 12))
                                     * Key3(t, 0.0f, 0.0f, 0.3f, 0.125f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Alpha);

                    const auto Base = FMath::Lerp(50.0f, 200.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.8f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(8.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart02;
                    break;
                }

                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 120.0f, FVector3f(0.0f, 0.0f, 0.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(3500.0f, 5000.0f, Rand(InSeed, 2)));

                    const auto Radius  = FVector2f(Spawn.X, Spawn.Y).Size();
                    const auto Angle0  = FMath::Atan2(Spawn.Y, Spawn.X);
                    const auto Falloff = Saturate(1.0f - Radius / VortexFalloff);

                    const auto Alpha = VortexForce * Falloff / FMath::Max(Radius, VortexMinLever);
                    const auto Swirl = Alpha * Life * Life * SwirlIntegral(t, 0.1f, 1.0f, 0.15f, 0.0f);
                    const auto Angle = Angle0 + Swirl;

                    const auto Rise = V0.Z * (Int3(t, 0.1f, 1.0f, 0.15f, 0.0f) * Life);

                    Out.Position = FVector3f(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Spawn.Z + Rise);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = WarmRamp(t, 0.078479f, 0.283731f, 0.625415f);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Key2(t, 0.08693f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(10.0f, 15.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(3.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01Br;
                }
                break;
            }
            case 29: // DebuffLoop — Vefects NS_DebuffLoop. Mirrors Behavior_DebuffLoop.ush; recipe Cookbook/NS_DebuffLoop.md.
            {
                constexpr auto RateTotal      = 36.0f;
                constexpr auto CumSparkles    =  4.0f;
                constexpr auto CumRing        =  7.0f;
                constexpr auto CumFlames      = 12.0f;
                constexpr auto CumGlow01      = 14.0f;
                constexpr auto CumGlow02      = 18.0f;
                constexpr auto CumGlow03      = 22.0f;
                constexpr auto CumArrowGreen  = 26.0f;
                constexpr auto CumArrowPurple = 30.0f;

                constexpr auto VisPart01   = 91;
                constexpr auto VisPart01Br = 92;
                constexpr auto VisRing01   = 93;
                constexpr auto VisFlames01 = 94;
                constexpr auto VisPart02   = 95;
                constexpr auto VisArrows   = 96;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto Arrow = [&Out, InSeed](float InT, float InLife, FVector3f InRgb) -> void
                {
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 130.0f, FVector3f(0.0f, 0.0f, 150.0f))
                                     + FVector3f(0.0f, 0.0f, -119.316f);
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(-150.0f, -300.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int2(InT, 1.0f, 0.2f) * InLife);
                    Out.Velocity = V0 * Key2(InT, 0.0f, 1.0f, 1.0f, 0.2f);

                    Out.Color = FLinearColor(InRgb.X, InRgb.Y, InRgb.Z,
                        Key3(InT, 0.0f, 0.0f, 0.085723f, 1.0f, 1.0f, 0.0f));

                    const auto Grow = Key2(InT, 0.0f, 1.0f, 1.0f, 0.4f);
                    Out.Size   = FVector2f(90.0f * Grow, 150.0f * Grow);
                    Out.VisTag = VisArrows;
                };

                const auto Glow = [&Out](float InT, float InSize, float InAlphaIn, float InAlphaOut) -> void
                {
                    Out.Color = FLinearColor(
                        Key3(InT, 0.150921f, 0.093059f,  0.457591f, 0.111932f,  0.772714f, 0.093059f),
                        Key3(InT, 0.150921f, 0.181164f,  0.457591f, 0.0409152f, 0.772714f, 0.181164f),
                        Key3(InT, 0.150921f, 0.0953075f, 0.457591f, 0.3564f,    0.772714f, 0.0953075f),
                        Key4(InT, 0.0f, 0.0f, InAlphaIn, 1.0f, InAlphaOut, 1.0f, 1.0f, 0.0f) * 0.8f);
                    Out.Size    = FVector2f(InSize, InSize);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                };

                const auto R = Rand(InSeed, 0) * RateTotal;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (R < CumSparkles)
                {
                    const auto Life = FMath::Lerp(1.0f, 1.5f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Spawn = CylinderPoint(InSeed, 120.0f, 150.0f, FVector3f(0.0f, 0.0f, 30.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(-200.0f, -700.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.1f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.1f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.113492f, 0.00102734f, 0.385149f, 0.000465223f),
                        Key2(t, 0.113492f, 0.002f,      0.385149f, 0.000531684f),
                        Key2(t, 0.113492f, 0.00105217f, 0.385149f, 0.002f),
                        Key3(t, 0.0f, 0.0f, 0.0857229f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(7.0f, 15.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (R < CumRing)
                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / Life);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(0.00182116f, 0.00182116f, 0.00212469f,
                        Key3(t, 0.0f, 0.0f, 0.499849f, 1.0f, 1.0f, 0.0f) * 0.35f);

                    const auto Base = FMath::Lerp(200.0f, 300.0f, Rand(InSeed, 8));
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -0.5f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                    break;
                }

                if (R < CumFlames)
                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / Life);
                    const auto Grow = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);

                    Out.Position = RandDir(InSeed) * 20.0f;

                    Out.Color = FLinearColor(
                        Key3(t, 0.113492f, 0.175111f, 0.545729f, 0.0144f, 0.984002f, 0.00560539f),
                        Key3(t, 0.113492f, 0.250158f, 0.545729f, 0.0144f, 0.984002f, 0.00802319f),
                        Key3(t, 0.113492f, 0.175111f, 0.545729f, 0.048f,  0.984002f, 0.00560539f),
                        0.05f);

                    const auto Base = FMath::Lerp(200.0f, 300.0f, Rand(InSeed, 8));
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = 90.0f + FMath::Lerp(-45.0f, 45.0f, Rand(InSeed, 5)) * InAge;

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 10.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisFlames01;
                    break;
                }

                if (R < CumGlow01)
                {
                    if (InAge > 2.0f)
                    {
                        Hide();
                        break;
                    }

                    Glow(Saturate(InAge / 2.0f), 500.0f, 0.172653f, 0.754603f);
                    break;
                }

                if (R < CumGlow02)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    Glow(Saturate(InAge), 250.0f, 0.176275f, 0.752188f);
                    break;
                }

                if (R < CumGlow03)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    // Two update modules only, so its Initialize colour is what renders.
                    Out.Color   = FLinearColor(0.00913406f, 0.00334653f, 0.0331048f, 0.4f);
                    Out.Size    = FVector2f(1000.0f, 1000.0f);
                    Out.Dynamic = FVector4f(0.7f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (R < CumArrowPurple)
                {
                    const auto Probability = FMath::Lerp(0.5f, 1.0f, Rand(InSeed, 11));

                    if (Rand(InSeed, 12) >= Probability)
                    {
                        Hide();
                        break;
                    }

                    const auto Life = FMath::Lerp(0.6f, 1.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    if (R < CumArrowGreen)
                    {
                        Arrow(t, Life, FVector3f(
                            Key2(t, 0.113492f, 0.0165f,    0.46725f, 0.00116306f),
                            Key2(t, 0.113492f, 0.03f,      0.46725f, 0.00132921f),
                            Key2(t, 0.113492f, 0.0169417f, 0.46725f, 0.005f)));
                        break;
                    }

                    Arrow(t, Life, FVector3f(
                        Key2(t, 0.113492f, 0.0137272f, 0.487775f, 0.00116306f),
                        Key2(t, 0.113492f, 0.009f,     0.487775f, 0.00132921f),
                        Key2(t, 0.113492f, 0.03f,      0.487775f, 0.005f)));
                    break;
                }

                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    Out.Position = CylinderPoint(InSeed, 90.0f, 130.0f, FVector3f(0.0f, 0.0f, 30.0f));

                    Out.Color = FLinearColor(0.00802319f, 0.00402472f, 0.0241576f,
                        0.8f * Key3(t, 0.0f, 0.0f, 0.3f, 0.125f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(50.0f, 200.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 0.6f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(8.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart02;
                }
                break;
            }
            case 30: // PickupCast — Vefects NS_PickupCast. Mirrors Behavior_PickupCast.ush; recipe Cookbook/NS_PickupCast.md.
            {
                constexpr auto NumLayers    = 22;
                constexpr auto LayerGlow01  =  0;
                constexpr auto LayerGlow02  =  1;
                constexpr auto FirstGlow03  =  2;
                constexpr auto LayerRainbow =  5;
                constexpr auto FirstSparkle =  6;
                constexpr auto LayerRing01  = 16;
                constexpr auto LayerFlash01 = 17;
                constexpr auto LayerFlash02 = 18;
                constexpr auto LayerStar01  = 19;
                constexpr auto LayerStar02  = 20;

                constexpr auto DelayMid    = 0.05f;
                constexpr auto DelayStar02 = 0.1f;
                constexpr auto DelayStar01 = 0.2f;

                constexpr auto VisPart01   =  97;
                constexpr auto VisPart02   =  98;
                constexpr auto VisRainbow  =  99;
                constexpr auto VisPart01Br = 100;
                constexpr auto VisRing01   = 101;
                constexpr auto VisPart03Br = 102;
                constexpr auto VisStar01   = 103;
                constexpr auto VisStar02   = 104;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto RingRamp = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        1.0f,
                        Key3(InT, 0.0f, 0.912198f, 0.405675f, 0.752942f, 1.0f, 0.450786f),
                        Key3(InT, 0.0f, 0.753f,    0.405675f, 0.304987f, 1.0f, 0.040915f));
                };

                const auto StarRamp = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        1.0f,
                        Key3(InT, 0.0f, 1.0f, 0.405675f, 0.74985f, 1.0f, 0.450786f),
                        Key3(InT, 0.0f, 1.0f, 0.405675f, 0.303f,   1.0f, 0.0409152f));
                };

                const auto GlowFade = [](float InT) -> float
                {
                    return Key2(InT, 0.0f, 1.0f, 1.0f, 0.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == LayerGlow01)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(1.0f, 0.266356f, 0.184475f, 0.35f * GlowFade(t));
                    Out.Size    = FVector2f(500.0f * Grow, 500.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color  = FLinearColor(0.913099f, 0.184475f, 0.0193824f, 0.7f * GlowFade(t));
                    Out.Size   = FVector2f(230.0f * Grow, 230.0f * Grow);
                    Out.VisTag = VisPart01;
                    break;
                }

                if (Layer >= FirstGlow03 && Layer < LayerRainbow)
                {
                    const auto td = InAge - DelayMid;

                    if (td < 0.0f || td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(1.0f, 0.947307f, 0.520996f, 0.3f * GlowFade(t));
                    Out.Size    = FVector2f(100.0f * Grow, 100.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart02;
                    break;
                }

                if (Layer == LayerRainbow)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.5f, 0.5f, 0.5f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(250.0f * Grow, 250.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRainbow;
                    break;
                }

                if (Layer >= FirstSparkle && Layer < LayerRing01)
                {
                    const auto Life = FMath::Lerp(0.5f, 1.0f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayMid;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t   = Saturate(td / Life);
                    const auto Dir = RandDir(InSeed);

                    const auto Spawn = Dir * (0.5f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(350.0f, 500.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.615756f, 1.0f,      0.936915f, 0.0f),
                        Key2(t, 0.615756f, 0.508881f, 0.936915f, 0.0896671f),
                        Key2(t, 0.615756f, 0.191202f, 0.936915f, 1.0f),
                        Key2(t, 0.613341f, 1.0f,      1.0f,      0.0f));

                    const auto Base = FMath::Lerp(7.0f, 10.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (Layer == LayerRing01)
                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 0.7f, 1.0f, 1.0f);
                    const auto Rgb  = RingRamp(t);

                    Out.Color    = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 1.0f);
                    Out.Size     = FVector2f(120.0f * Grow, 120.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                    break;
                }

                if (Layer == LayerFlash01)
                {
                    const auto td = InAge - DelayMid;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(1.0f, 0.916066f, 0.467f, 0.1f * GlowFade(t));
                    Out.Size    = FVector2f(800.0f * Grow, 800.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == LayerFlash02)
                {
                    const auto td = InAge - DelayMid;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color   = FLinearColor(3.0f, 1.91279f, 0.458779f, GlowFade(t));
                    Out.Size    = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart03Br;
                    break;
                }

                if (Layer == LayerStar01)
                {
                    const auto td = InAge - DelayStar01;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Rgb  = StarRamp(t);

                    Out.Color   = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 1.0f);
                    Out.Size    = FVector2f(40.0f * Grow, 40.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisStar01;
                    break;
                }

                if (Layer == LayerStar02)
                {
                    const auto td = InAge - DelayStar02;

                    if (td < 0.0f || td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);
                    const auto Rgb  = StarRamp(t);

                    Out.Color   = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 1.0f);
                    Out.Size    = FVector2f(100.0f * Grow, 100.0f * Grow);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, -0.125f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisStar02;
                    break;
                }

                {
                    if (InAge > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.2f, 0.7f, 1.0f, 1.0f);
                    const auto Rgb  = RingRamp(t);

                    Out.Color    = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 0.2f);
                    Out.Size     = FVector2f(140.0f * Grow, 140.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                }
                break;
            }
            case 31: // HealCast — Vefects NS_HealCast. Mirrors Behavior_HealCast.ush; recipe Cookbook/NS_HealCast.md.
            {
                constexpr auto Loop        = 2.0f;
                constexpr auto BurstSlots  = 17;
                constexpr auto RateTotal   = 50.0f;
                constexpr auto CumSparkles = 20.0f;
                constexpr auto CumStretch  = 30.0f;
                constexpr auto CumStar01   = 35.0f;
                constexpr auto CumStar02   = 40.0f;

                constexpr auto Window     = 0.3f;
                constexpr auto WindowLens = 0.2f;
                constexpr auto Delay      = 0.05f;

                constexpr auto LayerGlow01   = 0;
                constexpr auto LayerGlow02   = 1;
                constexpr auto LayerRainbow  = 2;
                constexpr auto LayerRing     = 3;
                constexpr auto LayerSparkles = 4;
                constexpr auto LayerStretch  = 5;
                constexpr auto LayerStar01   = 6;
                constexpr auto LayerStar02   = 7;
                constexpr auto LayerLens     = 8;

                constexpr auto VisPart01   = 105;
                constexpr auto VisRainbow  = 106;
                constexpr auto VisRing01   = 107;
                constexpr auto VisPart01Br = 108;
                constexpr auto VisPart04   = 109;
                constexpr auto VisStar02   = 110;
                constexpr auto VisStar01   = 111;
                constexpr auto VisPart07   = 112;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto HealRamp = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        Key4(InT, 0.0f, 0.715694f, 0.079686f, 1.0f,      0.290975f, 0.109462f, 0.713553f, 0.0241576f),
                        Key4(InT, 0.0f, 0.89627f,  0.079686f, 0.854993f, 0.290975f, 1.0f,      0.713553f, 0.913099f),
                        Key4(InT, 0.0f, 1.0f,      0.079686f, 0.376262f, 0.290975f, 0.617207f, 0.713553f, 0.141263f));
                };

                const auto StarRamp = [](float InT) -> FVector3f
                {
                    return FVector3f(
                        Key4(InT, 0.0f, 0.715694f, 0.0796861f, 1.0f,      0.234229f, 0.921582f, 0.80652f, 0.109462f),
                        Key4(InT, 0.0f, 0.89627f,  0.0796861f, 0.854993f, 0.234229f, 0.723055f, 0.80652f, 1.0f),
                        Key4(InT, 0.0f, 1.0f,      0.0796861f, 0.376262f, 0.234229f, 0.462077f, 0.80652f, 0.617207f));
                };

                const auto FadeInOut = [](float InT) -> float
                {
                    return Key3(InT, 0.0f, 0.0f, 0.0808934f, 1.0f, 1.0f, 0.0f);
                };

                const auto FadeOut = [](float InT) -> float
                {
                    return Key2(InT, 0.0808934f, 1.0f, 1.0f, 0.0f);
                };

                const auto BurstLayer = [](int32 InS) -> int32
                {
                    const auto S = ((InS % BurstSlots) + BurstSlots) % BurstSlots;

                    if (S == 0)  { return LayerGlow01; }
                    if (S == 1)  { return LayerGlow02; }
                    if (S == 2)  { return LayerRainbow; }
                    if (S == 3)  { return LayerRing; }
                    if (S < 7)   { return LayerSparkles; }
                    if (S < 12)  { return LayerStretch; }
                    if (S == 12) { return LayerStar01; }
                    if (S == 13) { return LayerStar02; }
                    return LayerLens;
                };

                const auto RateLayer = [](int32 InS) -> int32
                {
                    const auto R = Rand(InS, 0) * RateTotal;

                    if (R < CumSparkles) { return LayerSparkles; }
                    if (R < CumStretch)  { return LayerStretch; }
                    if (R < CumStar01)   { return LayerStar01; }
                    if (R < CumStar02)   { return LayerStar02; }
                    return LayerLens;
                };

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                const auto Phase   = SpawnPhase(InEmitterAge, InAge, Loop);
                const auto IsBurst = IsBurstSpawn(Phase, Loop);

                auto Layer      = LayerGlow01;
                auto LayerDelay = 0.0f;

                if (IsBurst)
                {
                    Layer      = BurstLayer(InSeed);
                    LayerDelay = Layer > LayerRainbow ? Delay : 0.0f;
                }
                else
                {
                    Layer = RateLayer(InSeed);

                    if (Phase > (Layer == LayerLens ? WindowLens : Window))
                    {
                        Hide();
                        break;
                    }
                }

                const auto td = InAge - LayerDelay;

                if (td < 0.0f)
                {
                    Hide();
                    break;
                }

                if (Layer == LayerGlow01)
                {
                    if (td > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 0.715694f, 0.0796861f, 1.0f,      0.234229f, 0.109462f, 1.0f, 0.017642f),
                        Key4(t, 0.0f, 0.89627f,  0.0796861f, 0.854993f, 0.234229f, 1.0f,      1.0f, 0.323143f),
                        Key4(t, 0.0f, 1.0f,      0.0796861f, 0.376262f, 0.234229f, 0.814847f, 1.0f, 0.0648033f),
                        0.5f * FadeInOut(t));
                    Out.Size    = FVector2f(500.0f * Grow, 500.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (td > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);
                    const auto Rgb  = HealRamp(t);

                    Out.Color  = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 0.5f * FadeInOut(t));
                    Out.Size   = FVector2f(260.0f * Grow, 260.0f * Grow);
                    Out.VisTag = VisPart01;
                    break;
                }

                if (Layer == LayerRainbow)
                {
                    if (td > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.5f, 0.5f, 0.5f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(350.0f * Grow, 350.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRainbow;
                    break;
                }

                if (Layer == LayerRing)
                {
                    if (td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Rgb  = HealRamp(t);

                    Out.Color    = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 0.15f * FadeInOut(t));
                    Out.Size     = FVector2f(140.0f * Grow, 140.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, -0.325f, 1.0f, -0.5f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                    break;
                }

                if (Layer == LayerSparkles)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(td / Life);
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 130.0f, FVector3f(0.0f, 0.0f, 0.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(1000.0f, 2000.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.1f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = HealRamp(t);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 0.15f * FadeOut(t));

                    const auto Base = FMath::Lerp(6.0f, 10.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(3.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01Br;
                    break;
                }

                if (Layer == LayerStretch)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(td / Life);
                    const auto Spawn = CylinderPoint(InSeed, 80.0f, 120.0f, FVector3f(0.0f, 0.0f, 0.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(1000.0f, 1700.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Rgb = HealRamp(t);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, 0.15f * FadeOut(t));

                    const auto Width  = FMath::Lerp(25.0f, 40.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(70.0f, 60.0f, Rand(InSeed, 3));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.3f, 0.25f, 1.0f, 0.2f);

                    const auto SpeedFactor = FMath::Lerp(1.0f, 2.0f, Saturate(Out.Velocity.Size() / 1000.0f));

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper * SpeedFactor);
                    Out.VisTag = VisPart04;
                    break;
                }

                if (Layer == LayerStar01 || Layer == LayerStar02)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(td / Life);
                    const auto Spawn = CylinderPoint(InSeed, 30.0f, 60.0f, FVector3f(0.0f, 0.0f, -30.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, 500.0f);

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.25f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, 0.0f);

                    const auto IsStar01 = Layer == LayerStar01;
                    const auto Alpha    = IsStar01 ? 1.0f : 0.7f;
                    const auto Base     = IsStar01 ? FMath::Lerp(40.0f, 50.0f, Rand(InSeed, 8))
                                                   : FMath::Lerp(30.0f, 40.0f, Rand(InSeed, 8));

                    const auto Rgb = StarRamp(t);
                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Alpha * FadeOut(t));

                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = IsStar01 ? VisStar02 : VisStar01;
                    break;
                }

                {
                    const auto Life = FMath::Lerp(1.0f, 1.5f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(td / Life);
                    const auto Spawn = CylinderPoint(InSeed, 40.0f, 70.0f, FVector3f(0.0f, 0.0f, -70.0f));
                    const auto V0    = FVector3f(0.0f, 0.0f, FMath::Lerp(150.0f, 300.0f, Rand(InSeed, 2)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.25f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0350136f, 1.0f,      0.200423f, 0.109462f, 0.776336f, 0.0241576f),
                        Key3(t, 0.0350136f, 0.854993f, 0.200423f, 1.0f,      0.776336f, 0.913099f),
                        Key3(t, 0.0350136f, 0.376262f, 0.200423f, 0.617207f, 0.776336f, 0.141263f),
                        0.7f);

                    Out.Size = FVector2f(FMath::Lerp(80.0f, 100.0f, Rand(InSeed, 8)),
                                         FMath::Lerp(350.0f, 400.0f, Rand(InSeed, 3)));

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key3(t, 0.0f, -1.0f, 0.2f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart07;
                }
                break;
            }
            case 32: // DebuffCast — Vefects NS_DebuffCast. Mirrors Behavior_DebuffCast.ush; recipe Cookbook/NS_DebuffCast.md.
            {
                constexpr auto Loop         = 2.0f;
                constexpr auto BurstSlots   = 30;
                constexpr auto RateTotal    = 65.0f;
                constexpr auto CumSparkDark = 20.0f;
                constexpr auto CumRing      = 25.0f;
                constexpr auto CumSparkBr   = 45.0f;

                constexpr auto Window = 0.3f;

                constexpr auto LayerBigArrow  = 0;
                constexpr auto LayerSparkDark = 1;
                constexpr auto LayerRing      = 2;
                constexpr auto LayerSparkBr   = 3;
                constexpr auto LayerFlames    = 4;
                constexpr auto LayerSlash     = 5;

                constexpr auto VisArrows   = 113;
                constexpr auto VisPart01Br = 114;
                constexpr auto VisRing01   = 115;
                constexpr auto VisPart03Br = 116;
                constexpr auto VisFlames   = 117;
                constexpr auto VisSlash    = 118;

                // The Curl Noise Force conversion. Read Behavior_DebuffCast.ush's header for the derivation:
                // the frequency is the source's own 15 in metres, and the strength is its 2500 acceleration
                // crushed by the layer's Scale Velocity plateau, expressed as the equal-ground velocity and
                // divided by this Fbm's measured mean field magnitude.
                constexpr auto CurlFreq         = 0.015f;
                constexpr auto CurlSrcStrength  = 2500.0f;
                constexpr auto CurlVelPlateau   = 0.1f;
                constexpr auto CurlFieldMean    = 0.736f;
                constexpr auto CurlSeed         = 11u;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto CurlOffset = [](FVector3f InSpawn, float InAgeSeconds, float InLife) -> FVector3f
                {
                    const auto Strength = 0.5f * CurlSrcStrength * CurlVelPlateau * InLife / CurlFieldMean;
                    return CurlPath(InSpawn, InAgeSeconds, CurlFreq, Strength, CurlSeed) - InSpawn;
                };

                const auto RandomRangeRgb = [](int32 InS, FVector3f InMin, FVector3f InMax) -> FVector3f
                {
                    return FMath::Lerp(InMin, InMax, Rand(InS, 13));
                };

                const auto BurstLayer = [](int32 InS) -> int32
                {
                    const auto S = ((InS % BurstSlots) + BurstSlots) % BurstSlots;

                    if (S == 0) { return LayerBigArrow; }
                    if (S < 8)  { return LayerSparkDark; }
                    if (S < 11) { return LayerRing; }
                    if (S < 18) { return LayerSparkBr; }
                    if (S < 23) { return LayerFlames; }
                    return LayerSlash;
                };

                const auto RateLayer = [](int32 InS) -> int32
                {
                    const auto R = Rand(InS, 0) * RateTotal;

                    if (R < CumSparkDark) { return LayerSparkDark; }
                    if (R < CumRing)      { return LayerRing; }
                    if (R < CumSparkBr)   { return LayerSparkBr; }
                    return LayerSlash;
                };

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                const auto Phase   = SpawnPhase(InEmitterAge, InAge, Loop);
                const auto IsBurst = IsBurstSpawn(Phase, Loop);

                auto Layer = LayerBigArrow;

                if (IsBurst)
                {
                    Layer = BurstLayer(InSeed);
                }
                else
                {
                    Layer = RateLayer(InSeed);

                    if (Phase > Window)
                    {
                        Hide();
                        break;
                    }
                }

                if (Layer == LayerBigArrow)
                {
                    if (InAge > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t  = Saturate(InAge / 1.5f);
                    const auto V0 = FVector3f(0.0f, 0.0f, -150.0f);

                    Out.Position = FVector3f(0.0f, 0.0f, 50.0f) + V0 * (Int3(t, 0.3f, 1.0f, 0.05f, 0.05f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.3f, 0.05f, 1.0f, 0.05f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0f, 0.00233049f, 0.460006f, 0.00116306f, 1.0f, 0.00182116f),
                        Key3(t, 0.0f, 0.00139829f, 0.460006f, 0.00132921f, 1.0f, 0.00182116f),
                        Key3(t, 0.0f, 0.005f,      0.460006f, 0.005f,      1.0f, 0.00212469f),
                        Key4(t, 0.0f, 0.0f, 0.15575f, 1.0f, 0.557803f, 1.0f, 1.0f, 0.0f));

                    Out.Size   = FVector2f(170.0f, 240.0f);
                    Out.VisTag = VisArrows;
                    break;
                }

                if (Layer == LayerSparkDark)
                {
                    const auto Life = FMath::Lerp(1.0f, 1.5f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Dir   = RandDir(InSeed);
                    const auto Spawn = Dir * 200.0f;
                    const auto V0    = Dir * -700.0f;

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.1f, 0.0f) * Life)
                                 + CurlOffset(Spawn, InAge, Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.1f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.113492f, 0.00102734f, 0.385149f, 0.000465223f),
                        Key2(t, 0.113492f, 0.002f,      0.385149f, 0.000531684f),
                        Key2(t, 0.113492f, 0.00105217f, 0.385149f, 0.002f),
                        Key3(t, 0.0f, 0.0f, 0.0857229f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(7.0f, 20.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (Layer == LayerRing)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.7f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / Life);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);
                    const auto Base = FMath::Lerp(220.0f, 250.0f, Rand(InSeed, 8));

                    Out.Color = FLinearColor(0.00182116f, 0.00182116f, 0.00212469f,
                                             0.45f * Key3(t, 0.0f, 0.0f, 0.499849f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(Base * Grow, Base * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -0.5f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                    break;
                }

                if (Layer == LayerSparkBr)
                {
                    const auto Life = FMath::Lerp(1.0f, 1.5f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t     = Saturate(InAge / Life);
                    const auto Dir   = RandDir(InSeed);
                    const auto Spawn = Dir * (5.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = Dir * FMath::Lerp(300.0f, 1000.0f, Rand(InSeed, 2));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.1f, 0.0f) * Life)
                                 + CurlOffset(Spawn, InAge, Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.1f, 1.0f, 0.0f);

                    const auto Rgb = RandomRangeRgb(InSeed,
                        FVector3f(0.093059f, 0.181164f, 0.0953075f), FVector3f(0.111932f, 0.0409152f, 0.3564f));

                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z, Key3(t, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(20.0f, 70.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.5f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.VisTag   = VisPart03Br;
                    break;
                }

                if (Layer == LayerFlames)
                {
                    const auto Life = FMath::Lerp(1.0f, 2.0f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    Out.Position = RandDir(InSeed) * 20.0f;

                    Out.Color = FLinearColor(
                        Key3(t, 0.113492f, 0.175111f, 0.545729f, 0.0144f, 0.984002f, 0.00560539f),
                        Key3(t, 0.113492f, 0.250158f, 0.545729f, 0.0144f, 0.984002f, 0.00802319f),
                        Key3(t, 0.113492f, 0.175111f, 0.545729f, 0.048f,  0.984002f, 0.00560539f),
                        1.0f);

                    const auto Base = FMath::Lerp(200.0f, 300.0f, Rand(InSeed, 8));
                    const auto Grow = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f
                                 + FMath::Lerp(-45.0f, 45.0f, Rand(InSeed, 5)) * InAge;

                    const auto Start = FMath::FloorToFloat(Rand(InSeed, 6) * 4.0f);
                    Out.SubImageIndex = FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(t * 4.0f), 3.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 10.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisFlames;
                    break;
                }

                {
                    const auto Life = FMath::Lerp(1.0f, 1.5f, Rand(InSeed, 1));

                    if (InAge > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / Life);

                    Out.Orientation = QuatFromZTo(RandDir(InSeed));
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    Out.Scale = FVector3f(FMath::Lerp(0.5f, 1.5f, Rand(InSeed, 3)),
                                          1.0f,
                                          FMath::Lerp(0.5f, 1.5f, Rand(InSeed, 5)));

                    const auto Rgb = RandomRangeRgb(InSeed,
                        FVector3f(0.0154102f, 0.03f, 0.0157825f), FVector3f(0.00942192f, 0.00344404f, 0.03f));

                    Out.Color = FLinearColor(Rgb.X, Rgb.Y, Rgb.Z,
                                             0.45f * Key3(t, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0.0f));

                    Out.Dynamic = FVector4f(0.1f, 0.0f, Key3(t, 0.0f, -0.5f, 0.3f, 0.3f, 1.0f, 0.5f), 0.0f);
                    Out.VisTag  = VisSlash;
                }
                break;
            }
            case 33: // GunshotCast — Vefects NS_Gunshot_Cast. Mirrors Behavior_GunshotCast.ush; recipe Cookbook/NS_Gunshot_Cast.md.
            {
                constexpr auto NumLayers      = 40;
                constexpr auto FirstSpark02   = 7;
                constexpr auto FirstGlow04    = 10;
                constexpr auto FirstGlow05    = 15;
                constexpr auto FirstSpike     = 18;
                constexpr auto LayerStrip     = 21;
                constexpr auto LayerStar01    = 22;
                constexpr auto LayerWindMesh  = 23;
                constexpr auto FirstWindStrk  = 30;
                constexpr auto LayerImpact    = 32;

                constexpr auto DelayEarly = 0.04f;
                constexpr auto DelayLate  = 0.05f;

                constexpr auto VisPart01     = 119;
                constexpr auto VisPart02     = 120;
                constexpr auto VisPart04     = 121;
                constexpr auto VisPart03Br   = 122;
                constexpr auto VisSpike      = 123;
                constexpr auto VisStrip      = 124;
                constexpr auto VisStar01     = 125;
                constexpr auto VisWindMesh   = 126;
                constexpr auto VisWindPuff   = 127;
                constexpr auto VisWindStreak = 128;
                constexpr auto VisImpact     = 129;
                constexpr auto VisPart01Br   = 130;

                constexpr auto Tau = 6.28318530718f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto HemiXPoint = [](int32 InS, float InRadius) -> FVector3f
                {
                    auto Dir = RandDir(InS);
                    Dir.X = FMath::Abs(Dir.X);
                    return Dir * (InRadius * 0.1f * FMath::Pow(Rand(InS, 7), 1.0f / 3.0f));
                };

                const auto WindAlpha = [](float InT) -> float
                {
                    return Key4(InT, 0.0f, 0.0f, 0.240266f, 1.0f, 0.676124f, 1.0f, 1.0f, 0.0f);
                };

                const auto SubImage = [](int32 InS, float InT, float InSteps) -> float
                {
                    const auto Start = FMath::FloorToFloat(Rand(InS, 6) * 4.0f);
                    return FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(InT * InSteps), InSteps - 1.0f), 4.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == 0)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 1.0f, 0.415334f, 0.693872f, 0.810142f, 0.0409152f),
                        Key3(t, 0.0f, 1.0f, 0.415334f, 0.147027f, 0.810142f, 0.0451862f),
                        Key2(t, 0.0f, 1.0f, 1.0f, 0.0f) * 0.3f);
                    Out.Size    = FVector2f(700.0f * Grow, 700.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == 1)
                {
                    if (InAge > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color  = FLinearColor(1.0f, 0.850349f, 0.329f, Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size   = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.VisTag = VisPart01;
                    break;
                }

                if (Layer < FirstSpark02)
                {
                    const auto td = InAge - DelayEarly;

                    if (td < 0.0f || td > 0.05f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.05f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.0f, 1.0f, 0.437066f, 0.945625f),
                        Key2(t, 0.0f, 1.0f, 0.437066f, 0.643f),
                        1.0f);
                    Out.Size   = FVector2f(150.0f, 150.0f);
                    Out.VisTag = VisPart02;
                    break;
                }

                if (Layer < FirstGlow04)
                {
                    const auto Life = FMath::Lerp(0.1f, 0.2f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto V0 = FVector3f(
                        FMath::Lerp(2000.0f, 7000.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-100.0f,  100.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-100.0f,  100.0f, Rand(InSeed, 5)));

                    Out.Position = HemiXPoint(InSeed, 100.0f) + V0 * (Int3(t, 0.2f, 1.0f, 0.35f, 0.05f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.35f, 1.0f, 0.05f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f,      0.297012f, 1.0f,       0.671295f, 1.0f,       0.910353f, 0.381326f),
                        Key4(t, 0.0f, 0.913099f, 0.297012f, 0.493097f,  0.671295f, 0.24027f,   0.910353f, 0.042927f),
                        Key4(t, 0.0f, 0.584079f, 0.297012f, 0.0409999f, 0.671295f, 0.0839999f, 0.910353f, 0.0366073f),
                        Key2(t, 0.243888f, 1.0f, 1.0f, 0.0f));

                    const auto Width  = FMath::Lerp( 20.0f,  25.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(130.0f, 150.0f, Rand(InSeed, 12));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key2(t, 0.0f, 1.0f, 1.0f, 0.6f);

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper);
                    Out.VisTag = VisPart04;
                    break;
                }

                if (Layer < FirstGlow05)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 0.947307f, 0.414126f, 0.693872f, 0.926049f, 0.450786f),
                        Key3(t, 0.0f, 0.665387f, 0.414126f, 0.147027f, 0.926049f, 0.0409152f),
                        Key2(t, 0.0f, 1.0f, 0.992454f, 0.0f) * 0.3f);
                    Out.Size   = FVector2f(800.0f * Grow, 800.0f * Grow);
                    Out.VisTag = VisPart01;
                    break;
                }

                if (Layer < FirstSpike)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.315122f, 3.0f,     1.0f, 1.0f),
                        Key2(t, 0.315122f, 1.95267f, 1.0f, 0.637597f),
                        Key2(t, 0.315122f, 0.552f,   1.0f, 0.152926f),
                        Key2(t, 0.312708f, 1.0f, 0.992454f, 0.0f));
                    Out.Size    = FVector2f(250.0f * Grow, 250.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart03Br;
                    break;
                }

                if (Layer < LayerStrip)
                {
                    const auto Life = FMath::Lerp(0.1f, 0.15f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto V0 = FVector3f(
                        FMath::Lerp(10.0f, 50.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-10.0f, 10.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-10.0f, 10.0f, Rand(InSeed, 5)));

                    Out.Position = HemiXPoint(InSeed, 10.0f) + V0 * td;
                    Out.Velocity = V0;

                    Out.Orientation = QuatFromZTo(V0 / V0.Size());
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(0.2f, 0.2f, FMath::Lerp(0.4f, 0.7f, Rand(InSeed, 4)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.5f, 1.0f, 0.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.4f, 1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 0.2f, 1.0f));

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 0.715694f, 0.0941745f, 1.0f,      0.21974f, 1.0f,      0.440688f, 0.854993f,  0.707516f, 0.0103298f),
                        Key5(t, 0.0f, 0.89627f,  0.0941745f, 0.752942f, 0.21974f, 0.341915f, 0.440688f, 0.135633f,  0.707516f, 0.00749903f),
                        Key5(t, 0.0f, 1.0f,      0.0941745f, 0.109462f, 0.21974f, 0.109462f, 0.440688f, 0.0512695f, 0.707516f, 0.00604883f),
                        1.0f);
                    Out.VisTag = VisSpike;
                    break;
                }

                if (Layer == LayerStrip)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.2f);

                    const auto V0 = FVector3f(0.1f, 0.0f, 0.0f);
                    Out.Position = FVector3f(354.572f, 0.0f, 0.0f) + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 0.2f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.319952f, 0.366253f, 1.0f, 0.450786f),
                        Key2(t, 0.319952f, 0.184475f, 1.0f, 0.040915f),
                        Key2(t, 0.327196f, 1.0f, 1.0f, 0.0f) * 0.1f);

                    Out.Size   = FVector2f(100.0f * Key3(t, 0.0f, 0.5f, 0.2f, 1.0f, 1.0f, 0.4f), 800.0f);
                    Out.VisTag = VisStrip;
                    break;
                }

                if (Layer == LayerStar01)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.4f, 1.0f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key3(t, 0.0f, 1.0f, 0.46725f, 0.693872f, 1.0f, 0.450786f),
                        Key3(t, 0.0f, 1.0f, 0.46725f, 0.147027f, 1.0f, 0.040915f),
                        1.0f);
                    Out.Size    = FVector2f(20.0f * Grow, 20.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisStar01;
                    break;
                }

                if (Layer == LayerWindMesh)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(150.0f, 0.0f, 0.0f);
                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.25f, 0.1f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, 0.1f);

                    const auto Spin = QuatFromAxisAngle(FVector3f(1.0f, 0.0f, 0.0f), Tau * 0.3f * td);
                    const auto Lay  = QuatFromAxisAngle(FVector3f(0.0f, 1.0f, 0.0f), Tau * 0.25f);
                    Out.Orientation = QuatMul(Spin, Lay);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    constexpr auto Uniform = 0.3f;
                    Out.Scale = FVector3f(
                        Uniform * Key2(t, 0.0f, 1.5f, 0.4f, 2.0f),
                        Uniform * Key2(t, 0.0f, 1.5f, 0.4f, 2.0f),
                        Uniform * 5.0f * Key4(t, 0.0f, 0.5f, 0.4f, 1.5f, 0.7f, 2.625f, 1.0f, 3.0f));

                    Out.Color   = FLinearColor(0.846873f, 0.921582f, 1.0f, WindAlpha(t) * 0.02f);
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, -0.2f, 1.0f, -1.0f),
                                            Key2(t, 0.0f, 0.0f, 0.9f, 0.2f), 0.0f, 0.0f);
                    Out.VisTag  = VisWindMesh;
                    break;
                }

                if (Layer < FirstWindStrk)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(
                        FMath::Lerp(200.0f, 1000.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-20.0f,   20.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-20.0f,   20.0f, Rand(InSeed, 5)));

                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Base = FMath::Lerp(130.0f, 230.0f, Rand(InSeed, 8));
                    const auto Grow = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;

                    Out.Color = FLinearColor(0.846873f, 0.921582f, 1.0f, WindAlpha(t) * 0.15f);

                    Out.SubImageIndex = SubImage(InSeed, t, 4.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisWindPuff;
                    break;
                }

                if (Layer < LayerImpact)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(
                        FMath::Lerp(250.0f, 500.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-20.0f,  20.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-20.0f,  20.0f, Rand(InSeed, 5)));

                    Out.Position = FVector3f(70.0f, 0.0f, 0.0f) + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Width  = FMath::Lerp( 60.0f,  80.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(400.0f, 500.0f, Rand(InSeed, 12));
                    const auto Grow   = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Width * Grow, Length * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;

                    Out.Color = FLinearColor(0.846873f, 0.921582f, 1.0f, WindAlpha(t) * 0.08f);

                    Out.SubImageIndex = SubImage(InSeed, t, 5.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.5f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisWindStreak;
                    break;
                }

                if (Layer == LayerImpact)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.2f);

                    const auto V0 = FVector3f(
                        FMath::Lerp(250.0f, 500.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-20.0f,  20.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-20.0f,  20.0f, Rand(InSeed, 5)));

                    Out.Position = FVector3f(132.846f, 0.0f, 0.0f) + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 0.2f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 3.57847f, 0.094174f, 3.0f,      0.315122f, 1.0f,      0.447932f, 0.854993f, 0.606097f, 0.01033f),
                        Key5(t, 0.0f, 4.48135f, 0.094174f, 2.25883f,  0.315122f, 0.341915f, 0.447932f, 0.135633f, 0.606097f, 0.007499f),
                        Key5(t, 0.0f, 5.0f,     0.094174f, 0.328386f, 0.315122f, 0.109462f, 0.447932f, 0.051269f, 0.606097f, 0.006049f),
                        1.0f);

                    const auto Width  = FMath::Lerp(100.0f, 120.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(170.0f, 200.0f, Rand(InSeed, 12));
                    Out.Size = FVector2f(Width * Key3(t, 0.0f, 0.5f, 0.2f, 1.0f, 1.0f, 0.4f), Length);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;

                    Out.SubImageIndex = SubImage(InSeed, t, 5.0f);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisImpact;
                    break;
                }

                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto V0 = FVector3f(
                        FMath::Lerp(500.0f, 4000.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-50.0f,   50.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-50.0f,   50.0f, Rand(InSeed, 5)));

                    Out.Position = HemiXPoint(InSeed, 100.0f) + V0 * (Int3(t, 0.1f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.1f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.562632f, 1.0f,      0.997283f, 0.112f),
                        Key2(t, 0.562632f, 0.603828f, 0.997283f, 0.0676287f),
                        Key2(t, 0.562632f, 0.296138f, 0.997283f, 0.0331675f),
                        Key2(t, 0.562632f, 1.0f, 1.0f, 0.0f) * 0.15f);

                    const auto Base = FMath::Lerp(6.0f, 10.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Dynamic = FVector4f(3.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01Br;
                }
                break;
            }
            case 34: // FireBallCast — Vefects NS_FireBall_Cast. Mirrors Behavior_FireBallCast.ush; recipe Cookbook/NS_FireBall_Cast.md.
            {
                constexpr auto NumLayers        = 50;
                constexpr auto LayerGlow01      = 12;
                constexpr auto LayerFlareS01    = 16;
                constexpr auto LayerFlareS02    = 17;
                constexpr auto LayerFlareS03    = 18;
                constexpr auto LayerFlareS04    = 19;
                constexpr auto LayerStar02      = 20;
                constexpr auto LayerStar01      = 21;
                constexpr auto LayerGlow02      = 22;
                constexpr auto LayerFirstGlow   = 23;
                constexpr auto LayerSecondGlow  = 24;
                constexpr auto LayerShoot01     = 25;
                constexpr auto LayerShoot02     = 26;
                constexpr auto LayerSecond01    = 31;
                constexpr auto LayerSecond02    = 32;
                constexpr auto LayerWindMesh    = 33;
                constexpr auto LayerStrip       = 39;
                constexpr auto FirstFlames      = 43;
                constexpr auto FirstSmokes      = 47;
                constexpr auto LayerFlare01     = 49;

                constexpr auto DelayRelease   = 0.5f;
                constexpr auto DelayTransient = 0.54f;
                constexpr auto DelayLate      = 0.55f;

                constexpr auto VisPart01    = 131;
                constexpr auto VisPart01Br  = 132;
                constexpr auto VisPart03Br  = 133;
                constexpr auto VisPart04    = 134;
                constexpr auto VisRainbow   = 135;
                constexpr auto VisRing01    = 136;
                constexpr auto VisStar01    = 137;
                constexpr auto VisStar02    = 138;
                constexpr auto VisStar03    = 139;
                constexpr auto VisWindPuff  = 140;
                constexpr auto VisWindMesh  = 141;
                constexpr auto VisStrip     = 142;
                constexpr auto VisSpike     = 143;
                constexpr auto VisFlames    = 144;
                constexpr auto VisSmoke     = 145;
                constexpr auto VisFlare01   = 146;

                constexpr auto Tau = 6.28318530718f;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto Key6 = [](float InT, float InT0, float InV0, float InT1, float InV1, float InT2, float InV2,
                                     float InT3, float InV3, float InT4, float InV4, float InT5, float InV5) -> float
                {
                    return InT <= InT2 ? Key3(InT, InT0, InV0, InT1, InV1, InT2, InV2)
                                       : Key4(InT, InT2, InV2, InT3, InV3, InT4, InV4, InT5, InV5);
                };

                const auto HemiXPoint = [](int32 InS, float InRadius) -> FVector3f
                {
                    auto Dir = RandDir(InS);
                    Dir.X = FMath::Abs(Dir.X);
                    return Dir * (InRadius * 0.1f * FMath::Pow(Rand(InS, 7), 1.0f / 3.0f));
                };

                const auto SubImage = [](int32 InS, float InT) -> float
                {
                    const auto Start = FMath::FloorToFloat(Rand(InS, 6) * 4.0f);
                    return FMath::Fmod(Start + FMath::Min(FMath::FloorToFloat(InT * 4.0f), 3.0f), 4.0f);
                };

                const auto Layer = ((InSeed % NumLayers) + NumLayers) % NumLayers;

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                if (Layer == 0)
                {
                    const auto td = InAge - DelayRelease;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.4565495f, 0.4565495f, 0.4565495f,
                                                0.2f * Key3(t, 0.0f, 0.0f, 0.328403f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(354.039f * Grow, 354.039f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRainbow;
                    break;
                }

                if (Layer == 1)
                {
                    const auto td = InAge - DelayRelease;

                    if (td < 0.0f || td > 0.4f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.4f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.118322f, 1.0f,      0.295804f, 1.0f,      0.542107f, 0.391573f, 0.843948f, 0.009134f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.693872f, 0.295804f, 0.040915f, 0.542107f, 0.003677f, 0.843948f, 0.004025f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.147027f, 0.295804f, 0.045186f, 0.542107f, 0.022174f, 0.843948f, 0.006995f),
                        0.5f);
                    Out.Size     = FVector2f(100.0f * Grow, 100.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                    break;
                }

                if (Layer < LayerGlow01)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto Spawn = RandDir(InSeed) * (20.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = FVector3f(
                        FMath::Lerp( 500.0f, 2500.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-300.0f,  300.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-300.0f,  300.0f, Rand(InSeed, 5)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(1.0f, 0.563224f, 0.224f, Key2(t, 0.613341f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(10.0f, 20.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (Layer == LayerGlow01)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0881376f, 1.0f,      0.377905f, 1.0f,       0.9345f, 0.391573f),
                        Key3(t, 0.0881376f, 0.693872f, 0.377905f, 0.0409152f, 0.9345f, 0.00367651f),
                        Key3(t, 0.0881376f, 0.147027f, 0.377905f, 0.0451862f, 0.9345f, 0.0221739f),
                        0.3f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer < LayerFlareS01)
                {
                    const auto Life = FMath::Lerp(0.1f, 0.2f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto Spawn = RandDir(InSeed) * (10.0f * FMath::Pow(Rand(InSeed, 7), 1.0f / 3.0f));
                    const auto V0    = FVector3f(
                        FMath::Lerp(3000.0f, 5000.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-800.0f,  800.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-800.0f,  800.0f, Rand(InSeed, 5)));

                    Out.Position = Spawn + V0 * (Int3(t, 0.2f, 1.0f, 0.25f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f, 0.088138f, 1.0f,      0.615756f, 1.0f,       0.98038f, 0.391573f),
                        Key4(t, 0.0f, 1.0f, 0.088138f, 0.671611f, 0.615756f, 0.025f,     0.98038f, 0.003677f),
                        Key4(t, 0.0f, 1.0f, 0.088138f, 0.085f,    0.615756f, 0.0293413f, 0.98038f, 0.022174f),
                        0.6f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));

                    const auto Width  = FMath::Lerp(25.0f, 40.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(70.0f, 60.0f, Rand(InSeed, 12));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.3f, 0.25f, 1.0f, 0.2f);

                    const auto SpeedFactor = FMath::Lerp(1.0f, 2.0f, Saturate(Out.Velocity.Size() / 1000.0f));

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper * SpeedFactor);
                    Out.VisTag = VisPart04;
                    break;
                }

                if (Layer <= LayerFlareS04)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t      = Saturate(InAge / 0.5f);
                    const auto Alpha  = Key2(t, 0.225777f, 1.0f, 1.0f, 0.0f);
                    const auto ScaleX = Key2(t, 0.0f, 1.0f, 0.9f, 0.0f);

                    if (Layer == LayerFlareS01)
                    {
                        Out.Color  = FLinearColor(1.0f, 0.111932f, 0.0343398f, Alpha);
                        Out.Size   = FVector2f(600.0f * ScaleX, 100.0f * Key2(t, 0.0f, 0.3f, 0.2f, 1.0f));
                        Out.VisTag = VisPart01;
                    }
                    else if (Layer == LayerFlareS02)
                    {
                        Out.Color  = FLinearColor(1.0f, 0.571125f, 0.0822827f, Alpha);
                        Out.Size   = FVector2f(800.0f * ScaleX, 200.0f * Key3(t, 0.0f, 0.3f, 0.2f, 1.0f, 0.9f, 0.2f));
                        Out.VisTag = VisPart03Br;
                    }
                    else if (Layer == LayerFlareS03)
                    {
                        Out.Color  = FLinearColor(0.323143f, 0.00560539f, 0.00802319f, Alpha);
                        Out.Size   = FVector2f(1300.0f * ScaleX, 200.0f * Key2(t, 0.0f, 0.3f, 0.2f, 1.0f));
                        Out.VisTag = VisPart03Br;
                    }
                    else
                    {
                        Out.Color  = FLinearColor(1.0f, 0.854993f, 0.508881f, Alpha);
                        Out.Size   = FVector2f(700.0f * ScaleX, 60.0f * Key3(t, 0.0f, 0.3f, 0.2f, 1.0f, 0.9f, 0.0999999f));
                        Out.VisTag = VisStar03;
                    }

                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    break;
                }

                if (Layer == LayerStar02 || Layer == LayerStar01)
                {
                    const auto IsStar02 = Layer == LayerStar02;
                    const auto Delay    = IsStar02 ? DelayRelease : DelayLate;
                    const auto Life     = IsStar02 ? 0.2f : 0.3f;
                    const auto td       = InAge - Delay;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    if (IsStar02)
                    {
                        Out.Color = FLinearColor(
                            Key4(t, 0.0f, 2.0f, 0.220948f, 1.0f,      0.775128f, 1.0f,      1.0f, 0.391573f),
                            Key4(t, 0.0f, 2.0f, 0.220948f, 0.714319f, 0.775128f, 0.040915f, 1.0f, 0.003677f),
                            Key4(t, 0.0f, 2.0f, 0.220948f, 0.204f,    0.775128f, 0.045186f, 1.0f, 0.022174f),
                            1.0f);
                        const auto Grow = Key2(t, 0.0f, 1.0f, 1.0f, 0.0f);
                        Out.Size     = FVector2f(150.0f * Grow, 150.0f * Grow);
                        Out.Rotation = 0.0f;
                        Out.VisTag   = VisStar02;
                    }
                    else
                    {
                        Out.Color = FLinearColor(
                            Key4(t, 0.0f, 2.0f, 0.234229f, 1.0f,      0.775128f, 1.0f,       1.0f, 0.391573f),
                            Key4(t, 0.0f, 2.0f, 0.234229f, 0.701399f, 0.775128f, 0.0509999f, 1.0f, 0.003677f),
                            Key4(t, 0.0f, 2.0f, 0.234229f, 0.168f,    0.775128f, 0.0552256f, 1.0f, 0.022174f),
                            1.0f);
                        const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                        Out.Size     = FVector2f(80.0f * Grow, 80.0f * Grow);
                        Out.Rotation = 0.1f;
                        Out.VisTag   = VisStar01;
                    }
                    break;
                }

                if (Layer == LayerGlow02)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.5f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.214911f, 0.693872f, 0.817386f, 0.0409152f),
                        Key2(t, 0.214911f, 0.147027f, 0.817386f, 0.0451862f),
                        0.5f * Key2(t, 0.406882f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(300.0f * Grow, 300.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == LayerFirstGlow)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(InAge / 0.5f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0301841f, 0.00518152f, 0.1147f, 0.00913406f, 0.226985f, 0.913099f, 0.618171f, 1.0f),
                        Key6(t, 0.0301841f, 0.00518152f, 0.1147f, 0.00402472f, 0.226985f, 0.0241576f,
                                0.618171f,  0.341915f,   0.792031f, 0.752942f, 1.0f,      0.938686f),
                        Key6(t, 0.0301841f, 0.00913406f, 0.1147f, 0.00699541f, 0.226985f, 0.0241576f,
                                0.618171f,  0.109462f,   0.792031f, 0.109462f, 1.0f,      0.791298f),
                        0.3f * Key2(t, 0.0f, 0.0f, 0.154543f, 1.0f));

                    Out.Size    = FVector2f(1000.0f * Key2(t, 0.0f, 1.0f, 1.0f, 0.4f),
                                            1000.0f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == LayerSecondGlow)
                {
                    if (InAge > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(InAge / 0.5f);
                    const auto Grow = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        1.0f,
                        Key2(t, 0.618171f, 0.341915f, 0.963477f, 0.752942f),
                        0.109462f,
                        0.5f * Key2(t, 0.0f, 0.0f, 0.246302f, 1.0f));
                    Out.Size    = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Dynamic = FVector4f(2.69821f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart03Br;
                    break;
                }

                if (Layer == LayerShoot01 || Layer == LayerShoot02)
                {
                    const auto td = InAge - DelayRelease;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    if (Layer == LayerShoot01)
                    {
                        Out.Color = FLinearColor(
                            Key3(t, 0.0410504f, 1.0f,      0.417748f, 1.0f,       0.9345f, 0.391573f),
                            Key3(t, 0.0410504f, 0.693872f, 0.417748f, 0.0409152f, 0.9345f, 0.00367651f),
                            Key3(t, 0.0410504f, 0.147027f, 0.417748f, 0.0451862f, 0.9345f, 0.0221739f),
                            0.3f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                        Out.Size = FVector2f(1000.0f * Grow, 1000.0f * Grow);
                    }
                    else
                    {
                        Out.Color = FLinearColor(
                            Key2(t, 0.004829f, 0.318547f, 0.10987f, 1.0f),
                            Key4(t, 0.004829f, 0.955974f, 0.10987f, 1.0f, 0.214911f, 0.693872f, 0.817386f, 0.040915f),
                            Key4(t, 0.004829f, 1.0f,      0.10987f, 1.0f, 0.214911f, 0.147027f, 0.817386f, 0.045186f),
                            0.7f * Key2(t, 0.406882f, 1.0f, 1.0f, 0.0f));
                        Out.Size = FVector2f(300.0f * Grow, 300.0f * Grow);
                    }

                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer < LayerSecond01)
                {
                    const auto td = InAge - DelayTransient;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0374283f, 0.715694f, 0.16058f, 1.0f,      0.312708f, 1.0f,      0.659221f, 0.913099f,  0.96227f, 0.0137021f),
                        Key5(t, 0.0374283f, 0.89627f,  0.16058f, 0.752942f, 0.312708f, 0.341915f, 0.659221f, 0.0241576f, 0.96227f, 0.00182116f),
                        Key5(t, 0.0374283f, 1.0f,      0.16058f, 0.109462f, 0.312708f, 0.109462f, 0.659221f, 0.0241576f, 0.96227f, 0.00802319f),
                        0.6f);
                    Out.Size    = FVector2f(400.0f * Grow, 400.0f * Grow);
                    Out.Dynamic = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart03Br;
                    break;
                }

                if (Layer == LayerSecond01)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.2f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0881376f, 1.0f,      0.377905f, 1.0f,       0.9345f, 0.391573f),
                        Key3(t, 0.0881376f, 0.693872f, 0.377905f, 0.0409152f, 0.9345f, 0.00367651f),
                        Key3(t, 0.0881376f, 0.147027f, 0.377905f, 0.0451862f, 0.9345f, 0.0221739f),
                        0.3f * Key2(t, 0.0f, 1.0f, 1.0f, 0.0f));
                    Out.Size    = FVector2f(900.0f * Grow, 900.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01;
                    break;
                }

                if (Layer == LayerSecond02)
                {
                    const auto td = InAge - DelayRelease;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.118322f, 1.0f,      0.295804f, 1.0f,      0.542107f, 0.391573f,  0.860851f, 0.00913406f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.693872f, 0.295804f, 0.103f,    0.542107f, 0.0258438f, 0.860851f, 0.00402472f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.147027f, 0.295804f, 0.106994f, 0.542107f, 0.043284f,  0.860851f, 0.00699541f),
                        Key3(t, 0.0f, 1.0f, 0.305463f, 1.0f, 0.993661f, 0.0f));
                    Out.Size    = FVector2f(70.0f * Grow, 70.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01Br;
                    break;
                }

                if (Layer == LayerWindMesh)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(-200.0f, 0.0f, 0.0f);
                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    const auto Spin = QuatFromAxisAngle(FVector3f(1.0f, 0.0f, 0.0f), Tau * 0.3f * td);
                    const auto Lay  = QuatFromAxisAngle(FVector3f(0.0f, 1.0f, 0.0f), Tau * 0.25f);
                    Out.Orientation = QuatMul(Spin, Lay);
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    constexpr auto Uniform = 0.3f;
                    Out.Scale = FVector3f(
                        Uniform * Key2(t, 0.0f, 1.5f, 0.2f, 2.0f),
                        Uniform * Key2(t, 0.0f, 1.5f, 0.2f, 2.0f),
                        Uniform * 5.0f * Key4(t, 0.0f, 0.5f, 0.4f, 3.0f, 0.7f, 4.75f, 1.0f, 5.0f));

                    Out.Color   = FLinearColor(0.00151763f, 0.00151763f, 0.00151763f,
                                               0.7f * Key4(t, 0.0f, 0.0f, 0.240266f, 1.0f, 0.676124f, 1.0f, 1.0f, 0.0f));
                    Out.Dynamic = FVector4f(Key2(t, 0.0f, -0.2f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisWindMesh;
                    break;
                }

                if (Layer < LayerStrip)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 1.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 1.5f);

                    const auto V0 = FVector3f(
                        FMath::Lerp(-100.0f, -700.0f, Rand(InSeed, 2)),
                        FMath::Lerp( -20.0f,   20.0f, Rand(InSeed, 3)),
                        FMath::Lerp( -20.0f,   20.0f, Rand(InSeed, 5)));

                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.25f, 0.0f) * 1.5f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.25f, 1.0f, 0.0f);

                    const auto Base = FMath::Lerp(130.0f, 230.0f, Rand(InSeed, 8));
                    const auto Grow = Key2(t, 0.0f, 0.5f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;

                    Out.Color = FLinearColor(
                        Key2(t, 0.0f, 0.0103298f,  0.75581f, 0.00518152f),
                        Key2(t, 0.0f, 0.00273174f, 0.75581f, 0.00518152f),
                        Key2(t, 0.0f, 0.00242822f, 0.75581f, 0.00913406f),
                        Key2(t, 0.0205252f, 1.0f, 1.0f, 0.0f));

                    Out.SubImageIndex = SubImage(InSeed, t);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisWindPuff;
                    break;
                }

                if (Layer == LayerStrip)
                {
                    const auto td = InAge - DelayLate;

                    if (td < 0.0f || td > 0.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.2f);

                    const auto V0 = FVector3f(10.0f, 0.0f, 0.0f);
                    Out.Position = FVector3f(179.377f, 0.0f, 0.0f) + V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * 0.2f);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color  = FLinearColor(1.0f, 0.366253f, 0.184475f,
                                              0.3f * Key3(t, 0.0f, 0.0f, 0.274072f, 1.0f, 1.0f, 0.0f));
                    Out.Size   = FVector2f(50.0f, 400.0f);
                    Out.VisTag = VisStrip;
                    break;
                }

                if (Layer < FirstFlames)
                {
                    const auto Life = FMath::Lerp(0.1f, 0.15f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayLate;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto V0 = FVector3f(
                        FMath::Lerp(10.0f, 50.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-10.0f, 10.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-10.0f, 10.0f, Rand(InSeed, 5)));

                    Out.Position = HemiXPoint(InSeed, 10.0f) + V0 * td;
                    Out.Velocity = V0;

                    Out.Orientation = QuatFromZTo(V0 / V0.Size());
                    Out.MeshIndex   = 0;
                    Out.Size        = FVector2f(0.0f, 0.0f);

                    const auto Base = FVector3f(0.2f, 0.2f, FMath::Lerp(0.4f, 0.7f, Rand(InSeed, 4)));

                    Out.Scale = Base * FVector3f(
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.5f, 1.0f, 0.0f),
                        Key3(t, 0.0f, 0.0f, 0.2f, 0.4f, 1.0f, 0.0f),
                        Key2(t, 0.0f, 0.0f, 0.2f, 1.0f));

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f, 0.118322f, 1.0f,      0.568669f, 1.0f,      0.703894f, 0.391573f, 0.880169f, 0.009134f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.693872f, 0.568669f, 0.040915f, 0.703894f, 0.003677f, 0.880169f, 0.004025f),
                        Key5(t, 0.0f, 1.0f, 0.118322f, 0.147027f, 0.568669f, 0.045186f, 0.703894f, 0.022174f, 0.880169f, 0.006995f),
                        1.0f);
                    Out.VisTag = VisSpike;
                    break;
                }

                if (Layer < FirstSmokes)
                {
                    const auto Life = FMath::Lerp(0.2f, 0.7f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayRelease;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    auto Dir = RandDir(InSeed);
                    Dir.Z = FMath::Abs(Dir.Z);

                    const auto V0 = FVector3f(
                        FMath::Lerp( 100.0f, 700.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-100.0f, 100.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-100.0f, 100.0f, Rand(InSeed, 5)));

                    Out.Position = Dir * 20.0f + V0 * (Int2(t, 1.0f, 0.2f) * Life);
                    Out.Velocity = V0 * Key2(t, 0.0f, 1.0f, 1.0f, 0.2f);

                    Out.Color = FLinearColor(
                        Key3(t, 0.0796861f, 5.0f,      0.368246f, 3.0f,       0.738907f, 0.250158f),
                        Key3(t, 0.0796861f, 3.43343f,  0.368246f, 0.67227f,   0.738907f, 0.00749903f),
                        Key3(t, 0.0796861f, 0.115767f, 0.368246f, 0.0841786f, 0.738907f, 0.00749903f),
                        Key3(t, 0.0f, 0.0f, 0.303049f, 1.0f, 0.992454f, 0.0f));

                    const auto Base = FMath::Lerp(50.0f, 100.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f
                                 + FMath::Lerp(-30.0f, 30.0f, Rand(InSeed, 11)) * td;

                    Out.SubImageIndex = SubImage(InSeed, t);

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 5.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisFlames;
                    break;
                }

                if (Layer < LayerFlare01)
                {
                    const auto Life = FMath::Lerp(0.7f, 1.3f, Rand(InSeed, 1));
                    const auto td   = InAge - DelayRelease;

                    if (td < 0.0f || td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    const auto Dir = RandDir(InSeed);

                    const auto V0 = FVector3f(
                        FMath::Lerp( 100.0f, 500.0f, Rand(InSeed, 2)),
                        FMath::Lerp(-100.0f, 100.0f, Rand(InSeed, 3)),
                        FMath::Lerp(-100.0f, 100.0f, Rand(InSeed, 5)));

                    Out.Position = FVector3f(Dir.X, Dir.Y, 0.0f) * 20.0f + V0 * (Int3(t, 0.2f, 1.0f, 0.3f, 0.1f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.3f, 1.0f, 0.1f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f, 0.0603682f, 1.0f,      0.292182f, 0.391573f,   0.527618f, 0.0f),
                        Key4(t, 0.0f, 1.0f, 0.0603682f, 0.693872f, 0.292182f, 0.00367701f, 0.527618f, 0.0f),
                        Key4(t, 0.0f, 1.0f, 0.0603682f, 0.147027f, 0.292182f, 0.0221739f,  0.527618f, 0.0f),
                        0.3f * Key2(t, 0.126773f, 1.0f, 0.514337f, 0.35f));

                    const auto Base = FMath::Lerp(100.0f, 200.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f
                                 + FMath::Lerp(-30.0f, 30.0f, Rand(InSeed, 11)) * td;

                    Out.Dynamic = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f,
                                            Key2(t, 0.0f, -1.0f, 0.4f, 1.0f));
                    Out.VisTag  = VisSmoke;
                    break;
                }

                {
                    const auto td = InAge - DelayRelease;

                    if (td < 0.0f || td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f, 0.142469f, 1.0f,      0.47208f, 1.0f,      0.9345f, 0.391573f),
                        Key4(t, 0.0f, 1.0f, 0.142469f, 0.693872f, 0.47208f, 0.040915f, 0.9345f, 0.003677f),
                        Key4(t, 0.0f, 1.0f, 0.142469f, 0.147027f, 0.47208f, 0.045186f, 0.9345f, 0.022174f),
                        0.6f * Key3(t, 0.0f, 0.0f, 0.214911f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(100.0f * Grow, 100.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 1.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisFlare01;
                }
                break;
            }
            case 35: // LightningCast — Vefects NS_Lightning_Cast. Mirrors Behavior_LightningCast.ush; recipe Cookbook/NS_Lightning_Cast.md.
            {
                constexpr auto Loop         = 2.0f;
                constexpr auto BurstSlots   = 30;
                constexpr auto RateTotal    = 40.0f;
                constexpr auto CumStretch   = 20.0f;

                constexpr auto WindowStretch   = 0.4f;
                constexpr auto WindowLightning = 0.5f;

                constexpr auto DelayRing   = 0.05f;
                constexpr auto DelayFlare  = 0.1f;
                constexpr auto DelayStar01 = 0.85f;
                constexpr auto DelayStar02 = 0.95f;

                constexpr auto LayerGlow01    = 0;
                constexpr auto LayerGlow02    = 1;
                constexpr auto LayerGlow03    = 2;
                constexpr auto LayerRaimbow   = 3;
                constexpr auto LayerRing      = 4;
                constexpr auto LayerSparkles  = 5;
                constexpr auto LayerFlare01   = 6;
                constexpr auto LayerFlare02   = 7;
                constexpr auto LayerBigStar   = 8;
                constexpr auto LayerFlares01  = 9;
                constexpr auto LayerFlares02  = 10;
                constexpr auto LayerFlares03  = 11;
                constexpr auto LayerFlares04  = 12;
                constexpr auto LayerStar01    = 13;
                constexpr auto LayerStar02    = 14;
                constexpr auto LayerLightning = 15;
                constexpr auto LayerFlare03   = 16;
                constexpr auto LayerFlare04   = 17;
                constexpr auto LayerStretch   = 18;

                constexpr auto VisPart01    = 147;
                constexpr auto VisPart02    = 148;
                constexpr auto VisRainbow   = 149;
                constexpr auto VisRing01    = 150;
                constexpr auto VisPart01Br  = 151;
                constexpr auto VisPart04    = 152;
                constexpr auto VisStar02    = 153;
                constexpr auto VisPart03Br  = 154;
                constexpr auto VisStar03    = 155;
                constexpr auto VisLightning = 156;

                const auto Hide = [&Out]() -> void
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                };

                const auto SharedFade = [](float InT) -> float
                {
                    return Key2(InT, 0.0f, 1.0f, 1.0f, 0.0f);
                };

                const auto SharedGrow = [](float InT) -> float
                {
                    return Key3(InT, 0.0f, 0.5f, 0.1f, 1.0f, 1.0f, 1.0f);
                };

                const auto SpinIntegral = [](float InT) -> float
                {
                    const auto t = Saturate(InT);
                    const auto A = FMath::Min(t, 0.1f);
                    auto Sum = 0.5f * A * (90.0f * A / 0.1f);

                    if (t > 0.1f)
                    {
                        const auto B = FMath::Min(t, 0.9f);
                        Sum += 0.5f * (B - 0.1f) * (90.0f + 90.0f * (0.9f - B) / 0.8f);
                    }
                    return Sum;
                };

                const auto BurstLayer = [](int32 InS) -> int32
                {
                    const auto S = ((InS % BurstSlots) + BurstSlots) % BurstSlots;

                    if (S == 0)  { return LayerGlow01; }
                    if (S == 1)  { return LayerGlow02; }
                    if (S == 2)  { return LayerGlow03; }
                    if (S == 3)  { return LayerRaimbow; }
                    if (S == 4)  { return LayerRing; }
                    if (S < 15)  { return LayerSparkles; }
                    if (S == 15) { return LayerFlare01; }
                    if (S == 16) { return LayerFlare02; }
                    if (S == 17) { return LayerBigStar; }
                    if (S == 18) { return LayerFlares01; }
                    if (S == 19) { return LayerFlares02; }
                    if (S == 20) { return LayerFlares03; }
                    if (S == 21) { return LayerFlares04; }
                    if (S == 22) { return LayerStar01; }
                    if (S == 23) { return LayerStar02; }
                    if (S < 27)  { return LayerLightning; }
                    if (S < 29)  { return LayerFlare03; }
                    return LayerFlare04;
                };

                const auto RateLayer = [](int32 InS) -> int32
                {
                    return Rand(InS, 0) * RateTotal < CumStretch ? LayerStretch : LayerLightning;
                };

                Out.Velocity = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Position = FVector3f(0.0f, 0.0f, 0.0f);
                Out.Dynamic  = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);

                const auto Phase   = SpawnPhase(InEmitterAge, InAge, Loop);
                const auto IsBurst = IsBurstSpawn(Phase, Loop);

                auto Layer      = LayerGlow01;
                auto LayerDelay = 0.0f;

                if (IsBurst)
                {
                    Layer = BurstLayer(InSeed);

                    if (Layer == LayerRing || Layer == LayerSparkles)
                    { LayerDelay = DelayRing; }
                    else if (Layer == LayerRaimbow || Layer == LayerFlare01
                          || Layer == LayerFlare02 || Layer == LayerFlare03)
                    { LayerDelay = DelayFlare; }
                    else if (Layer == LayerStar01)
                    { LayerDelay = DelayStar01; }
                    else if (Layer == LayerStar02)
                    { LayerDelay = DelayStar02; }
                }
                else
                {
                    Layer = RateLayer(InSeed);

                    if (Phase > (Layer == LayerStretch ? WindowStretch : WindowLightning))
                    {
                        Hide();
                        break;
                    }

                    if (Layer == LayerLightning && Rand(InSeed, 14) >= 1.0f - Phase / WindowLightning)
                    {
                        Hide();
                        break;
                    }
                }

                const auto td = InAge - LayerDelay;

                if (td < 0.0f)
                {
                    Hide();
                    break;
                }

                if (Layer <= LayerGlow03)
                {
                    if (td > 1.0f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td);
                    const auto Grow = SharedGrow(t);
                    const auto A    = SharedFade(t);

                    if (Layer == LayerGlow01)
                    {
                        Out.Color   = FLinearColor(0.0648033f, 0.0307135f, 1.0f, A);
                        Out.Size    = FVector2f(550.0f * Grow, 550.0f * Grow);
                        Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                        Out.VisTag  = VisPart01;
                    }
                    else if (Layer == LayerGlow02)
                    {
                        Out.Color  = FLinearColor(0.863157f, 0.0262412f, 1.0f, A);
                        Out.Size   = FVector2f(200.0f * Grow, 200.0f * Grow);
                        Out.VisTag = VisPart02;
                    }
                    else
                    {
                        Out.Color   = FLinearColor(1.0f, 0.819765f, 0.499f, A);
                        Out.Size    = FVector2f(250.0f * Grow, 250.0f * Grow);
                        Out.Dynamic = FVector4f(2.0f, 0.0f, 0.0f, 0.0f);
                        Out.VisTag  = VisPart01;
                    }
                    break;
                }

                if (Layer == LayerRaimbow)
                {
                    if (td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.2f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.4565495f, 0.4565495f, 0.4565495f,
                                                0.1f * Key3(t, 0.0f, 0.0f, 0.328403f, 1.0f, 1.0f, 0.0f));
                    Out.Size     = FVector2f(350.0f * Grow, 350.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(0.5f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRainbow;
                    break;
                }

                if (Layer == LayerRing)
                {
                    if (td > 0.75f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.75f);
                    const auto Grow = Key3(t, 0.0f, 0.5f, 0.1f, 0.9f, 1.0f, 1.0f);

                    Out.Color    = FLinearColor(0.913099f, 0.191202f, 1.0f, 0.608f);
                    Out.Size     = FVector2f(150.0f * Grow, 150.0f * Grow);
                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(Key2(t, 0.0f, 0.0f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisRing01;
                    break;
                }

                if (Layer == LayerSparkles)
                {
                    const auto Life = FMath::Lerp(0.5f, 1.5f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t  = Saturate(td / Life);
                    const auto V0 = RandDir(InSeed) * FMath::Lerp(350.0f, 500.0f, Rand(InSeed, 2));

                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key2(t, 0.615756f, 0.50417f, 0.936915f, 0.0f),
                        Key2(t, 0.615756f, 0.104f,   0.936915f, 0.0896671f),
                        1.0f,
                        Key2(t, 0.613341f, 1.0f, 1.0f, 0.0f));

                    const auto Base = FMath::Lerp(10.0f, 20.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f;
                    Out.Dynamic  = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag   = VisPart01Br;
                    break;
                }

                if (Layer == LayerFlare01 || Layer == LayerFlare02)
                {
                    if (td > 0.5f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.5f);
                    const auto Grow = SharedGrow(t);
                    const auto A    = SharedFade(t);

                    Out.Color = Layer == LayerFlare01
                        ? FLinearColor(0.102242f, 0.658375f, 1.0f,      0.2f   * A)
                        : FLinearColor(0.102242f, 1.0f,      0.838799f, 0.258f * A);
                    Out.Size    = FVector2f(50.0f * Grow, 50.0f * Grow);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart02;
                    break;
                }

                if (Layer == LayerBigStar)
                {
                    if (td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.3f);
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);

                    Out.Color  = FLinearColor(1.0f, 0.184475f, 0.386429f, 0.4f);
                    Out.Size   = FVector2f(200.0f * Grow, 200.0f * Grow);
                    Out.VisTag = VisStar02;
                    break;
                }

                if (Layer <= LayerFlares04)
                {
                    if (td > 1.2f)
                    {
                        Hide();
                        break;
                    }

                    const auto t      = Saturate(td / 1.2f);
                    const auto A      = Key2(t, 0.225777f, 1.0f, 1.0f, 0.0f);
                    const auto ScaleX = Key2(t, 0.0f, 1.0f, 0.9f, 0.0f);

                    if (Layer == LayerFlares01)
                    {
                        Out.Color  = FLinearColor(0.491021f, 0.00182116f, 1.0f, 0.5f * A);
                        Out.Size   = FVector2f(700.0f * ScaleX, 100.0f * Key2(t, 0.0f, 0.3f, 0.2f, 1.0f));
                        Out.VisTag = VisPart01;
                    }
                    else if (Layer == LayerFlares02)
                    {
                        Out.Color  = FLinearColor(1.0f, 0.508881f, 0.982251f, 0.3f * A);
                        Out.Size   = FVector2f(700.0f * ScaleX, 100.0f * Key3(t, 0.0f, 0.3f, 0.2f, 1.0f, 0.9f, 0.2f));
                        Out.VisTag = VisPart03Br;
                    }
                    else if (Layer == LayerFlares03)
                    {
                        Out.Color  = FLinearColor(0.0185002f, 0.00402472f, 0.130136f, A);
                        Out.Size   = FVector2f(1400.0f * ScaleX, 180.0f * Key2(t, 0.0f, 0.3f, 0.2f, 1.0f));
                        Out.VisTag = VisPart03Br;
                    }
                    else
                    {
                        Out.Color  = FLinearColor(1.0f, 0.854993f, 0.508881f, 0.6f * A);
                        Out.Size   = FVector2f(400.0f * ScaleX, 70.0f * Key3(t, 0.0f, 0.3f, 0.2f, 1.0f, 0.9f, 0.0999999f));
                        Out.VisTag = VisStar03;
                    }

                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    break;
                }

                if (Layer == LayerStar01 || Layer == LayerStar02)
                {
                    if (td > 0.1f)
                    {
                        Hide();
                        break;
                    }

                    const auto t    = Saturate(td / 0.1f);
                    const auto Grow = Key2(t, 0.0f, 1.0f, 1.0f, 0.0f);

                    Out.Color    = FLinearColor(1.0f, 0.184475f, 0.386429f, 0.4f);
                    Out.Size     = FVector2f(70.0f * Grow, 70.0f * Grow);
                    Out.Rotation = Layer == LayerStar01 ? 45.0f : 0.1f;
                    Out.VisTag   = VisStar02;
                    break;
                }

                if (Layer == LayerLightning)
                {
                    const auto Life = FMath::Lerp(0.3f, 0.5f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / Life);

                    Out.Color = FLinearColor(
                        Key5(t, 0.0f, 1.0f,      0.0748566f, 1.0f, 0.598853f, 1.0f,      0.811349f, 0.287441f,  1.0f, 0.0512695f),
                        Key5(t, 0.0f, 0.745404f, 0.0748566f, 1.0f, 0.598853f, 0.147027f, 0.811349f, 0.0409152f, 1.0f, 0.0409152f),
                        Key5(t, 0.0f, 0.304987f, 0.0748566f, 1.0f, 0.598853f, 0.982251f, 0.811349f, 1.0f,       1.0f, 1.0f),
                        Key5(t, 0.162994f, 1.0f, 0.329611f, 0.0f, 0.504679f, 1.0f, 0.746152f, 0.0f, 0.959855f, 1.0f));

                    const auto Base = FMath::Lerp(30.0f, 100.0f, Rand(InSeed, 8));
                    const auto Grow = Key3(t, 0.0f, 0.0f, 0.1f, 0.8f, 1.0f, 1.0f);
                    Out.Size = FVector2f(Base * Grow, Base * Grow);

                    Out.Rotation = Rand(InSeed, 4) * 360.0f + Life * SpinIntegral(t);

                    Out.SubImageIndex = FMath::Fmod(FMath::Min(FMath::FloorToFloat(t * 5.0f), 4.0f), 4.0f);

                    Out.Dynamic = FVector4f(Key4(t, 0.0f, 1.0f, 0.2f, -1.0f, 0.3f, 0.875f, 1.0f, -1.0f), 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisLightning;
                    break;
                }

                if (Layer == LayerFlare03)
                {
                    if (td > 0.4f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.4f);

                    Out.Color   = FLinearColor(1.0f, 0.4563f, 0.111f, 0.737104f * SharedFade(t));
                    Out.Size    = FVector2f(250.0f, 250.0f) * SharedGrow(t);
                    Out.Dynamic = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart03Br;
                    break;
                }

                if (Layer == LayerFlare04)
                {
                    if (td > 0.3f)
                    {
                        Hide();
                        break;
                    }

                    const auto t = Saturate(td / 0.3f);

                    Out.Color   = FLinearColor(1.0f, 0.384719f, 0.0889999f, 0.0371041f * SharedFade(t));
                    Out.Size    = FVector2f(80.0f, 80.0f) * SharedGrow(t);
                    Out.Dynamic = FVector4f(2.0f, 0.0f, 0.0f, 0.0f);
                    Out.VisTag  = VisPart01Br;
                    break;
                }

                {
                    const auto Life = FMath::Lerp(0.3f, 0.6f, Rand(InSeed, 1));

                    if (td > Life)
                    {
                        Hide();
                        break;
                    }

                    const auto t  = Saturate(td / Life);
                    const auto V0 = RandDir(InSeed) * FMath::Lerp(500.0f, 1500.0f, Rand(InSeed, 2));

                    Out.Position = V0 * (Int3(t, 0.2f, 1.0f, 0.15f, 0.0f) * Life);
                    Out.Velocity = V0 * Key3(t, 0.0f, 1.0f, 0.2f, 0.15f, 1.0f, 0.0f);

                    Out.Color = FLinearColor(
                        Key4(t, 0.0f, 1.0f,      0.079686f, 1.0f,      0.290975f, 0.646925f, 1.0f, 0.223228f),
                        Key4(t, 0.0f, 0.913099f, 0.079686f, 0.134f,    0.290975f, 0.14f,     1.0f, 0.0f),
                        Key4(t, 0.0f, 0.584079f, 0.079686f, 0.731716f, 0.290975f, 1.0f,      1.0f, 0.116971f),
                        0.8f * Key2(t, 0.080893f, 1.0f, 1.0f, 0.0f));

                    const auto Width  = FMath::Lerp(25.0f, 40.0f, Rand(InSeed, 8));
                    const auto Length = FMath::Lerp(70.0f, 60.0f, Rand(InSeed, 12));
                    const auto Grow   = Key3(t, 0.0f, 0.0f, 0.1f, 1.0f, 1.0f, 0.0f);
                    const auto Taper  = Key3(t, 0.0f, 1.0f, 0.3f, 0.25f, 1.0f, 0.2f);

                    const auto SpeedFactor = FMath::Lerp(1.0f, 2.0f, Saturate(Out.Velocity.Size() / 1000.0f));

                    Out.Size   = FVector2f(Width * Grow, Length * Grow * Taper * SpeedFactor);
                    Out.VisTag = VisPart04;
                }
                break;
            }
            case 0: // Gravity — constant downward accel, integrate, tint warm->dark over life.
            default:
            {
                const FVector3f Gravity(0.0f, 0.0f, -980.0f);
                Out.Velocity = InVelocity + Gravity * InDeltaTime;
                Out.Position = InPosition + Out.Velocity * InDeltaTime;
                Out.Color    = FMath::Lerp(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f), FLinearColor(0.6f, 0.1f, 0.0f, 1.0f), NormalizedAge);
                break;
            }
        }
        return Out;
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCkParticles_DataInterface::UCkParticles_DataInterface(FObjectInitializer const& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Proxy.Reset(new FNiagaraDataInterfaceProxyCkParticles());
}

void
    UCkParticles_DataInterface::
    PostInitProperties()
{
    Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        const ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
        FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
    }
}

#if WITH_EDITORONLY_DATA
void
    UCkParticles_DataInterface::
    GetFunctionsInternal(
        TArray<FNiagaraFunctionSignature>& OutFunctions) const
{
    FNiagaraFunctionSignature Sig;
    Sig.Name = NDICkParticlesLocal::NAME_ExecuteStage;
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()),       TEXT("ParticleScript")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),      TEXT("BehaviorId")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("DeltaTime")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Age")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Lifetime")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Position")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Velocity")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),      TEXT("Seed")));
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("EmitterAge")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutPosition")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutVelocity")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),   TEXT("OutColor")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(),    TEXT("OutSize")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutScale")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetQuatDef(),    TEXT("OutOrientation")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(),    TEXT("OutDynamic")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),   TEXT("OutRotation")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),     TEXT("OutMeshIndex")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(),     TEXT("OutVisTag")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutSpriteAlignment")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutSpriteFacing")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),   TEXT("OutSubImageIndex")));
    Sig.SetDescription(LOCTEXT("ExecuteStageDesc",
        "Runs the CkParticles behavior selected by BehaviorId. Logic lives in /CkParticles/*.ush (GPU) and the CPU mirror."));
    OutFunctions.Add(Sig);
}
#endif

void
    UCkParticles_DataInterface::
    GetVMExternalFunction(
        const FVMExternalFunctionBindingInfo& BindingInfo,
        void* InstanceData,
        FVMExternalFunction& OutFunc)
{
    if (BindingInfo.Name == NDICkParticlesLocal::NAME_ExecuteStage)
    {
        OutFunc = FVMExternalFunction::CreateUObject(this, &UCkParticles_DataInterface::VMExecuteStage);
    }
}

FCk_Particles_StageResult
    UCkParticles_DataInterface::
    Execute_Stage_CPU(
        int32     InBehaviorId,
        float     InDeltaTime,
        float     InAge,
        float     InLifetime,
        FVector3f InPosition,
        FVector3f InVelocity,
        int32     InSeed,
        float     InEmitterAge)
{
    return NDICkParticlesLocal::ExecuteStage_CPU(
        InBehaviorId, InDeltaTime, InAge, InLifetime, InPosition, InVelocity, InSeed, InEmitterAge);
}

void
    UCkParticles_DataInterface::
    VMExecuteStage(
        FVectorVMExternalFunctionContext& Context)
{
    // Order MUST match GetFunctionsInternal (inputs after the DI param, then outputs).
    FNDIInputParam<int32>         BehaviorId(Context);
    FNDIInputParam<float>         DeltaTime(Context);
    FNDIInputParam<float>         Age(Context);
    FNDIInputParam<float>         Lifetime(Context);
    FNDIInputParam<FVector3f>     InPosition(Context);
    FNDIInputParam<FVector3f>     InVelocity(Context);
    FNDIInputParam<int32>         Seed(Context);
    FNDIInputParam<float>         EmitterAge(Context);

    FNDIOutputParam<FVector3f>    OutPosition(Context);
    FNDIOutputParam<FVector3f>    OutVelocity(Context);
    FNDIOutputParam<FLinearColor> OutColor(Context);
    FNDIOutputParam<FVector2f>    OutSize(Context);
    FNDIOutputParam<FVector3f>    OutScale(Context);
    FNDIOutputParam<FQuat4f>      OutOrientation(Context);
    FNDIOutputParam<FVector4f>    OutDynamic(Context);
    FNDIOutputParam<float>       OutRotation(Context);
    FNDIOutputParam<int32>       OutMeshIndex(Context);
    FNDIOutputParam<int32>       OutVisTag(Context);
    FNDIOutputParam<FVector3f>   OutSpriteAlignment(Context);
    FNDIOutputParam<FVector3f>   OutSpriteFacing(Context);
    FNDIOutputParam<float>       OutSubImageIndex(Context);

    for (int32 i = 0; i < Context.GetNumInstances(); ++i)
    {
        const auto Result = NDICkParticlesLocal::ExecuteStage_CPU(
            BehaviorId.GetAndAdvance(),
            DeltaTime.GetAndAdvance(),
            Age.GetAndAdvance(),
            Lifetime.GetAndAdvance(),
            InPosition.GetAndAdvance(),
            InVelocity.GetAndAdvance(),
            Seed.GetAndAdvance(),
            EmitterAge.GetAndAdvance());

        OutPosition.SetAndAdvance(Result.Position);
        OutVelocity.SetAndAdvance(Result.Velocity);
        OutColor.SetAndAdvance(Result.Color);
        OutSize.SetAndAdvance(Result.Size);
        OutScale.SetAndAdvance(Result.Scale);
        OutOrientation.SetAndAdvance(Result.Orientation);
        OutDynamic.SetAndAdvance(Result.Dynamic);
        OutRotation.SetAndAdvance(Result.Rotation);
        OutMeshIndex.SetAndAdvance(Result.MeshIndex);
        OutVisTag.SetAndAdvance(Result.VisTag);
        OutSpriteAlignment.SetAndAdvance(Result.SpriteAlignment);
        OutSpriteFacing.SetAndAdvance(Result.SpriteFacing);
        OutSubImageIndex.SetAndAdvance(Result.SubImageIndex);
    }
}

#if WITH_EDITORONLY_DATA
void
    UCkParticles_DataInterface::
    GetParameterDefinitionHLSL(
        const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
        FString& OutHLSL)
{
    const TMap<FString, FStringFormatArg> TemplateArgs =
    {
        { TEXT("ParameterName"), ParamInfo.DataInterfaceHLSLSymbol },
    };
    AppendTemplateHLSL(OutHLSL, NDICkParticlesLocal::TemplateShaderFile, TemplateArgs);
}

bool
    UCkParticles_DataInterface::
    GetFunctionHLSL(
        const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
        const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
        int FunctionInstanceIndex,
        FString& OutHLSL)
{
    // The template (above) defines ExecuteStage_<symbol>; nothing per-function to emit here.
    return FunctionInfo.DefinitionName == NDICkParticlesLocal::NAME_ExecuteStage;
}

bool
    UCkParticles_DataInterface::
    AppendCompileHash(
        FNiagaraCompileHashVisitor* InVisitor) const
{
    bool bSuccess = Super::AppendCompileHash(InVisitor);
    for (const TCHAR* ShaderFile : NDICkParticlesLocal::DependentShaderFiles)
    {
        InVisitor->UpdateShaderFile(ShaderFile);
    }
    return bSuccess;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
