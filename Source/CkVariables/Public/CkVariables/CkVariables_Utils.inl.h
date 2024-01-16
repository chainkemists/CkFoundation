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

    template <typename T_VariableFragment>
    auto
        TUtils_Variables<T_VariableFragment>::
        Ensure(
            HandleType InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Variables Component"), InHandle)
        { return false; }

        return true;
    }

    template <typename T_VariableComponent>
    auto
        TUtils_Variables<T_VariableComponent>::
        Declare(
            HandleType   InHandle,
            FGameplayTag InVariableName,
            ArgType      InDefaultValue)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        auto& VariablesComp = InHandle.Get<FragmentType>();

        // perhaps this should be 'Add' and ensure if we find one that already exists
        VariablesComp.Get_Variables().FindOrAdd(InVariableName, InDefaultValue);
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

        if (NOT Ensure(InHandle))
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
        if (NOT Ensure(InHandle))
        { return; }

        auto& VariablesComp = InHandle.Get<FragmentType>();
        auto* FoundVariableWithName = VariablesComp.Get_Variables().Find(InVariableName);

        CK_ENSURE_IF_NOT(ck::IsValid(FoundVariableWithName, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Could not find the variable with name [{}] in Handle [{}]"), InVariableName, InHandle)
        { return; }

        *FoundVariableWithName = InValue;
    }
}

// --------------------------------------------------------------------------------------------------------------------
