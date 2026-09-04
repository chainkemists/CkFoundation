#pragma once

#include "CkVariables_Utils.h"

#include "CkCore/Validation/CkUntracedStructSafety.h"

#include <StructUtils/InstancedStruct.h>

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
        Has(
            HandleType InHandle,
            FGameplayTag InVariableName)
        -> bool
    {
        return Has(InHandle, InVariableName.GetTagName());
    }

    template <typename T_VariableComponent>
    auto
        TUtils_Variables<T_VariableComponent>::
        Get(
            HandleType   InHandle,
            FGameplayTag InVariableName)
        -> ArgType
    {
        return Get(InHandle, InVariableName.GetTagName());
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
        Set(InHandle, InVariableName.GetTagName(), InValue);
    }

    template <typename T_VariableFragment>
    auto
        TUtils_Variables<T_VariableFragment>::
        Has(
            HandleType InHandle,
            FName InVariableName)
        -> bool
    {
        if (NOT Has(InHandle))
        { return false; }

        const auto& VariablesComp = InHandle.Get<FragmentType>();
        auto* FoundVariableWithName = VariablesComp.Get_Variables().Find(InVariableName);

        if (ck::Is_NOT_Valid(FoundVariableWithName, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        return true;
    }

    template <typename T_VariableComponent>
    auto
        TUtils_Variables<T_VariableComponent>::
        Get(
            HandleType InHandle,
            FName InVariableName)
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
            HandleType InHandle,
            FName InVariableName,
            ArgType InValue)
        -> void
    {
        if constexpr (std::is_same_v<ValueType, FInstancedStruct>)
        {
            if (InValue.IsValid())
            {
                const auto* ScriptStruct = InValue.GetScriptStruct();
                const auto HasScriptStruct = ScriptStruct != nullptr;
                CK_ENSURE_IF_NOT(HasScriptStruct,
                    TEXT("Variable [{}] on Handle [{}] rejected an InstancedStruct without a reflected struct type"),
                    InVariableName,
                    InHandle)
                { return; }

                const auto Safety = ck::Analyze_UntracedStructSafety(ScriptStruct);
                const auto IsValueSafe = Safety.IsGcIndependent();
                CK_ENSURE_IF_NOT(IsValueSafe,
                    TEXT("Variable [{}] on Handle [{}] rejected unsafe InstancedStruct [{}]; [{}]: {}"),
                    InVariableName,
                    InHandle,
                    ScriptStruct->GetName(),
                    Safety.FailurePath,
                    Safety.FailureReason)
                { return; }
            }
        }

        auto& VariablesComp = InHandle.AddOrGet<FragmentType>();
        auto& FoundVariableWithName = VariablesComp.Get_Variables().FindOrAdd(InVariableName);
        FoundVariableWithName = InValue;
    }
}

// --------------------------------------------------------------------------------------------------------------------
