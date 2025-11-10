#pragma once

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCue/CkCueSubsystem_Base.h"
#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include <GameplayTagContainer.h>
#include <KismetNodes/SGraphNodeK2Base.h>

#include "CkCue_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Cue_EntityMode : uint8
{
    Owner,
    Transient
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_EntityMode);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract)
class CKCUEEDITOR_API UCk_K2Node_Cue_Base : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    friend class SCk_GraphNode_Cue_Base;

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
    auto CreateVisualWidget() -> TSharedPtr<SGraphNode> override;
    // End of K2Node implementation

    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    // End of UEdGraphNode implementation

public:
    auto DoGet_CueClass(TOptional<TArray<UEdGraphPin*>> InPinsToSearch = {}) const -> UClass*;

protected:
    auto DoAllocate_DefaultPins() -> void override;
    auto DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void override;
    auto DoGet_Menu_NodeTitle() const -> FText override;
    auto DoPinDefaultValueChanged(UEdGraphPin* InPin) -> void override;

    // Virtual method for display node title (can include UTF-8 symbols)
    virtual auto DoGet_DisplayNodeTitle() const -> FText
    {
        return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_Cue_Base"), TEXT("[Ck] Execute Cue"));
    }

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
    auto DoGet_CueName(TOptional<TArray<UEdGraphPin*>> InPinsToSearch = {}) const -> FGameplayTag;
    auto DoGet_ExecutionType() const -> ECk_Cue_ExecutionPolicy;
    auto DoGet_EntityMode() const -> ECk_Cue_EntityMode;
    auto DoGet_ReliabilityPolicy() const -> ECk_Cue_ReliabilityPolicy;
    auto DoGet_MulticastPolicy() const -> ECk_Cue_MulticastPolicy;
    auto DoGet_CueSubsystem() const -> UCk_CueSubsystem_Base_UE*;
    auto DoUpdateCachedCueClass() -> void;

    static auto DoGet_CueSpawnParamsStruct(
        UClass* InCueClass,
        FKismetCompilerContext& InCompilerContext) -> UScriptStruct*;

private:
    UPROPERTY()
    FGameplayTag _CachedCueName;

    UPROPERTY()
    TSubclassOf<UCk_CueBase_EntityScript> _CachedCueClass;

    UPROPERTY()
    ECk_Cue_ExecutionPolicy _ExecutionType = ECk_Cue_ExecutionPolicy::Replicated;

    UPROPERTY()
    ECk_Cue_EntityMode _EntityMode = ECk_Cue_EntityMode::Owner;

    UPROPERTY()
    ECk_Cue_ReliabilityPolicy _ReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable;

    UPROPERTY()
    ECk_Cue_MulticastPolicy _MulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients;

    TArray<UEdGraphPin*> _PinsGeneratedFromCue;
    TMap<FName, TMap<FName, FString>> _PinMetadataMap;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_GenericCue : public UCk_K2Node_Cue_Base
{
    GENERATED_BODY()

protected:
    auto Get_CueExecutorSubsystemClass() const -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> override;
    auto Get_CueTagCategory() const -> FString override;
    auto DoGet_Menu_NodeTitle() const -> FText override;
    auto DoGet_DisplayNodeTitle() const -> FText override;
};

// --------------------------------------------------------------------------------------------------------------------

class SCk_GraphNode_Cue_Base : public SGraphNodeK2Base
{
public:
    SLATE_BEGIN_ARGS(SCk_GraphNode_Cue_Base) {}
    SLATE_END_ARGS()

public:
    auto Construct(const FArguments& InArgs, UCk_K2Node_Cue_Base* InNode) -> void;
    auto CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) -> void override;

protected:
    TWeakObjectPtr<UCk_K2Node_Cue_Base> _CueNode;
};

// --------------------------------------------------------------------------------------------------------------------