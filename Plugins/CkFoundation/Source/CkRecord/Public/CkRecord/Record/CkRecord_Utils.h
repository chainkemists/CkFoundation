#pragma once

#include "CkRecord_Fragment.h"

#include "CkRecord/Record/CkRecord_Fragment_Params.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/CkEcs_Utils.h"

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
        using HandleType                   = FCk_Handle;
        using RecordType                   = T_DerivedRecord;
        using RecordEntityType             = typename T_DerivedRecord::EntityType;
        using RecordEntryEntityType        = typename T_DerivedRecord::EntityType;
        using RecordEntryHandleType        = HandleType;
        using RecordEntryListType          = typename RecordType::RecordEntriesType;

    public:
        static auto
        Add(
            FCk_Handle InHandle) -> void;

        static auto
        Has(
            FCk_Handle InHandle) -> bool;

        static auto
        Ensure(
            FCk_Handle InHandle) -> bool;

        static auto
        Get_AllRecordEntries(
            FCk_Handle InRecordHandle) -> TArray<FCk_Handle>;

    public:
        template <typename T_Predicate>
        static auto
        Get_HasRecordEntry(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate) -> bool;

        template <typename T_Predicate>
        static auto
        Get_RecordEntryIf(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate) -> FCk_Handle;

        template <typename T_Unary>
        static auto
        ForEachEntry(
            FCk_Handle InRecordHandle,
            T_Unary InUnaryFunc) -> void;

        template <typename T_Unary, typename T_Predicate>
        static auto
        ForEachEntryIf(
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
        Add(FCk_Handle InHandle)
        -> void
    {
        InHandle.Add<RecordType>();
    }

    template <typename T_DerivedRecord>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        Has(FCk_Handle InHandle)
        -> bool
    {
        return InHandle.Has<RecordType>();
    }

    template <typename T_DerivedRecord>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        Ensure(FCk_Handle InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have [{}]"), InHandle, ctti::nameof_v<RecordType>)
        { return false; }

        return true;
    }

    template <typename T_DerivedRecord>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_AllRecordEntries(
            FCk_Handle InRecordHandle)
        -> TArray<FCk_Handle>
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();

        auto RecordEntries = TArray<FCk_Handle>{};

        for(const auto& RecordEntry : Fragment.Get_RecordEntries())
        {
            RecordEntries.Emplace(ck::MakeHandle(RecordEntry, InRecordHandle));
        }

        return RecordEntries;
    }

    template <typename T_DerivedRecord>
    template <typename T_Predicate>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_HasRecordEntry(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate)
        -> bool
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();

        for(const auto& RecordEntry : Fragment.Get_RecordEntries())
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

            if (const auto Result = InPredicate(RecordEntryHandle))
            { return true; }
        }

        return false;
    }

    template <typename T_DerivedRecord>
    template <typename T_Predicate>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_RecordEntryIf(
            FCk_Handle InRecordHandle,
            T_Predicate InPredicate)
        -> FCk_Handle
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();

        for(const auto& RecordEntry : Fragment.Get_RecordEntries())
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

            if (const auto Result = InPredicate(RecordEntryHandle))
            { return RecordEntryHandle; }
        }

        return {};
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEachEntry(
            FCk_Handle InRecordHandle,
            T_Unary InUnaryFunc)
        -> void
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();

        for(const auto& RecordEntry : Fragment.Get_RecordEntries())
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);
            InUnaryFunc(RecordEntryHandle);
        }
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary, typename T_Predicate>
    auto
    TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEachEntryIf(
            FCk_Handle InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate)
        -> void
    {
        const auto& Fragment = InRecordHandle.Get<RecordType>();

        for(const auto& RecordEntry : Fragment.Get_RecordEntries())
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

            if (const auto Result = InPredicate(RecordEntryHandle))
            { InFunc(RecordEntryHandle); }
        }
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

        CK_ENSURE_IF_NOT(NOT RecordFragment.Get_RecordEntries().Contains(InRecordEntry.Get_Entity()),
            TEXT("The Record [{}] ALREADY contains the RecordEntry [{}]"), InRecordHandle, InRecordEntry)
        { return; }

        RecordFragment._RecordEntries.Emplace(InRecordEntry.Get_Entity());

        if (NOT UCk_Utils_RecordEntry_UE::Has(InRecordEntry))
        { UCk_Utils_RecordEntry_UE::Add(InRecordEntry); }

        auto& RecordEntryFragment = InRecordEntry.Get<ck::FFragment_RecordEntry>();
        RecordEntryFragment._Records.Emplace(InRecordHandle.Get_Entity());
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

        {
            auto& RecordFragment = InRecordHandle.Get<RecordType>();
            const auto& RemovalSuccess = RecordFragment._RecordEntries.Remove(InRecordEntry.Get_Entity());

            CK_ENSURE_IF_NOT(RemovalSuccess,
                TEXT("The Record [{}] couldn't remove the RecordEntry [{}]. Does the RecordEntry exist in the Record?"),
                InRecordHandle,
                InRecordEntry)
            { return; }
        }

        {
            auto& RecordEntryFragment = InRecordHandle.Get<ck::FFragment_RecordEntry>();
            const auto& RemovalSuccess = RecordEntryFragment._Records.Remove(InRecordHandle.Get_Entity());

            CK_ENSURE_IF_NOT(RemovalSuccess,
                TEXT("The RecordEntry [{}] does NOT have the Record [{}] even though the Record had the RecordEntry. "
                    "Somehow the RecordEntry was out of sync with the Record."),
                InRecordEntry,
                InRecordHandle)
            { return; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKRECORD_API UCk_Utils_RecordOfEntities_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RecordOfEntities_UE);

    friend class UCk_Utils_RecordEntry_UE;

public:
    using InternalUtils_Type = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfEntities>;

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

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static TArray<FCk_Handle>
    Get_AllRecordEntries(
        FCk_Handle InRecordHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static bool
    Get_HasRecordEntry(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static FCk_Handle
    Get_RecordEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    ForEachEntry(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record")
    static void
    ForEachEntryIf(
        FCk_Handle InRecordHandle,
        FCk_Lambda_InHandle InFunc,
        FCk_Predicate_InHandle_OutResult InPredicate);

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

private:
    static InternalUtils_Type _Utils;
};


// --------------------------------------------------------------------------------------------------------------------