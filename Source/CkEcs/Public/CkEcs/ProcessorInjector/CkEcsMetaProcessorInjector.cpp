#include "CkEcsMetaProcessorInjector.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ICk_MetaProcessorInjector_Interface::
    Get_ProcessorInjectors() const
    -> TArray<TSubclassOf<UCk_EcsWorld_ProcessorInjector_Base_UE>>
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_MetaProcessorInjectorGroup_UE::
    Get_ProcessorInjectors() const
    -> TArray<TSubclassOf<UCk_EcsWorld_ProcessorInjector_Base_UE>>
{
    using ProcessorInjectorListType = TArray<TSubclassOf<UCk_EcsWorld_ProcessorInjector_Base_UE>>;

    const auto& TransformFunc = [](TObjectPtr<UObject> InSubProcessorInjector) -> ProcessorInjectorListType
    {
        const TScriptInterface<ICk_MetaProcessorInjector_Interface> MetaProcessorInjectorInterface = InSubProcessorInjector;

        if (ck::Is_NOT_Valid(MetaProcessorInjectorInterface))
        { return {}; }

        // TODO: Add infinite loop detection
        return MetaProcessorInjectorInterface->Get_ProcessorInjectors();
    };

    const auto& AccumulateFunc = [](const ProcessorInjectorListType& InA, const ProcessorInjectorListType& InB)
    {
        auto Result = ProcessorInjectorListType{};
        Result.Append(InA);
        Result.Append(InB);

        return Result;
    };

    return Algo::TransformAccumulate(_SubMetaProcessorInjectors, TransformFunc, ProcessorInjectorListType{}, AccumulateFunc);
}

#if WITH_EDITOR
auto
    UCk_Ecs_MetaProcessorInjectorGroup_UE::
    IsDataValid(
        FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (IsTemplate())
    { return Result; }

    if (const auto& NumberOfInvalidEntries = ck::algo::CountIf(_SubMetaProcessorInjectors, ck::algo::Is_NOT_Valid{});
        NumberOfInvalidEntries > 0)
    {
        InContext.AddError(FText::FromString(ck::Format_UE(TEXT("MetaProcessorInjectorGroup [{}] has [{}] INVALID SubMetaProcessorInjectors"), this, NumberOfInvalidEntries)));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_MetaProcessorInjector_UE::
    Get_ProcessorInjectors() const
    -> TArray<TSubclassOf<UCk_EcsWorld_ProcessorInjector_Base_UE>>
{
    return _ProcessorInjectors;
}

#if WITH_EDITOR
auto
    UCk_Ecs_MetaProcessorInjector_UE::
    IsDataValid(
        FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (IsTemplate())
    { return Result; }

    if (const auto& NumberOfInvalidEntries = ck::algo::CountIf(_ProcessorInjectors, ck::algo::Is_NOT_Valid{});
        NumberOfInvalidEntries > 0)
    {
        InContext.AddError(FText::FromString(ck::Format_UE(TEXT("MetaProcessorInjector [{}] has [{}] INVALID ProcessorInjectors"), this, NumberOfInvalidEntries)));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Ecs_MetaProcessorInjectors_Info::
    Get_Description() const
    -> FName
{
#if WITH_EDITORONLY_DATA
    return _Description;
#else
    return TEXT("Non-editor build - No Description");
#endif
}

auto
    FCk_Ecs_MetaProcessorInjectors_Info::
    Get_MetaProcessorInjectors() const -> TArray<TScriptInterface<ICk_MetaProcessorInjector_Interface>>
{
    return ck::algo::Transform<TArray<TScriptInterface<ICk_MetaProcessorInjector_Interface>>>(_MetaProcessorInjectors, [](TObjectPtr<UObject> InSubProcessorInjector)
    {
        return InSubProcessorInjector;
    });
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_Ecs_ProcessorInjectors_PDA::
    IsDataValid(
        FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (IsTemplate())
    { return Result; }

    const auto& PushErrorToContext = [&](const FString& InErrorMessage)
    {
        InContext.AddError(FText::FromString(InErrorMessage));
        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    };

    for (const auto& MetaInjector : _MetaProcessorInjectors)
    {
        const auto& EcsWorldTickingGroup = MetaInjector.Get_EcsWorldTickingGroup();

        if (ck::Is_NOT_Valid(EcsWorldTickingGroup))
        {
            PushErrorToContext(ck::Format_UE(TEXT("MetaProcessorInjector [{}] has a ProcessorInjector entry with an INVALID Ecs World Ticking Group"), this));
        }

        for (const auto& MetaProcessorInjector : MetaInjector.Get_MetaProcessorInjectors())
        {
            if (ck::IsValid(MetaProcessorInjector))
            { continue; }

            PushErrorToContext(ck::Format_UE(TEXT("MetaProcessorInjector [{}] has an INVALID MetaProcessorInjector specified within the Ecs World Ticking Group [{}]"), this, EcsWorldTickingGroup));
        }
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
