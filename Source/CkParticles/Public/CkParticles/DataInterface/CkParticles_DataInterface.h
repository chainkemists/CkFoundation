#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"

#include "CkParticles_DataInterface.generated.h"

struct FNiagaraDataInterfaceGeneratedFunction;

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

protected:
#if WITH_EDITORONLY_DATA
    virtual void
    GetFunctionsInternal(
        TArray<FNiagaraFunctionSignature>& OutFunctions) const override;
#endif
};
