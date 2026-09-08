#include "CkSlateLayout/CkUiCollection.h"

#include "Styling/SlateBrush.h"

namespace ck_ui_collection
{
    constexpr int32 MaxSchemaFields = 128;
    constexpr int32 MaxRecords = 100000;
    constexpr int32 MaxSuppliedFields = 1000000;

    auto IsName(const FString& InValue) -> bool
    {
        if (InValue.IsEmpty() || !(FChar::IsAlpha(InValue[0]) || InValue[0] == TEXT('_') || InValue[0] == TEXT('-'))) { return false; }
        for (const TCHAR Character : InValue) if (!(FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-'))) { return false; }
        return true;
    }

    auto IsValidKind(const ECkUiFieldKind InKind) -> bool
    {
        return InKind == ECkUiFieldKind::Text || InKind == ECkUiFieldKind::Number || InKind == ECkUiFieldKind::Bool || InKind == ECkUiFieldKind::Color || InKind == ECkUiFieldKind::Image;
    }

    auto IsValidValue(const FCkUiFieldValue& InValue) -> bool
    {
        if (!IsValidKind(InValue.Kind)) { return false; }
        if (InValue.Kind == ECkUiFieldKind::Number) { return FMath::IsFinite(InValue.Number); }
        if (InValue.Kind == ECkUiFieldKind::Color) { return FMath::IsFinite(InValue.Color.R) && FMath::IsFinite(InValue.Color.G) && FMath::IsFinite(InValue.Color.B) && FMath::IsFinite(InValue.Color.A); }
        return true;
    }

    auto EqualValue(const FCkUiFieldValue& A, const FCkUiFieldValue& B) -> bool
    {
        if (A.Kind != B.Kind) { return false; }
        switch (A.Kind)
        {
        case ECkUiFieldKind::Text: return A.Text.EqualTo(B.Text);
        case ECkUiFieldKind::Number: return A.Number == B.Number;
        case ECkUiFieldKind::Bool: return A.Bool == B.Bool;
        case ECkUiFieldKind::Color: return A.Color == B.Color;
        case ECkUiFieldKind::Image: return A.Image.Get() == B.Image.Get();
        default: return false;
        }
    }

    auto EqualData(const FCkUiRecordData& A, const FCkUiRecordData& B) -> bool
    {
        if (A.Key != B.Key || A.Fields.Num() != B.Fields.Num()) { return false; }
        for (const auto& Pair : A.Fields) { const FCkUiFieldValue* Other = B.Fields.Find(Pair.Key); if (Other == nullptr || !EqualValue(Pair.Value, *Other)) { return false; } }
        return true;
    }

    auto Failure(const FString& InError) -> FCkUiLoadResult { return FCkUiLoadResult{.Succeeded = false, .Errors = {InError}}; }
}

auto FCkUiCollection::TryCreate(TArray<FCkUiFieldSchema> InSchema, TSharedPtr<FCkUiCollection>& OutCollection) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_collection::Failure(TEXT("collection creation must run on the game thread")); }
    if (InSchema.IsEmpty() || InSchema.Num() > ck_ui_collection::MaxSchemaFields) { return ck_ui_collection::Failure(TEXT("collection schema must contain 1..128 fields")); }
    auto Names = TSet<FString>{};
    for (const FCkUiFieldSchema& Field : InSchema)
    {
        if (!ck_ui_collection::IsName(Field.Name) || !ck_ui_collection::IsValidKind(Field.Kind) || Names.Contains(Field.Name)) { return ck_ui_collection::Failure(TEXT("collection schema contains an invalid or duplicate field")); }
        Names.Add(Field.Name);
    }
    TSharedPtr<FCkUiCollection> Candidate = MakeShareable(new FCkUiCollection(MoveTemp(InSchema)));
    for (const FCkUiFieldSchema& Field : Candidate->_Schema) { Candidate->_SchemaKinds.Add(Field.Name, Field.Kind); }
    OutCollection = MoveTemp(Candidate);
    return FCkUiLoadResult{.Succeeded = true};
}

