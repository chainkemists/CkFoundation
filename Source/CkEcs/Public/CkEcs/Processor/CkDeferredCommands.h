#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Thread-local deferred command buffer — NO synchronization.
    // Each ParallelFor worker thread gets its own instance via task contexts.
    // Must only be accessed from a single thread at a time.
    class CKECS_API FDeferredCommandBuffer
    {
    public:
        CK_GENERATED_BODY(FDeferredCommandBuffer);

        using CommandType = TFunction<void(FCk_Handle&)>;

        template <typename T_Fragment, typename... T_Args>
        auto
        DeferAdd(
            FCk_Entity InEntity,
            T_Args&&... InArgs) -> void
        {
            _Commands.Emplace(
                [Entity = InEntity, Args = std::make_tuple(std::forward<T_Args>(InArgs)...)]
                (FCk_Handle& InTransientEntity) mutable
            {
                auto Handle = ck::MakeHandle(Entity, InTransientEntity);
                std::apply([&](auto&&... UnpackedArgs)
                {
                    Handle.template Add<T_Fragment>(std::forward<decltype(UnpackedArgs)>(UnpackedArgs)...);
                }, std::move(Args));
            });
        }

        template <typename T_Fragment>
        auto
        DeferRemove(
            FCk_Entity InEntity) -> void
        {
            _Commands.Emplace([Entity = InEntity](FCk_Handle& InTransientEntity)
            {
                auto Handle = ck::MakeHandle(Entity, InTransientEntity);
                Handle.template Remove<T_Fragment>();
            });
        }

        template <typename T_Fragment>
        auto
        DeferTry_Remove(
            FCk_Entity InEntity) -> void
        {
            _Commands.Emplace([Entity = InEntity](FCk_Handle& InTransientEntity)
            {
                auto Handle = ck::MakeHandle(Entity, InTransientEntity);
                Handle.template Try_Remove<T_Fragment>();
            });
        }

        template <typename T_Fragment, typename... T_Args>
        auto
        DeferReplace(
            FCk_Entity InEntity,
            T_Args&&... InArgs) -> void
        {
            _Commands.Emplace(
                [Entity = InEntity, Args = std::make_tuple(std::forward<T_Args>(InArgs)...)]
                (FCk_Handle& InTransientEntity) mutable
            {
                auto Handle = ck::MakeHandle(Entity, InTransientEntity);
                std::apply([&](auto&&... UnpackedArgs)
                {
                    Handle.template Replace<T_Fragment>(std::forward<decltype(UnpackedArgs)>(UnpackedArgs)...);
                }, std::move(Args));
            });
        }

        template <typename T_Fragment, typename... T_Args>
        auto
        DeferAddOrReplace(
            FCk_Entity InEntity,
            T_Args&&... InArgs) -> void
        {
            _Commands.Emplace(
                [Entity = InEntity, Args = std::make_tuple(std::forward<T_Args>(InArgs)...)]
                (FCk_Handle& InTransientEntity) mutable
            {
                auto Handle = ck::MakeHandle(Entity, InTransientEntity);
                std::apply([&](auto&&... UnpackedArgs)
                {
                    Handle.template AddOrReplace<T_Fragment>(std::forward<decltype(UnpackedArgs)>(UnpackedArgs)...);
                }, std::move(Args));
            });
        }

        template <typename T_Fragment, typename... T_Args>
        auto
        DeferAddOrGet(
            FCk_Entity InEntity,
            T_Args&&... InArgs) -> void
        {
            _Commands.Emplace(
                [Entity = InEntity, Args = std::make_tuple(std::forward<T_Args>(InArgs)...)]
                (FCk_Handle& InTransientEntity) mutable
            {
                auto Handle = ck::MakeHandle(Entity, InTransientEntity);
                std::apply([&](auto&&... UnpackedArgs)
                {
                    Handle.template AddOrGet<T_Fragment>(std::forward<decltype(UnpackedArgs)>(UnpackedArgs)...);
                }, std::move(Args));
            });
        }

        auto
        DeferCustom(
            FCk_Entity InEntity,
            CommandType InCommand) -> void;

        auto
        DeferDestroy(
            FCk_Entity InEntity) -> void;

        auto
        Flush(
            FCk_Handle& InTransientEntity) -> void;

        auto
        IsEmpty() const -> bool;

    private:
        TArray<CommandType> _Commands;
    };
}

// --------------------------------------------------------------------------------------------------------------------
