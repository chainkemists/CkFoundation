#include "CkSlateLayout/CkUiCollection.h"
#include "CkSlateLayout/CkUiFloatSeries.h"

#include "Styling/SlateBrush.h"

namespace ck_ui_collection
{
    constexpr int32 MaxSchemaFields = 128;
    constexpr int32 MaxRecords = 100000;
    constexpr int32 MaxSuppliedFields = 1000000;
    constexpr int32 MaxChildDepth = 32;

    auto IsName(const FString& InValue) -> bool
    {
        if (InValue.IsEmpty() || !(FChar::IsAlpha(InValue[0]) || InValue[0] == TEXT('_') || InValue[0] == TEXT('-'))) { return false; }
        for (const TCHAR Character : InValue) if (!(FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-'))) { return false; }
        return true;
    }

    auto IsValidKind(const ECkUiFieldKind InKind) -> bool
    {
        return InKind == ECkUiFieldKind::Text || InKind == ECkUiFieldKind::Number || InKind == ECkUiFieldKind::Integer || InKind == ECkUiFieldKind::Bool || InKind == ECkUiFieldKind::Color || InKind == ECkUiFieldKind::Image || InKind == ECkUiFieldKind::FloatSeries;
    }

    auto IsValidValue(const FCkUiFieldValue& InValue) -> bool
    {
        if (!IsValidKind(InValue.Kind)) { return false; }
        if (InValue.Kind == ECkUiFieldKind::Number) { return FMath::IsFinite(InValue.Number); }
        if (InValue.Kind == ECkUiFieldKind::Color) { return FMath::IsFinite(InValue.Color.R) && FMath::IsFinite(InValue.Color.G) && FMath::IsFinite(InValue.Color.B) && FMath::IsFinite(InValue.Color.A); }
        if (InValue.Kind == ECkUiFieldKind::FloatSeries) { return InValue.FloatSeries.IsValid(); }
        return true;
    }

    auto EqualValue(const FCkUiFieldValue& A, const FCkUiFieldValue& B) -> bool
    {
        if (A.Kind != B.Kind) { return false; }
        switch (A.Kind)
        {
        case ECkUiFieldKind::Text: return A.Text.EqualTo(B.Text);
        case ECkUiFieldKind::Number: return A.Number == B.Number;
        case ECkUiFieldKind::Integer: return A.Integer == B.Integer;
        case ECkUiFieldKind::Bool: return A.Bool == B.Bool;
        case ECkUiFieldKind::Color: return A.Color == B.Color;
        case ECkUiFieldKind::Image: return A.Image.Get() == B.Image.Get();
        case ECkUiFieldKind::FloatSeries: return A.FloatSeries.Pin().Get() == B.FloatSeries.Pin().Get();
        default: return false;
        }
    }

    auto EqualData(const FCkUiRecordData& A, const FCkUiRecordData& B) -> bool
    {
        if (A.Key != B.Key || A.Fields.Num() != B.Fields.Num() || A.Children.Num() != B.Children.Num()) { return false; }
        for (const auto& Pair : A.Fields) { const FCkUiFieldValue* Other = B.Fields.Find(Pair.Key); if (Other == nullptr || !EqualValue(Pair.Value, *Other)) { return false; } }
        for (const auto& Pair : A.Children)
        {
            const TArray<FCkUiRecordData>* Other = B.Children.Find(Pair.Key);
            if (Other == nullptr || Other->Num() != Pair.Value.Num()) { return false; }
            for (int32 Index = 0; Index < Pair.Value.Num(); ++Index) if (!EqualData(Pair.Value[Index], (*Other)[Index])) { return false; }
        }
        return true;
    }

    auto Failure(const FString& InError) -> FCkUiLoadResult { return FCkUiLoadResult{.Succeeded = false, .Errors = {InError}}; }

    auto ValidateSchema(const FCkUiCollectionSchema& InSchema, const int32 InDepth = 0) -> FCkUiLoadResult
    {
        if (InDepth > MaxChildDepth) { return Failure(TEXT("collection schema child depth limit exceeded")); }
        if (InSchema.Fields.IsEmpty() || InSchema.Fields.Num() > MaxSchemaFields) { return Failure(TEXT("collection schema must contain 1..128 fields")); }
        if (InSchema.Children.Num() > MaxSchemaFields) { return Failure(TEXT("collection schema child collection limit exceeded")); }
        auto Names = TSet<FString>{};
        for (const FCkUiFieldSchema& Field : InSchema.Fields)
        {
            if (!IsName(Field.Name) || !IsValidKind(Field.Kind) || Names.Contains(Field.Name)) { return Failure(TEXT("collection schema contains an invalid or duplicate field")); }
            Names.Add(Field.Name);
        }
        auto ChildNames = TSet<FString>{};
        for (const FCkUiChildCollectionSchema& Child : InSchema.Children)
        {
            if (!IsName(Child.Name) || Names.Contains(Child.Name) || ChildNames.Contains(Child.Name) || Child.Fields.IsEmpty() || Child.Fields.Num() > MaxSchemaFields)
            { return Failure(TEXT("collection schema contains an invalid or duplicate child collection")); }
            ChildNames.Add(Child.Name);
            if (const FCkUiLoadResult ChildValidation = ValidateSchema(FCkUiCollectionSchema{.Fields = Child.Fields, .Children = Child.Children}, InDepth + 1); !ChildValidation.Succeeded)
            { return ChildValidation; }
        }
        return FCkUiLoadResult{.Succeeded = true};
    }

    auto ValidateRecords(const TArray<FCkUiFieldSchema>& InSchema, const TArray<FCkUiChildCollectionSchema>& InChildren,
        const TArray<FCkUiRecordData>& InRecords, const int32 InDepth) -> FCkUiLoadResult
    {
        if (InDepth > MaxChildDepth) { return Failure(TEXT("collection child depth limit exceeded")); }
        if (InRecords.Num() > MaxRecords) { return Failure(TEXT("collection record limit exceeded")); }
        auto Keys = TSet<FString>{};
        int32 FieldCount = 0;
        for (const FCkUiRecordData& Data : InRecords)
        {
            if (Data.Key.IsEmpty() || Data.Key.Len() > 1024 || Keys.Contains(Data.Key)) { return Failure(TEXT("collection records require unique nonempty keys")); }
            Keys.Add(Data.Key);
            if (Data.Fields.Num() > MaxSuppliedFields - FieldCount) { return Failure(TEXT("collection supplied field limit exceeded")); }
            FieldCount += Data.Fields.Num();
            for (const auto& Pair : Data.Fields)
            {
                const FCkUiFieldSchema* Expected = InSchema.FindByPredicate([&Pair](const auto& Field) { return Field.Name == Pair.Key; });
                if (Expected == nullptr || Pair.Value.Kind != Expected->Kind || !IsValidValue(Pair.Value)) { return Failure(TEXT("collection record has an unknown, mismatched, or invalid field")); }
            }
            for (const FCkUiFieldSchema& Field : InSchema) if (Field.Required && !Data.Fields.Contains(Field.Name)) { return Failure(TEXT("collection record is missing a required field")); }
            if (Data.Children.Num() != InChildren.Num()) { return Failure(TEXT("collection record has missing or undeclared child collection")); }
            for (const auto& Pair : Data.Children)
            {
                const FCkUiChildCollectionSchema* ChildSchema = InChildren.FindByPredicate([&Pair](const auto& Child) { return Child.Name == Pair.Key; });
                if (ChildSchema == nullptr) { return Failure(TEXT("collection record has missing or undeclared child collection")); }
                const FCkUiLoadResult ChildResult = ValidateRecords(ChildSchema->Fields, ChildSchema->Children, Pair.Value, InDepth + 1);
                if (!ChildResult.Succeeded) { return ChildResult; }
            }
        }
        return FCkUiLoadResult{.Succeeded = true};
    }

}

auto FCkUiCollection::TryCreate(TArray<FCkUiFieldSchema> InSchema, TSharedPtr<FCkUiCollection>& OutCollection) -> FCkUiLoadResult
{
    return TryCreateHierarchical(FCkUiCollectionSchema{.Fields = MoveTemp(InSchema)}, OutCollection);
}

auto FCkUiCollection::TryCreateHierarchical(FCkUiCollectionSchema InSchema, TSharedPtr<FCkUiCollection>& OutCollection) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_collection::Failure(TEXT("collection creation must run on the game thread")); }
    if (const FCkUiLoadResult Validation = ck_ui_collection::ValidateSchema(InSchema); !Validation.Succeeded) { return Validation; }
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
    auto Updates = TArray<FCkUiCollectionUpdate>{};
    Updates.Add({AsShared(), MoveTemp(InRecords)});
    return TrySetRecordsBatch(MoveTemp(Updates));
}

