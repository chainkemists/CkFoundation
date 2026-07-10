#include "CkArchetype_Registry.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/CkEcsLog.h"
#include "CkEcs/DebugFeatureFlags/CkDebugFeatureFlags.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_archetype_registry_impl
{
    struct FEntry
    {
        FCk_ArchetypeDescriptor _Descriptor;
        ck::archetype_registry::MatcherType _NativeMatcher;
        int32 _RegistrationOrder = 0;
    };

    struct FTable
    {
        TArray<FEntry> _Entries;
        int32 _NextRegistrationOrder = 0;
    };

    auto Get_Table() -> FTable&
    {
        static auto Table = FTable{};
        return Table;
    }

    auto SortByPrecedence(TArray<FEntry>& InEntries) -> void
    {
        InEntries.Sort([](const FEntry& InA, const FEntry& InB)
        {
            if (InA._Descriptor.Get_Priority() != InB._Descriptor.Get_Priority())
            { return InA._Descriptor.Get_Priority() > InB._Descriptor.Get_Priority(); }

            if (InA._Descriptor.Get_FeatureIds().Num() != InB._Descriptor.Get_FeatureIds().Num())
            { return InA._Descriptor.Get_FeatureIds().Num() > InB._Descriptor.Get_FeatureIds().Num(); }

            return InA._RegistrationOrder < InB._RegistrationOrder;
        });
    }

    auto Matches(const FEntry& InEntry, const FCk_Handle& InHandle) -> bool
    {
        if (InEntry._NativeMatcher)
        { return InEntry._NativeMatcher(InHandle); }

        const auto& FeatureIds = InEntry._Descriptor.Get_FeatureIds();
        if (FeatureIds.IsEmpty())
        { return false; }

        const auto& Registry = InHandle.Get_RegistryView();
        if (NOT ck::debug_feature_flags::Get_IsEnabled(Registry))
        { return false; }

        auto Required = uint64{0};
        for (const auto& FeatureId : FeatureIds)
        {
            const auto Bit = ck::debug_feature_flags::Get_BitIndex(FeatureId);
            if (Bit == INDEX_NONE)
            { return false; }

            Required |= uint64{1} << Bit;
        }

        const auto Flags = ck::debug_feature_flags::Get_Flags(Registry, InHandle.Get_Entity());
        return (Flags & Required) == Required;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::archetype_registry::
    Register(
        const FCk_ArchetypeDescriptor& InDescriptor,
        MatcherType InNativeMatcher)
    -> void
{
    CK_ENSURE_IF_NOT(NOT InDescriptor.Get_Name().IsNone(),
        TEXT("Cannot register an Archetype with no Name"))
    { return; }

    auto& Table = ck_archetype_registry_impl::Get_Table();

    const auto ExistingIndex = Table._Entries.IndexOfByPredicate(
        [&](const ck_archetype_registry_impl::FEntry& InEntry)
        { return InEntry._Descriptor.Get_Name() == InDescriptor.Get_Name(); });

    if (ExistingIndex != INDEX_NONE)
    {
        ecs::Verbose(TEXT("Archetype [{}] re-registered — replacing the previous registration"),
            InDescriptor.Get_Name());
        Table._Entries.RemoveAt(ExistingIndex);
    }

    Table._Entries.Emplace(ck_archetype_registry_impl::FEntry{
        InDescriptor, MoveTemp(InNativeMatcher), Table._NextRegistrationOrder++});
    ck_archetype_registry_impl::SortByPrecedence(Table._Entries);
}

auto
    ck::archetype_registry::
    Unregister(
        FName InName)
    -> void
{
    auto& Table = ck_archetype_registry_impl::Get_Table();
    Table._Entries.RemoveAll([&](const ck_archetype_registry_impl::FEntry& InEntry)
    {
        return InEntry._Descriptor.Get_Name() == InName;
    });
}

auto
    ck::archetype_registry::
    Find(
        FName InName)
    -> TOptional<FCk_ArchetypeDescriptor>
{
    const auto& Table = ck_archetype_registry_impl::Get_Table();

    const auto Found = Table._Entries.FindByPredicate(
        [&](const ck_archetype_registry_impl::FEntry& InEntry)
        { return InEntry._Descriptor.Get_Name() == InName; });

    if (Found == nullptr)
    { return {}; }

    return Found->_Descriptor;
}

auto
    ck::archetype_registry::
    Get_All()
    -> TArray<FCk_ArchetypeDescriptor>
{
    const auto& Table = ck_archetype_registry_impl::Get_Table();

    auto Result = TArray<FCk_ArchetypeDescriptor>{};
    Result.Reserve(Table._Entries.Num());
    for (const auto& Entry : Table._Entries)
    { Result.Add(Entry._Descriptor); }

    return Result;
}

auto
    ck::archetype_registry::
    Get_Matches(
        const FCk_Handle& InHandle,
        FName InName)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    const auto& Table = ck_archetype_registry_impl::Get_Table();

    const auto Found = Table._Entries.FindByPredicate(
        [&](const ck_archetype_registry_impl::FEntry& InEntry)
        { return InEntry._Descriptor.Get_Name() == InName; });

    if (Found == nullptr)
    { return false; }

    return ck_archetype_registry_impl::Matches(*Found, InHandle);
}

auto
    ck::archetype_registry::
    TryGet_BestMatchName(
        const FCk_Handle& InHandle)
    -> FName
{
    if (ck::Is_NOT_Valid(InHandle))
    { return NAME_None; }

    const auto& Table = ck_archetype_registry_impl::Get_Table();

    for (const auto& Entry : Table._Entries)
    {
        if (ck_archetype_registry_impl::Matches(Entry, InHandle))
        { return Entry._Descriptor.Get_Name(); }
    }

    return NAME_None;
}
