#pragma once

#include "CkRecord_Fragment.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Delegates/CkDelegates.h"
#include "CkLabel/Public/CkLabel/CkLabel_Fragment.h"
#include "CkLabel/Public/CkLabel/CkLabel_Utils.h"

#include "CkRecord_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedRecord>
    class TUtils_RecordOfEntities
    {
    public:
        CK_GENERATED_BODY(TUtils_RecordOfEntities<T_DerivedRecord>);

        friend class UCk_Utils_RecordEntry_UE;

    public:
        using HandleType            = FCk_Handle;
        using RecordType            = T_DerivedRecord;
        using RecordEntityType      = typename T_DerivedRecord::EntityType;
        using RecordEntryEntityType = typename T_DerivedRecord::EntityType;
        using RecordEntryHandleType = HandleType;
        using RecordEntryListType   = typename RecordType::RecordEntriesType;

    public:
        static auto
        AddIfMissing(
            FCk_Handle InHandle,
            ECk_Record_EntryHandlingPolicy _EntryHandlingPolicy = ECk_Record_EntryHandlingPolicy::Default) -> void;

        static auto
        Has(
            FCk_Handle InHandle) -> bool;

        static auto
        Ensure(
            FCk_Handle InHandle) -> bool;

        static auto
        Get_ValidEntriesCount(
            FCk_Handle InRecordHandle) -> int32;

    public:
        template <typename T_Predicate>
        static auto
        Get_HasValidEntry_If(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate) -> bool;

        template <typename T_Predicate>
        static auto
        Get_ValidEntry_If(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate) -> FCk_Handle;

    public:
        template <typename T_Func>
        static auto
        ForEach_ValidEntry(
            FCk_Handle InHandle,
            T_Func InFunc) -> void;

        template <typename T_Unary, typename T_Predicate>
        static auto
        ForEach_ValidEntry_If(
            FCk_Handle InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate) -> void;

    public:
        static auto
        Request_Connect(
            FCk_Handle InRecordHandle,
            FCk_Handle InRecordEntry) -> void;

        static auto
        Request_Disconnect(
            FCk_Handle InRecordHandle,
            FCk_Handle InRecordEntry) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        AddIfMissing(
            FCk_Handle InHandle,
            ECk_Record_EntryHandlingPolicy _EntryHandlingPolicy)
        -> void
    {
        const auto& Record = InHandle.AddOrGet<RecordType>(_EntryHandlingPolicy);
        CK_ENSURE_IF_NOT(Record.Get_EntryHandlingPolicy() == _EntryHandlingPolicy,
            TEXT("Trying to Add a Record with a different EntryHandlingPolicy to Entity [{}]"),
            InHandle)
        {}
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Has(
            FCk_Handle InHandle)
        -> bool
    {
        return InHandle.Has<RecordType>();
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Ensure(
            FCk_Handle InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have [{}]"), InHandle, ck::TypeToString<RecordType>)
        { return false; }

        return true;
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_ValidEntriesCount(
            FCk_Handle InRecordHandle)
        -> int32
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();
        return algo::CountIf(Fragment.Get_RecordEntries(), algo::IsValidEntityHandle{});
    }

    template <typename T_DerivedRecord>
    template <typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_HasValidEntry_If(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate)
        -> bool
    {
        return ck::IsValid(Get_ValidEntry_If(InRecordHandle, InPredicate));
    }

    template <typename T_DerivedRecord>
    template <typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_ValidEntry_If(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate)
        -> FCk_Handle
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();

        for (const auto& RecordEntry : Fragment.Get_RecordEntries())
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

            if (ck::Is_NOT_Valid(RecordEntryHandle))
            { continue; }

            if (const auto Result = InPredicate(RecordEntryHandle))
            { return RecordEntryHandle; }
        }

        return {};
    }

    template <typename T_DerivedRecord>
    template <typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_ValidEntry(
            FCk_Handle InHandle,
            T_Func InFunc) -> void
    {
        auto& Fragment = InHandle.Get<RecordType>();
        auto& RecordEntries = Fragment._RecordEntries;

        for (auto Index = 0; Index < RecordEntries.Num(); ++Index)
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntries[Index], InHandle);

            if (ck::Is_NOT_Valid(RecordEntryHandle))
            {
                RecordEntries.RemoveAtSwap(Index);
                --Index;
                continue;
            }

            InFunc(RecordEntryHandle);
        }
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_ValidEntry_If(
            FCk_Handle InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate)
        -> void
    {
        ForEach_ValidEntry(InRecordHandle, [&](FCk_Handle InRecordEntryHandle)
        {
            if (InPredicate(InRecordEntryHandle))
            { InFunc(InRecordEntryHandle); }
        });
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Request_Connect(
            FCk_Handle InRecordHandle,
            FCk_Handle InRecordEntry)
        -> void
    {
        if (NOT Ensure(InRecordHandle))
        { return; }

        auto& RecordFragment = InRecordHandle.Get<RecordType>();
        const auto& CurrentRecordEntries = RecordFragment.Get_RecordEntries();

        CK_ENSURE_IF_NOT(NOT CurrentRecordEntries.Contains(InRecordEntry),
            TEXT("The Record [{}] ALREADY contains the RecordEntry [{}]"), InRecordHandle, InRecordEntry)
        { return; }

        if (RecordFragment.Get_EntryHandlingPolicy() == ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames)
        {
            CK_ENSURE_IF_NOT(UCk_Utils_GameplayLabel_UE::Has(InRecordEntry),
                TEXT("Cannot Connect RecordEntry [{}] to Record [{}] because it does NOT have a GameplayLabel!"),
                InRecordEntry,
                InRecordHandle)
            { return; }

            const auto& RecordEntryLabel = UCk_Utils_GameplayLabel_UE::Get_Label(InRecordEntry);

            if (ck::IsValid(RecordEntryLabel))
            {
                CK_ENSURE_IF_NOT(NOT Get_HasValidEntry_If(InRecordHandle, ck::algo::MatchesGameplayLabelExact{RecordEntryLabel}),
                    TEXT("Cannot Connect RecordEntry [{}] to Record [{}] because there is already an entry with the GameplayLabel [{}]"),
                    InRecordEntry,
                    InRecordHandle,
                    RecordEntryLabel)
                { return; }
            }
        }

        RecordFragment._RecordEntries.Emplace(InRecordEntry);

        if (NOT UCk_Utils_RecordEntry_UE::Has(InRecordEntry))
        { UCk_Utils_RecordEntry_UE::Add(InRecordEntry); }

        auto& RecordEntryFragment = InRecordEntry.Get<ck::FFragment_RecordEntry>();
        RecordEntryFragment._Records.Emplace(InRecordHandle);

        RecordEntryFragment._DisconnectionFuncs.Add(InRecordHandle,
        [](FCk_Handle InRecordEntity, FCk_Handle InRecordEntryEntity)
        {
            InRecordEntity.Get<T_DerivedRecord>()._RecordEntries.Remove(InRecordEntryEntity);
        });
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Request_Disconnect(
            FCk_Handle InRecordHandle,
            FCk_Handle InRecordEntry)
        -> void
    {
        if (NOT Ensure(InRecordHandle))
        { return; }

        if (NOT UCk_Utils_RecordEntry_UE::Ensure(InRecordEntry))
        { return; }

        const auto& RecordEntryLabel = UCk_Utils_GameplayLabel_UE::Get_Label(InRecordEntry);

        {
            auto& RecordFragment = InRecordHandle.Get<RecordType>();
            const auto& RemovalSuccess = RecordFragment._RecordEntries.Remove(InRecordEntry);

            CK_ENSURE_IF_NOT(RemovalSuccess,
                TEXT("The Record [{}] couldn't remove the RecordEntry [{}] with name [{}]. Does the RecordEntry exist in the Record?"),
                InRecordHandle,
                InRecordEntry,
                RecordEntryLabel)
            { return; }
        }

        {
            auto& RecordEntryFragment = InRecordEntry.Get<ck::FFragment_RecordEntry>();
            const auto& RemovalSuccess = RecordEntryFragment._Records.Remove(InRecordHandle);

            RecordEntryFragment._DisconnectionFuncs.Remove(InRecordHandle);

            CK_ENSURE_IF_NOT(RemovalSuccess,
                TEXT("The RecordEntry [{}] with name [{}] does NOT have the Record [{}] even though the Record had the RecordEntry. "
                    "Somehow the RecordEntry was out of sync with the Record."),
                InRecordEntry,
                RecordEntryLabel,
                InRecordHandle)
            { return; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKRECORD_API UCk_Utils_RecordOfEntities_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RecordOfEntities_UE);

    friend class UCk_Utils_RecordEntry_UE;

public:
    using UtilsType = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfEntities>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              DisplayName = "Add Record of Entities")
    static void
    Add(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Record",
              DisplayName = "Has Record of Entities?")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Record",
              DisplayName = "Ensure Has Record of Entities")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static bool
    Get_HasValidEntry_If(
        FCk_Handle InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static FCk_Handle
    Get_ValidEntry_If(
        FCk_Handle InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              DisplayName="For Each RecordEntry",
              meta=(AutoCreateRefTerm="InFunc, InOptionalPayload"))
    static TArray<FCk_Handle>
    ForEach_ValidEntry(
        FCk_Handle InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InFunc);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              DisplayName="For Each RecordEntry IF",
              meta=(AutoCreateRefTerm="InOptionalPayload, InFunc"))
    static TArray<FCk_Handle>
    ForEach_ValidEntry_If(
        FCk_Handle InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InFunc,
        const FCk_Predicate_InHandle_OutResult& InPredicate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    Request_Connect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    Request_Disconnect(
        FCk_Handle InRecordHandle,
        FCk_Handle InRecordEntry);
};

// --------------------------------------------------------------------------------------------------------------------
