#pragma once

#include "CoreMinimal.h"
#include "CkSlateLayout/CkUiCollection.h"

struct FCkUiTreeNodeData
{
    FString Key;
    TOptional<FString> ParentKey;
    TMap<FString, FCkUiFieldValue> Fields;
};

class FCkUiTreeCollection;

class CKSLATELAYOUT_API FCkUiTreeNode final
{
public:
    const FString& GetKey() const { return _Data.Key; }
    const TOptional<FString>& GetParentKey() const { return _Data.ParentKey; }
    const FCkUiFieldValue* FindField(const FString& InName) const { return _Data.Fields.Find(InName); }

private:
    explicit FCkUiTreeNode(FCkUiTreeNodeData&& InData) : _Data(MoveTemp(InData)) {}

    FCkUiTreeNodeData _Data;

    friend class FCkUiTreeCollection;
};

/**
 * Game-thread tree model with atomic record and topology publication. Keep a shared node
 * reference rather than a field pointer: fields and parent links can change on publication.
 */
class CKSLATELAYOUT_API FCkUiTreeCollection final : public TSharedFromThis<FCkUiTreeCollection>
{
public:
    static auto TryCreate(TArray<FCkUiFieldSchema> InSchema, TSharedPtr<FCkUiTreeCollection>& OutCollection) -> FCkUiLoadResult;

    auto TrySetNodes(TArray<FCkUiTreeNodeData> InNodes) -> FCkUiLoadResult;
    const TArray<FCkUiFieldSchema>& GetSchema() const { return _Schema; }
    const TArray<TSharedPtr<const FCkUiTreeNode>>& GetNodes() const { return _Nodes; }
    const TArray<TSharedPtr<const FCkUiTreeNode>>& GetRoots() const { return _Roots; }
    TArray<TSharedPtr<const FCkUiTreeNode>> GetChildren(const FString& InKey) const;
    TSharedPtr<const FCkUiTreeNode> FindNode(const FString& InKey) const;
    int64 GetRevision() const { return _Revision; }
    FSimpleMulticastDelegate& OnChanged() { return _OnChanged; }

private:
    explicit FCkUiTreeCollection(TArray<FCkUiFieldSchema>&& InSchema) : _Schema(MoveTemp(InSchema)) {}

    TArray<FCkUiFieldSchema> _Schema;
    TMap<FString, ECkUiFieldKind> _SchemaKinds;
    TArray<TSharedPtr<const FCkUiTreeNode>> _Nodes;
    TArray<TSharedPtr<const FCkUiTreeNode>> _Roots;
    TMap<FString, TArray<TSharedPtr<const FCkUiTreeNode>>> _ChildrenByParent;
    TMap<FString, TSharedPtr<FCkUiTreeNode>> _ByKey;
    int64 _Revision = 0;
    bool _bPublishing = false;
    FSimpleMulticastDelegate _OnChanged;
};
