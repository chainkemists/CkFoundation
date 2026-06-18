#include "CkInteractTarget_ConstructionScript.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InteractTarget_ConstructionScript::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    // Mirrors UCk_Utils_InteractTarget_UE::Add's composition (minus the owner-side Record wiring, which
    // the replication-built entity does not own — Add connects the Record on the authority), then folds
    // the SM onto the SAME entity. CompletionPolicy rides separately because it is a private CK_PROPERTY
    // a subclass `default` can't reach.
    auto ParamsToUse = InteractTargetParams;
    ParamsToUse.Set_CompletionPolicy(CompletionPolicy);

    InHandle.Add<ck::FFragment_InteractTarget_Params>(ParamsToUse);
    InHandle.Add<ck::FFragment_InteractTarget_Current>();
    InHandle.Add<ck::FTag_InteractTarget_RequiresSetup>();

    UCk_Utils_GameplayLabel_UE::Add(InHandle, ParamsToUse.Get_InteractionChannel());

    // This construction script exists to NET-LINK the SM, so force Replicates regardless of the
    // CDO-baked default (SmParams only needs to carry the root state class). Setup then creates the
    // WithHistory container here because the entity has a driver (TryAdd in Request_BuildAndReplicate).
    auto SmParamsToUse = SmParams;
    SmParamsToUse.Set_Replication(ECk_Replication::Replicates);

    // Register the per-target override states through params so FProcessor_Sm_Setup builds the same
    // override table on the server and the client (the authority-only Request_AddOverrideState would be
    // dropped — and ensure — on a non-authority client).
    auto OverrideStates = TArray<TSubclassOf<UCk_SmState_EntityScript>>{};
    if (ck::IsValid(InteractionStateClass))
    { OverrideStates.Emplace(InteractionStateClass); }
    if (ck::IsValid(FocusedStateClass))
    { OverrideStates.Emplace(FocusedStateClass); }

    if (NOT OverrideStates.IsEmpty())
    { SmParamsToUse.Set_OverrideStates(OverrideStates); }

    UCk_Utils_StateMachine_UE::Add(InHandle, SmParamsToUse);
}
