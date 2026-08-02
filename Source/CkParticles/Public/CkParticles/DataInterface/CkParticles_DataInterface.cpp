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
        FVector3f InPosition, FVector3f InVelocity, int32 InSeed) -> FStageIO
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
        int32     InSeed)
{
    return NDICkParticlesLocal::ExecuteStage_CPU(
        InBehaviorId, InDeltaTime, InAge, InLifetime, InPosition, InVelocity, InSeed);
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
            Seed.GetAndAdvance());

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
