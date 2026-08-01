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
    struct FStageIO
    {
        FVector3f    Position    = FVector3f::ZeroVector;
        FVector3f    Velocity    = FVector3f::ZeroVector;
        FLinearColor Color       = FLinearColor::White;
        FVector2f    Size        = FVector2f(20.0f, 20.0f);
        FVector3f    Scale       = FVector3f(1.0f, 1.0f, 1.0f);
        FQuat4f      Orientation = FQuat4f::Identity;
        FVector4f    Dynamic     = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
        float        Rotation    = 0.0f;
        int32        MeshIndex   = 0;
        int32        VisTag      = 0;
        // VisTag 4 only. Never zero — a degenerate pair collapses the sprite instead of failing visibly.
        FVector3f    SpriteAlignment = FVector3f(0.0f, 1.0f, 0.0f);
        FVector3f    SpriteFacing    = FVector3f(0.0f, 0.0f, 1.0f);
    };

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
            case 7: // Slash — Vefects NS_BasicAttack swipe: sweep meshes + sparkle ring + wind ghost. Mirrors Behavior_Slash.ush.
            {
                const float k = Rand(InSeed, 0);

                if (k < 0.05f)
                {
                    const float t     = Saturate(InAge / 0.42f);
                    const float Pitch = 1.5707963f + (Rand(InSeed, 2) - 0.5f) * 0.35f;
                    const float Roll  = (Rand(InSeed, 1) - 0.5f) * 0.7f;

                    Out.Orientation = QuatMul(
                        QuatFromAxisAngle(FVector3f(0.0f, 1.0f, 0.0f), Roll),
                        QuatFromAxisAngle(FVector3f(1.0f, 0.0f, 0.0f), Pitch));
                    Out.Position  = FVector3f(0.0f, 0.0f, 90.0f);
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 0;
                    Out.Scale     = FVector3f(1.0f, 1.0f, 1.0f) * FMath::Lerp(1.35f, 1.75f, Rand(InSeed, 3));
                    Out.Dynamic   = FVector4f(FMath::Max(0.0f, t - 0.5f) * 2.0f, 0.35f, FMath::Lerp(-0.55f, 0.45f, t), 2.2f * (1.0f - t));

                    const float     Vis = Saturate(1.0f - FMath::Max(0.0f, InAge - 0.30f) / 0.12f);
                    const FVector3f c   = FMath::Lerp(FVector3f(5.0f, 3.9f, 2.0f), FVector3f(5.0f, 0.35f, 0.75f), Saturate(t * 1.3f));
                    Out.Color = FLinearColor(c.X * Vis, c.Y * Vis, c.Z * Vis, 0.6f * Vis);
                }
                else if (k < 0.30f)
                {
                    const float Life = FMath::Lerp(0.30f, 0.50f, Rand(InSeed, 4));
                    const float td   = FMath::Max(0.0f, InAge - 0.06f);
                    const float Arc  = Saturate(td / Life);
                    const float a    = 6.28318530718f * Rand(InSeed, 5);
                    const float jy   = (Rand(InSeed, 6) - 0.5f);

                    const FVector3f P0(FMath::Cos(a) * 55.0f, jy * 14.0f, 90.0f + FMath::Sin(a) * 55.0f);
                    const FVector3f d(FMath::Cos(a), jy * 0.6f, FMath::Sin(a));
                    const float     s  = FMath::Lerp(250.0f, 550.0f, Rand(InSeed, 7));
                    const float     kd = (1.0f - FMath::Exp(-2.4f * td)) / 2.4f;

                    Out.Position = P0 + d * s * kd;
                    Out.Velocity = d * s * FMath::Exp(-2.4f * td);
                    Out.VisTag   = 1;
                    Out.Size     = FVector2f(3.5f, FMath::Lerp(20.0f, 6.0f, Arc));

                    const float     Vis = Step(0.06f, InAge) * Saturate(1.0f - Arc);
                    const FVector3f c   = FMath::Lerp(FVector3f(4.0f, 3.8f, 2.7f), FVector3f(4.0f, 1.8f, 0.16f), Saturate((Arc - 0.15f) / 0.5f));
                    Out.Color = FLinearColor(c.X * Vis, c.Y * Vis, c.Z * Vis, Vis);
                }
                else if (k < 0.36f)
                {
                    const float tw   = Saturate(FMath::Max(0.0f, InAge - 0.05f) / 0.55f);
                    const float Roll = (Rand(InSeed, 1) - 0.5f) * 0.5f;

                    Out.Orientation = QuatMul(
                        QuatFromAxisAngle(FVector3f(0.0f, 1.0f, 0.0f), Roll),
                        QuatFromAxisAngle(FVector3f(1.0f, 0.0f, 0.0f), 1.5707963f));
                    Out.Position  = FVector3f(0.0f, 0.0f, 90.0f);
                    Out.Velocity  = FVector3f::ZeroVector;
                    Out.VisTag    = 3;
                    Out.MeshIndex = 0;
                    Out.Scale     = FVector3f(1.0f, 1.0f, 1.0f) * (1.95f * (1.0f + 0.1f * tw));
                    Out.Dynamic   = FVector4f(tw * 0.9f, 0.30f, FMath::Lerp(-0.38f, 0.38f, tw), 0.3f);

                    const float Vis = Saturate(tw / 0.25f) * Saturate(1.0f - FMath::Max(0.0f, tw - 0.6f) / 0.4f);
                    Out.Color = FLinearColor(1.0f * Vis, 0.62f * Vis, 0.72f * Vis, 0.22f * Vis);
                }
                else
                {
                    Out.Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    Out.Size  = FVector2f(0.0f, 0.0f);
                    Out.Scale = FVector3f(0.0f, 0.0f, 0.0f);
                }
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
