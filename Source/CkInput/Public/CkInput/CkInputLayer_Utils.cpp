#include "CkInputLayer_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkInput/CkInput_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InputLayer_UE,
    FCk_Handle_InputLayer,
    ck::FFragment_InputLayer_Params,
    ck::FFragment_InputLayer_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputLayer_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_InputLayer_ParamsData& InParams)
    -> FCk_Handle_InputLayer
{
    const auto PriorityIsAvailable = InParams.Get_Priority() != ck::input_layer::GlobalActionPriority;
    CK_ENSURE_IF_NOT(PriorityIsAvailable,
        TEXT("Add: priority [{}] is reserved for the global-action layer and cannot be claimed by an ordinary "
             "InputLayer on Handle [{}]"), InParams.Get_Priority(), InHandle)
    { return {}; }

    return DoAddLayer(InHandle, InParams);
}

auto
    UCk_Utils_InputLayer_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_InputLayer_ParamsData& InParams)
    -> FCk_Handle_InputLayer
{
    const auto PriorityIsAvailable = InParams.Get_Priority() != ck::input_layer::GlobalActionPriority;
    CK_ENSURE_IF_NOT(PriorityIsAvailable,
        TEXT("Create: priority [{}] is reserved for the global-action layer and cannot be claimed by an ordinary "
             "InputLayer under owner [{}]"), InParams.Get_Priority(), InOwner)
    { return {}; }

    return DoCreateLayer(InOwner, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputLayer_UE::
    Make_KeyCapture(
        const FKey& InKey,
        ECk_InputLayer_CaptureBehavior InBehavior)
    -> FCk_InputLayer_Capture
{
    return FCk_InputLayer_Capture{ECk_InputLayer_CaptureMatch::Key, InKey, InBehavior};
}

auto
    UCk_Utils_InputLayer_UE::
    Make_CatchAllCapture(
        ECk_InputLayer_CaptureBehavior InBehavior)
    -> FCk_InputLayer_Capture
{
    return FCk_InputLayer_Capture{ECk_InputLayer_CaptureMatch::CatchAll, FKey{}, InBehavior};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputLayer_UE::
    Get_Priority(
        const FCk_Handle_InputLayer& InLayer)
    -> int32
{
    return InLayer.Get<ck::FFragment_InputLayer_Params>().Get_Priority();
}

auto
    UCk_Utils_InputLayer_UE::
    Get_InputSource(
        const FCk_Handle_InputLayer& InLayer)
    -> FCk_Handle_InputSource
{
    return InLayer.Get<ck::FFragment_InputLayer_Params>().Get_InputSource();
}

auto
    UCk_Utils_InputLayer_UE::
    Get_Captures(
        const FCk_Handle_InputLayer& InLayer)
    -> TArray<FCk_InputLayer_Capture>
{
    return InLayer.Get<ck::FFragment_InputLayer_Current>().Get_Captures();
}

auto
    UCk_Utils_InputLayer_UE::
    Get_NumCaptures(
        const FCk_Handle_InputLayer& InLayer)
    -> int32
{
    return InLayer.Get<ck::FFragment_InputLayer_Current>().Get_Captures().Num();
}

auto
    UCk_Utils_InputLayer_UE::
    Get_HasCaptureForKey(
        const FCk_Handle_InputLayer& InLayer,
        ECk_InputLayer_CaptureMatch InMatchMode,
        const FKey& InKey)
    -> bool
{
    return InLayer.Get<ck::FFragment_InputLayer_Current>().Get_Captures().ContainsByPredicate(
    [&](const FCk_InputLayer_Capture& InCapture) -> bool
    {
        return InCapture.Get_MatchMode() == InMatchMode && InCapture.Get_Key() == InKey;
    });
}

auto
    UCk_Utils_InputLayer_UE::
    Get_GlobalActionPriority()
    -> int32
{
    return ck::input_layer::GlobalActionPriority;
}

auto
    UCk_Utils_InputLayer_UE::
    TryGet_LayerWithPriority(
        FCk_Handle_InputSource InInputSource,
        int32 InPriority)
    -> FCk_Handle_InputLayer
{
    if (ck::Is_NOT_Valid(InInputSource))
    { return {}; }

    const auto Registry = InInputSource.Get_RegistryView();

    auto FoundLayer = FCk_Handle_InputLayer{};

    InInputSource.View<
        ck::FFragment_InputLayer_Params,
        ck::FFragment_InputLayer_Current,
        ck::TExclude<ck::FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>().ForEach(
        [&](
            FCk_Entity InEntity,
            const ck::FFragment_InputLayer_Params& InParams,
            const ck::FFragment_InputLayer_Current&)
        {
            if (ck::IsValid(FoundLayer))
            { return; }

            if (InParams.Get_InputSource() != InInputSource)
            { return; }

            if (InParams.Get_Priority() != InPriority)
            { return; }

            auto EntityHandle = ck::MakeHandle(InEntity, Registry);
            FoundLayer = CastChecked(EntityHandle);
        });

    return FoundLayer;
}

auto
    UCk_Utils_InputLayer_UE::
    TryGet_GlobalActionLayer(
        FCk_Handle_InputSource InInputSource)
    -> FCk_Handle_InputLayer
{
    return TryGet_LayerWithPriority(InInputSource, ck::input_layer::GlobalActionPriority);
}

auto
    UCk_Utils_InputLayer_UE::
    TryGet_PressOwner(
        FCk_Handle_InputSource InInputSource,
        const FKey& InKey)
    -> FCk_Handle_InputLayer
{
    if (ck::Is_NOT_Valid(InInputSource))
    { return {}; }

    if (NOT InInputSource.Has<ck::FFragment_InputLayer_RouterState>())
    { return {}; }

    const auto& PressOwners = InInputSource.Get<ck::FFragment_InputLayer_RouterState>().Get_PressOwners();

    const auto OwnerIndex = PressOwners.IndexOfByPredicate(
    [&](const ck::FInputLayer_PressOwnership& InOwnership) -> bool
    {
        return InOwnership.Get_Key() == InKey;
    });

    if (OwnerIndex == INDEX_NONE)
    { return {}; }

    return PressOwners[OwnerIndex].Get_Owner();
}

auto
    UCk_Utils_InputLayer_UE::
    Get_RoutedEventsThisFrame(
        FCk_Handle_InputSource InInputSource)
    -> TArray<FCk_InputLayer_RoutedEvent>
{
    if (ck::Is_NOT_Valid(InInputSource))
    { return {}; }

    if (NOT InInputSource.Has<ck::FFragment_InputLayer_RoutedThisFrame>())
    { return {}; }

    return InInputSource.Get<ck::FFragment_InputLayer_RoutedThisFrame>().Get_RoutedEvents();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputLayer_UE::
    Request_AddCapture(
        FCk_Handle_InputLayer& InLayer,
        const FCk_Request_InputLayer_AddCapture& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputLayer
{
    const auto LayerIsValid = ck::IsValid(InLayer);
    CK_ENSURE_IF_NOT(LayerIsValid,
        TEXT("Request_AddCapture: invalid InputLayer handle [{}] — the capture is dropped"), InLayer)
    {
        InDelegate.ExecuteIfBound(InLayer, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InLayer;
    }

    const auto& Capture = InRequest.Get_Capture();

    const auto CaptureNamesAnInput = Capture.Get_MatchMode() == ECk_InputLayer_CaptureMatch::CatchAll ||
                                     Capture.Get_Key().IsValid();
    CK_ENSURE_IF_NOT(CaptureNamesAnInput,
        TEXT("Request_AddCapture: capture on InputLayer [{}] matches a specific key but names an invalid key, so "
             "it could never match anything"), InLayer)
    {
        InDelegate.ExecuteIfBound(InLayer, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InLayer;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InLayer.AddOrGet<ck::FFragment_InputLayer_Requests>()._Requests.Emplace(InRequest);

    return InLayer;
}

auto
    UCk_Utils_InputLayer_UE::
    Request_RemoveCapture(
        FCk_Handle_InputLayer& InLayer,
        const FCk_Request_InputLayer_RemoveCapture& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputLayer
{
    const auto LayerIsValid = ck::IsValid(InLayer);
    CK_ENSURE_IF_NOT(LayerIsValid,
        TEXT("Request_RemoveCapture: invalid InputLayer handle [{}] — nothing to remove a capture from"), InLayer)
    {
        InDelegate.ExecuteIfBound(InLayer, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InLayer;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InLayer.AddOrGet<ck::FFragment_InputLayer_Requests>()._Requests.Emplace(InRequest);

    return InLayer;
}

auto
    UCk_Utils_InputLayer_UE::
    Request_AddGlobalAction(
        FCk_Handle_InputSource& InInputSource,
        const FCk_Request_InputLayer_AddGlobalAction& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputSource
{
    const auto SourceIsValid = ck::IsValid(InInputSource);
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("Request_AddGlobalAction: invalid InputSource handle [{}] — a global action needs a stack to sit at "
             "the bottom of"), InInputSource)
    {
        InDelegate.ExecuteIfBound(InInputSource, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputSource;
    }

    const auto KeyIsValid = InRequest.Get_Key().IsValid();
    CK_ENSURE_IF_NOT(KeyIsValid,
        TEXT("Request_AddGlobalAction: invalid key on InputSource [{}] — a global action must name a real key"),
        InInputSource)
    {
        InDelegate.ExecuteIfBound(InInputSource, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputSource;
    }

    auto GlobalActionLayer = TryGet_GlobalActionLayer(InInputSource);

    if (ck::Is_NOT_Valid(GlobalActionLayer))
    {
        auto SourceAsOwner = InInputSource.ConvertToHandle();

        GlobalActionLayer = DoCreateLayer(SourceAsOwner,
            FCk_Fragment_InputLayer_ParamsData{InInputSource, ck::input_layer::GlobalActionPriority});
    }

    const auto GlobalActionLayerExists = ck::IsValid(GlobalActionLayer);
    CK_ENSURE_IF_NOT(GlobalActionLayerExists,
        TEXT("Request_AddGlobalAction: the reserved global-action layer for InputSource [{}] could not be created"),
        InInputSource)
    {
        InDelegate.ExecuteIfBound(InInputSource, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputSource;
    }

    auto CaptureRequest = FCk_Request_InputLayer_AddCapture
    {
        FCk_InputLayer_Capture
        {
            ECk_InputLayer_CaptureMatch::Key,
            InRequest.Get_Key(),
            ECk_InputLayer_CaptureBehavior::Consume
        }
    };

    Request_AddCapture(GlobalActionLayer, CaptureRequest, InDelegate);

    return InInputSource;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputLayer_UE::
    BindTo_OnCaptureTriggered(
        FCk_Handle_InputLayer& InLayer,
        const FCk_Delegate_InputLayer_CaptureTriggered& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_InputLayer
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnInputCaptureTriggered, InLayer, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InLayer;
}

auto
    UCk_Utils_InputLayer_UE::
    UnbindFrom_OnCaptureTriggered(
        FCk_Handle_InputLayer& InLayer,
        const FCk_Delegate_InputLayer_CaptureTriggered& InDelegate)
    -> FCk_Handle_InputLayer
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnInputCaptureTriggered, InLayer, InDelegate);

    return InLayer;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputLayer_UE::
    DoGet_RegistrationIsValid(
        const FCk_Handle& InContext,
        const FCk_Fragment_InputLayer_ParamsData& InParams)
    -> bool
{
    const auto SourceIsValid = ck::IsValid(InParams.Get_InputSource());
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("InputLayer registration for [{}] names an invalid InputSource [{}] — a layer with no stack could "
             "never be routed to"), InContext, InParams.Get_InputSource())
    { return false; }

    const auto CollidingLayer = TryGet_LayerWithPriority(InParams.Get_InputSource(), InParams.Get_Priority());

    const auto PriorityIsFree = ck::Is_NOT_Valid(CollidingLayer);
    CK_ENSURE_IF_NOT(PriorityIsFree,
        TEXT("InputLayer priority collision: priority [{}] on InputSource [{}] is already held by layer [{}], so "
             "the registration for [{}] is rejected — arbitration order must be unambiguous"),
        InParams.Get_Priority(), InParams.Get_InputSource(), CollidingLayer, InContext)
    { return false; }

    return true;
}

auto
    UCk_Utils_InputLayer_UE::
    DoAddLayer(
        FCk_Handle& InHandle,
        const FCk_Fragment_InputLayer_ParamsData& InParams)
    -> FCk_Handle_InputLayer
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an InputLayer onto it"), InHandle)
    { return {}; }

    const auto EntityIsNotAlreadyALayer = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(EntityIsNotAlreadyALayer,
        TEXT("Add: Handle [{}] is already an InputLayer — one entity holds at most one place in a stack"), InHandle)
    { return {}; }

    if (NOT DoGet_RegistrationIsValid(InHandle, InParams))
    { return {}; }

    InHandle.Add<ck::FFragment_InputLayer_Params>(InParams);
    InHandle.Add<ck::FFragment_InputLayer_Current>();

    ck::input::Verbose
    (
        TEXT("InputLayer [{}] joined InputSource [{}] at priority [{}]"),
        InHandle, InParams.Get_InputSource(), InParams.Get_Priority()
    );

    return CastChecked(InHandle);
}

auto
    UCk_Utils_InputLayer_UE::
    DoCreateLayer(
        FCk_Handle& InOwner,
        const FCk_Fragment_InputLayer_ParamsData& InParams)
    -> FCk_Handle_InputLayer
{
    const auto OwnerIsValid = ck::IsValid(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Create: invalid owner Handle [{}] — cannot create an InputLayer under it"), InOwner)
    { return {}; }

    if (NOT DoGet_RegistrationIsValid(InOwner, InParams))
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

    return DoAddLayer(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------
