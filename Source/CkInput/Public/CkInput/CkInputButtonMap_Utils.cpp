#include "CkInputButtonMap_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInput/CkInputSource_Utils.h"
#include "CkInput/CkInput_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InputButtonMap_UE,
    FCk_Handle_InputButtonMap,
    ck::FFragment_InputButtonMap_Params,
    ck::FFragment_InputButtonMap_Current);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_button_map_utils
{
    auto
        Get_IndexOfButton(
            const TArray<FCk_Input_ButtonAssociation>& InAssociations,
            const FCk_Input_ButtonId& InButtonId)
        -> int32
    {
        return InAssociations.IndexOfByPredicate(
        [&](const FCk_Input_ButtonAssociation& InExisting) -> bool
        {
            return InExisting.Get_ButtonId() == InButtonId;
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputButtonMap_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_InputButtonMap_ParamsData& InParams)
    -> FCk_Handle_InputButtonMap
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an InputButtonMap onto it"), InHandle)
    {}
    if (NOT HandleIsValid)
    { return {}; }

    const auto HandleIsAnInputSource = UCk_Utils_InputSource_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(HandleIsAnInputSource,
        TEXT("Add: Handle [{}] is not an InputSource, so an InputButtonMap on it would have no local player "
             "whose resolved mappings the mapped tier could derive from"), InHandle)
    {}
    if (NOT HandleIsAnInputSource)
    { return {}; }

    const auto EntityHasNoButtonMap = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(EntityHasNoButtonMap,
        TEXT("Add: Handle [{}] already carries an InputButtonMap — one source has exactly one button space, and "
             "a second would mint identities nothing could tell apart"), InHandle)
    {}
    if (NOT EntityHasNoButtonMap)
    { return {}; }

    if (NOT DoGet_PhysicalButtonsAreValid(InHandle, InParams.Get_PhysicalButtons()))
    { return {}; }

    InHandle.Add<ck::FFragment_InputButtonMap_Params>(InParams);
    InHandle.Add<ck::FFragment_InputButtonMap_Current>();

    auto ButtonMap = CastChecked(InHandle);

    // The declared tier-2 buttons and the first tier-1 derivation go through the ordinary request path so
    // composition-time state and steady-state state are produced by the same code. Queue order is preserved,
    // so the declarations are minted before the derivation runs.
    for (const auto& PhysicalButton : InParams.Get_PhysicalButtons())
    {
        Request_RegisterPhysicalButton(ButtonMap,
            FCk_Request_InputButtonMap_RegisterPhysicalButton{PhysicalButton}, {});
    }

    Request_Rederive(ButtonMap, FCk_Request_InputButtonMap_Rederive{}, {});

    ck::input::Verbose
    (
        TEXT("InputButtonMap composed onto InputSource [{}] with [{}] declared physical buttons"),
        InHandle, InParams.Get_PhysicalButtons().Num()
    );

    return ButtonMap;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputButtonMap_UE::
    Get_AllButtons(
        const FCk_Handle_InputButtonMap& InButtonMap)
    -> TArray<FCk_Input_ButtonId>
{
    const auto& Associations = InButtonMap.Get<ck::FFragment_InputButtonMap_Current>().Get_Associations();

    auto Result = TArray<FCk_Input_ButtonId>{};
    Result.Reserve(Associations.Num());

    for (const auto& Association : Associations)
    {
        Result.Emplace(Association.Get_ButtonId());
    }

    return Result;
}

auto
    UCk_Utils_InputButtonMap_UE::
    Get_ButtonIdsForKey(
        const FCk_Handle_InputButtonMap& InButtonMap,
        const FKey& InKey)
    -> TArray<FCk_Input_ButtonId>
{
    auto Result = TArray<FCk_Input_ButtonId>{};

    if (NOT InKey.IsValid())
    { return Result; }

    const auto& Associations = InButtonMap.Get<ck::FFragment_InputButtonMap_Current>().Get_Associations();

    for (const auto& Association : Associations)
    {
        if (Association.Get_Key() != InKey)
        { continue; }

        Result.Emplace(Association.Get_ButtonId());
    }

    return Result;
}

auto
    UCk_Utils_InputButtonMap_UE::
    TryGet_KeyForButton(
        const FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Input_ButtonId& InButtonId)
    -> FKey
{
    const auto& Associations = InButtonMap.Get<ck::FFragment_InputButtonMap_Current>().Get_Associations();

    const auto FoundIndex = ck_input_button_map_utils::Get_IndexOfButton(Associations, InButtonId);

    if (FoundIndex == INDEX_NONE)
    { return FKey{EKeys::Invalid}; }

    return Associations[FoundIndex].Get_Key();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputButtonMap_UE::
    Request_Rederive(
        FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Request_InputButtonMap_Rederive& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputButtonMap
{
    const auto ButtonMapIsValid = ck::IsValid(InButtonMap);
    CK_ENSURE_IF_NOT(ButtonMapIsValid,
        TEXT("Request_Rederive: invalid InputButtonMap handle [{}] — the derivation is dropped"), InButtonMap)
    {}
    if (NOT ButtonMapIsValid)
    {
        InDelegate.ExecuteIfBound(InButtonMap, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InButtonMap;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InButtonMap.AddOrGet<ck::FFragment_InputButtonMap_Requests>()._Requests.Emplace(InRequest);

    return InButtonMap;
}

auto
    UCk_Utils_InputButtonMap_UE::
    Request_RegisterPhysicalButton(
        FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Request_InputButtonMap_RegisterPhysicalButton& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputButtonMap
{
    const auto ButtonMapIsValid = ck::IsValid(InButtonMap);
    CK_ENSURE_IF_NOT(ButtonMapIsValid,
        TEXT("Request_RegisterPhysicalButton: invalid InputButtonMap handle [{}] — the registration is dropped"),
        InButtonMap)
    {}
    if (NOT ButtonMapIsValid)
    {
        InDelegate.ExecuteIfBound(InButtonMap, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InButtonMap;
    }

    const auto KeyIsValid = InRequest.Get_Key().IsValid();
    CK_ENSURE_IF_NOT(KeyIsValid,
        TEXT("Request_RegisterPhysicalButton on [{}] names an invalid key, which would mint a button nothing "
             "could ever press"), InButtonMap)
    {}
    if (NOT KeyIsValid)
    {
        InDelegate.ExecuteIfBound(InButtonMap, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InButtonMap;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InButtonMap.AddOrGet<ck::FFragment_InputButtonMap_Requests>()._Requests.Emplace(InRequest);

    return InButtonMap;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputButtonMap_UE::
    DoGet_PhysicalButtonsAreValid(
        const FCk_Handle& InContext,
        const TArray<FKey>& InPhysicalButtons)
    -> bool
{
    for (auto Index = 0; Index < InPhysicalButtons.Num(); ++Index)
    {
        const auto& PhysicalButton = InPhysicalButtons[Index];

        const auto KeyIsValid = PhysicalButton.IsValid();
        CK_ENSURE_IF_NOT(KeyIsValid,
            TEXT("InputButtonMap declaration on [{}] names an invalid physical key, which would mint a button "
                 "nothing could ever press"), InContext)
        {}
        if (NOT KeyIsValid)
        { return false; }

        const auto FirstIndexForKey = InPhysicalButtons.IndexOfByKey(PhysicalButton);

        const auto KeyIsUnclaimed = FirstIndexForKey == Index;
        CK_ENSURE_IF_NOT(KeyIsUnclaimed,
            TEXT("InputButtonMap declaration on [{}] declares physical key [{}] more than once — a key mints "
                 "exactly one physical button, so the duplicate can only be a mistake"),
            InContext, PhysicalButton)
        {}
        if (NOT KeyIsUnclaimed)
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
