#include "CkSlateLayout/CkUiTreeCollection.h"

namespace ck_ui_tree_collection
{
    constexpr int32 MaxSchemaFields = 128;
    constexpr int32 MaxNodes = 100000;
    constexpr int32 MaxSuppliedFields = 1000000;
    constexpr int32 MaxKeyLength = 1024;
    constexpr int32 MaxDepth = 256;

    auto Failure(const FString& InError) -> FCkUiLoadResult { return FCkUiLoadResult{.Succeeded = false, .Errors = {InError}}; }

    auto IsName(const FString& InValue) -> bool
    {
        if (InValue.IsEmpty() || !(FChar::IsAlpha(InValue[0]) || InValue[0] == TEXT('_') || InValue[0] == TEXT('-'))) { return false; }
        for (const TCHAR Character : InValue) if (!(FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-'))) { return false; }
        return true;
    }

    auto IsValidKind(const ECkUiFieldKind InKind) -> bool
    {
        return InKind == ECkUiFieldKind::Text || InKind == ECkUiFieldKind::Number || InKind == ECkUiFieldKind::Bool
            || InKind == ECkUiFieldKind::Color || InKind == ECkUiFieldKind::Image;
    }

    auto IsValidValue(const FCkUiFieldValue& InValue) -> bool
    {
        if (!IsValidKind(InValue.Kind)) { return false; }
        if (InValue.Kind == ECkUiFieldKind::Number) { return FMath::IsFinite(InValue.Number); }
        if (InValue.Kind == ECkUiFieldKind::Color)
        {
            return FMath::IsFinite(InValue.Color.R) && FMath::IsFinite(InValue.Color.G)
                && FMath::IsFinite(InValue.Color.B) && FMath::IsFinite(InValue.Color.A);
        }
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

    auto EqualData(const FCkUiTreeNodeData& A, const FCkUiTreeNodeData& B) -> bool
    {
        if (A.Key != B.Key || A.ParentKey != B.ParentKey || A.Fields.Num() != B.Fields.Num()) { return false; }
        for (const TPair<FString, FCkUiFieldValue>& Pair : A.Fields)
        {
            const FCkUiFieldValue* Other = B.Fields.Find(Pair.Key);
            if (Other == nullptr || !EqualValue(Pair.Value, *Other)) { return false; }
        }
        return true;
    }

    auto ValidateTopology(const TArray<FCkUiTreeNodeData>& InNodes, const TMap<FString, int32>& InIndices) -> FCkUiLoadResult
    {
        auto Parents = TArray<int32>{};
        Parents.Reserve(InNodes.Num());
        for (int32 Index = 0; Index < InNodes.Num(); ++Index)
        {
            const TOptional<FString>& ParentKey = InNodes[Index].ParentKey;
            if (!ParentKey.IsSet()) { Parents.Add(INDEX_NONE); continue; }
            const int32* ParentIndex = InIndices.Find(ParentKey.GetValue());
            if (ParentIndex == nullptr) { return Failure(TEXT("tree node parent does not exist")); }
            if (*ParentIndex == Index) { return Failure(TEXT("tree node cannot parent itself")); }
            Parents.Add(*ParentIndex);
        }

        auto States = TArray<uint8>{};
        auto Depths = TArray<int32>{};
        States.Init(0, InNodes.Num());
        Depths.Init(0, InNodes.Num());
        for (int32 Start = 0; Start < InNodes.Num(); ++Start)
        {
            if (States[Start] == 2) { continue; }
            auto Path = TArray<int32>{};
            int32 Current = Start;
            while (Current != INDEX_NONE && States[Current] == 0)
            {
                States[Current] = 1;
                Path.Add(Current);
                Current = Parents[Current];
            }
            if (Current != INDEX_NONE && States[Current] == 1) { return Failure(TEXT("tree node hierarchy contains a cycle")); }

            int32 Depth = Current == INDEX_NONE ? 0 : Depths[Current];
            for (int32 PathIndex = Path.Num() - 1; PathIndex >= 0; --PathIndex)
            {
                ++Depth;
                if (Depth > MaxDepth) { return Failure(TEXT("tree node depth limit exceeded")); }
                const int32 NodeIndex = Path[PathIndex];
                Depths[NodeIndex] = Depth;
                States[NodeIndex] = 2;
            }
        }
        return FCkUiLoadResult{.Succeeded = true};
    }
}

auto FCkUiTreeCollection::TryCreate(TArray<FCkUiFieldSchema> InSchema, TSharedPtr<FCkUiTreeCollection>& OutCollection) -> FCkUiLoadResult
{
    OutCollection.Reset();
    if (!IsInGameThread()) { return ck_ui_tree_collection::Failure(TEXT("tree collection creation must run on the game thread")); }
    if (InSchema.IsEmpty() || InSchema.Num() > ck_ui_tree_collection::MaxSchemaFields)
    {
        return ck_ui_tree_collection::Failure(TEXT("tree collection schema must contain 1..128 fields"));
    }

    auto Names = TSet<FString>{};
    for (const FCkUiFieldSchema& Field : InSchema)
    {
        if (!ck_ui_tree_collection::IsName(Field.Name) || !ck_ui_tree_collection::IsValidKind(Field.Kind) || Names.Contains(Field.Name))
        {
            return ck_ui_tree_collection::Failure(TEXT("tree collection schema contains an invalid or duplicate field"));
        }
        Names.Add(Field.Name);
    }

    TSharedPtr<FCkUiTreeCollection> Candidate = MakeShareable(new FCkUiTreeCollection(MoveTemp(InSchema)));
    for (const FCkUiFieldSchema& Field : Candidate->_Schema) { Candidate->_SchemaKinds.Add(Field.Name, Field.Kind); }
    OutCollection = MoveTemp(Candidate);
    return FCkUiLoadResult{.Succeeded = true};
}

auto FCkUiTreeCollection::GetChildren(const FString& InKey) const -> TArray<TSharedPtr<const FCkUiTreeNode>>
{
    if (const TArray<TSharedPtr<const FCkUiTreeNode>>* Found = _ChildrenByParent.Find(InKey)) { return *Found; }
    return {};
}

auto FCkUiTreeCollection::FindNode(const FString& InKey) const -> TSharedPtr<const FCkUiTreeNode>
{
    if (const TSharedPtr<FCkUiTreeNode>* Found = _ByKey.Find(InKey)) { return *Found; }
    return {};
}

auto FCkUiTreeCollection::TrySetNodes(TArray<FCkUiTreeNodeData> InNodes) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_tree_collection::Failure(TEXT("tree collection mutation must run on the game thread")); }
    if (_bPublishing) { return ck_ui_tree_collection::Failure(TEXT("tree collection mutation during change notification is rejected")); }
    if (InNodes.Num() > ck_ui_tree_collection::MaxNodes) { return ck_ui_tree_collection::Failure(TEXT("tree collection node limit exceeded")); }

    auto Indices = TMap<FString, int32>{};
    Indices.Reserve(InNodes.Num());
    int32 FieldCount = 0;
    for (int32 Index = 0; Index < InNodes.Num(); ++Index)
    {
        const FCkUiTreeNodeData& Data = InNodes[Index];
        if (Data.Key.IsEmpty() || Data.Key.Len() > ck_ui_tree_collection::MaxKeyLength || Indices.Contains(Data.Key))
        {
            return ck_ui_tree_collection::Failure(TEXT("tree nodes require unique nonempty keys"));
        }
        Indices.Add(Data.Key, Index);
        if (Data.ParentKey.IsSet() && (Data.ParentKey.GetValue().IsEmpty() || Data.ParentKey.GetValue().Len() > ck_ui_tree_collection::MaxKeyLength))
        {
            return ck_ui_tree_collection::Failure(TEXT("tree node parent key is invalid"));
        }
        if (Data.Fields.Num() > ck_ui_tree_collection::MaxSuppliedFields - FieldCount)
        {
            return ck_ui_tree_collection::Failure(TEXT("tree collection supplied field limit exceeded"));
        }
        FieldCount += Data.Fields.Num();
        for (const TPair<FString, FCkUiFieldValue>& Pair : Data.Fields)
        {
            const ECkUiFieldKind* Expected = _SchemaKinds.Find(Pair.Key);
            if (Expected == nullptr || Pair.Value.Kind != *Expected || !ck_ui_tree_collection::IsValidValue(Pair.Value))
            {
                return ck_ui_tree_collection::Failure(TEXT("tree node has an unknown, mismatched, or invalid field"));
            }
        }
        for (const FCkUiFieldSchema& Field : _Schema)
        {
            if (Field.Required && !Data.Fields.Contains(Field.Name)) { return ck_ui_tree_collection::Failure(TEXT("tree node is missing a required field")); }
        }
    }

    const FCkUiLoadResult Topology = ck_ui_tree_collection::ValidateTopology(InNodes, Indices);
    if (!Topology.Succeeded) { return Topology; }

    bool bSame = InNodes.Num() == _Nodes.Num();
    if (bSame)
    {
        for (int32 Index = 0; Index < InNodes.Num(); ++Index)
        {
            if (!ck_ui_tree_collection::EqualData(InNodes[Index], _Nodes[Index]->_Data)) { bSame = false; break; }
        }
    }
    if (bSame) { return FCkUiLoadResult{.Succeeded = true}; }

    const TSharedRef<FCkUiTreeCollection> KeepAlive = AsShared();
    TGuardValue<bool> PublishingGuard(_bPublishing, true);
    struct FPreparedNode
    {
        TSharedPtr<FCkUiTreeNode> Node;
        FCkUiTreeNodeData Data;
    };

    auto ParentKeys = TArray<TOptional<FString>>{};
    ParentKeys.Reserve(InNodes.Num());
    for (const FCkUiTreeNodeData& Data : InNodes) { ParentKeys.Add(Data.ParentKey); }
    auto Updates = TArray<FPreparedNode>{};
    auto NextNodes = TArray<TSharedPtr<const FCkUiTreeNode>>{};
    NextNodes.Reserve(InNodes.Num());
    auto NextByKey = TMap<FString, TSharedPtr<FCkUiTreeNode>>{};
    NextByKey.Reserve(InNodes.Num());
    for (FCkUiTreeNodeData& Data : InNodes)
    {
        TSharedPtr<FCkUiTreeNode> Node = _ByKey.FindRef(Data.Key);
        if (Node.IsValid())
        {
            if (!ck_ui_tree_collection::EqualData(Data, Node->_Data)) { Updates.Add({Node, MoveTemp(Data)}); }
        }
        else { Node = MakeShareable(new FCkUiTreeNode(MoveTemp(Data))); }
        NextByKey.Add(Node->GetKey(), Node);
        NextNodes.Add(Node);
    }

    auto NextRoots = TArray<TSharedPtr<const FCkUiTreeNode>>{};
    auto NextChildren = TMap<FString, TArray<TSharedPtr<const FCkUiTreeNode>>>{};
    for (int32 Index = 0; Index < NextNodes.Num(); ++Index)
    {
        if (!ParentKeys[Index].IsSet()) { NextRoots.Add(NextNodes[Index]); }
        else { NextChildren.FindOrAdd(ParentKeys[Index].GetValue()).Add(NextNodes[Index]); }
    }

    // Keep removed nodes, old arrays, and old values alive through observer callbacks.
    auto PreviousNodes = MoveTemp(_Nodes);
    auto PreviousRoots = MoveTemp(_Roots);
    auto PreviousChildren = MoveTemp(_ChildrenByParent);
    auto PreviousByKey = MoveTemp(_ByKey);
    for (FPreparedNode& Update : Updates) { Swap(Update.Node->_Data, Update.Data); }
    _Nodes = MoveTemp(NextNodes);
    _Roots = MoveTemp(NextRoots);
    _ChildrenByParent = MoveTemp(NextChildren);
    _ByKey = MoveTemp(NextByKey);
    ++_Revision;
    _OnChanged.Broadcast();
    return FCkUiLoadResult{.Succeeded = true};
}
