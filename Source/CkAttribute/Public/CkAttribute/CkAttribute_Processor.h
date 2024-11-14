#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::detail
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttribute>
    class TProcessor_Attribute_StorePreviousValue : public ck_exp::TProcessor<
            TProcessor_Attribute_StorePreviousValue<T_DerivedProcessor, T_DerivedAttribute>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            typename T_DerivedAttribute::FTag_RecomputeFinalValue,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_RecomputeFinalValue;

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using HandleType            = typename AttributeFragmentType::HandleType;
        using ThisType              = TProcessor_Attribute_StorePreviousValue<T_DerivedProcessor, T_DerivedAttribute>;
        using Super                 = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType              = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals : public ck_exp::TProcessor<
            TProcessor_Attribute_FireSignals<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            TFragment_Attribute_PreviousValues<T_DerivedAttribute>,
            typename T_DerivedAttribute::FTag_FireSignals,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_FireSignals;

    public:
        using AttributeFragmentType         = T_DerivedAttribute;
        using AttributeFragmentPreviousType = TFragment_Attribute_PreviousValues<T_DerivedAttribute>;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType::HandleType;
        using ThisType                      = TProcessor_Attribute_FireSignals<T_DerivedProcessor, AttributeFragmentType, T_MulticastType>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, AttributeFragmentPreviousType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute,
            AttributeFragmentPreviousType& InAttributePrevious) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeCurrent, typename T_DerivedAttributeMin>
    class TProcessor_Attribute_MinClamp : public ck_exp::TProcessor<
            TProcessor_Attribute_MinClamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeMin>,
            typename T_DerivedAttributeCurrent::HandleType,
            T_DerivedAttributeCurrent,
            T_DerivedAttributeMin,
            FTag_MayRequireClamping,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = T_DerivedAttributeMin;

    public:
        using AttributeFragmentType_Current = T_DerivedAttributeCurrent;
        using AttributeFragmentType_Min     = T_DerivedAttributeMin;
        using AttributeDataType             = typename AttributeFragmentType_Current::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType_Current::HandleType;
        using ThisType                      = TProcessor_Attribute_MinClamp<T_DerivedProcessor, AttributeFragmentType_Current, AttributeFragmentType_Min>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType_Current, MarkedDirtyBy, FTag_MayRequireClamping, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Min& InAttributeMin) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeCurrent, typename T_DerivedAttributeMax>
    class TProcessor_Attribute_MaxClamp : public ck_exp::TProcessor<
            TProcessor_Attribute_MaxClamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeMax>,
            typename T_DerivedAttributeCurrent::HandleType,
            T_DerivedAttributeCurrent,
            T_DerivedAttributeMax,
            FTag_MayRequireClamping,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = T_DerivedAttributeMax;

    public:
        using AttributeFragmentType_Current = T_DerivedAttributeCurrent;
        using AttributeFragmentType_Max     = T_DerivedAttributeMax;
        using AttributeDataType             = typename AttributeFragmentType_Current::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType_Current::HandleType;
        using ThisType                      = TProcessor_Attribute_MaxClamp<T_DerivedProcessor, AttributeFragmentType_Current, AttributeFragmentType_Max>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType_Current, MarkedDirtyBy, FTag_MayRequireClamping, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Max& InAttributeMax) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_DerivedAttribute_ReplicatedFragment>
    class TProcessor_Attribute_Replicate : public ck_exp::TProcessor<
            TProcessor_Attribute_Replicate<T_DerivedProcessor, T_DerivedAttribute, T_DerivedAttribute_ReplicatedFragment>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            typename T_DerivedAttribute::FTag_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_MayRequireReplication;

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using HandleType            = typename AttributeFragmentType::HandleType;
        using ThisType              = TProcessor_Attribute_Replicate<T_DerivedProcessor, T_DerivedAttribute, T_DerivedAttribute_ReplicatedFragment>;
        using Super                 = ck_exp::TProcessor<ThisType, HandleType, T_DerivedAttribute, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType              = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const T_DerivedAttribute& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll : public ck_exp::TProcessor<
            TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::AttributeFragmentType::HandleType,
            typename T_DerivedAttributeModifier::AttributeFragmentType,
            typename T_DerivedAttributeModifier::AttributeFragmentType::FTag_RecomputeFinalValue,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::AttributeFragmentType::FTag_RecomputeFinalValue;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType::HandleType;
        using ThisType                      = TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, AttributeModifierFragmentType>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevocableAdd_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_RevocableAdd_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyAdd,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ModificationType              = typename AttributeModifierFragmentType::FTag_ModifyAdd;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using ThisType                      = TProcessor_AttributeModifier_RevocableAdd_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevocableSubtract_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_RevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifySubtract,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ModificationType              = typename AttributeModifierFragmentType::FTag_ModifySubtract;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using ThisType                      = TProcessor_AttributeModifier_RevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevocableAdd_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_NotRevocableAdd_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyAdd,
            typename T_DerivedAttributeModifier::FTag_IsNotRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using ModificationType               = typename AttributeModifierFragmentType::FTag_ModifyAdd;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using HandleType                     = typename AttributeModifierFragmentType::HandleType;
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableAdd_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                       = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevocableSubtract_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_NotRevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifySubtract,
            typename T_DerivedAttributeModifier::FTag_IsNotRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using ModificationType               = typename AttributeModifierFragmentType::FTag_ModifySubtract;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using HandleType                     = typename AttributeModifierFragmentType::HandleType;
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                       = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Override_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_Override_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyOverride,
            typename T_DerivedAttributeModifier::FTag_IsNotRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using ModificationType               = typename AttributeModifierFragmentType::FTag_ModifyOverride;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using HandleType                     = typename AttributeModifierFragmentType::HandleType;
        using ThisType                       = TProcessor_AttributeModifier_Override_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                       = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevocableMultiply_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_RevocableMultiply_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyMultiply,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ModificationType              = typename AttributeModifierFragmentType::FTag_ModifyMultiply;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using ThisType                      = TProcessor_AttributeModifier_RevocableMultiply_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevocableDivide_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_RevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyDivide,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ModificationType              = typename AttributeModifierFragmentType::FTag_ModifyDivide;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using ThisType                      = TProcessor_AttributeModifier_RevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevocableMultiply_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_NotRevocableMultiply_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyMultiply,
            typename T_DerivedAttributeModifier::FTag_IsNotRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using ModificationType               = typename AttributeModifierFragmentType::FTag_ModifyMultiply;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using HandleType                     = typename AttributeModifierFragmentType::HandleType;
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableMultiply_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                       = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevocableDivide_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_NotRevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_ModifyDivide,
            typename T_DerivedAttributeModifier::FTag_IsNotRevocableModification,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using ModificationType               = typename AttributeModifierFragmentType::FTag_ModifyDivide;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using HandleType                     = typename AttributeModifierFragmentType::HandleType;
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                       = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_ComputeAll : public TProcessorBase<TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>>
    {
    public:
        using Super        = TProcessorBase<TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>>;
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        explicit
        TProcessor_AttributeModifier_ComputeAll(RegistryType InRegistry);

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    private:
        TProcessor_AttributeModifier_Override_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _Override_Compute;

        TProcessor_AttributeModifier_NotRevocableAdd_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableAdd_Compute;
        TProcessor_AttributeModifier_NotRevocableSubtract_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableSubtract_Compute;
        TProcessor_AttributeModifier_NotRevocableMultiply_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableMultiply_Compute;
        TProcessor_AttributeModifier_NotRevocableDivide_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableDivide_Compute;

        TProcessor_AttributeModifier_RevocableAdd_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableAdd_Compute;
        TProcessor_AttributeModifier_RevocableSubtract_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableSubtract_Compute;
        TProcessor_AttributeModifier_RevocableMultiply_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableMultiply_Compute;
        TProcessor_AttributeModifier_RevocableDivide_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableDivide_Compute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Teardown : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using AttributeHandleType           = typename AttributeModifierFragmentType::AttributeHandleType;
        using ThisType                      = TProcessor_AttributeModifier_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, IsRevocableModificationType, CK_IF_INITIATE_CONFIRM_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeRefill>
    class TProcessor_AttributeRefill_Update : public ck_exp::TProcessor<
            TProcessor_AttributeRefill_Update<T_DerivedProcessor, T_DerivedAttributeRefill>,
            typename T_DerivedAttributeRefill::AttributeHandleType,
            T_DerivedAttributeRefill,
            typename T_DerivedAttributeRefill::AttributeFragmentType,
            typename T_DerivedAttributeRefill::FTag_RefillRunning,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeRefill::FTag_RefillRunning;

    public:
        using AttributeRefillFragmentType = T_DerivedAttributeRefill;
        using AttributeModifierFragmentType = typename T_DerivedAttributeRefill::AttributeModifierFragmentType;
        using AttributeFragmentType       = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType           = typename AttributeFragmentType::AttributeDataType;
        using AttributeHandleType         = typename AttributeFragmentType::HandleType;
        using AttributeModifierHandleType = typename AttributeModifierFragmentType::HandleType;
        using ThisType                    = TProcessor_AttributeRefill_Update<T_DerivedProcessor, T_DerivedAttributeRefill>;
        using Super                       = ck_exp::TProcessor<ThisType,AttributeHandleType, AttributeRefillFragmentType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                    = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            AttributeHandleType InHandle,
            const AttributeRefillFragmentType& InAttributeRefill,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_FireSignals<
            TProcessor_Attribute_FireSignals_CurrentMinMax, T_DerivedAttribute<T_Component>, T_MulticastType>;

    public:
        explicit
        TProcessor_Attribute_FireSignals_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute, typename T_Attribute_ReplicatedFragment>
    class TProcessor_Attribute_Replicate_All
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_Replicate<
            TProcessor_Attribute_Replicate_All, T_DerivedAttribute<T_Component>, T_Attribute_ReplicatedFragment>;

    public:
        explicit
        TProcessor_Attribute_Replicate_All(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute>
    class TProcessor_Attribute_MinMaxClamp
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        explicit
        TProcessor_Attribute_MinMaxClamp(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        detail::TProcessor_Attribute_MinClamp<TProcessor_Attribute_MinMaxClamp,
            T_DerivedAttribute<ECk_MinMaxCurrent::Current>, T_DerivedAttribute<ECk_MinMaxCurrent::Min>> _MinClamp;

        detail::TProcessor_Attribute_MaxClamp<TProcessor_Attribute_MinMaxClamp,
            T_DerivedAttribute<ECk_MinMaxCurrent::Current>, T_DerivedAttribute<ECk_MinMaxCurrent::Max>> _MaxClamp;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_RecomputeAll<
            TProcessor_Attribute_RecomputeAll_CurrentMinMax, T_DerivedAttributeModifier<T_Component>>;

        template <ECk_MinMaxCurrent T_Component>
        using TStorePreviousValueProcessorType = detail::TProcessor_Attribute_StorePreviousValue<
            TProcessor_Attribute_RecomputeAll_CurrentMinMax, typename T_DerivedAttributeModifier<T_Component>::AttributeFragmentType>;

    public:
        explicit
        TProcessor_Attribute_RecomputeAll_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TStorePreviousValueProcessorType<ECk_MinMaxCurrent::Current> _Current_Previous;
        TStorePreviousValueProcessorType<ECk_MinMaxCurrent::Min> _Min_Previous;
        TStorePreviousValueProcessorType<ECk_MinMaxCurrent::Max> _Max_Previous;

        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_ComputeAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier<T_Component>>;

    public:
        explicit
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_TeardownAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_AttributeModifier_Teardown<
            TProcessor_AttributeModifier_TeardownAll_CurrentMinMax, T_DerivedAttributeModifier<T_Component>>;

    public:
        explicit
        TProcessor_AttributeModifier_TeardownAll_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeRefill>
    class TProcessor_AttributeRefill_Update
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        explicit
        TProcessor_AttributeRefill_Update(
            RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        detail::TProcessor_AttributeRefill_Update<
            TProcessor_AttributeRefill_Update, T_DerivedAttributeRefill> _Refill_Update;

    private:
        RegistryType _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Processor.inl.h"