#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"

#include "CkParticles_DataInterface.generated.h"

struct FNiagaraDataInterfaceGeneratedFunction;

// --------------------------------------------------------------------------------------------------------------------
// Stateless Niagara Data Interface that exposes a single generic, pure function `ExecuteStage`.
//
// The particle/system LOGIC is authored entirely in HLSL (USF/USH) and C++:
//   - GPU: /CkParticles/CkParticles_DataInterfaceTemplate.ush -> #includes /CkParticles/CkParticles_Behaviors.ush
//          which switches on BehaviorId and dispatches into /CkParticles/Behaviors/<Name>.ush.
//   - CPU: VMExecuteStage mirrors the same switch in C++ (see .cpp). The two paths MUST stay in sync.
//
// A new particle behavior is added by editing the .ush (GPU) + the C++ switch (CPU) — never the Niagara asset.
// The asset is a thin template that calls ExecuteStage with a per-asset BehaviorId (see CkParticlesEditor).
// --------------------------------------------------------------------------------------------------------------------

UCLASS(EditInlineNew, Category = "CkParticles", CollapseCategories, meta = (DisplayName = "Ck Particle Script"))
class CKPARTICLES_API UCkParticles_DataInterface : public UNiagaraDataInterface
{
    GENERATED_UCLASS_BODY()

public:
    // UObject Interface
    virtual void
    PostInitProperties() override;
    // UObject Interface End

    // UNiagaraDataInterface Interface
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
    // UNiagaraDataInterface Interface End

    // CPU VM implementation of ExecuteStage. Mirrors the HLSL behavior switch.
    void
    VMExecuteStage(
        FVectorVMExternalFunctionContext& Context);

protected:
#if WITH_EDITORONLY_DATA
    virtual void
    GetFunctionsInternal(
        TArray<FNiagaraFunctionSignature>& OutFunctions) const override;
#endif
};
