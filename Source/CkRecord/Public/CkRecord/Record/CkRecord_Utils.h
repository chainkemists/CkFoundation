#pragma once

#include "CkRecord_Fragment.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Utils.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Fragment.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Delegates/CkDelegates.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkLabel/Public/CkLabel/CkLabel_Fragment.h"
#include "CkLabel/Public/CkLabel/CkLabel_Utils.h"
#include "CkRecord/RecordEntry/CkRecordEntry_Fragment_Data.h"

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
        using RecordType            = T_DerivedRecord;
        using MaybeTypeSafeHandle   = typename T_DerivedRecord::MaybeTypeSafeHandle;
        using RecordEntityType      = typename T_DerivedRecord::EntityType;
        using RecordEntryEntityType = typename T_DerivedRecord::EntityType;
        using RecordEntryMaybeTypeSafeHandle = MaybeTypeSafeHandle;
        using RecordEntryListType   = typename RecordType::RecordEntriesType;

    public:
        static auto
        AddIfMissing(
            FCk_Handle& InHandle,
            ECk_Record_EntryHandlingPolicy _EntryHandlingPolicy = ECk_Record_EntryHandlingPolicy::Default) -> void;

        [[nodiscard]]
        static auto
        Has(
            const FCk_Handle& InHandle) -> bool;

        [[nodiscard]]
        static auto
        Ensure(
            const FCk_Handle& InHandle) -> bool;

        [[nodiscard]]
        static auto
        Get_ValidEntriesCount(
            const FCk_Handle& InRecordHandle) -> int32;

        [[nodiscard]]
        static auto
        Get_ContainsEntry(
            const FCk_Handle& InRecordHandle,
            const MaybeTypeSafeHandle& InRecordEntry) -> bool;

    public:
        template <typename T_Predicate>
        [[nodiscard]]
        static auto
        Get_ValidEntriesCount_If(
            const FCk_Handle& InRecordHandle,
            T_Predicate InPredicate) -> int32;

        template <typename T_Predicate>
        [[nodiscard]]
        static auto
        Get_HasValidEntry_If(
            const FCk_Handle& InRecordHandle,
            T_Predicate InPredicate) -> bool;

        template <typename T_Predicate>
        [[nodiscard]]
        static auto
        Get_ValidEntry_If(
            const FCk_Handle& InRecordHandle,
            T_Predicate InPredicate) -> MaybeTypeSafeHandle;

    public:
        template <typename T_Func>
        static auto
        ForEach_Entry(
            FCk_Handle& InHandle,
            T_Func InFunc) -> void;

        template <typename T_Func>
        static auto
        ForEach_Entry(
            const FCk_Handle& InHandle,
            T_Func InFunc) -> void;

        template <typename T_Unary, typename T_Predicate>
        static auto
        ForEach_Entry_If(
            FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate) -> void;

        template <typename T_Unary, typename T_Predicate>
        static auto
        ForEach_Entry_If(
            const FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate) -> void;

    public:
        template <typename T_Func>
        static auto
        ForEach_ValidEntry(
            FCk_Handle& InHandle,
            T_Func InFunc,
            ECk_Record_ForEach_Policy InPolicy = ECk_Record_ForEach_Policy::EnsureRecordExists) -> void;

        template <typename T_Func>
        static auto
        ForEach_ValidEntry(
            const FCk_Handle& InHandle,
            T_Func InFunc,
            ECk_Record_ForEach_Policy InPolicy = ECk_Record_ForEach_Policy::EnsureRecordExists) -> void;

        template <typename T_Unary, typename T_Predicate>
        static auto
        ForEach_ValidEntry_If(
            FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate,
            ECk_Record_ForEach_Policy InPolicy = ECk_Record_ForEach_Policy::EnsureRecordExists) -> void;

        template <typename T_Unary, typename T_Predicate>
        static auto
        ForEach_ValidEntry_If(
            const FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate,
            ECk_Record_ForEach_Policy InPolicy = ECk_Record_ForEach_Policy::EnsureRecordExists) -> void;

    protected:
        template <typename T_ValidationPolicy, typename T_Func>
        static auto
        DoForEach_Entry(
            FCk_Handle& InHandle,
            T_Func InFunc) -> void;

        template <typename T_ValidationPolicy, typename T_Func>
        static auto
        DoForEach_Entry(
            const FCk_Handle& InHandle,
            T_Func InFunc) -> void;

        template <typename T_ValidationPolicy, typename T_Unary, typename T_Predicate>
        static auto
        DoForEach_Entry_If(
            FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate) -> void;

        template <typename T_ValidationPolicy, typename T_Unary, typename T_Predicate>
        static auto
        DoForEach_Entry_If(
            const FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate) -> void;

    public:
        static auto
        Request_Connect(
            FCk_Handle& InRecordHandle,
            MaybeTypeSafeHandle& InRecordEntry) -> void;

        static auto
        Request_Disconnect(
            FCk_Handle& InRecordHandle,
            MaybeTypeSafeHandle& InRecordEntry) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class RecordOfEntityExtensions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfEntityExtensions>
    {
        template <typename T>
        friend class TUtils_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        AddIfMissing(
            FCk_Handle& InHandle,
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
            const FCk_Handle& InHandle)
        -> bool
    {
        return InHandle.Has<RecordType>();
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Ensure(
            const FCk_Handle& InHandle)
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
            const FCk_Handle& InRecordHandle)
        -> int32
    {
        return Get_ValidEntriesCount_If(InRecordHandle, algo::IsValidEntityHandle{});
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_ContainsEntry(
            const FCk_Handle& InRecordHandle,
            const MaybeTypeSafeHandle& InRecordEntry)
        -> bool
    {
        if (NOT InRecordHandle.Has<RecordType>())
        { return {}; }

        const auto& Fragment = InRecordHandle.Get<RecordType>();

        if (const auto MaybeContainsEntry = Fragment.Get_RecordEntries().Contains(InRecordEntry))
        { return true; }

        auto ExtensionContainEntry = false;
        RecordOfEntityExtensions_Utils::ForEach_Entry(InRecordHandle,
        [&](const FCk_Handle_EntityExtension& InEntityExtension)
        {
            if (Get_ContainsEntry(InEntityExtension, InRecordEntry))
            {
                ExtensionContainEntry = true;
                return ECk_Record_ForEachIterationResult::Break;
            }

            return ECk_Record_ForEachIterationResult::Continue;
        });

        return ExtensionContainEntry;
    }

    template <typename T_DerivedRecord>
    template <typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_ValidEntriesCount_If(
            const FCk_Handle& InRecordHandle,
            T_Predicate InPredicate) -> int32
    {
        auto Count = 0;

        if (InRecordHandle.Has<RecordType>())
        {
            const auto& Fragment = InRecordHandle.Get<RecordType>();
            Count = algo::CountIf(Fragment.Get_RecordEntries(), InPredicate);
        }

        RecordOfEntityExtensions_Utils::ForEach_Entry(InRecordHandle,
        [&](const FCk_Handle_EntityExtension& InEntityExtension)
        {
            Count += Get_ValidEntriesCount_If(InEntityExtension, InPredicate);
        });

        return Count;
    }

    template <typename T_DerivedRecord>
    template <typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Get_HasValidEntry_If(
            const FCk_Handle& InRecordHandle,
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
            const FCk_Handle& InRecordHandle,
            T_Predicate InPredicate)
        -> MaybeTypeSafeHandle
    {
        if (NOT InRecordHandle.Has<RecordType>())
        { return {}; }

        auto MaybeValidEntry = [&]() -> MaybeTypeSafeHandle
        {
            for (const auto& Fragment = InRecordHandle.Get<RecordType>();
                 const auto& RecordEntry : Fragment.Get_RecordEntries())
            {
                auto RecordEntryHandle = ck::MakeHandle(RecordEntry, InRecordHandle);

                if (ck::Is_NOT_Valid(RecordEntryHandle))
                { continue; }

                if (const auto Result = InPredicate(RecordEntryHandle))
                { return ck::StaticCast<MaybeTypeSafeHandle>(RecordEntryHandle); }
            }

            return {};
        }();

        if (ck::IsValid(MaybeValidEntry))
        { return MaybeValidEntry; }

        RecordOfEntityExtensions_Utils::ForEach_Entry(InRecordHandle, [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            MaybeValidEntry = Get_ValidEntry_If(InEntityExtension, InPredicate);

            if (ck::IsValid(MaybeValidEntry))
            { return ECk_Record_ForEachIterationResult::Break; }

            return ECk_Record_ForEachIterationResult::Continue;
        });

        return MaybeValidEntry;
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedRecord>
    template <typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_Entry(
            FCk_Handle& InHandle,
            T_Func InFunc)
        -> void
    {
        DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InHandle, InFunc);

        RecordOfEntityExtensions_Utils::DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InEntityExtension, InFunc);
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_Entry(
            const FCk_Handle& InHandle,
            T_Func InFunc)
        -> void
    {
        DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InHandle, InFunc);

        RecordOfEntityExtensions_Utils::DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InEntityExtension, InFunc);
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_Entry_If(
            FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate)
        -> void
    {
        DoForEach_Entry_If<IsValid_Policy_IncludePendingKill>(InRecordHandle, InFunc, InPredicate);

        RecordOfEntityExtensions_Utils::DoForEach_Entry_If<IsValid_Policy_IncludePendingKill>(InRecordHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InEntityExtension, InFunc);
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_Entry_If(
            const FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate)
        -> void
    {
        DoForEach_Entry_If<IsValid_Policy_IncludePendingKill>(InRecordHandle, InFunc, InPredicate);

        RecordOfEntityExtensions_Utils::DoForEach_Entry_If<IsValid_Policy_IncludePendingKill>(InRecordHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry<IsValid_Policy_IncludePendingKill>(InEntityExtension, InFunc);
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedRecord>
    template <typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_ValidEntry(
            FCk_Handle& InHandle,
            T_Func InFunc,
            ECk_Record_ForEach_Policy InPolicy)
        -> void
    {
        DoForEach_Entry<IsValid_Policy_Default>(InHandle, InFunc);

        RecordOfEntityExtensions_Utils::DoForEach_Entry<IsValid_Policy_Default>(InHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry<IsValid_Policy_Default>(InEntityExtension, InFunc);
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_ValidEntry(
            const FCk_Handle& InHandle,
            T_Func InFunc,
            ECk_Record_ForEach_Policy InPolicy)
        -> void
    {
        DoForEach_Entry<IsValid_Policy_Default>(InHandle, InFunc);

        RecordOfEntityExtensions_Utils::DoForEach_Entry<IsValid_Policy_Default>(InHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry<IsValid_Policy_Default>(InEntityExtension, InFunc);
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_ValidEntry_If(
            FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate,
            ECk_Record_ForEach_Policy InPolicy)
        -> void
    {
        DoForEach_Entry_If<IsValid_Policy_Default>(InRecordHandle, InFunc, InPredicate);

        RecordOfEntityExtensions_Utils::DoForEach_Entry<IsValid_Policy_Default>(InRecordHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry_If<IsValid_Policy_Default>(InEntityExtension, InFunc, InPredicate);
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        ForEach_ValidEntry_If(
            const FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate,
            ECk_Record_ForEach_Policy InPolicy)
        -> void
    {
        DoForEach_Entry_If<IsValid_Policy_Default>(InRecordHandle, InFunc, InPredicate);

        RecordOfEntityExtensions_Utils::DoForEach_Entry<IsValid_Policy_Default>(InRecordHandle,
        [&](FCk_Handle_EntityExtension InEntityExtension)
        {
            DoForEach_Entry_If<IsValid_Policy_Default>(InEntityExtension, InFunc, InPredicate);
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedRecord>
    template <typename T_ValidationPolicy, typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        DoForEach_Entry(
            FCk_Handle& InHandle,
            T_Func InFunc)
        -> void
    {
        if (NOT InHandle.Has<RecordType>())
        { return; }

        auto& Fragment      = InHandle.Get<RecordType, T_ValidationPolicy>();
        auto& RecordEntries = Fragment._RecordEntries;

        for (auto Index = 0; Index < RecordEntries.Num(); ++Index)
        {
            const auto RecordEntryHandle = ck::MakeHandle(RecordEntries[Index], InHandle);

            if (ck::Is_NOT_Valid(RecordEntryHandle, T_ValidationPolicy{}))
            {
                if (ck::Is_NOT_Valid(RecordEntryHandle, ck::IsValid_Policy_IncludePendingKill{}))
                {
                    RecordEntries.RemoveAtSwap(Index);
                    --Index;
                }
                continue;
            }

            if constexpr (std::is_void_v<decltype(InFunc(RecordEntryHandle))>)
            {
                InFunc(RecordEntryHandle);
            }
            else
            {
                if (InFunc(RecordEntryHandle) == ECk_Record_ForEachIterationResult::Break)
                { break; }
            }
        }
    }

    template <typename T_DerivedRecord>
    template <typename T_ValidationPolicy, typename T_Func>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        DoForEach_Entry(
            const FCk_Handle& InHandle,
            T_Func InFunc)
        -> void
    {
        if (NOT InHandle.Has<RecordType>())
        { return; }

        const auto& Fragment      = InHandle.Get<RecordType, T_ValidationPolicy>();
        const auto& RecordEntries = Fragment._RecordEntries;

        for (auto Index = 0; Index < RecordEntries.Num(); ++Index)
        {
            const auto RecordEntryHandle = ck::StaticCast<const MaybeTypeSafeHandle>(ck::MakeHandle(RecordEntries[Index], InHandle));

            if (ck::Is_NOT_Valid(RecordEntryHandle, T_ValidationPolicy{}))
            {
                if (ck::Is_NOT_Valid(RecordEntryHandle, ck::IsValid_Policy_IncludePendingKill{}))
                {
                    // not using RecordEntries local variable to keep mutability
                    Fragment._RecordEntries.RemoveAtSwap(Index);
                    --Index;
                }
                continue;
            }

            if constexpr (std::is_void_v<decltype(InFunc(RecordEntryHandle))>)
            {
                InFunc(RecordEntryHandle);
            }
            else
            {
                if (InFunc(RecordEntryHandle) == ECk_Record_ForEachIterationResult::Break)
                { break; }
            }
        }
    }

    template <typename T_DerivedRecord>
    template <typename T_ValidationPolicy, typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        DoForEach_Entry_If(
            FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate)
        -> void
    {
        DoForEach_Entry<T_ValidationPolicy>(InRecordHandle, [&](MaybeTypeSafeHandle InRecordEntryHandle)
        {
            if (InPredicate(InRecordEntryHandle))
            { InFunc(InRecordEntryHandle); }
        });
    }

    template <typename T_DerivedRecord>
    template <typename T_ValidationPolicy, typename T_Unary, typename T_Predicate>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        DoForEach_Entry_If(
            const FCk_Handle& InRecordHandle,
            T_Unary InFunc,
            T_Predicate InPredicate)
        -> void
    {
        DoForEach_Entry<T_ValidationPolicy>(InRecordHandle, [&](const MaybeTypeSafeHandle& InRecordEntryHandle)
        {
            if (InPredicate(InRecordEntryHandle))
            { InFunc(InRecordEntryHandle); }
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Request_Connect(
            FCk_Handle& InRecordHandle,
            MaybeTypeSafeHandle& InRecordEntry)
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

        auto& RecordEntryFragment = InRecordEntry.template Get<ck::FFragment_RecordEntry>();
        RecordEntryFragment._Records.Emplace(InRecordHandle);

        RecordEntryFragment._DisconnectionFuncs.Add(InRecordHandle,
        [](FCk_Handle InRecordEntity, FCk_Handle InRecordEntryEntity)
        {
            InRecordEntity.Get<T_DerivedRecord>()._RecordEntries.Remove(ck::StaticCast<MaybeTypeSafeHandle>(InRecordEntryEntity));
        });
    }

    template <typename T_DerivedRecord>
    auto
        TUtils_RecordOfEntities<T_DerivedRecord>::
        Request_Disconnect(
            FCk_Handle& InRecordHandle,
            MaybeTypeSafeHandle& InRecordEntry)
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
            auto& RecordEntryFragment = InRecordEntry.template Get<ck::FFragment_RecordEntry>();
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
    using UtilsType = ck::TUtils_RecordOfEntities<ck::TFragment_RecordOfEntities<FCk_Handle>>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Record",
              DisplayName = "[Ck][Record] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Record",
              DisplayName = "[Ck][Record] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Record",
              DisplayName = "[Ck][Record] Ensure Has Feature")
    static bool
    Ensure(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Record] Get Has Valid Entry If",
              Category = "Ck|Utils|Record",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static bool
    Get_HasValidEntry_If(
        const FCk_Handle& InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Record] Get Valid Entry If",
              Category = "Ck|Utils|Record",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static FCk_Handle
    Get_ValidEntry_If(
        const FCk_Handle& InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Record] For Each Valid Entry",
              Category = "Ck|Utils|Record",
              meta=(AutoCreateRefTerm="InFunc, InOptionalPayload"))
    static TArray<FCk_Handle>
    ForEach_ValidEntry(
        FCk_Handle& InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InFunc);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Record] For Each Valid Entry If",
              Category = "Ck|Utils|Record",
              meta=(AutoCreateRefTerm="InOptionalPayload, InFunc"))
    static TArray<FCk_Handle>
    ForEach_ValidEntry_If(
        FCk_Handle& InRecordHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InFunc,
        const FCk_Predicate_InHandle_OutResult& InPredicate);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Record] Request Connect",
              Category = "Ck|Utils|Record")
    static void
    Request_Connect(
        UPARAM(ref) FCk_Handle& InRecordHandle,
        UPARAM(ref) FCk_Handle& InRecordEntry);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][Record] Request Disconnect",
              Category = "Ck|Utils|Record")
    static void
    Request_Disconnect(
        UPARAM(ref) FCk_Handle& InRecordHandle,
        UPARAM(ref) FCk_Handle& InRecordEntry);
};

// --------------------------------------------------------------------------------------------------------------------
