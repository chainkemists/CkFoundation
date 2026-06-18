#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment_Data.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"
// Full definition (not just the forward decl in _Fragment_Data.h) is required: the scalar
// TSubclassOf<UCk_SmState_EntityScript> UPROPERTYs below make UHT emit UCk_SmState_EntityScript::StaticClass().
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkInteractTarget_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The construction-script recipe for a NET-LINKED InteractTarget whose interaction SM replicates.
// It composes the InteractTarget's (C++-internal) fragments AND a replicated SM onto the SAME entity.
// Built via Request_BuildAndReplicate from a driver-bearing owner so the InteractTarget gets its own
// replication driver (TryAdd) — which is exactly what FProcessor_Sm_Setup's TryAddContainerFragment
// needs to create the SM's WithHistory container server-side. The stored ConstructionInfos then
// rebuild InteractTarget+SM on the client, where the replicated history applies via the rep handler.
//
// The framework names no game-specific state class: the SM root + per-target override states ride in
// via SmParams / the override-class fields. Callers bake config into a SUBCLASS CDO and pass the CLASS
// (not a transient archetype) to Request_BuildAndReplicate — the class is net-stable, so the client
// rebuilds IT+SM from the same CDO. A runtime-NewObject archetype would serialize as null on the
// client (no netGUID) and the client would fall back to the empty-CDO else-branch, silently dropping
// the SM. AngelScript subclasses set these via `default ... = ...;` (no DoConstruct override needed):
// the scalar fields exist because `default` can populate neither a struct's private CK_PROPERTY
// sub-field (CompletionPolicy) nor a TArray (override states) — DoConstruct assembles them.
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class CKINTERACTION_API UCk_InteractTarget_ConstructionScript : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InteractTarget_ConstructionScript);

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const -> void override;

public:
    // Public + non-underscore on purpose: AngelScript subclasses CDO-bake these via `default`. Not the
    // usual private+CK_PROPERTY shape because they must be settable from script defaults.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractTarget")
    FCk_Fragment_InteractTarget_ParamsData InteractTargetParams;

    // _CompletionPolicy is a private CK_PROPERTY on the params struct, so a subclass `default` can't
    // reach it. Carry it as a scalar and apply it onto a copy of InteractTargetParams in DoConstruct.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractTarget")
    ECk_Interaction_CompletionPolicy CompletionPolicy = ECk_Interaction_CompletionPolicy::Timed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractTarget")
    FCk_Fragment_StateMachine_ParamsData SmParams;

    // Per-target override states folded into SmParams.OverrideStates (symmetric build on every machine).
    // Scalar (not a TArray) because a subclass `default` can't populate an array sub-field.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractTarget")
    TSubclassOf<UCk_SmState_EntityScript> InteractionStateClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractTarget")
    TSubclassOf<UCk_SmState_EntityScript> FocusedStateClass;
};

// --------------------------------------------------------------------------------------------------------------------
