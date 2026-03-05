#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Processor/CkDeferredCommands.h"
#include "CkEcs/Registry/CkRegistry.h"

namespace ck
{
    template <typename, typename, typename...>
    class TParallelProcessor;
}

// --------------------------------------------------------------------------------------------------------------------

struct CKECS_API FCk_Handle_ReadOnly
{
    CK_GENERATED_BODY(FCk_Handle_ReadOnly);

    // ---- Read-only operations — safe for concurrent access ----

    template <typename T_Fragment>
    auto
    Has() const -> bool;

    template <typename... T_Fragment>
    auto
    Has_Any() const -> bool;

    template <typename... T_Fragment>
    auto
    Has_All() const -> bool;

    template <typename T_Fragment>
    auto
    Get() const -> const T_Fragment&;

    auto
    Get_Entity() const -> const FCk_Entity&;

    auto
    Get_DebugName() const -> FName;

    // ---- Cross-entity read access — safe for concurrent access ----
    // Returns a new ReadOnly handle for a different entity using the same registry.
    // Deferred commands on the returned handle go to the same per-task command buffer.

    auto
    ReadEntity(
        FCk_Entity InOtherEntity) const -> FCk_Handle_ReadOnly;

    // ---- Deferred mutation operations — thread-safe ----

    template <typename T_Fragment, typename... T_Args>
    auto
    DeferAdd(
        T_Args&&... InArgs) const -> void;

    template <typename T_Fragment>
    auto
    DeferRemove() const -> void;

    template <typename T_Fragment>
    auto
    DeferTry_Remove() const -> void;

    auto
    DeferDestroy() const -> void;

    template <typename T_Fragment, typename... T_Args>
    auto
    DeferReplace(
        T_Args&&... InArgs) const -> void;

    template <typename T_Fragment, typename... T_Args>
    auto
    DeferAddOrReplace(
        T_Args&&... InArgs) const -> void;

    template <typename T_Fragment, typename... T_Args>
    auto
    DeferAddOrGet(
        T_Args&&... InArgs) const -> void;

    auto
    DeferCustom(
        TFunction<void(FCk_Handle&)> InCommand) const -> void;

    // ---- Direct mutations are DELETED — compile error with helpful message ----

    template <typename T_Fragment, typename... T_Args>
    [[deprecated("Direct Add() is not thread-safe in TParallelProcessor. Use DeferAdd<>() instead.")]]
    auto Add(T_Args&&...) -> T_Fragment& = delete;

    template <typename T_Fragment, typename... T_Args>
    [[deprecated("Direct AddOrGet() is not thread-safe. Use DeferAddOrGet<>() instead.")]]
    auto AddOrGet(T_Args&&...) -> T_Fragment& = delete;

    template <typename T_Fragment>
    [[deprecated("Direct Remove() is not thread-safe. Use DeferRemove<>() instead.")]]
    auto Remove() -> void = delete;

    template <typename T_Fragment>
    [[deprecated("Direct Try_Remove() is not thread-safe. Use DeferTry_Remove<>() instead.")]]
    auto Try_Remove() -> int32 = delete;

    template <typename T_Fragment, typename... T_Args>
    [[deprecated("Direct Replace() is not thread-safe. Use DeferReplace<>() instead.")]]
    auto Replace(T_Args&&...) -> T_Fragment& = delete;

    template <typename T_Fragment, typename... T_Args>
    [[deprecated("Direct AddOrReplace() is not thread-safe. Use DeferAddOrReplace<>() instead.")]]
    auto AddOrReplace(T_Args&&...) -> T_Fragment& = delete;

    template <typename... T_Fragments>
    [[deprecated("Direct Clear() is not thread-safe. Use DeferRemove<>() for each fragment.")]]
    auto Clear() -> void = delete;

private:
    FCk_Entity _Entity;
    TOptional<FCk_Registry> _Registry;
    // Lifetime scoped by ParallelForWithTaskContext — the TArray<FDeferredCommandBuffer>
    // lives for the duration of the ParallelFor call. No smart pointer overhead needed.
    ck::FDeferredCommandBuffer* _DeferredCommands = nullptr;

    FCk_Handle_ReadOnly() = default;

    FCk_Handle_ReadOnly(
        FCk_Entity InEntity,
        const FCk_Registry& InRegistry,
        ck::FDeferredCommandBuffer& InDeferredCommands);

    template <typename, typename, typename...>
    friend class ck::TParallelProcessor;
};

// ---- Type-safe variant ----

template <typename T_HandleType>
struct TCk_Handle_ReadOnly : public FCk_Handle_ReadOnly
{
    using FCk_Handle_ReadOnly::FCk_Handle_ReadOnly;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions
// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
auto
    FCk_Handle_ReadOnly::
    Has() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has query with Fragment [{}]. ReadOnly Handle does NOT have a valid Registry."),
        ck::Get_RuntimeTypeToString<T_Fragment>())
    { return false; }

    return _Registry->Has<T_Fragment>(_Entity);
}

template <typename... T_Fragment>
auto
    FCk_Handle_ReadOnly::
    Has_Any() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_Any query. ReadOnly Handle does NOT have a valid Registry."))
    { return false; }

    return _Registry->Has_Any<T_Fragment...>(_Entity);
}

template <typename... T_Fragment>
auto
    FCk_Handle_ReadOnly::
    Has_All() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Has_All query. ReadOnly Handle does NOT have a valid Registry."))
    { return false; }

    return _Registry->Has_All<T_Fragment...>(_Entity);
}

template <typename T_Fragment>
auto
    FCk_Handle_ReadOnly::
    Get() const
    -> const T_Fragment&
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to Get Fragment [{}]. ReadOnly Handle does NOT have a valid Registry."),
        ck::Get_RuntimeTypeToString<T_Fragment>())
    {
        thread_local T_Fragment Invalid_Fragment{};
        return Invalid_Fragment;
    }

    return _Registry->Get<T_Fragment>(_Entity);
}

// ---- Deferred mutation implementations ----

template <typename T_Fragment, typename... T_Args>
auto
    FCk_Handle_ReadOnly::
    DeferAdd(
        T_Args&&... InArgs) const
    -> void
{
    _DeferredCommands->DeferAdd<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment>
auto
    FCk_Handle_ReadOnly::
    DeferRemove() const
    -> void
{
    _DeferredCommands->DeferRemove<T_Fragment>(_Entity);
}

template <typename T_Fragment>
auto
    FCk_Handle_ReadOnly::
    DeferTry_Remove() const
    -> void
{
    _DeferredCommands->DeferTry_Remove<T_Fragment>(_Entity);
}

template <typename T_Fragment, typename... T_Args>
auto
    FCk_Handle_ReadOnly::
    DeferReplace(
        T_Args&&... InArgs) const
    -> void
{
    _DeferredCommands->DeferReplace<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename... T_Args>
auto
    FCk_Handle_ReadOnly::
    DeferAddOrReplace(
        T_Args&&... InArgs) const
    -> void
{
    _DeferredCommands->DeferAddOrReplace<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

template <typename T_Fragment, typename... T_Args>
auto
    FCk_Handle_ReadOnly::
    DeferAddOrGet(
        T_Args&&... InArgs) const
    -> void
{
    _DeferredCommands->DeferAddOrGet<T_Fragment>(_Entity, std::forward<T_Args>(InArgs)...);
}

// --------------------------------------------------------------------------------------------------------------------
