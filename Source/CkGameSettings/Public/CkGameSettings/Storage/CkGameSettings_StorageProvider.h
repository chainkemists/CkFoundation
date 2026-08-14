#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include "CkGameSettings_StorageProvider.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_GameSettings_StoredValue
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameSettings_StoredValue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _Value;

public:
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GameSettings_StoredValue, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Storage seam for GameSettings values. Implementable from C++, Blueprint, AND AngelScript —
 * a game can route Player-scope values into its own save system (SPUD, EOS cloud, ...) by
 * subclassing this and selecting the class in the GameSettings project settings.
 *
 * InPlatformUserId carries FPlatformUserId::GetInternalId() (FPlatformUserId itself is not
 * BP-exposable). The Get_StoredValues contract is ORDERED: iteration order == application order.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class CKGAMESETTINGS_API UCk_GameSettings_StorageProvider_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_StorageProvider_UE);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|GameSettings|Storage")
    TArray<FCk_GameSettings_StoredValue>
    Get_StoredValues(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|GameSettings|Storage")
    void
    Request_StoreValue(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId,
        FName InKey,
        const FString& InValue);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|GameSettings|Storage")
    void
    Request_RemoveValue(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId,
        FName InKey);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|GameSettings|Storage")
    void
    Request_Flush();
};

// --------------------------------------------------------------------------------------------------------------------
