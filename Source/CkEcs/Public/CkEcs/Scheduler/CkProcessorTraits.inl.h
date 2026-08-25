#pragma once

#include "CkProcessorDescriptor.h"

#include "CkCore/Time/CkTime.h"

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
        template <typename... T_Markers>
        auto
        ExtractDirtyMarkers(
            entt::type_list<T_Markers...>,
            FProcessorDescriptor& OutDescriptor) -> void
        {
            static_assert(sizeof...(T_Markers) > 0, "MarkedDirtyByAnyOf must list at least one marker fragment");

            OutDescriptor._HasDirtyMarker = true;
            (OutDescriptor._DirtyMarkerHashes.Add(static_cast<uint32>(entt::type_hash<T_Markers>::value())), ...);
            (OutDescriptor._DirtyMarkerNames.Add(Get_ProcessorCanonicalName<T_Markers>()), ...);
            OutDescriptor._IsDirtyChecker = [](const FCk_Registry& InRegistry) -> bool
            {
                return (InRegistry.Has_AnyLiveEntityWith<T_Markers>() || ...);
            };
            OutDescriptor._IsDirtyChecker_Consumable = [](const FCk_Registry& InRegistry) -> bool
            {
                return (InRegistry.Has_AnyLiveEntityWith_Excluding<T_Markers,
                    ck::FTag_DestroyEntity_EndPlay, ck::FTag_DestroyEntity_Teardown,
                    ck::FTag_DestroyEntity_Await, ck::FTag_DestroyEntity_Finalize>() || ...);
            };
        }

        // ----------------------------------------------------------------------------------------------------------------
        template <typename... T_Fragments>
        auto
        MakeViewConsumableChecker(
            entt::type_list<T_Fragments...>)
            -> FDirtyChecker
        {
            return [](const FCk_Registry& InRegistry) -> bool
            {
                return InRegistry.View<UnwrapAccessPolicy_T<T_Fragments>...>().HasAny();
            };
        }

        // ----------------------------------------------------------------------------------------------------------------
        // ck::TProcessor / ck_exp::TProcessor static_assert that every non-excluded, non-empty fragment is
        // wrapped in TReadOnly<T> or TReadWrite<T>, which is what makes the else-branch below exhaustive.
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

        // ----------------------------------------------------------------------------------------------------------------
        // An exclude only shrinks a view, so it can never make an empty intersection non-empty and cannot
        // help prove emptiness. TIgnoreInEditor is stripped too: its participation depends on which world
        // variant the view runs in, so a skip may only fire when BOTH variants' views are guaranteed empty.
        template <typename T_Fragment>
        using ViewIncludeOf = std::conditional_t<
            TIsExcludedPolicy<T_Fragment>::value || TIsIgnoreInEditor<T_Fragment>::value,
            entt::type_list<>,
            entt::type_list<UnwrapAccessPolicy_T<T_Fragment>>>;

        template <typename T_FragmentList>
        struct TViewIncludesOf;

        template <typename... T_Fragments>
        struct TViewIncludesOf<entt::type_list<T_Fragments...>>
        {
            using Type = entt::type_list_cat_t<ViewIncludeOf<T_Fragments>...>;
        };

        template <typename... T_Includes>
        auto
        ExtractViewIncludeMetadata(
            entt::type_list<T_Includes...>,
            FProcessorDescriptor& OutDescriptor) -> void
        {
            if constexpr (sizeof...(T_Includes) > 0)
            {
                (OutDescriptor._ViewIncludeHashes.Add(static_cast<uint32>(entt::type_hash<T_Includes>::value())), ...);
                (OutDescriptor._ViewIncludeNames.Add(Get_ProcessorCanonicalName<T_Includes>()), ...);
                OutDescriptor._IsViewProvablyEmpty = [](const FCk_Registry& InRegistry) -> bool
                {
                    // "Some include has zero live entities", spelled NOT(all non-empty) because MSVC
                    // mis-parses a parenthesized unary operand inside a pack fold in a lambda.
                    return NOT (InRegistry.Has_AnyLiveEntityWith<T_Includes>() && ...);
                };
                OutDescriptor._CanSkipWhenViewEmpty = true;
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // A processor whose DoTick is the inherited, template-generated view iteration may be empty-view skipped
    // automatically. A custom DoTick is ineligible unless it explicitly declares MainPassRequiredFragments —
    // the author is then asserting that ALL custom main-pass work is gated by those live fragments.
    // Generated-DoTick detection leans on name hiding: a pointer-to-member-of-derived will not convert to the
    // base's type.

    template <typename T_Processor>
    constexpr auto
    Get_HasGeneratedViewDoTick() -> bool
    {
        if constexpr (requires { typename T_Processor::GeneratedDoTickHost; })
        {
            using HostType = typename T_Processor::GeneratedDoTickHost;
            using GeneratedDoTickPtrType = auto (HostType::*)(FCk_Time) -> void;
            return requires { GeneratedDoTickPtrType{&T_Processor::DoTick}; };
        }
        else
        { return false; }
    }

    template <typename T_Processor>
    constexpr auto
    Get_EmptyViewPolicyAllowsSkip() -> bool
    {
        if constexpr (requires { T_Processor::EmptyViewPolicy; })
        { return T_Processor::EmptyViewPolicy == ECk_ProcessorEmptyViewPolicy::SkipWhenProvablyEmpty; }
        else
        { return true; }
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

        if constexpr (requires { typename T_Processor::LocalSettleAfter; })
        {
            Descriptor._LocalSettleAfterGroupName =
                Get_ProcessorCanonicalName<typename T_Processor::LocalSettleAfter>();
        }

        if constexpr (requires { T_Processor::LocalSettleTrigger; })
        {
            static_assert(requires { typename T_Processor::LocalSettleAfter; },
                "LocalSettleTrigger requires LocalSettleAfter to name the barrier.");
            Descriptor._IsLocalSettleTrigger = static_cast<bool>(T_Processor::LocalSettleTrigger);
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
            Descriptor._DirtyMarkerHashes.Add(static_cast<uint32>(entt::type_hash<DirtyFragment>::value()));
            const auto DirtyMarkerTypeName = entt::type_name<DirtyFragment>::value();
            Descriptor._DirtyMarkerNames.Add(FName{static_cast<int32>(DirtyMarkerTypeName.size()), DirtyMarkerTypeName.data()});
            Descriptor._IsDirtyChecker = [](const FCk_Registry& InRegistry) -> bool
            {
                return InRegistry.Has_AnyLiveEntityWith<DirtyFragment>();
            };
            Descriptor._IsDirtyChecker_Consumable = [](const FCk_Registry& InRegistry) -> bool
            {
                return InRegistry.Has_AnyLiveEntityWith_Excluding<DirtyFragment,
                    ck::FTag_DestroyEntity_EndPlay, ck::FTag_DestroyEntity_Teardown,
                    ck::FTag_DestroyEntity_Await, ck::FTag_DestroyEntity_Finalize>();
            };
        }
        else if constexpr (requires { typename T_Processor::MarkedDirtyByAnyOf; })
        {
            detail::ExtractDirtyMarkers(
                typename T_Processor::MarkedDirtyByAnyOf::Types{},
                Descriptor);
        }

        // The settle barrier needs "consumable work", not "marker present": a marker stranded on an
        // entity the processor's own view skips (dying, exited, request-excluded) must not read as
        // dirty, or the barrier spins its whole pass budget on work no pass can drain. Rebuild the
        // consumable checker from the processor's actual runtime view when one is exposed; the
        // pending-kill tag approximation set above remains the fallback (script processors).
        if (Descriptor._HasDirtyMarker)
        {
            if constexpr (requires { typename T_Processor::RuntimeVariantFragments; })
            {
                Descriptor._IsDirtyChecker_Consumable =
                    detail::MakeViewConsumableChecker(typename T_Processor::RuntimeVariantFragments{});
            }
            else if constexpr (requires { typename T_Processor::FragmentList; })
            {
                Descriptor._IsDirtyChecker_Consumable =
                    detail::MakeViewConsumableChecker(typename T_Processor::FragmentList{});
            }
        }

        if constexpr (requires { typename T_Processor::FragmentList; })
        {
            detail::ExtractFragmentAccessHashes(
                typename T_Processor::FragmentList{},
                Descriptor._RO_FragmentHashes,
                Descriptor._RW_FragmentHashes,
                Descriptor._RO_FragmentNames,
                Descriptor._RW_FragmentNames);
        }

        if constexpr (Get_EmptyViewPolicyAllowsSkip<T_Processor>())
        {
            if constexpr (requires { typename T_Processor::MainPassRequiredFragments; })
            {
                detail::ExtractViewIncludeMetadata(
                    typename T_Processor::MainPassRequiredFragments{},
                    Descriptor);
            }
            else if constexpr (Get_HasGeneratedViewDoTick<T_Processor>())
            {
                detail::ExtractViewIncludeMetadata(
                    typename detail::TViewIncludesOf<typename T_Processor::FragmentList>::Type{},
                    Descriptor);
            }
        }

        if constexpr (requires { T_Processor::NetModeRequirement; })
        {
            Descriptor._NetModeRequirement = ECk_ProcessorNetMode::Override;
            Descriptor._NetModeRequirementValue = T_Processor::NetModeRequirement;
        }

        if constexpr (requires { T_Processor::WorldTypeRequirement; })
        {
            Descriptor._WorldTypeRequirement = T_Processor::WorldTypeRequirement;
        }

        if constexpr (requires { T_Processor::TickGroup; })
        {
            Descriptor._TickGroupMode = ECk_TickGroupMode::Explicit;
            Descriptor._TickGroupValue = T_Processor::TickGroup;
        }

        if constexpr (requires { T_Processor::PumpPolicy; })
        {
            Descriptor._PumpPolicy = T_Processor::PumpPolicy;
        }

        if constexpr (requires { T_Processor::LoadPolicy; })
        {
            Descriptor._LoadPolicy = T_Processor::LoadPolicy;
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
