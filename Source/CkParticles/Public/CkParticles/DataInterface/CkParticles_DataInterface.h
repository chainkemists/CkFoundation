#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"

#include "CkParticles_DataInterface.generated.h"

struct FNiagaraDataInterfaceGeneratedFunction;

// --------------------------------------------------------------------------------------------------------------------
// One particle's stage outputs — the C++ mirror of FCkParticles_StageOutput (/CkParticles/Common.ush).
// Defaults must match CkParticles_DefaultOutput's, including the non-zero sprite alignment/facing pair.
// --------------------------------------------------------------------------------------------------------------------
struct FCk_Particles_StageResult
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
    FVector3f    SpriteAlignment = FVector3f(0.0f, 1.0f, 0.0f);
    FVector3f    SpriteFacing    = FVector3f(0.0f, 0.0f, 1.0f);
};

// --------------------------------------------------------------------------------------------------------------------
// Stateless Niagara DI exposing one pure `ExecuteStage`; the behavior logic lives in /CkParticles/*.ush (GPU)
// and its C++ mirror (CPU), which must stay in lockstep. See CkParticles/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(EditInlineNew, Category = "CkParticles", CollapseCategories, meta = (DisplayName = "Ck Particle Script"))
class CKPARTICLES_API UCkParticles_DataInterface : public UNiagaraDataInterface
{
    GENERATED_UCLASS_BODY()

public:
    virtual void
    PostInitProperties() override;

    virtual bool
    CanExecuteOnTarget(
        ENiagaraSimTarget Target) const override { return true; }

    virtual void
    GetVMExternalFunction(
        const FVMExternalFunctionBindingInfo& BindingInfo,
        void* InstanceData,
        FVMExternalFunction& OutFunc) override;

#if WITH_EDITORONLY_DATA
    virtual bool
    AppendCompileHash(
        FNiagaraCompileHashVisitor* InVisitor) const override;

    virtual void
    GetParameterDefinitionHLSL(
        const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
        FString& OutHLSL) override;

    virtual bool
    GetFunctionHLSL(
        const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
        const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
        int FunctionInstanceIndex,
        FString& OutHLSL) override;
#endif

    // CPU VM implementation of ExecuteStage. Mirrors the HLSL behavior switch.
    void
    VMExecuteStage(
        FVectorVMExternalFunctionContext& Context);

    // Evaluates ONE particle through the CPU mirror. This is the same code path a CPU sim runs — not a
    // test-only shim — exposed so behavior math can be asserted without Niagara, a template asset, or an RHI.
    // That matters: the template-shaped gates (asset loads, component spawned) cannot tell a correct behavior
    // from one that does nothing, so this is where a behavior's numbers actually get checked.
    static FCk_Particles_StageResult
    Execute_Stage_CPU(
        int32     InBehaviorId,
        float     InDeltaTime,
        float     InAge,
        float     InLifetime,
        FVector3f InPosition,
        FVector3f InVelocity,
        int32     InSeed);

protected:
#if WITH_EDITORONLY_DATA
    virtual void
    GetFunctionsInternal(
        TArray<FNiagaraFunctionSignature>& OutFunctions) const override;
#endif
};