struct FCkUiCollectionTransaction
{
    struct FRecordUpdate
    {
        TSharedPtr<FCkUiRecord> Record;
        FCkUiRecordData Data;
        TMap<FString, TSharedPtr<FCkUiCollection>> Children;
        bool Changed = false;
    };
    struct FPreparedCollection
    {
        TSharedPtr<FCkUiCollection> Collection;
        bool Changed = false;
        TArray<FRecordUpdate> RecordUpdates;
        TArray<TSharedPtr<const FCkUiRecord>> NextRecords;
        TMap<FString, TSharedPtr<FCkUiRecord>> NextByKey;
        TArray<TSharedPtr<const FCkUiRecord>> PreviousRecords;
        TMap<FString, TSharedPtr<FCkUiRecord>> PreviousByKey;
        TArray<TSharedPtr<FPreparedCollection>> Descendants;
    };

    TArray<TSharedPtr<FCkUiCollection>> Participants;
    TSet<FCkUiCollection*> ParticipantSet;
    TArray<TSharedPtr<FPreparedCollection>> Roots;

    auto Prepare(const TSharedPtr<FCkUiCollection>& InCollection, const TArray<FCkUiRecordData>& InRecords,
        TSharedPtr<FPreparedCollection>& OutPrepared) -> FCkUiLoadResult
    {
        if (!InCollection.IsValid() || ParticipantSet.Contains(InCollection.Get()))
        { return ck_ui_collection::Failure(TEXT("collection hierarchy contains a duplicate participant")); }
        ParticipantSet.Add(InCollection.Get());
        Participants.Add(InCollection);
        if (InCollection->_bPublishing) { return ck_ui_collection::Failure(TEXT("collection mutation during change notification is rejected")); }

        OutPrepared = MakeShared<FPreparedCollection>();
        OutPrepared->Collection = InCollection;
        OutPrepared->Changed = InRecords.Num() != InCollection->_Records.Num();
        OutPrepared->NextRecords.Reserve(InRecords.Num());
        OutPrepared->NextByKey.Reserve(InRecords.Num());
        for (const FCkUiRecordData& Source : InRecords)
        {
            FRecordUpdate Update;
            Update.Record = InCollection->_ByKey.FindRef(Source.Key);
            Update.Data = Source;
            Update.Changed = !Update.Record.IsValid() || !ck_ui_collection::EqualData(Source, Update.Record->_Data);
            if (!Update.Record.IsValid()) { Update.Record = MakeShareable(new FCkUiRecord(FCkUiRecordData{})); }
            for (const FCkUiChildCollectionSchema& ChildSchema : InCollection->_ChildSchemas)
            {
                const TArray<FCkUiRecordData>& ChildData = Source.Children.FindChecked(ChildSchema.Name);
                TSharedPtr<FCkUiCollection> Child = Update.Record->_Children.FindRef(ChildSchema.Name);
                if (!Child.IsValid())
                {
                    Child = MakeShareable(new FCkUiCollection(FCkUiCollectionSchema{.Fields = ChildSchema.Fields, .Children = ChildSchema.Children}));
                    for (const FCkUiFieldSchema& Field : Child->_Schema) { Child->_SchemaKinds.Add(Field.Name, Field.Kind); }
                }
                TSharedPtr<FPreparedCollection> PreparedChild;
                if (const FCkUiLoadResult ChildResult = Prepare(Child, ChildData, PreparedChild); !ChildResult.Succeeded) { return ChildResult; }
                Update.Children.Add(ChildSchema.Name, Child);
                OutPrepared->Descendants.Add(PreparedChild);
                if (PreparedChild->Changed) { Update.Changed = true; }
            }
            if (Update.Changed) { OutPrepared->Changed = true; }
            OutPrepared->NextByKey.Add(Source.Key, Update.Record);
            OutPrepared->NextRecords.Add(Update.Record);
            OutPrepared->RecordUpdates.Add(MoveTemp(Update));
        }
        return FCkUiLoadResult{.Succeeded = true};
    }

