#pragma once

#include "CkParticles/DataInterface/CkParticles_PartTuning.h"

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"

#include "CkParticles_DataInterface.generated.h"

struct FNiagaraDataInterfaceGeneratedFunction;
class FNiagaraShaderParametersBuilder;

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
    float        SubImageIndex   = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------
// Niagara DI exposing one pure `ExecuteStage`; the behavior logic lives in /CkParticles/*.ush (GPU) and its C++
// mirror (CPU), which must stay in lockstep. See CkParticles/CLAUDE.md.
//
// The stage function is pure — its output is a function of its inputs alone — but the DI is no longer stateless:
// it carries ONE per-instance datum, the PART tuning block (CkParticles_PartTuning.h), which cannot ride a Niagara
// User parameter because there is no float4-array user type. It is applied centrally after the stage returns.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(EditInlineNew, Category = "CkParticles", CollapseCategories, meta = (DisplayName = "Ck Particle Script"))
class CKPARTICLES_API UCkParticles_DataInterface : public UNiagaraDataInterface
{
    GENERATED_UCLASS_BODY()

public:
    virtual void
    PostInitProperties() override;

    // ---- Per-instance data. The DI was stateless until per-part tuning needed a per-component block; the block
    //      lives in ck::particles' component registry and is pulled here each tick, then pushed to the RT proxy.
    virtual bool
    InitPerInstanceData(
        void*                   PerInstanceData,
        FNiagaraSystemInstance* SystemInstance) override;

    virtual void
    DestroyPerInstanceData(
        void*                   PerInstanceData,
        FNiagaraSystemInstance* SystemInstance) override;

    virtual int32
    PerInstanceDataSize() const override;

    virtual bool
    PerInstanceTick(
        void*                   PerInstanceData,
        FNiagaraSystemInstance* SystemInstance,
        float                   DeltaSeconds) override;

    virtual bool
    HasPreSimulateTick() const override { return true; }

    virtual void
    ProvidePerInstanceDataForRenderThread(
        void*                            DataForRenderThread,
        void*                            PerInstanceData,
        const FNiagaraSystemInstanceID&  SystemInstance) override;

    virtual void
    BuildShaderParameters(
        FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const override;

    virtual void
    SetShaderParameters(
        const FNiagaraDataInterfaceSetShaderParametersContext& Context) const override;

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
    //
    // InEmitterAge is the emitter's clock (Emitter.Age), deliberately WITHOUT a default: a behavior that reads
    // the spawn phase is silently wrong if a caller omits it, and a stage evaluation whose whole purpose is
    // exact mirroring must not let an input default itself into existence.
    //
    // InTuning is the User.CkTuning float4 and DOES default, to its identity — it scales a behavior's result
    // rather than feeding its math, so omitting it is a caller asking for the untuned behavior, which is a
    // meaningful request rather than a missing input.
    //
    // InPartTuning is the per-instance PART tuning block and defaults to ABSENT (nullptr), which is not the same
    // shape of default: an identity block and no block produce identical output, but a real sim reaches this with
    // whatever the component registry holds, so the absent case is the caller saying "no instance" rather than
    // "an untuned instance".
    static FCk_Particles_StageResult
    Execute_Stage_CPU(
        int32     InBehaviorId,
        float     InDeltaTime,
        float     InAge,
        float     InLifetime,
        FVector3f InPosition,
        FVector3f InVelocity,
        int32     InSeed,
        float     InEmitterAge,
        FVector4f InTuning = FVector4f(1.0f, 1.0f, 1.0f, 1.0f),
        const FCkParticles_PartTuningBlock* InPartTuning = nullptr);

protected:
#if WITH_EDITORONLY_DATA
    virtual void
    GetFunctionsInternal(
        TArray<FNiagaraFunctionSignature>& OutFunctions) const override;
#endif
};
