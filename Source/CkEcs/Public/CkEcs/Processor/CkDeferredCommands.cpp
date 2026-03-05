#include "CkDeferredCommands.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FDeferredCommandBuffer::
    DeferCustom(
        FCk_Entity InEntity,
        CommandType InCommand)
    -> void
{
    _Commands.Emplace(
        [Entity = InEntity, Command = MoveTemp(InCommand)]
        (FCk_Handle& InTransientEntity) mutable
    {
        auto Handle = ck::MakeHandle(Entity, InTransientEntity);
        Command(Handle);
    });
}

auto
    ck::FDeferredCommandBuffer::
    DeferDestroy(
        FCk_Entity InEntity)
    -> void
{
    _Commands.Emplace([Entity = InEntity](FCk_Handle& InTransientEntity)
    {
        auto Handle = ck::MakeHandle(Entity, InTransientEntity);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Handle);
    });
}

auto
    ck::FDeferredCommandBuffer::
    Flush(
        FCk_Handle& InTransientEntity)
    -> void
{
    for (auto& Command : _Commands)
    {
        Command(InTransientEntity);
    }

    _Commands.Reset();
}

auto
    ck::FDeferredCommandBuffer::
    IsEmpty() const
    -> bool
{
    return _Commands.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------