    auto Install(const TSharedPtr<FPreparedCollection>& InPrepared) -> void
    {
        for (FRecordUpdate& Update : InPrepared->RecordUpdates)
        {
            if (Update.Changed)
            {
                Swap(Update.Record->_Data, Update.Data);
                Swap(Update.Record->_Children, Update.Children);
                ++Update.Record->_Revision;
            }
        }
        if (InPrepared->Changed)
        {
            Swap(InPrepared->Collection->_Records, InPrepared->PreviousRecords);
            Swap(InPrepared->Collection->_ByKey, InPrepared->PreviousByKey);
            InPrepared->Collection->_Records = MoveTemp(InPrepared->NextRecords);
            InPrepared->Collection->_ByKey = MoveTemp(InPrepared->NextByKey);
            ++InPrepared->Collection->_Revision;
        }
        for (const TSharedPtr<FPreparedCollection>& Child : InPrepared->Descendants) { Install(Child); }
    }

    auto Broadcast(const TSharedPtr<FPreparedCollection>& InPrepared) -> void
    {
        if (InPrepared->Changed) { InPrepared->Collection->_OnChanged.Broadcast(); }
        for (const TSharedPtr<FPreparedCollection>& Child : InPrepared->Descendants) { Broadcast(Child); }
    }

    auto ReleaseOld(const TSharedPtr<FPreparedCollection>& InPrepared) -> void
    {
        for (const TSharedPtr<FPreparedCollection>& Child : InPrepared->Descendants) { ReleaseOld(Child); }
        InPrepared->PreviousRecords.Reset();
        InPrepared->PreviousByKey.Reset();
        for (FRecordUpdate& Update : InPrepared->RecordUpdates)
        {
            Update.Data = {};
            Update.Children.Reset();
        }
    }

