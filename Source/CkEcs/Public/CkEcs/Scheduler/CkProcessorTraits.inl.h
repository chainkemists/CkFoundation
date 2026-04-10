#pragma once

#include "CkProcessorDescriptor.h"

#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"

#include <entt/core/type_info.hpp>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename... T_Processors>
    struct TDepList
    {
        using Types = entt::type_list<T_Processors...>;
    };

    // ----------------------------------------------------------------------------------------------------------------

    template <typename T>
    auto
    Get_ProcessorCanonicalName() -> FName
    {
        const auto TypeName = entt::type_name<T>::value();
        return FName{static_cast<int32>(TypeName.size()), TypeName.data()};
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace detail
    {
        template <typename... T_Types>
        auto
        ExtractCanonicalNames(
            entt::type_list<T_Types...>,
            TArray<FName>& OutNames) -> void
        {
            (OutNames.Add(Get_ProcessorCanonicalName<T_Types>()), ...);
        }

        // ----------------------------------------------------------------------------------------------------------------
        // Fragment access extraction — walks a processor's FragmentList (the pack of T_Fragments in the CRTP base) and
        // sorts each entry into either the read-only or read-write hash bucket on the descriptor. Excluded filter
        // wrappers (TExclude<X>) and empty tag types are skipped because they do not participate in data access.
        //
        // Relies on the static_assert on ck::TProcessor / ck_exp::TProcessor: every non-excluded, non-empty fragment is
        // wrapped in TReadOnly<T> or TReadWrite<T>. The FragmentList contract is: entries are ordered exactly as declared
        // on the processor template, and each non-skipped entry is one of:
        //     - ck::TReadOnly<F>    → hash(F) added to OutRO
        //     - ck::TReadWrite<F>   → hash(F) added to OutRW
        //
        // Hashes use entt::type_hash so they agree with any downstream consumer that speaks EnTT hashes.
        // ----------------------------------------------------------------------------------------------------------------

        template <typename T_Fragment>
        auto
        ExtractSingleFragmentAccess(
            TArray<uint32>& OutRO_Hashes,
            TArray<uint32>& OutRW_Hashes,
            TArray<FName>&  OutRO_Names,
            TArray<FName>&  OutRW_Names) -> void
        {
            if constexpr (TIsExcludedPolicy<T_Fragment>::value || TIsEmptyPolicy<T_Fragment>::value)
            {
                // Skip: TExclude<...> filter or empty tag type (not an access, just a filter)
            }
            else
            {
                using RawFragmentType = UnwrapAccessPolicy_T<T_Fragment>;
                const auto Hash = static_cast<uint32>(entt::type_hash<RawFragmentType>::value());
                const auto RawTypeName = entt::type_name<RawFragmentType>::value();
                const auto NameForDiagnostics =
                    FName{static_cast<int32>(RawTypeName.size()), RawTypeName.data()};

                if constexpr (TAccessPolicyTraits<T_Fragment>::IsReadOnly)
                {
                    OutRO_Hashes.Add(Hash);
                    OutRO_Names.Add(NameForDiagnostics);
                }
                else
                {
                    OutRW_Hashes.Add(Hash);
                    OutRW_Names.Add(NameForDiagnostics);
                }
            }
        }

        template <typename... T_Fragments>
        auto
        ExtractFragmentAccessHashes(
            entt::type_list<T_Fragments...>,
            TArray<uint32>& OutRO_Hashes,
            TArray<uint32>& OutRW_Hashes,
            TArray<FName>&  OutRO_Names,
            TArray<FName>&  OutRW_Names) -> void
        {
            (ExtractSingleFragmentAccess<T_Fragments>(
                OutRO_Hashes, OutRW_Hashes, OutRO_Names, OutRW_Names), ...);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    template <typename T_Processor>
    auto
    BuildDescriptor(
        FProcessorFactory InFactory) -> FProcessorDescriptor
    {
        auto Descriptor = FProcessorDescriptor{};

        Descriptor._Name = Get_ProcessorCanonicalName<T_Processor>();
        Descriptor._Factory = MoveTemp(InFactory);

        if constexpr (requires { typename T_Processor::Group; })
        {
            Descriptor._GroupName = Get_ProcessorCanonicalName<typename T_Processor::Group>();
        }

        if constexpr (requires { typename T_Processor::RunAfter; })
        {
            detail::ExtractCanonicalNames(
                typename T_Processor::RunAfter::Types{},
                Descriptor._RunAfter);
        }

        if constexpr (requires { typename T_Processor::RunBefore; })
        {
            detail::ExtractCanonicalNames(
                typename T_Processor::RunBefore::Types{},
                Descriptor._RunBefore);
        }

        if constexpr (requires { typename T_Processor::RunAfterTags; })
        {
            T_Processor::RunAfterTags::AppendTo(Descriptor._RunAfterTags);
        }

        if constexpr (requires { typename T_Processor::RunBeforeTags; })
        {
            T_Processor::RunBeforeTags::AppendTo(Descriptor._RunBeforeTags);
        }

        if constexpr (requires { typename T_Processor::MarkedDirtyBy; })
        {
            using DirtyFragment = typename T_Processor::MarkedDirtyBy;
            Descriptor._HasDirtyMarker = true;
            Descriptor._DirtyMarkerHash = static_cast<uint32>(entt::type_hash<DirtyFragment>::value());
            Descriptor._IsDirtyChecker = [](const FCk_Registry& InRegistry) -> bool
            {
                return InRegistry.Has_AnyEntityWith<DirtyFragment>();
            };
        }

        // ---- Extract fragment access metadata from the processor's FragmentList ----
        // The FragmentList alias is exposed by ck::TProcessor and ck_exp::TProcessor and holds the raw T_Fragments
        // template pack including TReadOnly<>/TReadWrite<> wrappers, TExclude<> filters, and empty tag types.
        if constexpr (requires { typename T_Processor::FragmentList; })
        {
            detail::ExtractFragmentAccessHashes(
                typename T_Processor::FragmentList{},
                Descriptor._RO_FragmentHashes,
                Descriptor._RW_FragmentHashes,
                Descriptor._RO_FragmentNames,
                Descriptor._RW_FragmentNames);
        }

        if constexpr (requires { T_Processor::NetModeRequirement; })
        {
            Descriptor._NetModeRequirement = ECk_ProcessorNetMode::Override;
            Descriptor._NetModeRequirementValue = T_Processor::NetModeRequirement;
        }

        if constexpr (requires { T_Processor::TickGroup; })
        {
            Descriptor._TickGroupMode = ECk_TickGroupMode::Explicit;
            Descriptor._TickGroupValue = T_Processor::TickGroup;
        }

        return Descriptor;
    }

    // ----------------------------------------------------------------------------------------------------------------

    template <typename T_Group>
    auto
    BuildGroupDescriptor() -> FProcessorDescriptor
    {
        auto Descriptor = FProcessorDescriptor{};

        Descriptor._Name = Get_ProcessorCanonicalName<T_Group>();

        if constexpr (requires { typename T_Group::Group; })
        {
            Descriptor._GroupName = Get_ProcessorCanonicalName<typename T_Group::Group>();
        }

        if constexpr (requires { typename T_Group::RunAfter; })
        {
            detail::ExtractCanonicalNames(
                typename T_Group::RunAfter::Types{},
                Descriptor._RunAfter);
        }

        if constexpr (requires { typename T_Group::RunBefore; })
        {
            detail::ExtractCanonicalNames(
                typename T_Group::RunBefore::Types{},
                Descriptor._RunBefore);
        }

        if constexpr (requires { T_Group::TickGroup; })
        {
            Descriptor._TickGroupMode = ECk_TickGroupMode::Explicit;
            Descriptor._TickGroupValue = T_Group::TickGroup;
        }

        return Descriptor;
    }
}

// --------------------------------------------------------------------------------------------------------------------
