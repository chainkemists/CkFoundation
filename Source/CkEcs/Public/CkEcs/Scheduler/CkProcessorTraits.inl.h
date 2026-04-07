#pragma once

#include "CkProcessorDescriptor.h"

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
            Descriptor._IsDirtyChecker = [](const FCk_Registry& InRegistry) -> bool
            {
                return InRegistry.Has_AnyEntityWith<DirtyFragment>();
            };
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
