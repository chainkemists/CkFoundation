#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkDialog/CkDialogBank_DataAsset.h"
#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkDialog_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Per-(game/PIE)-world registry of dialogue-line entities. Owns a transient registry-root entity under which every
// line entity (and its condition child entities) lives; destroying the root on Deinitialize cascades the whole set.
UCLASS(DisplayName = "CkSubsystem_DialogRegistry")
class CKDIALOG_API UCk_DialogRegistry_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DialogRegistry_Subsystem_UE);

public:
    auto PostInitialize() -> void override;
    auto Deinitialize() -> void override;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Get Registry",
              meta = (WorldContext = "InWorldContextObject"))
    static UCk_DialogRegistry_Subsystem_UE*
    Get_DialogRegistry(
        const UObject* InWorldContextObject);

public:
    // Registry is ready once the initial settings banks are processed AND every queued condition child entity has
    // finished constructing (a query issued before then is deferred by the emitter query processor).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Get Is Ready")
    bool
    Get_IsReady() const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Get All Lines")
    TArray<FCk_Handle_DialogLine>
    Get_AllLines();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Get Lines By Enter Tag")
    TArray<FCk_Handle_DialogLine>
    Get_Lines_ByEventTag(
        UPARAM(meta = (Categories = "Ck.Dialog.Event")) FGameplayTag InEventTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Get Registered Banks")
    TArray<UCk_DialogBank_DataAsset*>
    Get_RegisteredBanks() const;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Request Register Bank")
    void
    Request_RegisterBank(
        UCk_DialogBank_DataAsset* InBank);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Request Unregister Bank")
    void
    Request_UnregisterBank(
        UCk_DialogBank_DataAsset* InBank);

    // Registers a single line (dynamic content + the test seam). Returns the new line handle (valid immediately; its
    // ENTER tag becomes query-visible one pump later, and any conditions finish constructing shortly after).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Request Register Line")
    FCk_Handle_DialogLine
    Request_RegisterLine(
        FCk_DialogBank_LineData InLineData,
        FGameplayTagContainer InBankTags);

    // Register a line with a single already-instanced condition, avoiding the bank's instanced-object array (an
    // awkward BP/AS marshalling case). Pass null for no condition.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Request Register Line (With Condition)")
    FCk_Handle_DialogLine
    Request_RegisterLine_WithCondition(
        FCk_DialogBank_LineData InLineData,
        FGameplayTagContainer InBankTags,
        UCk_DialogCondition_EntityScript* InCondition);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Registry",
              DisplayName = "[Ck][Dialog] Request Unregister Line")
    void
    Request_UnregisterLine(
        FCk_Handle_DialogLine InLine);

private:
    auto
    DoRegisterBank(
        UCk_DialogBank_DataAsset* InBank) -> void;

    auto
    DoOnConditionConstructed() -> void;

    auto
    DoPruneInvalidLines() -> void;

private:
    FCk_Handle _RegistryRoot;

    TArray<FCk_Handle_DialogLine> _AllLines;

    // Fast duplicate-ID check; kept consistent with _AllLines by DoPruneInvalidLines via _LineHandleToID.
    TSet<FName> _LineIDs;

    TMap<FCk_Handle_DialogLine, FName> _LineHandleToID;

    // UPROPERTY so registered banks (and their instanced condition archetypes) are GC-rooted while registered.
    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_DialogBank_DataAsset>> _RegisteredBanks;

    // Keys stay GC-alive via _RegisteredBanks.
    TMap<TObjectPtr<UCk_DialogBank_DataAsset>, TArray<FCk_Handle_DialogLine>> _BankToLines;

    bool _AllBanksProcessed = false;

    int32 _PendingConditionSpawns = 0;
};

// --------------------------------------------------------------------------------------------------------------------
