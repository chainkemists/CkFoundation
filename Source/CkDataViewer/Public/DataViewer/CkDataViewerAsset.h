#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "EdGraph/EdGraphPin.h"

#include "Engine/DataAsset.h"

#include <CoreMinimal.h>

#include "CkDataViewerAsset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDATAVIEWER_API FCk_DataViewer_ObjectsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DataViewer_ObjectsInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    TSubclassOf<UObject> _Class;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    FString _OptionalFriendlyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess))
    TArray<FString> _PropertiesToIgnore;

public:
    CK_PROPERTY_GET(_Class);
    CK_PROPERTY_GET(_OptionalFriendlyName);
    CK_PROPERTY_GET(_PropertiesToIgnore);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKDATAVIEWER_API UCk_DataViewerAsset_PDA : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DataViewerAsset_PDA);

public:
    auto
    PostLoad() -> void override;

    auto
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;

public:
    UFUNCTION(BlueprintNativeEvent)
    TArray<FCk_DataViewer_ObjectsInfo>
    DoGetObjectsToView() const;

    UFUNCTION(BlueprintNativeEvent)
    TArray<FString>
    DoGetPropertiesToIgnore() const;

private:
    enum class ECompileOrNot
    {
        Compile,
        DoNotCompile
    };

    auto
    DoUpdatePropertyOnBlueprint(
        const FGuid& InGuid,
        FName InPropertyName,
        const FString& InValue) -> void;

    auto
    DoGet_ValueFromProperty(
        FProperty* InProperty) const
        -> FString;

    static auto
    DoGet_ValueFromProperty(
        FProperty* InProperty,
        const uint8* InContainer) -> FString;

    auto
    DoResetView(
        ECompileOrNot InCompileOrNot = ECompileOrNot::Compile) -> void;

    auto
    DoForcePackageMarkNotDirty(
        UBlueprint* InBlueprint) -> void;

public:
    static auto
    TryGet_PinType(
        const FStructProperty* InProperty) -> TOptional<FEdGraphPinType>;

public:
    UFUNCTION(CallInEditor)
    void
    ResetView();

    UFUNCTION(CallInEditor)
    void
    WriteAll();

    UFUNCTION(CallInEditor)
    void
    Reload();

private:
    auto
    DoGet_Blueprint(
        const FGuid& InGuid) const
        -> UBlueprint*;

};
