#include "CkDebugFeatureFlags.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_debug_feature_flags_impl
{
    struct FFlagTable
    {
        TArray<ck::debug_feature_flags::FConnector> _Connectors;
        TMap<FName, int32> _BitByFeatureId;
    };

    auto Get_FlagTable() -> FFlagTable&
    {
        static auto Table = FFlagTable{};
        return Table;
    }

    auto Get_Impl(ck::registry_table::EnttRegistryType& InRegistry) -> ck::FCtx_DebugFeatureFlags::FImpl*
    {
        const auto Found = InRegistry.ctx().find<ck::FCtx_DebugFeatureFlags>();
        return Found != nullptr && Found->_Impl.IsValid() ? Found->_Impl.Get() : nullptr;
    }

    auto Get_Impl(const ck::registry_table::EnttRegistryType& InRegistry) -> const ck::FCtx_DebugFeatureFlags::FImpl*
    {
        const auto Found = InRegistry.ctx().find<ck::FCtx_DebugFeatureFlags>();
        return Found != nullptr && Found->_Impl.IsValid() ? Found->_Impl.Get() : nullptr;
    }

    auto Get_EntityIndex(FCk_Entity::IdType InEntity) -> int32
    {
        return static_cast<int32>(entt::entt_traits<FCk_Entity::IdType>::to_entity(InEntity));
    }

    auto Set_Bit(ck::registry_table::EnttRegistryType& InRegistry, FCk_Entity::IdType InEntity, int32 InBit, bool InSet) -> void
    {
        auto Impl = Get_Impl(InRegistry);
        if (Impl == nullptr)
        { return; }

        const auto Index = Get_EntityIndex(InEntity);
        if (InSet)
        {
            if (Index >= Impl->_Rows.Num())
            { Impl->_Rows.AddZeroed(Index - Impl->_Rows.Num() + 1); }

            Impl->_Rows[Index] |= uint64{1} << InBit;
        }
        else if (Impl->_Rows.IsValidIndex(Index))
        {
            Impl->_Rows[Index] &= ~(uint64{1} << InBit);
        }

        ++Impl->_Revision;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::debug_feature_flags::FBitListener::
    OnAdded(
        registry_table::EnttRegistryType& InRegistry,
        FCk_Entity::IdType InEntity)
    -> void
{
    ck_debug_feature_flags_impl::Set_Bit(InRegistry, InEntity, _Bit, true);
}

auto
    ck::debug_feature_flags::FBitListener::
    OnRemoved(
        registry_table::EnttRegistryType& InRegistry,
        FCk_Entity::IdType InEntity)
    -> void
{
    ck_debug_feature_flags_impl::Set_Bit(InRegistry, InEntity, _Bit, false);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::debug_feature_flags::
    DoRegister(
        FConnector InConnector)
    -> int32
{
    auto& Table = ck_debug_feature_flags_impl::Get_FlagTable();

    if (const auto* Existing = Table._BitByFeatureId.Find(InConnector._FeatureId))
    {
        ecs::VeryVerbose(TEXT("DebugFeatureFlag [{}] already registered at bit [{}] — re-registration is a no-op"),
            InConnector._FeatureId, *Existing);
        return *Existing;
    }

    CK_ENSURE_IF_NOT(Table._Connectors.Num() < MaxFlags,
        TEXT("DebugFeatureFlags is full ([{}] flags) — cannot register [{}]. Raise MaxFlags (widen the row type) if the feature set legitimately grew."),
        MaxFlags, InConnector._FeatureId)
    { return INDEX_NONE; }

    const auto Bit = Table._Connectors.Num();
    Table._BitByFeatureId.Add(InConnector._FeatureId, Bit);
    Table._Connectors.Emplace(MoveTemp(InConnector));

    return Bit;
}

auto
    ck::debug_feature_flags::
    Get_BitIndex(
        FName InFeatureId)
    -> int32
{
    const auto& Table = ck_debug_feature_flags_impl::Get_FlagTable();
    const auto* Found = Table._BitByFeatureId.Find(InFeatureId);
    return Found != nullptr ? *Found : INDEX_NONE;
}

auto
    ck::debug_feature_flags::
    Get_RegisteredFeatureIds()
    -> TArray<FName>
{
    const auto& Table = ck_debug_feature_flags_impl::Get_FlagTable();

    auto Result = TArray<FName>{};
    Result.Reserve(Table._Connectors.Num());
    for (const auto& Connector : Table._Connectors)
    { Result.Add(Connector._FeatureId); }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::debug_feature_flags::
    Enable(
        const FCk_Registry& InRegistry)
    -> void
{
    auto Registry = registry_table::Resolve(InRegistry.Get_RegistryHandle());

    CK_ENSURE_IF_NOT(Registry != nullptr,
        TEXT("Cannot Enable DebugFeatureFlags — Registry [{}] does not resolve"), InRegistry)
    { return; }

    if (ck_debug_feature_flags_impl::Get_Impl(*Registry) != nullptr)
    { return; }

    auto& Ctx = Registry->ctx().emplace<FCtx_DebugFeatureFlags>();
    Ctx._Impl = MakeShared<FCtx_DebugFeatureFlags::FImpl>();
    auto& Impl = *Ctx._Impl;

    const auto& Table = ck_debug_feature_flags_impl::Get_FlagTable();
    for (auto Bit = 0; Bit < Table._Connectors.Num(); ++Bit)
    {
        const auto& Connector = Table._Connectors[Bit];

        auto& Listener = *Impl._Listeners.Emplace_GetRef(MakeUnique<FBitListener>());
        Listener._Bit = Bit;

        Connector._Connect(*Registry, Listener, Impl._Connections);
        Connector._Seed(*Registry, Listener);
    }

    ecs::Verbose(TEXT("DebugFeatureFlags ENABLED on Registry [{}] — [{}] flags connected"),
        InRegistry, Table._Connectors.Num());
}

auto
    ck::debug_feature_flags::
    Disable(
        const FCk_Registry& InRegistry)
    -> void
{
    auto Registry = registry_table::TryResolve(InRegistry.Get_RegistryHandle());
    if (Registry == nullptr)
    { return; }

    if (ck_debug_feature_flags_impl::Get_Impl(*Registry) == nullptr)
    { return; }

    // scoped_connections release in the ctx payload's destructor.
    Registry->ctx().erase<FCtx_DebugFeatureFlags>();

    ecs::Verbose(TEXT("DebugFeatureFlags DISABLED on Registry [{}]"), InRegistry);
}

auto
    ck::debug_feature_flags::
    Get_IsEnabled(
        const FCk_Registry& InRegistry)
    -> bool
{
    auto Registry = registry_table::TryResolve(InRegistry.Get_RegistryHandle());
    if (Registry == nullptr)
    { return false; }

    return ck_debug_feature_flags_impl::Get_Impl(*Registry) != nullptr;
}

auto
    ck::debug_feature_flags::
    Get_Flags(
        const FCk_Registry& InRegistry,
        FCk_Entity InEntity)
    -> uint64
{
    auto Registry = registry_table::TryResolve(InRegistry.Get_RegistryHandle());
    if (Registry == nullptr)
    { return 0; }

    const auto Impl = ck_debug_feature_flags_impl::Get_Impl(*Registry);
    if (Impl == nullptr)
    { return 0; }

    const auto Index = ck_debug_feature_flags_impl::Get_EntityIndex(InEntity.Get_ID());
    return Impl->_Rows.IsValidIndex(Index) ? Impl->_Rows[Index] : 0;
}

auto
    ck::debug_feature_flags::
    Get_Revision(
        const FCk_Registry& InRegistry)
    -> uint64
{
    auto Registry = registry_table::TryResolve(InRegistry.Get_RegistryHandle());
    if (Registry == nullptr)
    { return 0; }

    const auto Impl = ck_debug_feature_flags_impl::Get_Impl(*Registry);
    return Impl != nullptr ? Impl->_Revision : 0;
}
