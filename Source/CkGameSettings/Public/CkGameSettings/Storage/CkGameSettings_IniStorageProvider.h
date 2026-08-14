#pragma once

#include "CkGameSettings/Storage/CkGameSettings_StorageProvider.h"

#include "CkGameSettings_IniStorageProvider.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Default storage provider: a hand-serialized, ORDER-PRESERVING ini file at
 * Saved/Config/<Platform>/CkGameSettings.ini that never touches GConfig.
 *
 * Format: [Machine] and [Player.<Id>] sections with Key=Value lines. File order == application
 * order; an existing key keeps its position on update, a new key appends. Unknown lines
 * (comments, blanks, hand-written extras) are preserved verbatim across rewrites. Flush rewrites
 * the whole file atomically (temp file + move) and only when dirty.
 */
UCLASS(NotBlueprintable)
class CKGAMESETTINGS_API UCk_GameSettings_IniStorageProvider_UE : public UCk_GameSettings_StorageProvider_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_IniStorageProvider_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings|Storage",
              DisplayName = "[Ck][GameSettings] Get Storage File Path")
    FString
    Get_StorageFilePath() const;

    /** Redirect the provider at a different file (test seam). Drops any unflushed state; the new file loads lazily. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings|Storage",
              DisplayName = "[Ck][GameSettings] Set File Path Override")
    void
    Set_FilePathOverride(
        const FString& InFilePath);

private:
    auto Get_StoredValues_Implementation(ECk_GameSettings_Scope InScope, int32 InPlatformUserId) const -> TArray<FCk_GameSettings_StoredValue> override;
    auto Request_StoreValue_Implementation(ECk_GameSettings_Scope InScope, int32 InPlatformUserId, FName InKey, const FString& InValue) -> void override;
    auto Request_RemoveValue_Implementation(ECk_GameSettings_Scope InScope, int32 InPlatformUserId, FName InKey) -> void override;
    auto Request_Flush_Implementation() -> void override;

private:
    struct FIniLine
    {
        FString _Raw;
        FName _Key;
    };

    struct FIniSection
    {
        FString _Name;
        TArray<FIniLine> _Lines;
    };

private:
    auto DoEnsureLoaded() const -> void;
    auto DoFindSection(const FString& InSectionName) const -> const FIniSection*;
    auto DoFindSection(const FString& InSectionName) -> FIniSection*;
    auto DoFindOrAddSection(const FString& InSectionName) -> FIniSection&;

    static auto Get_SectionName(ECk_GameSettings_Scope InScope, int32 InPlatformUserId) -> FString;

private:
    FString _FilePathOverride;

    mutable TArray<FString> _PreambleLines;
    mutable TArray<FIniSection> _Sections;
    mutable bool _Loaded = false;

    bool _Dirty = false;
};

// --------------------------------------------------------------------------------------------------------------------
