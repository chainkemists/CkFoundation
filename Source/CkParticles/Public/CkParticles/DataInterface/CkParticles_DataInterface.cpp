#include "CkParticles/DataInterface/CkParticles_DataInterface.h"

#include "NiagaraCompileHashVisitor.h"
#include "NiagaraTypeRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_DataInterface)

#define LOCTEXT_NAMESPACE "UCkParticles_DataInterface"

// --------------------------------------------------------------------------------------------------------------------

namespace NDICkParticlesLocal
{
    static const FName NAME_ExecuteStage(TEXT("ExecuteStage"));

    // GPU: this template defines ExecuteStage_<symbol> and #includes the behavior .ush files.
    static const TCHAR* TemplateShaderFile = TEXT("/CkParticles/CkParticles_DataInterfaceTemplate.ush");

    // Every .ush the GPU function transitively depends on, so editing any of them busts the shader cache.
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
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Trivial render-thread proxy. The DI is stateless (pure function of its pins) so the proxy carries no data;
// it exists only because a GPU-capable DI requires one.
struct FNiagaraDataInterfaceProxyCkParticles : public FNiagaraDataInterfaceProxy
{
    virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override { check(false); }
    virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }
};

// --------------------------------------------------------------------------------------------------------------------
// CPU mirror of /CkParticles/CkParticles_Behaviors.ush. The GPU (USF) and CPU (here) implementations of each
// behavior MUST stay in lockstep — same BehaviorId, same math — otherwise CPU and GPU emitters diverge.
namespace NDICkParticlesLocal
{
    struct FStageIO
    {
        FVector3f    Position = FVector3f::ZeroVector;
        FVector3f    Velocity = FVector3f::ZeroVector;
        FLinearColor Color    = FLinearColor::White;
        FVector2f    Size     = FVector2f(20.0f, 20.0f);
    };

    // CPU mirrors of CkParticles_Rand / CkParticles_RandDir (Common.ush). 24-bit normalization keeps these
    // bit-identical to the GPU path.
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

    static auto ExecuteStage_CPU(
        int32 InBehaviorId, float InDeltaTime, float InAge, float InLifetime,
        FVector3f InPosition, FVector3f InVelocity, int32 InSeed) -> FStageIO
    {
        FStageIO Out;
        Out.Position = InPosition;
        Out.Velocity = InVelocity;

        const float NormalizedAge = InLifetime > 0.0f ? FMath::Clamp(InAge / InLifetime, 0.0f, 1.0f) : 0.0f;

        // Default size mirrors CkParticles_DefaultOutput (Common.ush): per-Seed varied, shrinking over life.
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
            case 6: // Beam — per-Seed converging stream down +X (aim via spawn rotation). White -> blue.
            {
                const float Along  = FMath::Lerp(0.0f, 450.0f, NormalizedAge);
                const float Spread = FMath::Lerp(28.0f, 3.0f, NormalizedAge);
                const float A      = Rand(InSeed, 1) * 6.28318530718f;
                const float R      = Spread * Rand(InSeed, 2);
                Out.Position = FVector3f(Along, FMath::Cos(A) * R, FMath::Sin(A) * R);
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 3.0f, 2.2f, 1.0f), FLinearColor(0.2f, 0.5f, 2.0f, 1.0f), NormalizedAge);
                break;
            }
            case 7: // Slash — swept arc crescent in local XY, per-Seed radius jitter. White -> cyan.
            {
                const float Ang = FMath::Lerp(-1.3f, 1.3f, NormalizedAge);
                const float Rad = FMath::Lerp(120.0f, 165.0f, Rand(InSeed, 1));
                const float Z   = (Rand(InSeed, 2) - 0.5f) * 12.0f;
                Out.Position = FVector3f(FMath::Cos(Ang) * Rad, FMath::Sin(Ang) * Rad, Z);
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 3.0f, 3.0f, 1.0f), FLinearColor(0.3f, 1.2f, 2.0f, 1.0f), NormalizedAge) * (1.0f - NormalizedAge * 0.5f);
                break;
            }
            case 8: // Nova — expanding flat shockwave ring in local XY. White-hot -> orange, fades.
            {
                const float A      = Rand(InSeed, 1) * 6.28318530718f;
                const float Rad    = FMath::Lerp(10.0f, 380.0f, NormalizedAge);
                const float Jitter = (Rand(InSeed, 2) - 0.5f) * 16.0f;
                Out.Position = FVector3f(FMath::Cos(A) * (Rad + Jitter), FMath::Sin(A) * (Rad + Jitter), 0.0f);
                Out.Color    = FMath::Lerp(FLinearColor(3.0f, 2.4f, 1.0f, 1.0f), FLinearColor(1.2f, 0.3f, 0.0f, 1.0f), NormalizedAge) * (1.0f - NormalizedAge);
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
