#pragma once
#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::detail
{
    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals : public ck_exp::TProcessor<
            TProcessor_Attribute_FireSignals<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            typename T_DerivedAttribute::FTag_FireSignals,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_FireSignals;

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using HandleType            = typename AttributeFragmentType::HandleType;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using ThisType              = TProcessor_Attribute_FireSignals<T_DerivedProcessor, AttributeFragmentType, T_MulticastType>;
        using Super                 = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType              = typename Super::TimeType;

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

    template <typename T_DerivedProcessor, typename T_DerivedAttributeCurrent, typename T_DerivedAttributeMin>
    class TProcessor_Attribute_MinClamp : public TProcessor<
            TProcessor_Attribute_MinClamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeMin>,
            T_DerivedAttributeCurrent,
            T_DerivedAttributeMin,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = T_DerivedAttributeMin;

    public:
        using AttributeFragmentType_Current = T_DerivedAttributeCurrent;
        using AttributeFragmentType_Min     = T_DerivedAttributeMin;
        using AttributeDataType             = typename AttributeFragmentType_Current::AttributeDataType;
        using ThisType                      = TProcessor_Attribute_MinClamp<T_DerivedProcessor, AttributeFragmentType_Current, AttributeFragmentType_Min>;
        using Super                         = TProcessor<ThisType, AttributeFragmentType_Current, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
    class TProcessor_Attribute_MaxClamp : public TProcessor<
            TProcessor_Attribute_MaxClamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeMax>,
            T_DerivedAttributeCurrent,
            T_DerivedAttributeMax,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = T_DerivedAttributeMax;

    public:
        using AttributeFragmentType_Current = T_DerivedAttributeCurrent;
        using AttributeFragmentType_Max     = T_DerivedAttributeMax;
        using AttributeDataType             = typename AttributeFragmentType_Current::AttributeDataType;
        using ThisType                      = TProcessor_Attribute_MaxClamp<T_DerivedProcessor, AttributeFragmentType_Current, AttributeFragmentType_Max>;
        using Super                         = TProcessor<ThisType, AttributeFragmentType_Current, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Max& InAttributeMin) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll : public TProcessor<
            TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, T_DerivedAttributeModifier>,
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
        using ThisType                      = TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, AttributeModifierFragmentType>;
        using Super                         = TProcessor<ThisType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
    class TProcessor_AttributeModifier_RevocableAdditive_Compute : public TProcessor<
            TProcessor_AttributeModifier_RevocableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_AdditiveModification,
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
        using ModificationType              = typename AttributeModifierFragmentType::FTag_AdditiveModification;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using ThisType                      = TProcessor_AttributeModifier_RevocableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
    class TProcessor_AttributeModifier_RevocableSubtract_Compute : public TProcessor<
            TProcessor_AttributeModifier_RevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
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
        using ThisType                      = TProcessor_AttributeModifier_RevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
    class TProcessor_AttributeModifier_NotRevocableAdditive_Compute : public TProcessor<
            TProcessor_AttributeModifier_NotRevocableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_AdditiveModification,
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
        using ModificationType               = typename AttributeModifierFragmentType::FTag_AdditiveModification;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                     = typename Super::HandleType;
        using TimeType                       = typename Super::TimeType;

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
    class TProcessor_AttributeModifier_NotRevocableSubtract_Compute : public TProcessor<
            TProcessor_AttributeModifier_NotRevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
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
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                     = typename Super::HandleType;
        using TimeType                       = typename Super::TimeType;

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
    class TProcessor_AttributeModifier_RevocableMultiplicative_Compute : public TProcessor<
            TProcessor_AttributeModifier_RevocableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_MultiplicativeModification,
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
        using ModificationType              = typename AttributeModifierFragmentType::FTag_MultiplicativeModification;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using ThisType                      = TProcessor_AttributeModifier_RevocableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
    class TProcessor_AttributeModifier_RevocableDivide_Compute : public TProcessor<
            TProcessor_AttributeModifier_RevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
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
        using ThisType                      = TProcessor_AttributeModifier_RevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
    class TProcessor_AttributeModifier_NotRevocableMultiplicative_Compute : public TProcessor<
            TProcessor_AttributeModifier_NotRevocableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_MultiplicativeModification,
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
        using ModificationType               = typename AttributeModifierFragmentType::FTag_MultiplicativeModification;
        using IsNotRevocableModificationType = typename AttributeModifierFragmentType::FTag_IsNotRevocableModification;
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                     = typename Super::HandleType;
        using TimeType                       = typename Super::TimeType;

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
    class TProcessor_AttributeModifier_NotRevocableDivide_Compute : public TProcessor<
            TProcessor_AttributeModifier_NotRevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
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
        using ThisType                       = TProcessor_AttributeModifier_NotRevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = TProcessor<ThisType, AttributeModifierFragmentType, ModificationType, IsNotRevocableModificationType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using HandleType                     = typename Super::HandleType;
        using TimeType                       = typename Super::TimeType;

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
    class TProcessor_AttributeModifier_ComputeAll
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        explicit
        TProcessor_AttributeModifier_ComputeAll(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TProcessor_AttributeModifier_NotRevocableAdditive_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableAdditive_Compute;
        TProcessor_AttributeModifier_NotRevocableSubtract_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableSubtract_Compute;
        TProcessor_AttributeModifier_NotRevocableMultiplicative_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableMultiplicative_Compute;
        TProcessor_AttributeModifier_NotRevocableDivide_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableDivide_Compute;

        TProcessor_AttributeModifier_RevocableAdditive_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableAdditive_Compute;
        TProcessor_AttributeModifier_RevocableSubtract_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableSubtract_Compute;
        TProcessor_AttributeModifier_RevocableMultiplicative_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableMultiplicative_Compute;
        TProcessor_AttributeModifier_RevocableDivide_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableDivide_Compute;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Teardown : public TProcessor<
            TProcessor_AttributeModifier_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            CK_IF_PENDING_KILL>
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using ThisType                      = TProcessor_AttributeModifier_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, IsRevocableModificationType, CK_IF_PENDING_KILL>;
        using HandleType                    = typename Super::HandleType;
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
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

    // --------------------------------------------------------------------------------------------------------------------

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

    public:
        explicit
        TProcessor_Attribute_RecomputeAll_CurrentMinMax(RegistryType InRegistry);

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
    class TProcessor_AttributeModifier_ComputeAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_AttributeModifier_ComputeAll<
            TProcessor_AttributeModifier_ComputeAll_CurrentMinMax, T_DerivedAttributeModifier<T_Component>>;

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
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Processor.inl.h"