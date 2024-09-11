#include "CkEcsMetaProcessorInjector.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_JustBeforeDestructionPhase_Name, TEXT("MetaProcessorInjector.JustBeforeDestructionPhase"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_EntityDestructionInPhases_Name, TEXT("MetaProcessorInjector.EntityDestructionInPhases"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_DeltaTime_Name, TEXT("MetaProcessorInjector.DeltaTime"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_GameplayWithPump_Name, TEXT("MetaProcessorInjector.GameplayWithPump"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_GameplayWithoutPump_Name, TEXT("MetaProcessorInjector.GameplayWithoutPump"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_Script_Name, TEXT("MetaProcessorInjector.Script"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_ChaosDestruction_Name, TEXT("MetaProcessorInjector.ChaosDestruction"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_Physics_Name, TEXT("MetaProcessorInjector.Physics"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_Misc_Name, TEXT("MetaProcessorInjector.Misc"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_Replication_Name, TEXT("MetaProcessorInjector.Replication"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_EntityCreationAndDestruction_Name, TEXT("MetaProcessorInjector.EntityCreationAndDestruction"));
UE_DEFINE_GAMEPLAY_TAG(TAG_MetaProcessorInjector_Overlap_Name, TEXT("MetaProcessorInjector.Overlap"));

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
        const auto& Name = MetaInjector.Get_Name();

        if (ck::Is_NOT_Valid(Name))
        {
            PushErrorToContext(ck::Format_UE(TEXT("MetaProcessorInjector [{}] has a ProcessorInjector entry with an INVALID Name"), this));
        }

        for (const auto& MetaProcessorInjector : MetaInjector.Get_MetaProcessorInjectors())
        {
            if (ck::IsValid(MetaProcessorInjector))
            { continue; }

            PushErrorToContext(ck::Format_UE(TEXT("MetaProcessorInjector [{}] has an INVALID MetaProcessorInjector specified under the [{}] entry"), this, Name));
        }
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
