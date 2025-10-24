#pragma once

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCue/CkCueSubsystem_Base.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include <GameplayTagContainer.h>

#include "CkCue_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, MinimalAPI)
class UCk_K2Node_Cue_Base : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    // UObject interface
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // K2Node implementation
    auto IsNodePure() const -> bool override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto GetMenuCategory() const -> FText override;
    auto IsCompatibleWithGraph(UEdGraph const* InGraph) const -> bool override;
    auto PinConnectionListChanged(UEdGraphPin* InPin) -> void override;
    auto GetPinMetaData(FName InPinName, FName InKey) -> FString override;
    auto GetJumpTargetForDoubleClick() const -> UObject* override;
    // End of K2Node implementation

    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    // End of UEdGraphNode implementation

protected:
    auto DoAllocate_DefaultPins() -> void override;
    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;
    auto DoGet_Menu_NodeTitle() const -> FText override;
    auto DoPinDefaultValueChanged(UEdGraphPin* InPin) -> void override;

protected:
    // Must be implemented by derived classes to specify which executor subsystem to use
    virtual auto Get_CueExecutorSubsystemClass() const -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE>
        PURE_VIRTUAL(UCk_K2Node_Cue_Base::Get_CueExecutorSubsystemClass, return {};);

    // Must be implemented by derived classes to specify the gameplay tag category
    virtual auto Get_CueTagCategory() const -> FString
        PURE_VIRTUAL(UCk_K2Node_Cue_Base::Get_CueTagCategory, return TEXT("Cue"););

private:
    auto DoCreatePinsFromCue(UClass* InCueClass) -> void;
    auto DoOnCueNamePinChanged() -> void;
    auto DoGet_CueClass(TOptional<TArray<UEdGraphPin*>> InPinsToSearch = {}) const -> UClass*;
    auto DoGet_CueName(TOptional<TArray<UEdGraphPin*>> InPinsToSearch = {}) const -> FGameplayTag;
    auto DoGet_ExecutionType() const -> ECk_Cue_ExecutionPolicy;
    auto DoGet_CueSubsystem() const -> UCk_CueSubsystem_Base_UE*;

    static auto DoGet_CueSpawnParamsStruct(
        UClass* InCueClass,
        FKismetCompilerContext& InCompilerContext) -> UScriptStruct*;

private:
    UPROPERTY()
    FGameplayTag _CachedCueName;

    UPROPERTY()
    ECk_Cue_ExecutionPolicy _ExecutionType = ECk_Cue_ExecutionPolicy::Replicated;

    TArray<UEdGraphPin*> _PinsGeneratedFromCue;
    TMap<FName, TMap<FName, FString>> _PinMetadataMap;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_GenericCue : public UCk_K2Node_Cue_Base
{
    GENERATED_BODY()

protected:
    auto Get_CueExecutorSubsystemClass() const -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> override
    {
        return UCk_GenericCueExecutor_Subsystem_UE::StaticClass();
    }

    auto Get_CueTagCategory() const -> FString override
    {
        return TEXT("Cue");
    }

    auto DoGet_Menu_NodeTitle() const -> FText override
    {
        return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_GenericCue"), TEXT("[Ck] Execute Generic Cue"));
    }
};

// --------------------------------------------------------------------------------------------------------------------