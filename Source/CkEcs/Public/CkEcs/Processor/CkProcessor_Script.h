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
// Where a C++ TProcessor encodes its query as template parameters, a script processor declares it via
// Configure(Query) and receives the natively-joined batch through ForEachBatch — authors usually write a plain
// ForEachEntity and let the codegen emit a <Dev>_Driver subclass that loops the batch into it. Registration is
// automatic via class discovery; every concrete subclass contributes one FProcessorDescriptor at graph build.
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
    // Registered FGroup_* type by canonical name; NAME_None means the scheduler's ungrouped default location.
    UPROPERTY(EditDefaultsOnly,
              Category = "Ck|Processor|Scheduling",
              meta = (AllowPrivateAccess = true))
    FName _Group;

    // Optional: when set, a pump pass skips this processor's Tick if no entity holds the fragment (nullptr =
    // every pass). Dirty lookup goes through the type-erased storage path, keyed by StorageId.
    UPROPERTY(EditDefaultsOnly,
              Category = "Ck|Processor|Scheduling",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UScriptStruct> _MarkedDirtyBy;

    // Transient entity handle into the hosted registry, set by FProcessor_ScriptQueryHosted before BeginPlay.
    UPROPERTY(BlueprintReadOnly, Transient,
              Category = "Ck|Processor",
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Handle;

    // Explicit ordering edges, resolved like _Group and sourced from the CDO at descriptor-build time. Name
    // other script processors by their dev-class path name (the descriptor's _Name).
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

    // Optional query customization. The generated driver overrides this to add the slots inferred from the dev
    // class's ForEachEntity signature, then calls Super::Configure for the dev class's own Require/Exclude/
    // NoEntities. BlueprintCallable so the C++ host can drive the CDO query harvest and script Super:: calls.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Ck|Processor|Events",
              meta = (DisplayName = "[Ck][ScriptProcessor] Configure"))
    void Configure(UPARAM(ref) FCk_ScriptProcessorQuery& Query);

    // Invoked once per tick with the natively-joined batch. The generated driver overrides this to loop the
    // batch into the typed ForEachEntity; a genuinely cross-entity processor overrides it directly.
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
