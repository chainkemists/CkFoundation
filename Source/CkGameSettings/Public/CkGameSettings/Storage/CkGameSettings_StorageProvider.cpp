#include "CkGameSettings_StorageProvider.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_StorageProvider_UE::
    Get_StoredValues_Implementation(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId) const
    -> TArray<FCk_GameSettings_StoredValue>
{
    return {};
}

auto
    UCk_GameSettings_StorageProvider_UE::
    Request_StoreValue_Implementation(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId,
        FName InKey,
        const FString& InValue)
    -> void
{
}

auto
    UCk_GameSettings_StorageProvider_UE::
    Request_RemoveValue_Implementation(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId,
        FName InKey)
    -> void
{
}

auto
    UCk_GameSettings_StorageProvider_UE::
    Request_Flush_Implementation()
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------
