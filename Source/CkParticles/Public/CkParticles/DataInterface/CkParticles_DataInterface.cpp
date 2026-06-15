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
    };

    static auto ExecuteStage_CPU(
        int32 InBehaviorId, float InDeltaTime, float InAge, float InLifetime,
        FVector3f InPosition, FVector3f InVelocity) -> FStageIO
    {
        FStageIO Out;
        Out.Position = InPosition;
        Out.Velocity = InVelocity;

        const float NormalizedAge = InLifetime > 0.0f ? FMath::Clamp(InAge / InLifetime, 0.0f, 1.0f) : 0.0f;

        switch (InBehaviorId)
        {
            case 1: // Swirl — rotate the XY velocity around +Z, integrate, tint blue->magenta over life.
            {
                constexpr float AngularSpeed = 2.0f;
                const float Angle = AngularSpeed * InDeltaTime;
                const float S = FMath::Sin(Angle);
                const float C = FMath::Cos(Angle);
                Out.Velocity = FVector3f(C * InVelocity.X - S * InVelocity.Y, S * InVelocity.X + C * InVelocity.Y, InVelocity.Z);
                Out.Position = InPosition + Out.Velocity * InDeltaTime;
                Out.Color    = FMath::Lerp(FLinearColor(0.1f, 0.4f, 1.0f, 1.0f), FLinearColor(1.0f, 0.2f, 0.6f, 1.0f), NormalizedAge);
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
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutPosition")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),    TEXT("OutVelocity")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),   TEXT("OutColor")));
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

    FNDIOutputParam<FVector3f>    OutPosition(Context);
    FNDIOutputParam<FVector3f>    OutVelocity(Context);
    FNDIOutputParam<FLinearColor> OutColor(Context);

    for (int32 i = 0; i < Context.GetNumInstances(); ++i)
    {
        const auto Result = NDICkParticlesLocal::ExecuteStage_CPU(
            BehaviorId.GetAndAdvance(),
            DeltaTime.GetAndAdvance(),
            Age.GetAndAdvance(),
            Lifetime.GetAndAdvance(),
            InPosition.GetAndAdvance(),
            InVelocity.GetAndAdvance());

        OutPosition.SetAndAdvance(Result.Position);
        OutVelocity.SetAndAdvance(Result.Velocity);
        OutColor.SetAndAdvance(Result.Color);
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
