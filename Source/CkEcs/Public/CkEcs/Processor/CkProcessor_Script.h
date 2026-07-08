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
// declares its per-entity query via Configure(Query) and receives the natively-joined batch through ForEachBatch.
// Authors typically write a plain `ForEachEntity(FCk_Time, [FCk_Handle,] <fragments>...)` method and let the
// codegen emit a <Dev>_Driver subclass whose ForEachBatch loops the batch into it. The scheduler's contribution:
//   - placing the processor into the correct execution group (_Group) + RunAfter/RunBefore ordering
//   - conflict-ordering from the Configure query's read/write fragment sets
//   - skipping the dispatch during a pump pass when no entity carries the _MarkedDirtyBy fragment
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

    // Transient entity handle pointing into the hosted registry. Set by FProcessor_ScriptQueryHosted before
    // BeginPlay is called. Used for registry access (e.g. resolving the world-context / transient entity).
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

public:
    // Called once when the hosted instance is first constructed during graph build.
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] BeginPlay"))
    void BeginPlay();

    // Called once when the graph is torn down (world shutdown or live rebuild).
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] EndPlay"))
    void EndPlay();

    // Optional query customization. The generated driver (a SUBCLASS of the dev class) overrides this to add the
    // slots inferred from the dev class's ForEachEntity signature, then calls Super::Configure when the dev class
    // itself overrides Configure (for Require/Exclude/NoEntities). A class that overrides ForEachBatch directly
    // declares its whole query here instead. BlueprintCallable so the C++ host can invoke it for the CDO query
    // harvest and script Super:: calls dispatch cleanly.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] Configure"))
    void Configure(UPARAM(ref) FCk_ScriptProcessorQuery& Query);

    // Invoked once per tick with the natively-joined entity batch. The generated driver (subclass of the dev class)
    // overrides this to loop the batch and call the inherited typed ForEachEntity; a genuinely cross-entity
    // processor overrides it directly.
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] ForEachBatch"))
    void ForEachBatch(FCk_ScriptQueryBatch Batch, FCk_Time InDeltaT);

public:
    CK_PROPERTY_GET(_Group);
    CK_PROPERTY_GET(_MarkedDirtyBy);
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_RunAfter);
    CK_PROPERTY_GET(_RunBefore);

    // Called by the hosted wrapper to inject the transient entity handle before BeginPlay.
    auto Set_Handle(const FCk_Handle& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
