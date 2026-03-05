#include "CkHandle_ReadOnly.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_ReadOnly::
    FCk_Handle_ReadOnly(
        FCk_Entity InEntity,
        const FCk_Registry& InRegistry,
        ck::FDeferredCommandBuffer& InDeferredCommands)
    : _Entity(InEntity)
    , _Registry(InRegistry)
    , _DeferredCommands(&InDeferredCommands)
{
}

auto
    FCk_Handle_ReadOnly::
    Get_Entity() const
    -> const FCk_Entity&
{
    return _Entity;
}

auto
    FCk_Handle_ReadOnly::
    Get_DebugName() const
    -> FName
{
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    if (ck::Is_NOT_Valid(_Registry))
    { return NAME_None; }

    if (NOT _Registry->IsValid(_Entity))
    { return NAME_None; }

    if (NOT _Registry->Has<DEBUG_NAME>(_Entity))
    { return NAME_None; }

    return _Registry->Get<DEBUG_NAME>(_Entity).Get_Name();
#else
    return NAME_None;
#endif
}

auto
    FCk_Handle_ReadOnly::
    DeferDestroy() const
    -> void
{
    _DeferredCommands->DeferDestroy(_Entity);
}

auto
    FCk_Handle_ReadOnly::
    DeferCustom(
        TFunction<void(FCk_Handle&)> InCommand) const
    -> void
{
    _DeferredCommands->DeferCustom(_Entity, MoveTemp(InCommand));
}

auto
    FCk_Handle_ReadOnly::
    ReadEntity(
        FCk_Entity InOtherEntity) const
    -> FCk_Handle_ReadOnly
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to ReadEntity. ReadOnly Handle does NOT have a valid Registry."))
    {
        auto InvalidHandle = FCk_Handle_ReadOnly{};
        InvalidHandle._Entity = InOtherEntity;
        InvalidHandle._DeferredCommands = _DeferredCommands;
        return InvalidHandle;
    }

    return FCk_Handle_ReadOnly{InOtherEntity, *_Registry, *_DeferredCommands};
}

// --------------------------------------------------------------------------------------------------------------------
