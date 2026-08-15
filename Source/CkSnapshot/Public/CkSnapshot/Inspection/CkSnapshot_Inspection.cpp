#include "CkSnapshot/Inspection/CkSnapshot_Inspection.h"

#include "CkSnapshot/Inspection/CkSnapshot_Inspection_Sha256.h"
#include "CkSnapshot/SaveGame/CkSnapshot_SaveGame.h"
#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"

#include "Containers/Set.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Templates/Function.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

#include <StructUtils/InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_inspection
    {
        // Mirrors ck_snapshot_subsystem::UserIndex — the slot index every CK save is written under.
        constexpr auto k_UserIndex = 0;

        // FSaveGameHeader::FileTypeTag, written first as an int32 by every engine save envelope. The engine's own
        // UE_SAVEGAME_FILE_TYPE_TAG lives in Runtime/Engine/Private/GameplayStatics.cpp as a file-local static and
        // is exported by no public header, so it is named here rather than left bare at the comparison site.
        constexpr auto k_SaveGameFileTypeTag = 0x53415647;

        // ------------------------------------------------------------------------------------------------------------

        auto
            Get_HasSaveGameEnvelopeTag(
                const TArray<uint8>& InBytes)
            -> bool
        {
            if (InBytes.Num() < 4)
            { return false; }

            const auto Tag = static_cast<int32>(
                  static_cast<uint32>(InBytes[0])
                | (static_cast<uint32>(InBytes[1]) << 8)
                | (static_cast<uint32>(InBytes[2]) << 16)
                | (static_cast<uint32>(InBytes[3]) << 24));

            return Tag == k_SaveGameFileTypeTag;
        }

        // ------------------------------------------------------------------------------------------------------------

        auto
            DoAdd_Diagnostic(
                FCk_SnapshotInspection_Document& InOutDocument,
                ECk_SnapshotInspection_Severity InSeverity,
                ECk_SnapshotInspection_DiagnosticCode InCode,
                FString InMessage,
                TArray<uint32> InRelatedSavedIds = {},
                TArray<int32> InRelatedPayloadIndices = {})
            -> void
        {
            auto Diagnostic = FCk_SnapshotInspection_Diagnostic{};
            Diagnostic.Set_Severity(InSeverity)
                      .Set_Code(InCode)
                      .Set_Message(MoveTemp(InMessage))
                      .Set_RelatedSavedIds(MoveTemp(InRelatedSavedIds))
                      .Set_RelatedPayloadIndices(MoveTemp(InRelatedPayloadIndices));

            InOutDocument.Get_Diagnostics().Add(MoveTemp(Diagnostic));
        }

        // ------------------------------------------------------------------------------------------------------------

        auto
            Get_IsHandleStruct(
                const UScriptStruct* InStruct)
            -> bool
        {
            return InStruct != nullptr && InStruct->IsChildOf(FCk_Handle::StaticStruct());
        }

        auto
            DoClamp_Text(
                FString InText)
            -> FString
        {
            if (InText.Len() <= inspection_limits::MaxTextLen)
            { return InText; }

            return InText.Left(inspection_limits::MaxTextLen) + TEXT(" ...(text truncated)");
        }

        // ------------------------------------------------------------------------------------------------------------
        // Value tree projection. A blob is untrusted input, so every recursion is bounded and every cut is announced
        // by an explicit Truncated node rather than silently shortening the tree.

        struct FCk_ValueTreeBudget
        {
            int32 _NodesUsed = 0;

            auto TryConsume() -> bool
            {
                if (_NodesUsed >= inspection_limits::MaxTotalNodes)
                { return false; }

                ++_NodesUsed;
                return true;
            }
        };

        using ValueNodePtr = TSharedPtr<FCk_SnapshotInspection_ValueNode>;

        auto
            DoMake_Node(
                ECk_SnapshotInspection_ValueKind InKind,
                const FString& InFieldName)
            -> ValueNodePtr
        {
            auto Node = MakeShared<FCk_SnapshotInspection_ValueNode>();
            Node->Set_Kind(InKind).Set_FieldName(InFieldName);
            return Node;
        }

        auto
            DoMake_TruncatedNode(
                const FString& InReason)
            -> ValueNodePtr
        {
            auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Truncated, TEXT("..."));
            Node->Set_ValueText(InReason);
            return Node;
        }

        auto
            DoProject_Property(
                const FProperty* InProperty,
                const void* InValuePtr,
                const FString& InFieldName,
                int32 InDepth,
                FCk_ValueTreeBudget& InOutBudget)
            -> ValueNodePtr;

        auto
            DoProject_StructFields(
                const UScriptStruct* InStruct,
                const void* InMemory,
                int32 InDepth,
                FCk_ValueTreeBudget& InOutBudget,
                FCk_SnapshotInspection_ValueNode& InOutParent)
            -> void
        {
            if (InStruct == nullptr || InMemory == nullptr)
            { return; }

            if (InDepth >= inspection_limits::MaxDepth)
            {
                InOutParent.Get_Children().Add(DoMake_TruncatedNode(TEXT("max depth reached")));
                return;
            }

            for (TFieldIterator<FProperty> PropertyIt(InStruct); PropertyIt; ++PropertyIt)
            {
                const auto* Property = *PropertyIt;
                auto Child = DoProject_Property(Property, Property->ContainerPtrToValuePtr<void>(InMemory),
                    Property->GetName(), InDepth + 1, InOutBudget);

                if (NOT Child.IsValid())
                {
                    InOutParent.Get_Children().Add(DoMake_TruncatedNode(TEXT("node budget exhausted")));
                    break;
                }

                InOutParent.Get_Children().Add(Child);
            }
        }

        auto
            DoProject_CollectionElements(
                const FProperty* InElementProperty,
                const TArray<const void*>& InElementPtrs,
                int32 InDepth,
                FCk_ValueTreeBudget& InOutBudget,
                FCk_SnapshotInspection_ValueNode& InOutParent)
            -> void
        {
            if (InDepth >= inspection_limits::MaxDepth)
            {
                InOutParent.Get_Children().Add(DoMake_TruncatedNode(TEXT("max depth reached")));
                return;
            }

            for (auto Index = 0; Index < InElementPtrs.Num(); ++Index)
            {
                if (Index >= inspection_limits::MaxChildrenPerCollection)
                {
                    InOutParent.Get_Children().Add(DoMake_TruncatedNode(
                        ck::Format_UE(TEXT("[{}] more element(s) not shown"), InElementPtrs.Num() - Index)));
                    break;
                }

                auto Child = DoProject_Property(InElementProperty, InElementPtrs[Index],
                    ck::Format_UE(TEXT("[{}]"), Index), InDepth + 1, InOutBudget);

                if (NOT Child.IsValid())
                {
                    InOutParent.Get_Children().Add(DoMake_TruncatedNode(TEXT("node budget exhausted")));
                    break;
                }

                InOutParent.Get_Children().Add(Child);
            }
        }

        auto
            DoMake_SavedEntityRefNode(
                const FProperty* InProperty,
                const FStructProperty* InStructProperty,
                const void* InValuePtr,
                const FString& InFieldName)
            -> ValueNodePtr
        {
            auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::SavedEntityRef, InFieldName);
            Node->Set_TypeName(InStructProperty->Struct->GetName());

            // The save's handle walk skips CPF_Transient fields on BOTH sides, so a transient handle field carries
            // no saved id at all — reporting the freshly constructed in-place value as a saved reference would be a
            // lie about what the file contains.
            if (InProperty->HasAnyPropertyFlags(CPF_Transient))
            {
                Node->Set_ValueText(TEXT("transient - not persisted"));
                return Node;
            }

            const auto& Handle = *static_cast<const FCk_Handle*>(InValuePtr);
            const auto& Entity = Handle.Get_Entity();
            const auto  SavedId = static_cast<uint32>(Entity.Get_ID());

            Node->Set_SavedId(SavedId);

            if (SavedId == k_NoSavedEntity)
            {
                Node->Set_ValueText(TEXT("SavedEntityRef: none"));
                return Node;
            }

            Node->Set_ValueText(ck::Format_UE(TEXT("SavedEntityRef: {} (id {} | v{})"),
                SavedId,
                static_cast<int32>(Entity.Get_EntityNumber()),
                static_cast<int32>(Entity.Get_VersionNumber())));

            return Node;
        }

        auto
            DoProject_Property(
                const FProperty* InProperty,
                const void* InValuePtr,
                const FString& InFieldName,
                int32 InDepth,
                FCk_ValueTreeBudget& InOutBudget)
            -> ValueNodePtr
        {
            if (InProperty == nullptr || InValuePtr == nullptr)
            { return {}; }

            if (NOT InOutBudget.TryConsume())
            { return {}; }

            if (const auto* BoolProperty = CastField<FBoolProperty>(InProperty))
            {
                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Bool, InFieldName);
                Node->Set_TypeName(TEXT("bool"))
                     .Set_ValueText(BoolProperty->GetPropertyValue(InValuePtr) ? TEXT("true") : TEXT("false"));
                return Node;
            }

            if (const auto* EnumProperty = CastField<FEnumProperty>(InProperty))
            {
                const auto* Enum = EnumProperty->GetEnum();
                const auto Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(InValuePtr);

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Enum, InFieldName);
                Node->Set_TypeName(Enum != nullptr ? Enum->GetName() : FString{TEXT("enum")})
                     .Set_EnumUnderlyingValue(Value)
                     .Set_ValueText(Enum != nullptr ? Enum->GetNameStringByValue(Value) : LexToString(Value));
                return Node;
            }

            if (const auto* ByteProperty = CastField<FByteProperty>(InProperty);
                ByteProperty != nullptr && ByteProperty->Enum != nullptr)
            {
                const auto Value = ByteProperty->GetSignedIntPropertyValue(InValuePtr);

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Enum, InFieldName);
                Node->Set_TypeName(ByteProperty->Enum->GetName())
                     .Set_EnumUnderlyingValue(Value)
                     .Set_ValueText(ByteProperty->Enum->GetNameStringByValue(Value));
                return Node;
            }

            if (const auto* NumericProperty = CastField<FNumericProperty>(InProperty))
            {
                const auto IsUnsigned = InProperty->IsA<FByteProperty>()
                    || InProperty->IsA<FUInt16Property>()
                    || InProperty->IsA<FUInt32Property>()
                    || InProperty->IsA<FUInt64Property>();

                const auto Kind = NumericProperty->IsFloatingPoint()
                    ? ECk_SnapshotInspection_ValueKind::Float
                    : (IsUnsigned ? ECk_SnapshotInspection_ValueKind::UInt : ECk_SnapshotInspection_ValueKind::Int);

                auto Node = DoMake_Node(Kind, InFieldName);
                Node->Set_TypeName(NumericProperty->GetCPPType())
                     .Set_ValueText(NumericProperty->GetNumericPropertyValueToString(InValuePtr));
                return Node;
            }

            if (const auto* StringProperty = CastField<FStrProperty>(InProperty))
            {
                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::String, InFieldName);
                Node->Set_TypeName(TEXT("FString"))
                     .Set_ValueText(DoClamp_Text(StringProperty->GetPropertyValue(InValuePtr)));
                return Node;
            }

            if (const auto* NameProperty = CastField<FNameProperty>(InProperty))
            {
                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Name, InFieldName);
                Node->Set_TypeName(TEXT("FName"))
                     .Set_ValueText(DoClamp_Text(NameProperty->GetPropertyValue(InValuePtr).ToString()));
                return Node;
            }

            if (const auto* TextProperty = CastField<FTextProperty>(InProperty))
            {
                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Text, InFieldName);
                Node->Set_TypeName(TEXT("FText"))
                     .Set_ValueText(DoClamp_Text(TextProperty->GetPropertyValue(InValuePtr).ToString()));
                return Node;
            }

            if (const auto* SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty))
            {
                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::SoftObjectPath, InFieldName);
                Node->Set_TypeName(SoftObjectProperty->GetCPPType(nullptr, 0))
                     .Set_ValueText(DoClamp_Text(
                         SoftObjectProperty->GetPropertyValue(InValuePtr).ToSoftObjectPath().ToString()));
                return Node;
            }

            if (const auto* ObjectProperty = CastField<FObjectPropertyBase>(InProperty))
            {
                const auto* Object = ObjectProperty->GetObjectPropertyValue(InValuePtr);

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::ObjectPath, InFieldName);
                Node->Set_TypeName(ObjectProperty->GetCPPType())
                     .Set_ValueText(Object != nullptr ? DoClamp_Text(Object->GetPathName()) : FString{TEXT("None")});
                return Node;
            }

            if (const auto* StructProperty = CastField<FStructProperty>(InProperty))
            {
                // An FInstancedStruct is opaque to TFieldIterator — its payload hides behind GetScriptStruct().
                // This test stays ahead of the handle test, exactly as the save-side walk orders them.
                if (StructProperty->Struct == FInstancedStruct::StaticStruct())
                {
                    const auto& Instanced = *static_cast<const FInstancedStruct*>(InValuePtr);

                    auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Struct, InFieldName);
                    Node->Set_TypeName(Instanced.IsValid()
                        ? Instanced.GetScriptStruct()->GetPathName()
                        : FString{TEXT("FInstancedStruct")});

                    if (Instanced.IsValid())
                    { DoProject_StructFields(Instanced.GetScriptStruct(), Instanced.GetMemory(), InDepth, InOutBudget, *Node); }
                    else
                    { Node->Set_ValueText(TEXT("empty")); }

                    return Node;
                }

                if (Get_IsHandleStruct(StructProperty->Struct))
                { return DoMake_SavedEntityRefNode(InProperty, StructProperty, InValuePtr, InFieldName); }

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Struct, InFieldName);
                Node->Set_TypeName(StructProperty->Struct->GetName());
                DoProject_StructFields(StructProperty->Struct, InValuePtr, InDepth, InOutBudget, *Node);
                return Node;
            }

            if (const auto* ArrayProperty = CastField<FArrayProperty>(InProperty))
            {
                auto Helper = FScriptArrayHelper{ArrayProperty, InValuePtr};

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Array, InFieldName);
                Node->Set_TypeName(ArrayProperty->GetCPPType(nullptr, 0))
                     .Set_ValueText(ck::Format_UE(TEXT("[{}] element(s)"), Helper.Num()));

                auto ElementPtrs = TArray<const void*>{};
                ElementPtrs.Reserve(Helper.Num());
                for (auto Index = 0; Index < Helper.Num(); ++Index)
                { ElementPtrs.Add(Helper.GetRawPtr(Index)); }

                DoProject_CollectionElements(ArrayProperty->Inner, ElementPtrs, InDepth, InOutBudget, *Node);
                return Node;
            }

            if (const auto* SetProperty = CastField<FSetProperty>(InProperty))
            {
                auto Helper = FScriptSetHelper{SetProperty, InValuePtr};

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Set, InFieldName);
                Node->Set_TypeName(SetProperty->GetCPPType(nullptr, 0))
                     .Set_ValueText(ck::Format_UE(TEXT("[{}] element(s)"), Helper.Num()));

                auto ElementPtrs = TArray<const void*>{};
                ElementPtrs.Reserve(Helper.Num());
                for (auto It = Helper.CreateIterator(); It; ++It)
                { ElementPtrs.Add(Helper.GetElementPtr(It)); }

                DoProject_CollectionElements(SetProperty->ElementProp, ElementPtrs, InDepth, InOutBudget, *Node);
                return Node;
            }

            if (const auto* MapProperty = CastField<FMapProperty>(InProperty))
            {
                auto Helper = FScriptMapHelper{MapProperty, InValuePtr};

                auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Map, InFieldName);
                Node->Set_TypeName(MapProperty->GetCPPType(nullptr, 0))
                     .Set_ValueText(ck::Format_UE(TEXT("[{}] pair(s)"), Helper.Num()));

                if (InDepth >= inspection_limits::MaxDepth)
                {
                    Node->Get_Children().Add(DoMake_TruncatedNode(TEXT("max depth reached")));
                    return Node;
                }

                auto PairIndex = 0;
                for (auto It = Helper.CreateIterator(); It; ++It, ++PairIndex)
                {
                    if (PairIndex >= inspection_limits::MaxChildrenPerCollection)
                    {
                        Node->Get_Children().Add(DoMake_TruncatedNode(
                            ck::Format_UE(TEXT("[{}] more pair(s) not shown"), Helper.Num() - PairIndex)));
                        break;
                    }

                    if (NOT InOutBudget.TryConsume())
                    {
                        Node->Get_Children().Add(DoMake_TruncatedNode(TEXT("node budget exhausted")));
                        break;
                    }

                    auto PairNode = DoMake_Node(ECk_SnapshotInspection_ValueKind::Struct,
                        ck::Format_UE(TEXT("[{}]"), PairIndex));

                    auto KeyNode = DoProject_Property(MapProperty->KeyProp, Helper.GetKeyPtr(It),
                        TEXT("Key"), InDepth + 1, InOutBudget);
                    auto ValueNode = DoProject_Property(MapProperty->ValueProp, Helper.GetValuePtr(It),
                        TEXT("Value"), InDepth + 1, InOutBudget);

                    if (NOT KeyNode.IsValid() || NOT ValueNode.IsValid())
                    {
                        Node->Get_Children().Add(DoMake_TruncatedNode(TEXT("node budget exhausted")));
                        break;
                    }

                    PairNode->Get_Children().Add(KeyNode);
                    PairNode->Get_Children().Add(ValueNode);
                    Node->Get_Children().Add(PairNode);
                }

                return Node;
            }

            auto Node = DoMake_Node(ECk_SnapshotInspection_ValueKind::Unsupported, InFieldName);
            Node->Set_TypeName(InProperty->GetCPPType());

            auto Exported = FString{};
            InProperty->ExportTextItem_Direct(Exported, InValuePtr, nullptr, nullptr, PPF_None);
            Node->Set_ValueText(DoClamp_Text(Exported));

            return Node;
        }

        auto
            DoProject_ValueTree(
                const FInstancedStruct& InDecoded)
            -> ValueNodePtr
        {
            auto Budget = FCk_ValueTreeBudget{};
            auto Root = DoMake_Node(ECk_SnapshotInspection_ValueKind::Struct, InDecoded.GetScriptStruct()->GetName());
            Root->Set_TypeName(InDecoded.GetScriptStruct()->GetPathName());

            DoProject_StructFields(InDecoded.GetScriptStruct(), InDecoded.GetMemory(), 0, Budget, *Root);
            return Root;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Duplicate detection. Groups are reported at the FIRST row carrying the key, so the diagnostic list is a
        // deterministic function of table order.

        struct FCk_DuplicateGroup
        {
            FString        _Key;
            TArray<uint32> _SavedIds;
        };

        auto
            DoCollect_DuplicateGroups(
                const TArray<FCk_SnapshotInspection_EntitySummary>& InEntities,
                const TFunctionRef<bool(const FCk_Snapshot_V3_EntityEntry&, FString&)>& InTryMakeKey)
            -> TArray<FCk_DuplicateGroup>
        {
            auto Groups = TMap<FString, FCk_DuplicateGroup>{};
            auto KeyOrder = TArray<FString>{};

            for (const auto& Summary : InEntities)
            {
                auto Key = FString{};
                if (NOT InTryMakeKey(Summary.Get_Entry(), Key))
                { continue; }

                if (auto* Existing = Groups.Find(Key))
                {
                    Existing->_SavedIds.Add(Summary.Get_Entry().Get_SavedId());
                    continue;
                }

                auto Group = FCk_DuplicateGroup{};
                Group._Key = Key;
                Group._SavedIds.Add(Summary.Get_Entry().Get_SavedId());

                Groups.Add(Key, MoveTemp(Group));
                KeyOrder.Add(Key);
            }

            auto Result = TArray<FCk_DuplicateGroup>{};
            for (const auto& Key : KeyOrder)
            {
                const auto& Group = Groups[Key];
                if (Group._SavedIds.Num() > 1)
                { Result.Add(Group); }
            }

            return Result;
        }

        // ------------------------------------------------------------------------------------------------------------

        auto
            DoAnalyze_HeaderCensus(
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            const auto& Header = InOutDocument.Get_Header();
            const auto& Census = InOutDocument.Get_Census();

            const auto Matches =
                   Header.Get_EntityCount()           == Census.Get_EntityCount()
                && Header.Get_EngineOwnedCount()      == Census.Get_EngineOwnedCount()
                && Header.Get_ConstructSpawnedCount() == Census.Get_ConstructSpawnedCount()
                && Header.Get_RuntimeSpawnedCount()   == Census.Get_RuntimeSpawnedCount()
                && Header.Get_DefinitionBuiltCount()  == Census.Get_DefinitionBuiltCount()
                && Header.Get_PayloadCount()          == Census.Get_PayloadCount();

            if (Matches)
            { return; }

            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                ECk_SnapshotInspection_DiagnosticCode::HeaderCensusMismatch,
                ck::Format_UE(TEXT("Header census disagrees with the tables. Header: entities [{}] = EngineOwned [{}] "
                    "+ ConstructSpawned [{}] + RuntimeSpawned [{}] + DefinitionBuilt [{}], payloads [{}]. Tables: "
                    "entities [{}] = EngineOwned [{}] + ConstructSpawned [{}] + RuntimeSpawned [{}] + "
                    "DefinitionBuilt [{}], payloads [{}]"),
                    Header.Get_EntityCount(), Header.Get_EngineOwnedCount(), Header.Get_ConstructSpawnedCount(),
                    Header.Get_RuntimeSpawnedCount(), Header.Get_DefinitionBuiltCount(), Header.Get_PayloadCount(),
                    Census.Get_EntityCount(), Census.Get_EngineOwnedCount(), Census.Get_ConstructSpawnedCount(),
                    Census.Get_RuntimeSpawnedCount(), Census.Get_DefinitionBuiltCount(), Census.Get_PayloadCount()));
        }

        auto
            DoAnalyze_OwnerCycles(
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            const auto& Entities = InOutDocument.Get_Entities();
            const auto Count = Entities.Num();

            auto Successor = TArray<int32>{};
            Successor.Reserve(Count);
            for (const auto& Summary : Entities)
            {
                const auto OwnerId = Summary.Get_Entry().Get_LifetimeOwnerSavedId();
                Successor.Add(OwnerId == k_NoSavedEntity
                    ? INDEX_NONE
                    : InOutDocument.TryGet_EntityIndexForSavedId(OwnerId));
            }

            // 0 = unvisited, 1 = on the current walk, 2 = settled. Single-successor graph, so one linear sweep
            // classifies every row and a revisit of an on-walk node IS the cycle.
            auto States = TArray<uint8>{};
            States.SetNumZeroed(Count);

            for (auto Start = 0; Start < Count; ++Start)
            {
                if (States[Start] != 0)
                { continue; }

                auto Path = TArray<int32>{};
                auto Current = Start;

                while (Current != INDEX_NONE && States[Current] == 0)
                {
                    States[Current] = 1;
                    Path.Add(Current);
                    Current = Successor[Current];
                }

                if (Current != INDEX_NONE && States[Current] == 1)
                {
                    auto CycleIds = TArray<uint32>{};
                    const auto CycleStart = Path.Find(Current);
                    for (auto Index = CycleStart; Index >= 0 && Index < Path.Num(); ++Index)
                    { CycleIds.Add(Entities[Path[Index]].Get_Entry().Get_SavedId()); }

                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                        ECk_SnapshotInspection_DiagnosticCode::OwnerCycle,
                        ck::Format_UE(TEXT("Lifetime-owner chain cycles across [{}] saved entities"), CycleIds.Num()),
                        CycleIds);
                }

                for (const auto Index : Path)
                { States[Index] = 2; }
            }
        }

        auto
            DoAnalyze_Entities(
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            const auto& Entities = InOutDocument.Get_Entities();

            auto SeenSavedIds = TSet<uint32>{};
            for (const auto& Summary : Entities)
            {
                const auto& Entry = Summary.Get_Entry();
                const auto SavedId = Entry.Get_SavedId();

                if (SeenSavedIds.Contains(SavedId))
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                        ECk_SnapshotInspection_DiagnosticCode::DuplicateSavedId,
                        ck::Format_UE(TEXT("Saved id [{}] appears on more than one entity row"), SavedId),
                        TArray<uint32>{SavedId});
                }
                SeenSavedIds.Add(SavedId);

                const auto OwnerId = Entry.Get_LifetimeOwnerSavedId();
                const auto OwnerIndex = OwnerId == k_NoSavedEntity
                    ? INDEX_NONE
                    : InOutDocument.TryGet_EntityIndexForSavedId(OwnerId);

                const auto ContextOwnerId = Entry.Get_ContextOwnerSavedId();
                const auto ContextOwnerIndex = ContextOwnerId == k_NoSavedEntity
                    ? INDEX_NONE
                    : InOutDocument.TryGet_EntityIndexForSavedId(ContextOwnerId);

                // An owner id absent from the entity table is the format's only encoding of "owned by an
                // entity that was never persisted" — most commonly the saving world's transient root, which
                // capture deliberately never records. The loader defines what that means per provenance, so
                // severity follows the load consequence instead of treating every absent owner as corruption.
                if (OwnerId != k_NoSavedEntity && OwnerIndex == INDEX_NONE)
                {
                    switch (Entry.Get_Provenance())
                    {
                        case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                                ECk_SnapshotInspection_DiagnosticCode::OwnerNotPersisted,
                                ck::Format_UE(TEXT("Entity [{}] lifetime owner [{}] is not persisted — the loader treats "
                                    "it as the world root: snapshot-respawnable entities respawn there, the rest are "
                                    "skipped as boot infrastructure"), SavedId, OwnerId),
                                TArray<uint32>{SavedId, OwnerId});
                            break;
                        }
                        case ECk_Snapshot_V3_Provenance::EngineOwned:
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                                ECk_SnapshotInspection_DiagnosticCode::OwnerNotPersisted,
                                ck::Format_UE(TEXT("Entity [{}] lifetime owner [{}] is not persisted — EngineOwned rows "
                                    "rendezvous by save key or player id, so the owner link does not drive resolution"),
                                    SavedId, OwnerId),
                                TArray<uint32>{SavedId, OwnerId});
                            break;
                        }
                        case ECk_Snapshot_V3_Provenance::ConstructSpawned:
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Warning,
                                ECk_SnapshotInspection_DiagnosticCode::OwnerNotPersisted,
                                ck::Format_UE(TEXT("Entity [{}] lifetime owner [{}] is not persisted — the adopt key can "
                                    "never match a mapped owner, so this row orphans on load"), SavedId, OwnerId),
                                TArray<uint32>{SavedId, OwnerId});
                            break;
                        }
                        case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                        {
                            if (ContextOwnerIndex != INDEX_NONE)
                            {
                                DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                                    ECk_SnapshotInspection_DiagnosticCode::OwnerNotPersisted,
                                    ck::Format_UE(TEXT("Entity [{}] lifetime owner [{}] is not persisted — the loader "
                                        "builds it under its persisted context owner [{}] instead"),
                                        SavedId, OwnerId, ContextOwnerId),
                                    TArray<uint32>{SavedId, OwnerId});
                            }
                            else
                            {
                                DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Warning,
                                    ECk_SnapshotInspection_DiagnosticCode::OwnerNotPersisted,
                                    ck::Format_UE(TEXT("Entity [{}] lifetime owner [{}] is not persisted and no persisted "
                                        "context owner remains to build under — the loader drops this row"),
                                        SavedId, OwnerId),
                                    TArray<uint32>{SavedId, OwnerId});
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }

                // Capture sorts owners ahead of dependents and the load-side adopt/owner-mapping passes rely on it.
                if (OwnerIndex != INDEX_NONE && OwnerIndex > Summary.Get_TableIndex())
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                        ECk_SnapshotInspection_DiagnosticCode::DependentBeforeOwner,
                        ck::Format_UE(TEXT("Entity [{}] at row [{}] precedes its lifetime owner [{}] at row [{}]"),
                            SavedId, Summary.Get_TableIndex(), OwnerId, OwnerIndex),
                        TArray<uint32>{SavedId, OwnerId});
                }

                if (ContextOwnerId != k_NoSavedEntity && ContextOwnerIndex == INDEX_NONE)
                {
                    if (Entry.Get_Provenance() == ECk_Snapshot_V3_Provenance::DefinitionBuilt &&
                        OwnerId == k_NoSavedEntity)
                    {
                        DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Warning,
                            ECk_SnapshotInspection_DiagnosticCode::ContextOwnerNotPersisted,
                            ck::Format_UE(TEXT("DefinitionBuilt entity [{}] context owner [{}] is not persisted and no "
                                "lifetime owner is named — the loader drops this row"), SavedId, ContextOwnerId),
                            TArray<uint32>{SavedId, ContextOwnerId});
                    }
                    else
                    {
                        DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                            ECk_SnapshotInspection_DiagnosticCode::ContextOwnerNotPersisted,
                            ck::Format_UE(TEXT("Entity [{}] context owner [{}] is not persisted — ownership restore keeps "
                                "the rebuild-time context relationship on load"), SavedId, ContextOwnerId),
                            TArray<uint32>{SavedId, ContextOwnerId});
                    }
                }

                switch (Entry.Get_Provenance())
                {
                    case ECk_Snapshot_V3_Provenance::ConstructSpawned:
                    {
                        if (Entry.Get_Label().IsEmpty())
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                                ECk_SnapshotInspection_DiagnosticCode::ConstructSpawnedNoLabel,
                                ck::Format_UE(TEXT("ConstructSpawned entity [{}] carries no label — capture rule 3 "
                                    "skips unlabeled construct-spawned entities, so a healthy save cannot contain this row"),
                                    SavedId),
                                TArray<uint32>{SavedId});
                        }
                        break;
                    }
                    case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                    {
                        if (Entry.Get_ScriptClassPath().IsEmpty() && Entry.Get_ActorClassPath().IsEmpty())
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                                ECk_SnapshotInspection_DiagnosticCode::RuntimeSpawnedNoRecipe,
                                ck::Format_UE(TEXT("RuntimeSpawned entity [{}] carries neither a script class nor an "
                                    "actor class — nothing can rebuild it"), SavedId),
                                TArray<uint32>{SavedId});
                        }
                        break;
                    }
                    case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                    {
                        if (OwnerId == k_NoSavedEntity && ContextOwnerId == k_NoSavedEntity)
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                                ECk_SnapshotInspection_DiagnosticCode::DefinitionBuiltNoOwner,
                                ck::Format_UE(TEXT("DefinitionBuilt entity [{}] names neither a lifetime owner nor a "
                                    "context owner to be built under"), SavedId),
                                TArray<uint32>{SavedId});
                        }

                        const auto HasUsableStep = Entry.Get_BuildRecipe().ContainsByPredicate(
                            [](const auto& InStep) { return NOT InStep.Get_ScriptClassPath().IsEmpty(); });

                        if (NOT HasUsableStep)
                        {
                            DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                                ECk_SnapshotInspection_DiagnosticCode::DefinitionBuiltNoUsableStep,
                                ck::Format_UE(TEXT("DefinitionBuilt entity [{}] has no build step carrying a script "
                                    "class path"), SavedId),
                                TArray<uint32>{SavedId});
                        }
                        break;
                    }
                    case ECk_Snapshot_V3_Provenance::EngineOwned:
                    default:
                        break;
                }
            }

            const auto SaveKeyGroups = DoCollect_DuplicateGroups(Entities,
                [](const FCk_Snapshot_V3_EntityEntry& InEntry, FString& OutKey) -> bool
                {
                    if (NOT InEntry.Get_SaveKey().IsValid())
                    { return false; }

                    OutKey = InEntry.Get_SaveKey().ToString();
                    return true;
                });

            for (const auto& Group : SaveKeyGroups)
            {
                DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                    ECk_SnapshotInspection_DiagnosticCode::DuplicateSaveKey,
                    ck::Format_UE(TEXT("SaveKey [{}] is carried by [{}] entity rows"), Group._Key, Group._SavedIds.Num()),
                    Group._SavedIds);
            }

            const auto AdoptKeyGroups = DoCollect_DuplicateGroups(Entities,
                [](const FCk_Snapshot_V3_EntityEntry& InEntry, FString& OutKey) -> bool
                {
                    if (InEntry.Get_Provenance() != ECk_Snapshot_V3_Provenance::ConstructSpawned)
                    { return false; }

                    OutKey = ck::Format_UE(TEXT("owner [{}] label [{}]"),
                        InEntry.Get_LifetimeOwnerSavedId(), InEntry.Get_Label());
                    return true;
                });

            for (const auto& Group : AdoptKeyGroups)
            {
                DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                    ECk_SnapshotInspection_DiagnosticCode::DuplicateAdoptKey,
                    ck::Format_UE(TEXT("Adopt key {} is carried by [{}] ConstructSpawned rows — the double-population "
                        "signature: the loader can only bind them positionally"), Group._Key, Group._SavedIds.Num()),
                    Group._SavedIds);
            }

            // Predicate, verbatim: a RuntimeSpawned row is a recipe-identity duplicate of another when the triple
            // (ScriptClassPath, LifetimeOwnerSavedId, Label) matches exactly. The non-zero-SaveKey collision is
            // already reported as DuplicateSaveKey and is deliberately not re-reported here.
            const auto RecipeGroups = DoCollect_DuplicateGroups(Entities,
                [](const FCk_Snapshot_V3_EntityEntry& InEntry, FString& OutKey) -> bool
                {
                    if (InEntry.Get_Provenance() != ECk_Snapshot_V3_Provenance::RuntimeSpawned)
                    { return false; }

                    OutKey = ck::Format_UE(TEXT("script [{}] owner [{}] label [{}]"),
                        InEntry.Get_ScriptClassPath(), InEntry.Get_LifetimeOwnerSavedId(), InEntry.Get_Label());
                    return true;
                });

            for (const auto& Group : RecipeGroups)
            {
                DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Warning,
                    ECk_SnapshotInspection_DiagnosticCode::DuplicateRecipeIdentity,
                    ck::Format_UE(TEXT("[{}] RuntimeSpawned rows share recipe identity {}"),
                        Group._SavedIds.Num(), Group._Key),
                    Group._SavedIds);
            }

            DoAnalyze_OwnerCycles(InOutDocument);
        }

        auto
            DoAnalyze_Payloads(
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            for (auto Index = 0; Index < InOutDocument.Get_Payloads().Num(); ++Index)
            {
                const auto& Summary = InOutDocument.Get_Payloads()[Index];
                const auto& Entry = Summary.Get_Entry();
                const auto OwnerId = Entry.Get_OwnerSavedId();

                if (InOutDocument.TryGet_EntityIndexForSavedId(OwnerId) == INDEX_NONE)
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Error,
                        ECk_SnapshotInspection_DiagnosticCode::PayloadOwnerMissing,
                        ck::Format_UE(TEXT("Payload row [{}] names owner [{}], which is absent from the entity table"),
                            Index, OwnerId),
                        TArray<uint32>{OwnerId}, TArray<int32>{Index});
                }

                if (Entry.Get_PayloadBytes().IsEmpty())
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Warning,
                        ECk_SnapshotInspection_DiagnosticCode::PayloadBytesEmpty,
                        ck::Format_UE(TEXT("Payload row [{}] on owner [{}] carries no bytes — nothing to hydrate"),
                            Index, OwnerId),
                        TArray<uint32>{OwnerId}, TArray<int32>{Index});
                }

                if (Entry.Get_TypePath().IsEmpty())
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Warning,
                        ECk_SnapshotInspection_DiagnosticCode::PayloadTypePathEmpty,
                        ck::Format_UE(TEXT("Payload row [{}] on owner [{}] declares no type path"), Index, OwnerId),
                        TArray<uint32>{OwnerId}, TArray<int32>{Index});
                }
                else if (NOT Summary.Get_TypeAvailable())
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                        ECk_SnapshotInspection_DiagnosticCode::PayloadTypeUnavailable,
                        ck::Format_UE(TEXT("Payload type [{}] is not available in this editor"), Entry.Get_TypePath()),
                        TArray<uint32>{OwnerId}, TArray<int32>{Index});
                }
            }
        }

        // Environment observations only: resolve with FindObject, NEVER LoadObject. A miss says what this editor can
        // see right now, which is not a property of the file.
        auto
            DoAnalyze_Environment(
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            for (const auto& Summary : InOutDocument.Get_Entities())
            {
                const auto& Entry = Summary.Get_Entry();
                const auto SavedId = Entry.Get_SavedId();

                if (const auto& ScriptClassPath = Entry.Get_ScriptClassPath();
                    NOT ScriptClassPath.IsEmpty() && FSoftClassPath{ScriptClassPath}.ResolveClass() == nullptr)
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                        ECk_SnapshotInspection_DiagnosticCode::ScriptClassUnavailable,
                        ck::Format_UE(TEXT("Script class [{}] is not available in this editor"), ScriptClassPath),
                        TArray<uint32>{SavedId});
                }

                if (const auto& ActorClassPath = Entry.Get_ActorClassPath();
                    NOT ActorClassPath.IsEmpty() && FSoftClassPath{ActorClassPath}.ResolveClass() == nullptr)
                {
                    DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                        ECk_SnapshotInspection_DiagnosticCode::ActorClassUnavailable,
                        ck::Format_UE(TEXT("Actor class [{}] is not available in this editor"), ActorClassPath),
                        TArray<uint32>{SavedId});
                }

                for (const auto& Step : Entry.Get_BuildRecipe())
                {
                    if (const auto& ArchetypePath = Step.Get_ArchetypePath();
                        NOT ArchetypePath.IsEmpty() && FSoftObjectPath{ArchetypePath}.ResolveObject() == nullptr)
                    {
                        DoAdd_Diagnostic(InOutDocument, ECk_SnapshotInspection_Severity::Info,
                            ECk_SnapshotInspection_DiagnosticCode::ArchetypeUnavailable,
                            ck::Format_UE(TEXT("Archetype [{}] is not available in this editor"), ArchetypePath),
                            TArray<uint32>{SavedId});
                    }
                }
            }
        }

        auto
            DoMark_Problems(
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            auto ProblemSavedIds = TSet<uint32>{};
            auto ProblemPayloadIndices = TSet<int32>{};

            for (const auto& Diagnostic : InOutDocument.Get_Diagnostics())
            {
                if (Diagnostic.Get_Severity() == ECk_SnapshotInspection_Severity::Info)
                { continue; }

                ProblemSavedIds.Append(Diagnostic.Get_RelatedSavedIds());
                ProblemPayloadIndices.Append(Diagnostic.Get_RelatedPayloadIndices());
            }

            for (auto& Summary : InOutDocument.Get_Entities())
            { Summary.Set_HasProblems(ProblemSavedIds.Contains(Summary.Get_Entry().Get_SavedId())); }

            for (auto& Summary : InOutDocument.Get_Payloads())
            { Summary.Set_HasProblems(ProblemPayloadIndices.Contains(Summary.Get_TableIndex())); }
        }

        // ------------------------------------------------------------------------------------------------------------

        auto
            DoBuild_Tables(
                const FCk_Snapshot_V3_Tables& InTables,
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            auto Census = FCk_SnapshotInspection_Census{};

            InOutDocument.Get_Entities().Reserve(InTables.Get_Entities().Num());
            for (auto Index = 0; Index < InTables.Get_Entities().Num(); ++Index)
            {
                const auto& Entry = InTables.Get_Entities()[Index];

                auto Summary = FCk_SnapshotInspection_EntitySummary{};
                Summary.Set_Entry(Entry)
                       .Set_TableIndex(Index)
                       .Set_SpawnParamsByteCount(Entry.Get_SpawnParamsBytes().Num())
                       .Set_ActorSaveFieldByteCount(Entry.Get_ActorSaveFieldBytes().Num())
                       .Set_BuildStepCount(Entry.Get_BuildRecipe().Num())
                       .Set_IdentityText(Get_IdentityText(Entry));

                InOutDocument.Get_Entities().Add(MoveTemp(Summary));

                if (NOT InOutDocument.Get_SavedIdToEntityIndex().Contains(Entry.Get_SavedId()))
                { InOutDocument.Get_SavedIdToEntityIndex().Add(Entry.Get_SavedId(), Index); }

                switch (Entry.Get_Provenance())
                {
                    case ECk_Snapshot_V3_Provenance::EngineOwned:
                        Census.Set_EngineOwnedCount(Census.Get_EngineOwnedCount() + 1); break;
                    case ECk_Snapshot_V3_Provenance::ConstructSpawned:
                        Census.Set_ConstructSpawnedCount(Census.Get_ConstructSpawnedCount() + 1); break;
                    case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                        Census.Set_RuntimeSpawnedCount(Census.Get_RuntimeSpawnedCount() + 1); break;
                    case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                        Census.Set_DefinitionBuiltCount(Census.Get_DefinitionBuiltCount() + 1); break;
                }
            }

            InOutDocument.Get_Payloads().Reserve(InTables.Get_Payloads().Num());
            for (auto Index = 0; Index < InTables.Get_Payloads().Num(); ++Index)
            {
                const auto& Entry = InTables.Get_Payloads()[Index];

                const auto TypeAvailable = NOT Entry.Get_TypePath().IsEmpty()
                    && FindObject<UScriptStruct>(nullptr, *Entry.Get_TypePath()) != nullptr;

                auto Summary = FCk_SnapshotInspection_PayloadSummary{};
                Summary.Set_Entry(Entry)
                       .Set_TableIndex(Index)
                       .Set_ByteCount(Entry.Get_PayloadBytes().Num())
                       .Set_PayloadHashHex(Get_Sha256Hex(Entry.Get_PayloadBytes()))
                       .Set_TypeAvailable(TypeAvailable);

                InOutDocument.Get_Payloads().Add(MoveTemp(Summary));
            }

            Census.Set_EntityCount(InTables.Get_Entities().Num())
                  .Set_PayloadCount(InTables.Get_Payloads().Num());

            InOutDocument.Set_Census(Census);

            DoAnalyze_HeaderCensus(InOutDocument);
            DoAnalyze_Entities(InOutDocument);
            DoAnalyze_Payloads(InOutDocument);
            DoAnalyze_Environment(InOutDocument);
            DoMark_Problems(InOutDocument);
        }

        auto
            DoInspect_SaveGame(
                const UCk_Snapshot_SaveGame& InSaveGame,
                FCk_SnapshotInspection_Document& InOutDocument)
            -> void
        {
            InOutDocument.Set_SaveGameClassPath(InSaveGame.GetClass()->GetPathName())
                         .Set_HeaderRecovered(true)
                         .Set_Header(InSaveGame._HeaderV3);

            const auto FormatVersion = InSaveGame._HeaderV3.Get_FormatVersion();
            InOutDocument.Set_Compatibility(
                  FormatVersion == FCk_Snapshot_HeaderV3::CurrentFormatVersion
                    ? ECk_SnapshotInspection_Compatibility::Current
                : FormatVersion <  FCk_Snapshot_HeaderV3::CurrentFormatVersion
                    ? ECk_SnapshotInspection_Compatibility::Older
                    : ECk_SnapshotInspection_Compatibility::Newer);

            InOutDocument.Set_SnapshotByteCount(InSaveGame._SnapshotBytesV3.Num())
                         .Set_SnapshotHashHex(Get_Sha256Hex(InSaveGame._SnapshotBytesV3));

            // An incompatible stream is never fed to the CURRENT table struct: SerializeItem would read a different
            // layout and report success on nonsense. The header metadata above still stands on its own.
            if (FormatVersion != FCk_Snapshot_HeaderV3::CurrentFormatVersion)
            {
                InOutDocument.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::UnsupportedFormat);
                return;
            }

            if (InSaveGame._SnapshotBytesV3.IsEmpty())
            {
                InOutDocument.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::CorruptEnvelope);
                return;
            }

            auto Tables = FCk_Snapshot_V3_Tables{};
            auto Reader = FMemoryReader{InSaveGame._SnapshotBytesV3, /*bIsPersistent=*/true};
            FCk_Snapshot_V3_Tables::StaticStruct()->SerializeItem(Reader, &Tables, /*Defaults=*/nullptr);

            InOutDocument.Set_TableParseAttempted(true);

            // A short read leaves Tell() behind TotalSize() without setting the error flag, and trailing bytes mean
            // the stream is not the table struct the header claims.
            if (Reader.IsError() || Reader.Tell() != Reader.TotalSize())
            {
                InOutDocument.Set_TableParseSucceeded(false)
                             .Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::CorruptTables);
                return;
            }

            InOutDocument.Set_TableParseSucceeded(true)
                         .Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::Success);

            DoBuild_Tables(Tables, InOutDocument);
        }

        auto
            DoInspect_Bytes(
                const TArray<uint8>& InBytes,
                ECk_SnapshotInspection_SourceKind InSourceKind,
                const FString& InSourceDescription)
            -> FCk_SnapshotInspection_Document
        {
            auto Document = FCk_SnapshotInspection_Document{};
            Document.Set_SourceKind(InSourceKind)
                    .Set_SourceDescription(InSourceDescription)
                    .Set_SourceByteCount(InBytes.Num())
                    .Set_SourceHashHex(Get_Sha256Hex(InBytes));

            if (NOT Get_HasSaveGameEnvelopeTag(InBytes))
            {
                Document.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::NotCkSnapshot);
                return Document;
            }

            auto* SaveGame = UGameplayStatics::LoadGameFromMemory(InBytes);
            if (ck::Is_NOT_Valid(SaveGame))
            {
                Document.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::CorruptEnvelope);
                return Document;
            }

            Document.Set_SaveGameClassPath(SaveGame->GetClass()->GetPathName());

            auto* SnapshotSaveGame = Cast<UCk_Snapshot_SaveGame>(SaveGame);
            if (ck::Is_NOT_Valid(SnapshotSaveGame))
            {
                Document.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::NotCkSnapshot);
                return Document;
            }

            DoInspect_SaveGame(*SnapshotSaveGame, Document);
            return Document;
        }

        auto
            DoMake_OffGameThreadDocument(
                ECk_SnapshotInspection_SourceKind InSourceKind,
                const FString& InSourceDescription)
            -> FCk_SnapshotInspection_Document
        {
            auto Document = FCk_SnapshotInspection_Document{};
            Document.Set_SourceKind(InSourceKind)
                    .Set_SourceDescription(InSourceDescription)
                    .Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::IoFailure);
            return Document;
        }

        // Projects an already-loaded sidecar object onto the document. Shared by the file and slot routes so both
        // report identical fields for the same sidecar.
        auto
            DoProject_SlotMeta(
                FCk_SnapshotInspection_Document& InOutDocument,
                const UCk_Snapshot_SlotMetaSaveGame* InMetaSaveGame,
                const FString& InSourceDescription)
            -> void
        {
            if (ck::Is_NOT_Valid(InMetaSaveGame))
            { return; }

            const auto& Meta = InMetaSaveGame->_Meta;

            auto Projected = FCk_SnapshotInspection_SlotMeta{};
            Projected.Set_Found(true)
                     .Set_SourceDescription(InSourceDescription)
                     .Set_Title(Meta.Get_Title())
                     .Set_TimestampUTC(Meta.Get_TimestampUTC())
                     .Set_WorldAssetPath(Meta.Get_WorldAssetPath().ToString())
                     .Set_CustomFields(Meta.Get_CustomFields())
                     .Set_ScreenshotPng(Meta.Get_ScreenshotPng())
                     .Set_ScreenshotByteCount(Meta.Get_ScreenshotPng().Num())
                     .Set_ScreenshotHashHex(Get_Sha256Hex(Meta.Get_ScreenshotPng()));

            InOutDocument.Set_SlotMeta(Projected);
        }

        auto
            DoAttach_SlotMeta_FromFile(
                FCk_SnapshotInspection_Document& InOutDocument,
                const FString& InAbsolutePath)
            -> void
        {
            // Slot "<Name>.meta" lands on disk as "<Name>.meta.sav", so the sidecar of any save file is its own
            // path with the extension swapped. Works for a file picked from anywhere, not just the save directory.
            const auto SidecarPath = FPaths::Combine(
                FPaths::GetPath(InAbsolutePath),
                FPaths::GetBaseFilename(InAbsolutePath) + ck::snapshot::slot_meta::MetaSlotSuffix + TEXT(".sav"));

            if (NOT FPaths::FileExists(SidecarPath))
            { return; }

            auto SidecarBytes = TArray<uint8>{};
            if (NOT FFileHelper::LoadFileToArray(SidecarBytes, *SidecarPath))
            { return; }

            const auto* MetaSaveGame = Cast<UCk_Snapshot_SlotMetaSaveGame>(
                UGameplayStatics::LoadGameFromMemory(SidecarBytes));

            DoProject_SlotMeta(InOutDocument, MetaSaveGame, SidecarPath);
        }

        auto
            DoAttach_SlotMeta_FromSlot(
                FCk_SnapshotInspection_Document& InOutDocument,
                FName InSlotName)
            -> void
        {
            const auto SidecarSlot = ck::snapshot::slot_meta::Get_MetaSlotName(InSlotName);

            if (NOT UGameplayStatics::DoesSaveGameExist(SidecarSlot, k_UserIndex))
            { return; }

            const auto* MetaSaveGame = Cast<UCk_Snapshot_SlotMetaSaveGame>(
                UGameplayStatics::LoadGameFromSlot(SidecarSlot, k_UserIndex));

            DoProject_SlotMeta(InOutDocument, MetaSaveGame, SidecarSlot);
        }

        auto
            DoMake_FailedDecodeResult(
                ECk_SnapshotInspection_DecodeStatus InStatus,
                const FString& InErrorText)
            -> FCk_SnapshotInspection_DecodeResult
        {
            auto Result = FCk_SnapshotInspection_DecodeResult{};
            Result.Set_Status(InStatus).Set_ErrorText(InErrorText);
            return Result;
        }

        // The read-side mirror of the capture's SerializeInstancedStruct, with two deliberate differences: the
        // proxy's LoadIfFindFails is the caller's policy rather than an unconditional true, and the snapshot context
        // is DEFAULT-constructed — no saved-id map and no registry, so RemapHandles writes each handle's raw saved
        // id straight back and never re-homes it onto a live world.
        auto
            DoDecode_Blob(
                const TArray<uint8>& InBytes,
                ECk_SnapshotInspection_DecodePolicy InPolicy,
                const FString& InDeclaredTypePath)
            -> FCk_SnapshotInspection_DecodeResult
        {
            auto Result = FCk_SnapshotInspection_DecodeResult{};
            Result.Set_DeclaredTypePath(InDeclaredTypePath);

            if (InBytes.IsEmpty())
            {
                return Result.Set_Status(ECk_SnapshotInspection_DecodeStatus::DeserializeFailed)
                             .Set_ErrorText(TEXT("empty blob"));
            }

            const auto DeclaredTypeResolvable = NOT InDeclaredTypePath.IsEmpty()
                && FindObject<UScriptStruct>(nullptr, *InDeclaredTypePath) != nullptr;

            auto Decoded = FInstancedStruct{};
            auto Reader = FMemoryReader{InBytes, /*bIsPersistent=*/true};
            const auto LoadIfFindFails = InPolicy == ECk_SnapshotInspection_DecodePolicy::AllowTypeLoad;
            auto Proxy = FObjectAndNameAsStringProxyArchive{Reader, LoadIfFindFails};
            Proxy.ArIsSaveGame = false;
            Proxy.SetIsPersistent(true);

            auto Context = ck::FSnapshotContext{};

            Decoded.Serialize(Proxy);

            if (NOT Decoded.IsValid())
            {
                const auto Status = InPolicy == ECk_SnapshotInspection_DecodePolicy::AvailableTypesOnly
                        && NOT DeclaredTypeResolvable
                    ? ECk_SnapshotInspection_DecodeStatus::TypeUnavailable
                    : ECk_SnapshotInspection_DecodeStatus::DeserializeFailed;

                return Result.Set_Status(Status)
                             .Set_ErrorText(Status == ECk_SnapshotInspection_DecodeStatus::TypeUnavailable
                                 ? FString{TEXT("the blob's struct type is not available in this editor")}
                                 : FString{TEXT("the blob produced no struct")});
            }

            ck::snapshot::RemapHandles(Decoded.GetScriptStruct(), Decoded.GetMutableMemory(), Proxy, Context);

            if (Reader.IsError())
            {
                return Result.Set_Status(ECk_SnapshotInspection_DecodeStatus::DeserializeFailed)
                             .Set_ErrorText(TEXT("the blob stream reported an error"));
            }

            // A healthy blob is exactly [tagged payload][one uint32 per handle] — RemapHandles consumes the last
            // byte. Anything left over means the stream is not the struct it claims, same as the table-level check.
            if (Reader.Tell() != Reader.TotalSize())
            {
                return Result.Set_Status(ECk_SnapshotInspection_DecodeStatus::DeserializeFailed)
                             .Set_ErrorText(ck::Format_UE(TEXT("the blob carries [{}] trailing byte(s) past the decoded struct"),
                                 Reader.TotalSize() - Reader.Tell()));
            }

            const auto DecodedTypePath = Decoded.GetScriptStruct()->GetPathName();
            Result.Set_DecodedTypePath(DecodedTypePath);

            // A tree whose type claim is a lie is worse than no tree, so the mismatch is reported bare.
            if (NOT InDeclaredTypePath.IsEmpty() && DecodedTypePath != InDeclaredTypePath)
            {
                return Result.Set_Status(ECk_SnapshotInspection_DecodeStatus::DeclaredTypeMismatch)
                             .Set_ErrorText(ck::Format_UE(TEXT("declared type [{}] but decoded [{}]"),
                                 InDeclaredTypePath, DecodedTypePath));
            }

            Result.Set_Status(ECk_SnapshotInspection_DecodeStatus::Decoded)
                  .Set_ValueTreeRoot(DoProject_ValueTree(Decoded));

            return Result;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ProvenanceText(
            ECk_Snapshot_V3_Provenance InProvenance)
        -> const TCHAR*
    {
        switch (InProvenance)
        {
            case ECk_Snapshot_V3_Provenance::EngineOwned:      return TEXT("EngineOwned");
            case ECk_Snapshot_V3_Provenance::ConstructSpawned: return TEXT("ConstructSpawned");
            case ECk_Snapshot_V3_Provenance::RuntimeSpawned:   return TEXT("RuntimeSpawned");
            case ECk_Snapshot_V3_Provenance::DefinitionBuilt:  return TEXT("DefinitionBuilt");
        }
        return TEXT("Unknown");
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IdentityText(
            const FCk_Snapshot_V3_EntityEntry& InEntry)
        -> FString
    {
        switch (InEntry.Get_Provenance())
        {
            case ECk_Snapshot_V3_Provenance::EngineOwned:
                return InEntry.Get_SaveKey().IsValid() ? InEntry.Get_SaveKey().ToString() : InEntry.Get_PlayerId();
            case ECk_Snapshot_V3_Provenance::ConstructSpawned:
                return InEntry.Get_Label();
            case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                return NOT InEntry.Get_ActorClassPath().IsEmpty()
                    ? InEntry.Get_ActorClassPath()
                    : InEntry.Get_ScriptClassPath();
            case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                return NOT InEntry.Get_BuildRecipe().IsEmpty()
                    ? InEntry.Get_BuildRecipe()[0].Get_ScriptClassPath()
                    : InEntry.Get_ScriptClassPath();
        }
        return {};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ReadStatusText(
            ECk_SnapshotInspection_ReadStatus InStatus)
        -> const TCHAR*
    {
        switch (InStatus)
        {
            case ECk_SnapshotInspection_ReadStatus::Success:           return TEXT("Success");
            case ECk_SnapshotInspection_ReadStatus::FileNotFound:      return TEXT("FileNotFound");
            case ECk_SnapshotInspection_ReadStatus::IoFailure:         return TEXT("IoFailure");
            case ECk_SnapshotInspection_ReadStatus::NotCkSnapshot:     return TEXT("NotCkSnapshot");
            case ECk_SnapshotInspection_ReadStatus::UnsupportedFormat: return TEXT("UnsupportedFormat");
            case ECk_SnapshotInspection_ReadStatus::CorruptEnvelope:   return TEXT("CorruptEnvelope");
            case ECk_SnapshotInspection_ReadStatus::CorruptTables:     return TEXT("CorruptTables");
        }
        return TEXT("Unknown");
    }

    auto
        Get_CompatibilityText(
            ECk_SnapshotInspection_Compatibility InCompatibility)
        -> const TCHAR*
    {
        switch (InCompatibility)
        {
            case ECk_SnapshotInspection_Compatibility::Current: return TEXT("Current");
            case ECk_SnapshotInspection_Compatibility::Older:   return TEXT("Older");
            case ECk_SnapshotInspection_Compatibility::Newer:   return TEXT("Newer");
            case ECk_SnapshotInspection_Compatibility::Unknown: return TEXT("Unknown");
        }
        return TEXT("Unknown");
    }

    auto
        Get_DecodeStatusText(
            ECk_SnapshotInspection_DecodeStatus InStatus)
        -> const TCHAR*
    {
        switch (InStatus)
        {
            case ECk_SnapshotInspection_DecodeStatus::NotRequested:         return TEXT("NotRequested");
            case ECk_SnapshotInspection_DecodeStatus::Decoded:              return TEXT("Decoded");
            case ECk_SnapshotInspection_DecodeStatus::TypeUnavailable:      return TEXT("TypeUnavailable");
            case ECk_SnapshotInspection_DecodeStatus::DeserializeFailed:    return TEXT("DeserializeFailed");
            case ECk_SnapshotInspection_DecodeStatus::DeclaredTypeMismatch: return TEXT("DeclaredTypeMismatch");
            case ECk_SnapshotInspection_DecodeStatus::UnsupportedBlob:      return TEXT("UnsupportedBlob");
        }
        return TEXT("Unknown");
    }

    auto
        Get_SeverityText(
            ECk_SnapshotInspection_Severity InSeverity)
        -> const TCHAR*
    {
        switch (InSeverity)
        {
            case ECk_SnapshotInspection_Severity::Info:    return TEXT("Info");
            case ECk_SnapshotInspection_Severity::Warning: return TEXT("Warning");
            case ECk_SnapshotInspection_Severity::Error:   return TEXT("Error");
        }
        return TEXT("Unknown");
    }

    auto
        Get_DiagnosticCodeText(
            ECk_SnapshotInspection_DiagnosticCode InCode)
        -> const TCHAR*
    {
        switch (InCode)
        {
            case ECk_SnapshotInspection_DiagnosticCode::HeaderCensusMismatch:        return TEXT("HeaderCensusMismatch");
            case ECk_SnapshotInspection_DiagnosticCode::DuplicateSavedId:            return TEXT("DuplicateSavedId");
            case ECk_SnapshotInspection_DiagnosticCode::OwnerNotPersisted:           return TEXT("OwnerNotPersisted");
            case ECk_SnapshotInspection_DiagnosticCode::ContextOwnerNotPersisted:    return TEXT("ContextOwnerNotPersisted");
            case ECk_SnapshotInspection_DiagnosticCode::OwnerCycle:                  return TEXT("OwnerCycle");
            case ECk_SnapshotInspection_DiagnosticCode::DependentBeforeOwner:        return TEXT("DependentBeforeOwner");
            case ECk_SnapshotInspection_DiagnosticCode::DuplicateSaveKey:            return TEXT("DuplicateSaveKey");
            case ECk_SnapshotInspection_DiagnosticCode::DuplicateAdoptKey:           return TEXT("DuplicateAdoptKey");
            case ECk_SnapshotInspection_DiagnosticCode::ConstructSpawnedNoLabel:     return TEXT("ConstructSpawnedNoLabel");
            case ECk_SnapshotInspection_DiagnosticCode::RuntimeSpawnedNoRecipe:      return TEXT("RuntimeSpawnedNoRecipe");
            case ECk_SnapshotInspection_DiagnosticCode::DefinitionBuiltNoOwner:      return TEXT("DefinitionBuiltNoOwner");
            case ECk_SnapshotInspection_DiagnosticCode::DefinitionBuiltNoUsableStep: return TEXT("DefinitionBuiltNoUsableStep");
            case ECk_SnapshotInspection_DiagnosticCode::PayloadOwnerMissing:         return TEXT("PayloadOwnerMissing");
            case ECk_SnapshotInspection_DiagnosticCode::PayloadBytesEmpty:           return TEXT("PayloadBytesEmpty");
            case ECk_SnapshotInspection_DiagnosticCode::PayloadTypePathEmpty:        return TEXT("PayloadTypePathEmpty");
            case ECk_SnapshotInspection_DiagnosticCode::DuplicateRecipeIdentity:     return TEXT("DuplicateRecipeIdentity");
            case ECk_SnapshotInspection_DiagnosticCode::PayloadTypeUnavailable:      return TEXT("PayloadTypeUnavailable");
            case ECk_SnapshotInspection_DiagnosticCode::ScriptClassUnavailable:      return TEXT("ScriptClassUnavailable");
            case ECk_SnapshotInspection_DiagnosticCode::ActorClassUnavailable:       return TEXT("ActorClassUnavailable");
            case ECk_SnapshotInspection_DiagnosticCode::ArchetypeUnavailable:        return TEXT("ArchetypeUnavailable");
        }
        return TEXT("Unknown");
    }

    auto
        Get_ValueKindText(
            ECk_SnapshotInspection_ValueKind InKind)
        -> const TCHAR*
    {
        switch (InKind)
        {
            case ECk_SnapshotInspection_ValueKind::Bool:           return TEXT("Bool");
            case ECk_SnapshotInspection_ValueKind::Int:            return TEXT("Int");
            case ECk_SnapshotInspection_ValueKind::UInt:           return TEXT("UInt");
            case ECk_SnapshotInspection_ValueKind::Float:          return TEXT("Float");
            case ECk_SnapshotInspection_ValueKind::Enum:           return TEXT("Enum");
            case ECk_SnapshotInspection_ValueKind::String:         return TEXT("String");
            case ECk_SnapshotInspection_ValueKind::Name:           return TEXT("Name");
            case ECk_SnapshotInspection_ValueKind::Text:           return TEXT("Text");
            case ECk_SnapshotInspection_ValueKind::ObjectPath:     return TEXT("ObjectPath");
            case ECk_SnapshotInspection_ValueKind::SoftObjectPath: return TEXT("SoftObjectPath");
            case ECk_SnapshotInspection_ValueKind::Struct:         return TEXT("Struct");
            case ECk_SnapshotInspection_ValueKind::Array:          return TEXT("Array");
            case ECk_SnapshotInspection_ValueKind::Set:            return TEXT("Set");
            case ECk_SnapshotInspection_ValueKind::Map:            return TEXT("Map");
            case ECk_SnapshotInspection_ValueKind::SavedEntityRef: return TEXT("SavedEntityRef");
            case ECk_SnapshotInspection_ValueKind::Unsupported:    return TEXT("Unsupported");
            case ECk_SnapshotInspection_ValueKind::Truncated:      return TEXT("Truncated");
        }
        return TEXT("Unknown");
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Inspect_SaveFile(
            const FString& InAbsolutePath)
        -> FCk_SnapshotInspection_Document
    {
        const auto IsOnGameThread = IsInGameThread();
        CK_ENSURE_IF_NOT(IsOnGameThread,
            TEXT("Inspect_SaveFile [{}] must be called on the game thread — the envelope route creates a USaveGame UObject"),
            InAbsolutePath)
        { return ck_snapshot_inspection::DoMake_OffGameThreadDocument(ECk_SnapshotInspection_SourceKind::File, InAbsolutePath); }

        auto Document = FCk_SnapshotInspection_Document{};

        if (NOT FPaths::FileExists(InAbsolutePath))
        {
            Document.Set_SourceKind(ECk_SnapshotInspection_SourceKind::File)
                    .Set_SourceDescription(InAbsolutePath)
                    .Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::FileNotFound);
            return Document;
        }

        auto Bytes = TArray<uint8>{};
        if (NOT FFileHelper::LoadFileToArray(Bytes, *InAbsolutePath))
        {
            Document.Set_SourceKind(ECk_SnapshotInspection_SourceKind::File)
                    .Set_SourceDescription(InAbsolutePath)
                    .Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::IoFailure);
            return Document;
        }

        auto FileDocument = ck_snapshot_inspection::DoInspect_Bytes(Bytes, ECk_SnapshotInspection_SourceKind::File, InAbsolutePath);

        // The sidecar sits beside the save the user picked, so opening a .sav brings its metadata along without a
        // second file prompt. Absent is ordinary — see FCk_SnapshotInspection_SlotMeta.
        ck_snapshot_inspection::DoAttach_SlotMeta_FromFile(FileDocument, InAbsolutePath);
        return FileDocument;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Inspect_SaveSlot(
            FName InSlotName)
        -> FCk_SnapshotInspection_Document
    {
        const auto SlotName = InSlotName.ToString();

        const auto IsOnGameThread = IsInGameThread();
        CK_ENSURE_IF_NOT(IsOnGameThread,
            TEXT("Inspect_SaveSlot [{}] must be called on the game thread — the slot route creates a USaveGame UObject"),
            InSlotName)
        { return ck_snapshot_inspection::DoMake_OffGameThreadDocument(ECk_SnapshotInspection_SourceKind::Slot, SlotName); }

        auto Document = FCk_SnapshotInspection_Document{};
        Document.Set_SourceKind(ECk_SnapshotInspection_SourceKind::Slot)
                .Set_SourceDescription(SlotName);

        if (NOT UGameplayStatics::DoesSaveGameExist(SlotName, ck_snapshot_inspection::k_UserIndex))
        {
            Document.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::FileNotFound);
            return Document;
        }

        // The slot's raw bytes go through the same envelope route as an arbitrary file, so the slot form reports
        // the same source byte count and hash the file form does.
        auto Bytes = TArray<uint8>{};
        if (NOT UGameplayStatics::LoadDataFromSlot(Bytes, SlotName, ck_snapshot_inspection::k_UserIndex))
        {
            Document.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::IoFailure);
            return Document;
        }

        auto SlotDocument = ck_snapshot_inspection::DoInspect_Bytes(Bytes, ECk_SnapshotInspection_SourceKind::Slot, SlotName);

        ck_snapshot_inspection::DoAttach_SlotMeta_FromSlot(SlotDocument, InSlotName);
        return SlotDocument;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Inspect_SaveBytes(
            const TArray<uint8>& InBytes,
            const FString& InSourceDescription)
        -> FCk_SnapshotInspection_Document
    {
        const auto IsOnGameThread = IsInGameThread();
        CK_ENSURE_IF_NOT(IsOnGameThread,
            TEXT("Inspect_SaveBytes [{}] must be called on the game thread — the envelope route creates a USaveGame UObject"),
            InSourceDescription)
        { return ck_snapshot_inspection::DoMake_OffGameThreadDocument(ECk_SnapshotInspection_SourceKind::Bytes, InSourceDescription); }

        return ck_snapshot_inspection::DoInspect_Bytes(InBytes, ECk_SnapshotInspection_SourceKind::Bytes, InSourceDescription);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Inspect_SaveGameObject(
            UCk_Snapshot_SaveGame* InSaveGame,
            const FString& InSourceDescription)
        -> FCk_SnapshotInspection_Document
    {
        const auto IsOnGameThread = IsInGameThread();
        CK_ENSURE_IF_NOT(IsOnGameThread,
            TEXT("Inspect_SaveGameObject [{}] must be called on the game thread — it resolves reflected types"),
            InSourceDescription)
        { return ck_snapshot_inspection::DoMake_OffGameThreadDocument(ECk_SnapshotInspection_SourceKind::SaveGameObject, InSourceDescription); }

        auto Document = FCk_SnapshotInspection_Document{};
        Document.Set_SourceKind(ECk_SnapshotInspection_SourceKind::SaveGameObject)
                .Set_SourceDescription(InSourceDescription);

        if (ck::Is_NOT_Valid(InSaveGame))
        {
            Document.Set_ReadStatus(ECk_SnapshotInspection_ReadStatus::NotCkSnapshot);
            return Document;
        }

        ck_snapshot_inspection::DoInspect_SaveGame(*InSaveGame, Document);
        return Document;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryDecode_Payload(
            const FCk_SnapshotInspection_Document& InDocument,
            int32 InPayloadIndex,
            ECk_SnapshotInspection_DecodePolicy InPolicy)
        -> FCk_SnapshotInspection_DecodeResult
    {
        const auto IsOnGameThread = IsInGameThread();
        CK_ENSURE_IF_NOT(IsOnGameThread,
            TEXT("TryDecode_Payload must be called on the game thread — it resolves (and may load) a UScriptStruct"))
        {
            return ck_snapshot_inspection::DoMake_FailedDecodeResult(
                ECk_SnapshotInspection_DecodeStatus::DeserializeFailed, TEXT("called off the game thread"));
        }

        const auto IndexIsInRange = InDocument.Get_Payloads().IsValidIndex(InPayloadIndex);
        CK_ENSURE_IF_NOT(IndexIsInRange,
            TEXT("TryDecode_Payload: payload index [{}] is out of range ([{}] payload rows)"),
            InPayloadIndex, InDocument.Get_Payloads().Num())
        {
            return ck_snapshot_inspection::DoMake_FailedDecodeResult(
                ECk_SnapshotInspection_DecodeStatus::DeserializeFailed, TEXT("payload index out of range"));
        }

        const auto& Entry = InDocument.Get_Payloads()[InPayloadIndex].Get_Entry();
        return ck_snapshot_inspection::DoDecode_Blob(Entry.Get_PayloadBytes(), InPolicy, Entry.Get_TypePath());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        TryDecode_SpawnParams(
            const FCk_SnapshotInspection_Document& InDocument,
            int32 InEntityIndex,
            ECk_SnapshotInspection_DecodePolicy InPolicy)
        -> FCk_SnapshotInspection_DecodeResult
    {
        const auto IsOnGameThread = IsInGameThread();
        CK_ENSURE_IF_NOT(IsOnGameThread,
            TEXT("TryDecode_SpawnParams must be called on the game thread — it resolves (and may load) a UScriptStruct"))
        {
            return ck_snapshot_inspection::DoMake_FailedDecodeResult(
                ECk_SnapshotInspection_DecodeStatus::DeserializeFailed, TEXT("called off the game thread"));
        }

        const auto IndexIsInRange = InDocument.Get_Entities().IsValidIndex(InEntityIndex);
        CK_ENSURE_IF_NOT(IndexIsInRange,
            TEXT("TryDecode_SpawnParams: entity index [{}] is out of range ([{}] entity rows)"),
            InEntityIndex, InDocument.Get_Entities().Num())
        {
            return ck_snapshot_inspection::DoMake_FailedDecodeResult(
                ECk_SnapshotInspection_DecodeStatus::DeserializeFailed, TEXT("entity index out of range"));
        }

        const auto& Entry = InDocument.Get_Entities()[InEntityIndex].Get_Entry();
        return ck_snapshot_inspection::DoDecode_Blob(Entry.Get_SpawnParamsBytes(), InPolicy, FString{});
    }
}

// --------------------------------------------------------------------------------------------------------------------
