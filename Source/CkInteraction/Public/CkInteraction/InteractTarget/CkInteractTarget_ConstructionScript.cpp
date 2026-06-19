#include "CkInteractTarget_ConstructionScript.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_interact_target_construction_script
{
    // Mirrors UCk_Utils_InteractTarget_UE::Add's composition (minus the owner-side Record wiring, which
    // the replication-built entity does not own — Add connects the Record on the authority), then folds
    // the SM onto the SAME entity. CompletionPolicy rides separately because it is a private CK_PROPERTY
    // neither a subclass `default` nor an FInstancedStruct field can set on the params struct directly.
    static auto
        DoBuild(
            FCk_Handle& InHandle,
            const FCk_Fragment_InteractTarget_ParamsData& InInteractTargetParams,
            ECk_Interaction_CompletionPolicy InCompletionPolicy,
            const FCk_Fragment_StateMachine_ParamsData& InSmParams,
            const TSubclassOf<UCk_SmState_EntityScript>& InInteractionStateClass,
            const TSubclassOf<UCk_SmState_EntityScript>& InFocusedStateClass)
        -> void
    {
        auto ParamsToUse = InInteractTargetParams;
        ParamsToUse.Set_CompletionPolicy(InCompletionPolicy);

        InHandle.Add<ck::FFragment_InteractTarget_Params>(ParamsToUse);
        InHandle.Add<ck::FFragment_InteractTarget_Current>();
        InHandle.Add<ck::FTag_InteractTarget_RequiresSetup>();

        UCk_Utils_GameplayLabel_UE::Add(InHandle, ParamsToUse.Get_InteractionChannel());

        // This construction script exists to NET-LINK the SM, so force Replicates regardless of the
        // baked default (SmParams only needs to carry the root state class). Setup then creates the
        // WithHistory container here because the entity has a driver (TryAdd in Request_BuildAndReplicate).
        auto SmParamsToUse = InSmParams;
        SmParamsToUse.Set_Replication(ECk_Replication::Replicates);

        // Register the per-target override states through params so FProcessor_Sm_Setup builds the same
        // override table on the server and the client (the authority-only Request_AddOverrideState would be
        // dropped — and ensure — on a non-authority client).
        auto OverrideStates = TArray<TSubclassOf<UCk_SmState_EntityScript>>{};
        if (ck::IsValid(InInteractionStateClass))
        { OverrideStates.Emplace(InInteractionStateClass); }
        if (ck::IsValid(InFocusedStateClass))
        { OverrideStates.Emplace(InFocusedStateClass); }

        if (NOT OverrideStates.IsEmpty())
        { SmParamsToUse.Set_OverrideStates(OverrideStates); }

        UCk_Utils_StateMachine_UE::Add(InHandle, SmParamsToUse);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InteractTarget_ConstructionScript::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    // Prefer the by-value config stamped on the entity (the GENERIC-recipe path: one shared CDO + a
    // replicated FCk_InteractTarget_ConstructionConfig). Falls back to this instance's CDO fields when no
    // config is present, so existing per-type subclasses build byte-identically to before.
    if (InHandle.Has<ck::FFragment_EntityConstructionConfig>())
    {
        if (const auto* Config = InHandle.Get<ck::FFragment_EntityConstructionConfig>().Get_Config()
                .GetPtr<FCk_InteractTarget_ConstructionConfig>())
        {
            ck_interact_target_construction_script::DoBuild(InHandle, Config->InteractTargetParams,
                Config->CompletionPolicy, Config->SmParams, Config->InteractionStateClass, Config->FocusedStateClass);
            return;
        }
    }

    ck_interact_target_construction_script::DoBuild(InHandle, InteractTargetParams, CompletionPolicy, SmParams,
        InteractionStateClass, FocusedStateClass);
}