auto FCkUiCollection::FindRecord(const FString& InKey) const -> TSharedPtr<const FCkUiRecord>
{
    if (const TSharedPtr<FCkUiRecord>* Found = _ByKey.Find(InKey)) { return *Found; }
    return {};
}

auto FCkUiCollection::TrySetRecords(TArray<FCkUiRecordData> InRecords) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_collection::Failure(TEXT("collection mutation must run on the game thread")); }
    if (_bPublishing) { return ck_ui_collection::Failure(TEXT("collection mutation during change notification is rejected")); }
    if (InRecords.Num() > ck_ui_collection::MaxRecords) { return ck_ui_collection::Failure(TEXT("collection record limit exceeded")); }
    auto Keys = TSet<FString>{}; int32 FieldCount = 0;
    for (const FCkUiRecordData& Data : InRecords)
    {
        if (Data.Key.IsEmpty() || Data.Key.Len() > 1024 || Keys.Contains(Data.Key)) { return ck_ui_collection::Failure(TEXT("collection records require unique nonempty keys")); }
        Keys.Add(Data.Key);
        if (Data.Fields.Num() > ck_ui_collection::MaxSuppliedFields - FieldCount) { return ck_ui_collection::Failure(TEXT("collection supplied field limit exceeded")); }
        FieldCount += Data.Fields.Num();
        for (const auto& Pair : Data.Fields)
        {
            const ECkUiFieldKind* Expected = _SchemaKinds.Find(Pair.Key);
            if (Expected == nullptr || Pair.Value.Kind != *Expected || !ck_ui_collection::IsValidValue(Pair.Value)) { return ck_ui_collection::Failure(TEXT("collection record has an unknown, mismatched, or invalid field")); }
        }
        for (const FCkUiFieldSchema& Field : _Schema) if (Field.Required && !Data.Fields.Contains(Field.Name)) { return ck_ui_collection::Failure(TEXT("collection record is missing a required field")); }
    }
    bool bSame = InRecords.Num() == _Records.Num();
    if (bSame) for (int32 Index = 0; Index < InRecords.Num(); ++Index) { if (!ck_ui_collection::EqualData(InRecords[Index], _Records[Index]->_Data)) { bSame = false; break; } }
    if (bSame) { return FCkUiLoadResult{.Succeeded = true}; }

    const TSharedRef<FCkUiCollection> KeepAlive = AsShared();
    TGuardValue<bool> PublishingGuard(_bPublishing, true);
    struct FPreparedRecord
    {
        TSharedPtr<FCkUiRecord> Record;
        FCkUiRecordData Data;
    };
    auto Updates = TArray<FPreparedRecord>{};
    auto NextRecords = TArray<TSharedPtr<const FCkUiRecord>>{};
    NextRecords.Reserve(InRecords.Num());
    auto NextByKey = TMap<FString, TSharedPtr<FCkUiRecord>>{};
    NextByKey.Reserve(InRecords.Num());
    for (FCkUiRecordData& Data : InRecords)
    {
        TSharedPtr<FCkUiRecord> Record = _ByKey.FindRef(Data.Key);
        if (Record.IsValid())
        {
            if (!ck_ui_collection::EqualData(Data, Record->_Data)) { Updates.Add({Record, MoveTemp(Data)}); }
        }
        else { Record = MakeShareable(new FCkUiRecord(MoveTemp(Data))); }
        NextByKey.Add(Record->GetKey(), Record);
        NextRecords.Add(Record);
    }
    // Keep replaced values and removed rows alive until the entire publication and
    // its notification finish; resource destruction must not observe a partial map.
    auto PreviousRecords = MoveTemp(_Records);
    auto PreviousByKey = MoveTemp(_ByKey);
    for (FPreparedRecord& Update : Updates) { Swap(Update.Record->_Data, Update.Data); }
    _Records = MoveTemp(NextRecords);
    _ByKey = MoveTemp(NextByKey);
    ++_Revision;
    _OnChanged.Broadcast();
    return FCkUiLoadResult{.Succeeded = true};
}