    auto Execute(TArray<FCkUiCollectionUpdate> InUpdates) -> FCkUiLoadResult
    {
        if (InUpdates.IsEmpty()) { return FCkUiLoadResult{.Succeeded = true}; }
        for (const FCkUiCollectionUpdate& Update : InUpdates)
        {
            if (!Update.Collection.IsValid())
            { return ck_ui_collection::Failure(TEXT("collection batch contains a null participant")); }
            const FCkUiLoadResult Validation = ck_ui_collection::ValidateRecords(
                Update.Collection->_Schema, Update.Collection->_ChildSchemas, Update.Records, 0);
            if (!Validation.Succeeded) { return Validation; }
        }
        for (const FCkUiCollectionUpdate& Update : InUpdates)
        {
            TSharedPtr<FPreparedCollection> Root;
            if (const FCkUiLoadResult Result = Prepare(Update.Collection, Update.Records, Root); !Result.Succeeded) { return Result; }
            Roots.Add(Root);
        }
        for (const TSharedPtr<FCkUiCollection>& Collection : Participants) { Collection->_bPublishing = true; }
        for (const TSharedPtr<FPreparedCollection>& Root : Roots) { Install(Root); }
        for (const TSharedPtr<FPreparedCollection>& Root : Roots) { Broadcast(Root); }
        for (const TSharedPtr<FPreparedCollection>& Root : Roots) { ReleaseOld(Root); }
        for (const TSharedPtr<FCkUiCollection>& Collection : Participants) { Collection->_bPublishing = false; }
        return FCkUiLoadResult{.Succeeded = true};
    }
};

auto FCkUiCollection::TrySetRecordsBatch(TArray<FCkUiCollectionUpdate> InUpdates) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_collection::Failure(TEXT("collection mutation must run on the game thread")); }
    return FCkUiCollectionTransaction{}.Execute(MoveTemp(InUpdates));
}
