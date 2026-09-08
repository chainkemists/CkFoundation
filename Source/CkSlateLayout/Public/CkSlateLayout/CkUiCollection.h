#pragma once

#include "CoreMinimal.h"
#include "CkSlateLayout/CkUiDocument.h"

struct FSlateBrush;

enum class ECkUiFieldKind : uint8 { Text, Number, Bool, Color, Image };

struct FCkUiFieldSchema
{
    FString Name;
    ECkUiFieldKind Kind = ECkUiFieldKind::Text;
    bool Required = true;
};

struct FCkUiFieldValue
{
    ECkUiFieldKind Kind = ECkUiFieldKind::Text;
    FText Text;
    float Number = 0.0f;
    bool Bool = false;
    FLinearColor Color = FLinearColor::White;
    // The shared brush lifetime is retained; any UObject used by its resource remains consumer-owned.
    TSharedPtr<const FSlateBrush> Image;
};

struct FCkUiRecordData
{
    FString Key;
    TMap<FString, FCkUiFieldValue> Fields;
};

class FCkUiCollection;

class CKSLATELAYOUT_API FCkUiRecord final
{
public:
    const FString& GetKey() const { return _Data.Key; }
    const FCkUiFieldValue* FindField(const FString& InName) const { return _Data.Fields.Find(InName); }

private:
    explicit FCkUiRecord(FCkUiRecordData&& InData) : _Data(MoveTemp(InData)) {}
    FCkUiRecordData _Data;
    friend class FCkUiCollection;
};

/** Game-thread model. Schema is immutable; readers must use this model on the game thread.
 * Bind change listeners weakly when owned by widgets. Record/field references can change
 * on successful publication; retain the const shared record, not a pointer into its fields.
 */
class CKSLATELAYOUT_API FCkUiCollection final : public TSharedFromThis<FCkUiCollection>
{
public:
    static auto TryCreate(TArray<FCkUiFieldSchema> InSchema, TSharedPtr<FCkUiCollection>& OutCollection) -> FCkUiLoadResult;

    auto TrySetRecords(TArray<FCkUiRecordData> InRecords) -> FCkUiLoadResult;
    const TArray<FCkUiFieldSchema>& GetSchema() const { return _Schema; }
    const TArray<TSharedPtr<const FCkUiRecord>>& GetRecords() const { return _Records; }
    TSharedPtr<const FCkUiRecord> FindRecord(const FString& InKey) const;
    int64 GetRevision() const { return _Revision; }
    FSimpleMulticastDelegate& OnChanged() { return _OnChanged; }

private:
    explicit FCkUiCollection(TArray<FCkUiFieldSchema>&& InSchema) : _Schema(MoveTemp(InSchema)) {}

    TArray<FCkUiFieldSchema> _Schema;
    TMap<FString, ECkUiFieldKind> _SchemaKinds;
    TArray<TSharedPtr<const FCkUiRecord>> _Records;
    TMap<FString, TSharedPtr<FCkUiRecord>> _ByKey;
    int64 _Revision = 0;
    bool _bPublishing = false;
    FSimpleMulticastDelegate _OnChanged;
};
