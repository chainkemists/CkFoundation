#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"
#include "CkSignal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedAttribute>
    class TUtils_Attribute
    {
    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using HandleType            = FCk_Handle;

    public:
        template <typename>
        friend class TUtils_AttributeModifier;

        template <typename, typename>
        friend class TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Additive_Teardown;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Multiplicative_Teardown;

    public:
        static auto
        Add(
            HandleType InHandle,
            AttributeDataType InBaseValue) -> void;

        static auto
        Has(
            HandleType InHandle) -> bool;

        static auto
        Ensure(
            HandleType InHandle) -> bool;

        static auto
        Get_BaseValue(
            HandleType InHandle) -> AttributeDataType;

        static auto
        Get_FinalValue(
            HandleType InHandle) -> AttributeDataType;

    private:
        static auto
        Request_RecomputeFinalValue(
            HandleType InHandle) -> void;

        static auto
        Request_FireSignals(
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    class TUtils_AttributeModifier
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = FCk_Handle;

    public:
        template <typename, typename>
        friend class TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Additive_Teardown;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Multiplicative_Teardown;

    public:
        struct AttributeModifierTarget_Utils : TUtils_EntityHolder<FFragment_AttributeModifierTarget> {};
        struct RecordOfAttributeModifiers_Utils : TUtils_RecordOfEntities<FFragment_RecordOfAttributeModifiers>{};

    public:
        static auto
        Add(
            HandleType InHandle,
            AttributeDataType InModifierDelta,
            HandleType InTarget,
            ECk_ModifierOperation InModifierOperation) -> void;

        static auto
        Has(
            HandleType InHandle) -> bool;

        static auto
        Ensure(
            HandleType InHandle) -> bool;

        static auto
        Get_ModifierDeltaValue(
            HandleType InHandle) -> const AttributeDataType&;

    private:
        static auto
        Request_ComputeResult(
            HandleType InHandle) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Utils.inl.h"