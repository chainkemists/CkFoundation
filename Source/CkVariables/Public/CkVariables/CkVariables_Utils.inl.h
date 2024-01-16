#pragma once

#include "CkVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_VariableFragment>
    auto
        TUtils_Variables<T_VariableFragment>::
        Add(
            HandleType InHandle)
        -> void
    {
        InHandle.AddOrGet<FragmentType>();
    }

    template <typename T_VariableFragment>
    auto
        TUtils_Variables<T_VariableFragment>::
        Has(
            HandleType InHandle)
        -> bool
    {
        return InHandle.Has_Any<FragmentType>();
    }

    template <typename T_VariableComponent>
    auto
        TUtils_Variables<T_VariableComponent>::
        Get(
            HandleType   InHandle,
            FGameplayTag InVariableName)
        -> ArgType
    {
        static ValueType Invalid = ValueType{};

        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Could not find the variable with name [{}] in Handle [{}]. "
            "No variables were ever added to this Handle"), InVariableName, InHandle)
        { return Invalid; }

        const auto& VariablesComp = InHandle.Get<FragmentType>();
        auto* FoundVariableWithName = VariablesComp.Get_Variables().Find(InVariableName);

        CK_ENSURE_IF_NOT(ck::IsValid(FoundVariableWithName, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Could not find the variable with name [{}] in Handle [{}]"), InVariableName, InHandle)
        { return Invalid; }

        return *FoundVariableWithName;
    }

    template <typename T_VariableComponent>
    auto
        TUtils_Variables<T_VariableComponent>::
        Set(
            HandleType   InHandle,
            FGameplayTag InVariableName,
            ArgType      InValue)
        -> void
    {
        auto& VariablesComp = InHandle.AddOrGet<FragmentType>();
        auto& FoundVariableWithName = VariablesComp.Get_Variables().FindOrAdd(InVariableName);
        FoundVariableWithName = InValue;
    }
}

// --------------------------------------------------------------------------------------------------------------------
