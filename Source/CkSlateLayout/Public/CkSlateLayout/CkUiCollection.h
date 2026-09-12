#pragma once

#include "CoreMinimal.h"
#include "CkSlateLayout/CkUiDocument.h"

struct FSlateBrush;
class FCkUiFloatSeries;

enum class ECkUiFieldKind : uint8 { Text, Number, Bool, Color, Image, Integer, FloatSeries };

struct FCkUiFieldSchema
{
    FString Name;
    ECkUiFieldKind Kind = ECkUiFieldKind::Text;
    bool Required = true;
};

struct FCkUiChildCollectionSchema
{
    FString Name;
    TArray<FCkUiFieldSchema> Fields;
    TArray<FCkUiChildCollectionSchema> Children;
};

struct FCkUiCollectionSchema
{
    TArray<FCkUiFieldSchema> Fields;
    TArray<FCkUiChildCollectionSchema> Children;
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
    int32 Integer = 0;
    /** Host-owned mutable samples. Collection records retain only a weak handle. */
    TWeakPtr<FCkUiFloatSeries> FloatSeries;
};

struct FCkUiRecordData
{
    FString Key;
    TMap<FString, FCkUiFieldValue> Fields;
    TMap<FString, TArray<FCkUiRecordData>> Children;
};

class FCkUiCollection;
struct FCkUiCollectionTransaction;

struct FCkUiCollectionUpdate
{
    TSharedPtr<FCkUiCollection> Collection;
    TArray<FCkUiRecordData> Records;
};

class CKSLATELAYOUT_API FCkUiRecord final
{
public:
    const FString& GetKey() const { return _Data.Key; }
    int64 GetRevision() const { return _Revision; }
    const FCkUiFieldValue* FindField(const FString& InName) const { return _Data.Fields.Find(InName); }
    TSharedPtr<const FCkUiCollection> FindChildCollection(const FString& InName) const { return _Children.FindRef(InName); }

private:
    explicit FCkUiRecord(FCkUiRecordData&& InData) : _Data(MoveTemp(InData)) {}
    FCkUiRecordData _Data;
    int64 _Revision = 0;
    TMap<FString, TSharedPtr<FCkUiCollection>> _Children;
    friend class FCkUiCollection;
    friend struct FCkUiCollectionTransaction;
};

/** Game-thread model. Schema is immutable; readers must use this model on the game thread.
 * Bind change listeners weakly when owned by widgets. Record/field references can change
 * on successful publication; retain the const shared record, not a pointer into its fields.
 */
class CKSLATELAYOUT_API FCkUiCollection final : public TSharedFromThis<FCkUiCollection>
{
public:
    static auto TryCreate(TArray<FCkUiFieldSchema> InSchema, TSharedPtr<FCkUiCollection>& OutCollection) -> FCkUiLoadResult;
    static auto TryCreateHierarchical(FCkUiCollectionSchema InSchema, TSharedPtr<FCkUiCollection>& OutCollection) -> FCkUiLoadResult;

    auto TrySetRecords(TArray<FCkUiRecordData> InRecords) -> FCkUiLoadResult;
    static auto TrySetRecordsBatch(TArray<FCkUiCollectionUpdate> InUpdates) -> FCkUiLoadResult;
    const TArray<FCkUiFieldSchema>& GetSchema() const { return _Schema; }
    const TArray<FCkUiChildCollectionSchema>& GetChildSchemas() const { return _ChildSchemas; }
    const TArray<TSharedPtr<const FCkUiRecord>>& GetRecords() const { return _Records; }
    TSharedPtr<const FCkUiRecord> FindRecord(const FString& InKey) const;
    int64 GetRevision() const { return _Revision; }
    FSimpleMulticastDelegate& OnChanged() const { return _OnChanged; }

private:
    explicit FCkUiCollection(FCkUiCollectionSchema&& InSchema)
        : _Schema(MoveTemp(InSchema.Fields)), _ChildSchemas(MoveTemp(InSchema.Children)) {}

    TArray<FCkUiFieldSchema> _Schema;
    TArray<FCkUiChildCollectionSchema> _ChildSchemas;
    TMap<FString, ECkUiFieldKind> _SchemaKinds;
    TArray<TSharedPtr<const FCkUiRecord>> _Records;
    TMap<FString, TSharedPtr<FCkUiRecord>> _ByKey;
    int64 _Revision = 0;
    bool _bPublishing = false;
    mutable FSimpleMulticastDelegate _OnChanged;
    friend struct FCkUiCollectionTransaction;
};
