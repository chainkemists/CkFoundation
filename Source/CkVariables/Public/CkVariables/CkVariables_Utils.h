#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkVariables_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_VariableFragment>
    class TUtils_Variables
    {
    public:
        using HandleType   = FCk_Handle;
        using FragmentType = T_VariableFragment;
        using ValueType    = typename T_VariableFragment::ValueType;
        using ArgType      = typename T_VariableFragment::ArgType;

    public:
        static auto
        Add(
            HandleType InHandle) -> void;

        static auto
        Has(
            HandleType InHandle) -> bool;

    public:
        static auto
        Get(
            HandleType InHandle,
            FGameplayTag InVariableName) -> ArgType;

        static auto
        Set(
            HandleType InHandle,
            FGameplayTag InVariableName,
            ArgType InValue) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkVariables_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------

