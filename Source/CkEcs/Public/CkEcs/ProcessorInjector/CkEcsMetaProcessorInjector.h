#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include <Misc/DataValidation.h>
#include <GameplayTagContainer.h>

#include "CkEcsMetaProcessorInjector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_WorldStatCollection_Policy : uint8
{
    DoNotCollect,
    CollectOnLocalClientOnly,
    CollectOnServerOnly,
    CollectOnLocalClientAndServer
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ecs_WorldStatCollection_Policy);

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class CKECS_API UCk_MetaProcessorInjector_Interface : public UInterface { GENERATED_BODY() };
class CKECS_API ICk_MetaProcessorInjector_Interface
{
    GENERATED_BODY()

public:
    virtual auto
    Get_ProcessorInjectors() const -> TArray<TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>>;

    virtual auto
    Request_Refresh() -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_Ecs_MetaProcessorInjectorGroup_UE : public UObject, public ICk_MetaProcessorInjector_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_MetaProcessorInjectorGroup_UE);

public:
    auto
    Get_ProcessorInjectors() const -> TArray<TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>> override;

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditDefaultsOnly, Instanced,
        meta=(AllowPrivateAccess, MustImplement = "/Script/CkEcs.Ck_MetaProcessorInjector_Interface"))
    TArray<TObjectPtr<UObject>> _SubMetaProcessorInjectors;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(NotBlueprintType)
struct CKECS_API FCk_Ecs_InheritedProcessorInjectors
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ecs_InheritedProcessorInjectors);

public:
    auto PostInitProperties() -> void;
    auto UpdateInherited(
        const ThisType* Parent) -> void;

private:
    UPROPERTY(VisibleDefaultsOnly, meta=(AllowPrivateAccess), DisplayName = "Processor Injectors (Combined)")
    TArray<TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>>  _ProcessorInjectors_Combined;

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess), DisplayName = "Processor Injectors (New)")
    TArray<TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>>  _ProcessorInjectors_Overriden;

public:
    CK_PROPERTY_GET(_ProcessorInjectors_Combined);
    CK_PROPERTY_GET(_ProcessorInjectors_Overriden);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_Ecs_MetaProcessorInjector_UE : public UObject, public ICk_MetaProcessorInjector_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_MetaProcessorInjector_UE);

public:
    auto
    Get_ProcessorInjectors() const -> TArray<TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>> override;

    auto
    Request_Refresh() -> void;

    auto PostLoad() -> void override;
    auto PostInitProperties() -> void override;

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;

    auto PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;

    auto PostCDOCompiled(
        const FPostCDOCompiledContext& Context) -> void override;
#endif

    auto Get_InheritedParent() const -> ThisType*;

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, ShowOnlyInnerProperties))
    FCk_Ecs_InheritedProcessorInjectors  _InheritedProcessorInjectors;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Ecs_MetaProcessorInjectors_Info
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ecs_MetaProcessorInjectors_Info);

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, Categories = "EcsWorldTickingGroup"))
    FGameplayTag _EcsWorldTickingGroup = FGameplayTag::EmptyTag;

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, InvalidEnumValues = "TG_MAX"))
    TEnumAsByte<ETickingGroup> _UnrealTickingGroup = TG_PrePhysics;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, InlineEditConditionToggle))
    bool _HasDisplayName = false;

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, EditCondition = "_HasDisplayName"))
    FName _DisplayName = NAME_None;

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    FName _Description = NAME_None;
#endif

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    ECk_Ecs_WorldStatCollection_Policy _StatCollectionPolicy = ECk_Ecs_WorldStatCollection_Policy::DoNotCollect;

    // Processors can be pumped multiple times _if_ they have requests that still need to be processed
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, ClampMin="0", UIMin="0"))
    int32 _MaximumNumberOfPumps = 1;

    UPROPERTY(EditDefaultsOnly, Instanced,
        meta=(AllowPrivateAccess, MustImplement = "/Script/CkEcs.Ck_MetaProcessorInjector_Interface"))
    TArray<TObjectPtr<UObject>> _MetaProcessorInjectors;

public:
    auto Get_Description() const -> FName;
    auto Get_DisplayName() const -> FName;
    auto Get_MetaProcessorInjectors() const -> TArray<TScriptInterface<ICk_MetaProcessorInjector_Interface>>;

    auto Request_RefreshAllMetaProcessor() -> void;

public:
    CK_PROPERTY(_EcsWorldTickingGroup);
    CK_PROPERTY(_UnrealTickingGroup);
    CK_PROPERTY(_StatCollectionPolicy);
    CK_PROPERTY_GET(_MaximumNumberOfPumps);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable)
class CKECS_API UCk_Ecs_ProcessorInjectors_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ProcessorInjectors_PDA);

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;

    auto PostCDOCompiled(
        const FPostCDOCompiledContext& Context) -> void override;
#endif

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, TitleProperty = "[{_UnrealTickingGroup}] - {_EcsWorldTickingGroup}"))
    TArray<FCk_Ecs_MetaProcessorInjectors_Info> _MetaProcessorInjectors;

public:
    CK_PROPERTY_GET(_MetaProcessorInjectors);
};

// --------------------------------------------------------------------------------------------------------------------
