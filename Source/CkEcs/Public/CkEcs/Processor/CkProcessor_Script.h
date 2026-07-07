#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Object/CkWorldContextObject.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include "CkProcessor_Script.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

// --------------------------------------------------------------------------------------------------------------------
// Base class for AngelScript- or Blueprint-authored processors that participate in the ECS scheduler pump.
//
// Unlike a C++ TProcessor (which encodes its fragment query as a template parameter list), a script processor
// does not express a per-entity query at the type level. Instead, authors override Tick(DeltaT) and invoke the
// existing ForEach_EntityWith*Fragments helpers themselves. The scheduler's only contribution is:
//   - placing the processor into the correct execution group (_Group)
//   - skipping Tick entirely during a pump pass when no entity carries the _MarkedDirtyBy fragment
//
// Registration is automatic via class discovery (see UCk_EcsWorld_Subsystem_UE::DoBuildGraphAndSpawnActors).
// Every concrete subclass contributes one FProcessorDescriptor at graph-build time.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_Processor_Script_Base_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_Processor_Script_Base_UE);

public:
    using TimeType = FCk_Time;

private:
    // Maps to a registered FGroup_* type by canonical name. Leaving this as NAME_None places the processor at
    // the scheduler's default location for ungrouped entries (equivalent to "no group").
    UPROPERTY(EditDefaultsOnly,
              Category = "Ck|Processor|Scheduling",
              meta = (AllowPrivateAccess = true))
    FName _Group;

    // Optional. When set, the scheduler skips this processor's Tick during a pump pass if no entity currently
    // holds this fragment. Nullptr means Tick runs every pass.
    //
    // Dirty lookup is performed through the type-erased storage path (FFragment_DynamicFragment_Data keyed by
    // StorageId), matching how AngelScript authors already query fragments via UCk_Utils_DynamicFragment_UE.
    UPROPERTY(EditDefaultsOnly,
              Category = "Ck|Processor|Scheduling",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UScriptStruct> _MarkedDirtyBy;

    // Transient entity handle pointing into the hosted registry. Set by FProcessor_ScriptHosted before
    // BeginPlay is called. Script overrides use this to call ForEach_EntityWith*Fragments.
    UPROPERTY(BlueprintReadOnly, Transient,
              Category = "Ck|Processor",
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Handle;

    // Explicit scheduler ordering edges, resolved through DoResolveGroupName exactly like _Group. Reference other
    // script processors by their dev-class path name (which is the descriptor's _Name). Sourced from the CDO by the
    // host at descriptor-build time.
    UPROPERTY(EditDefaultsOnly,
              Category = "Ck|Processor|Scheduling",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _RunAfter;

    UPROPERTY(EditDefaultsOnly,
              Category = "Ck|Processor|Scheduling",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _RunBefore;

    // Set by FProcessor_ScriptQueryHosted on the generated DRIVER instance: points at the dev-class instance whose
    // ForEachEntity the driver forwards each batch element to. Null on a dev instance running in direct mode (the dev
    // class overrides ForEachBatch itself). BlueprintReadOnly so the generated driver can Cast<> it in AngelScript.
    UPROPERTY(BlueprintReadOnly, Transient,
              Category = "Ck|Processor",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Processor_Script_Base_UE> _IterationTarget;

public:
    // Called once per scheduler Tick (and once per Pump pass when _MarkedDirtyBy is clean-skipped).
    // Author calls UCk_Utils_DynamicFragment_UE::ForEach_EntityWith*Fragments from this body.
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] Tick"))
    void Tick(FCk_Time InDeltaT);

    // Called once when the hosted instance is first constructed during graph build.
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] BeginPlay"))
    void BeginPlay();

    // Called once when the graph is torn down (world shutdown or live rebuild).
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] EndPlay"))
    void EndPlay();

    // Optional query customization. On the generated driver, Configure adds the slots inferred from the dev class's
    // ForEachEntity signature and then forwards to the dev class's own Configure (if any) for Require/Exclude/
    // NoEntities. A class that overrides ForEachBatch directly declares its whole query here instead.
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] Configure"))
    void Configure(UPARAM(ref) FCk_ScriptProcessorQuery& Query);

    // Invoked once per tick with the natively-joined entity batch. The generated driver overrides this to loop the
    // batch and call the dev class's typed ForEachEntity; a genuinely cross-entity processor overrides it directly.
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] ForEachBatch"))
    void ForEachBatch(FCk_ScriptQueryBatch Batch, FCk_Time InDeltaT);

public:
    CK_PROPERTY_GET(_Group);
    CK_PROPERTY_GET(_MarkedDirtyBy);
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_RunAfter);
    CK_PROPERTY_GET(_RunBefore);
    CK_PROPERTY_GET(_IterationTarget);

    // Called by the hosted wrapper to inject the transient entity handle before BeginPlay.
    auto Set_Handle(const FCk_Handle& InHandle) -> void;

    // Called by FProcessor_ScriptQueryHosted on the generated driver instance to point it at the dev instance.
    auto Set_IterationTarget(UCk_Processor_Script_Base_UE* InTarget) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
