#include "CkEcs/Snapshot/CkSnapshot_Posture.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Request/CkRequest_Data.h" // FCk_Request_Base — the C++ arm of the request-queue derivation

#include <UObject/PropertyOptional.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_posture
{
    auto Is_ChildOfOrSame(const UScriptStruct* InStruct, const UScriptStruct* InBase) -> bool
    { return InStruct != nullptr && InBase != nullptr && InStruct->IsChildOf(InBase); }

    auto Is_SessionMarkerType(const UScriptStruct* InStruct) -> bool
    { return Is_ChildOfOrSame(InStruct, FCk_Snapshot_Session::StaticStruct()); }

    auto Is_DurableMarkerType(const UScriptStruct* InStruct) -> bool
    { return Is_ChildOfOrSame(InStruct, FCk_Snapshot_Durable::StaticStruct()); }

    auto Is_PostureMarkerType(const UScriptStruct* InStruct) -> bool
    { return Is_SessionMarkerType(InStruct) || Is_DurableMarkerType(InStruct); }

    // Both declaration spellings: derived (C++) or carried as a field (AngelScript structs cannot inherit).
    auto Has_Marker(const UScriptStruct* InStruct, bool InSession) -> bool
    {
        const auto Matches = [InSession](const UScriptStruct* InCandidate) -> bool
        { return InSession ? Is_SessionMarkerType(InCandidate) : Is_DurableMarkerType(InCandidate); };

        if (Matches(InStruct))
        { return true; }

        for (TFieldIterator<FStructProperty> It{InStruct}; It; ++It)
        {
            if (Matches(It->Struct))
            { return true; }
        }

        return false;
    }

    // A request payload type. C++ requests derive FCk_Request_Base; AngelScript requests cannot inherit anything
    // (measured: zero AS structs inherit), so their convention is the reflected type NAME — F<Proj>_Request_<Feature>_<Verb>
    // reflects as <Proj>_Request_<...>. That naming is what separates a queue of genuine request payloads from a
    // queue whose payloads are named for something else and which is therefore a "split me" red instead.
    auto Is_RequestStructType(const UScriptStruct* InStruct) -> bool
    {
        if (InStruct == nullptr)
        { return false; }

        return InStruct->IsChildOf(FCk_Request_Base::StaticStruct()) ||
               InStruct->GetName().Contains(TEXT("Request_"));
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Which leaf kinds a property reaches once containers are peeled and nested structs are walked. Value means
    // "durable data": anything that is not a posture marker, a delegate, or a request payload.
    struct FLeaves
    {
        bool Value    = false;
        bool Delegate = false;
        bool Request  = false;
        bool Marker   = false;

        auto Merge(const FLeaves& InOther) -> void
        {
            Value    |= InOther.Value;
            Delegate |= InOther.Delegate;
            Request  |= InOther.Request;
            Marker   |= InOther.Marker;
        }
    };

    auto Analyze_Struct(const UScriptStruct* InStruct, TSet<const UScriptStruct*>& InOutActiveStructs) -> FLeaves;

    // Same recursion shape as ck_untraced_struct_safety::AnalyzeProperty — the one other TYPE-level property walk
    // in the plugin — including its cycle guard. It answers a different question, so the predicates differ; the
    // traversal must not.
    auto Analyze_Property(const FProperty* InProperty, TSet<const UScriptStruct*>& InOutActiveStructs) -> FLeaves
    {
        // Unknown reflection is treated as durable content: a derivation may never DROP data it cannot prove absent.
        if (InProperty == nullptr)
        { return FLeaves{.Value = true}; }

        if (CastField<const FDelegateProperty>(InProperty) != nullptr ||
            CastField<const FMulticastDelegateProperty>(InProperty) != nullptr)
        { return FLeaves{.Delegate = true}; }

        if (const auto* ArrayProperty = CastField<const FArrayProperty>(InProperty))
        { return Analyze_Property(ArrayProperty->Inner, InOutActiveStructs); }

        if (const auto* SetProperty = CastField<const FSetProperty>(InProperty))
        { return Analyze_Property(SetProperty->ElementProp, InOutActiveStructs); }

        if (const auto* MapProperty = CastField<const FMapProperty>(InProperty))
        {
            auto Leaves = Analyze_Property(MapProperty->KeyProp, InOutActiveStructs);
            Leaves.Merge(Analyze_Property(MapProperty->ValueProp, InOutActiveStructs));
            return Leaves;
        }

        if (const auto* OptionalProperty = CastField<const FOptionalProperty>(InProperty))
        { return Analyze_Property(OptionalProperty->GetValueProperty(), InOutActiveStructs); }

        if (const auto* StructProperty = CastField<const FStructProperty>(InProperty))
        {
            if (Is_PostureMarkerType(StructProperty->Struct))
            { return FLeaves{.Marker = true}; }

            if (Is_RequestStructType(StructProperty->Struct))
            { return FLeaves{.Request = true}; }

            return Analyze_Struct(StructProperty->Struct, InOutActiveStructs);
        }

        return FLeaves{.Value = true};
    }

    auto Analyze_Struct(const UScriptStruct* InStruct, TSet<const UScriptStruct*>& InOutActiveStructs) -> FLeaves
    {
        if (ck::Is_NOT_Valid(InStruct))
        { return FLeaves{.Value = true}; }

        if (InOutActiveStructs.Contains(InStruct))
        { return FLeaves{}; }

        InOutActiveStructs.Add(InStruct);

        auto Leaves = FLeaves{};
        auto HasAnyProperty = false;
        for (TFieldIterator<FProperty> It{InStruct, EFieldIterationFlags::IncludeSuper}; It; ++It)
        {
            HasAnyProperty = true;
            Leaves.Merge(Analyze_Property(*It, InOutActiveStructs));
        }

        InOutActiveStructs.Remove(InStruct);

        // A nested struct with no reflected fields is opaque, not empty: FCk_Entity reflects nothing in a cooked
        // build yet carries the whole identity of a handle. Opaque counts as durable content.
        if (NOT HasAnyProperty)
        { return FLeaves{.Value = true}; }

        return Leaves;
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct FContent
    {
        bool    ReachesDelegate            = false;
        bool    ReachesRequest             = false;
        int32   ValueFieldCount            = 0;
        int32   TransientValueFieldCount   = 0;
        FString FirstValueFieldName;
        FString FirstTransientValueFieldName;
    };

    auto Analyze_Content(const UScriptStruct* InStruct) -> FContent
    {
        auto Content = FContent{};

        for (TFieldIterator<FProperty> It{InStruct, EFieldIterationFlags::IncludeSuper}; It; ++It)
        {
            const auto* Property = *It;

            auto ActiveStructs = TSet<const UScriptStruct*>{};
            const auto Leaves = Analyze_Property(Property, ActiveStructs);

            Content.ReachesDelegate |= Leaves.Delegate;
            Content.ReachesRequest  |= Leaves.Request;

            if (NOT Leaves.Value)
            { continue; }

            ++Content.ValueFieldCount;
            if (Content.FirstValueFieldName.IsEmpty())
            { Content.FirstValueFieldName = Property->GetName(); }

            // Field-level UPROPERTY(Transient) is retired as an author-facing persistence opt-out: on a Durable
            // fragment it is a "split the fragment" red. Top level only — FCk_Handle's own internals are
            // Transient by design and must not make every handle-bearing fragment red.
            if (Property->HasAnyPropertyFlags(CPF_Transient))
            {
                ++Content.TransientValueFieldCount;
                if (Content.FirstTransientValueFieldName.IsEmpty())
                { Content.FirstTransientValueFieldName = Property->GetName(); }
            }
        }

        return Content;
    }

    auto Get_NameEndsWithRequests(const UScriptStruct* InStruct) -> bool
    {
        // The looser suffix (not "_Requests") deliberately catches spellings like ...SuppressRequests /
        // ...TransactionRequests, while still missing a fragment that merely HOLDS a request beside durable state.
        return InStruct->GetName().EndsWith(TEXT("Requests"), ESearchCase::CaseSensitive);
    }

    auto Describe_DerivedShape(bool InIsTag, bool InReachesDelegate, bool InIsRequestQueue) -> FString
    {
        if (InIsTag)
        { return TEXT("it has no reflected field other than a posture marker (a tag)"); }

        if (InReachesDelegate)
        { return TEXT("its type walk reaches a delegate property"); }

        if (InIsRequestQueue)
        { return TEXT("it is a request queue (name ends in 'Requests', or it reaches an FCk_Request_Base)"); }

        return TEXT("<no derived shape>");
    }

    // OutDidEnsure reports whether this resolution fired an ensure, which is what makes it uncacheable (see
    // ck::Resolve_FragmentPosture).
    auto Resolve(const UScriptStruct* InStructType, bool& OutDidEnsure) -> ck::FCk_Snapshot_PostureResolution
    {
        using EPosture = ECk_Snapshot_Posture;

        OutDidEnsure = false;

        const auto HasSession = Has_Marker(InStructType, /*InSession=*/true);
        const auto HasDurable = Has_Marker(InStructType, /*InSession=*/false);

        // 1 — a contradiction. Never resolved in favour of either marker.
        if (HasSession && HasDurable)
        {
            OutDidEnsure = true;
            CK_TRIGGER_ENSURE(TEXT("Snapshot posture: [{}] declares BOTH FCk_Snapshot_Durable and "
                "FCk_Snapshot_Session. A fragment has exactly one posture — remove one marker."), InStructType);

            return {.Posture = EPosture::Undeclared, .Reason = TEXT("declares both posture markers")};
        }

        // 2 — a Session declaration is final: there is no shape that makes session state durable.
        if (HasSession)
        { return {.Posture = EPosture::Session, .Reason = TEXT("declares FCk_Snapshot_Session")}; }

        const auto Content = Analyze_Content(InStructType);

        const auto IsTag = Content.ValueFieldCount == 0 && NOT Content.ReachesDelegate && NOT Content.ReachesRequest;
        const auto IsRequestQueue = Get_NameEndsWithRequests(InStructType) || Content.ReachesRequest;
        const auto MatchesDerivedSession = IsTag || Content.ReachesDelegate || IsRequestQueue;
        const auto DerivedShape = Describe_DerivedShape(IsTag, Content.ReachesDelegate, IsRequestQueue);

        // 3 — a Durable declaration, checked against the derivations. A declaration never overrides a derivation:
        // the derivation is a proof that the declared posture is unachievable, not a competing opinion.
        if (HasDurable)
        {
            if (MatchesDerivedSession)
            {
                OutDidEnsure = true;
                CK_TRIGGER_ENSURE(TEXT("Snapshot posture: [{}] declares FCk_Snapshot_Durable, but {} — that content "
                    "cannot round-trip. Split the fragment: the durable fields in one Durable fragment, the "
                    "session content in a Session one."), InStructType, DerivedShape);

                return {
                    .Posture       = EPosture::Undeclared,
                    .Reason        = FString::Printf(TEXT("declares Durable but %s"), *DerivedShape),
                    .RequiresSplit = true};
            }

            if (Content.TransientValueFieldCount > 0)
            {
                OutDidEnsure = true;
                CK_TRIGGER_ENSURE(TEXT("Snapshot posture: [{}] declares FCk_Snapshot_Durable and carries "
                    "UPROPERTY(Transient) field [{}]. Field-level opt-out is retired — split the fragment so each "
                    "half declares one posture."), InStructType, Content.FirstTransientValueFieldName);

                return {
                    .Posture       = EPosture::Undeclared,
                    .Reason        = FString::Printf(TEXT("declares Durable and carries UPROPERTY(Transient) field [%s]"),
                                        *Content.FirstTransientValueFieldName),
                    .RequiresSplit = true};
            }

            return {.Posture = EPosture::Durable, .Reason = TEXT("declares FCk_Snapshot_Durable")};
        }

        // 4 — derived. Mixed content is a red, never a silent reclassification: a fragment matching the
        // request-queue shape can still hold a real accumulator, and a silent derive would have deleted it.
        if (MatchesDerivedSession)
        {
            if (Content.ValueFieldCount > 0)
            {
                return {
                    .Posture       = EPosture::Undeclared,
                    .Reason        = FString::Printf(
                                        TEXT("SPLIT ME — %s, yet it also carries [%d] value field(s) (first: [%s])"),
                                        *DerivedShape, Content.ValueFieldCount, *Content.FirstValueFieldName),
                    .RequiresSplit = true};
            }

            return {
                .Posture = EPosture::Session,
                .Reason  = FString::Printf(TEXT("derived Session — %s"), *DerivedShape)};
        }

        // 5
        return {
            .Posture = EPosture::Undeclared,
            .Reason  = FString::Printf(TEXT("no posture declared and no derivation applies ([%d] value field(s))"),
                          Content.ValueFieldCount)};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::
    Resolve_FragmentPosture(
        const UScriptStruct* InStructType)
    -> ck::FCk_Snapshot_PostureResolution
{
    if (ck::Is_NOT_Valid(InStructType))
    { return {.Posture = ECk_Snapshot_Posture::Undeclared, .Reason = TEXT("struct type is invalid")}; }

    // Keyed by weak pointer so an AngelScript live-reload — which replaces UScriptStruct objects, potentially at a
    // recycled address — re-computes instead of serving a stale posture. Game-thread-only, matching every caller
    // (same rationale as UCk_Utils_DynamicFragment_UE::Get_StorageId's cache).
    auto DidEnsure = false;

    const auto IsGameThreadLookup = IsInGameThread();
    CK_ENSURE_IF_NOT(IsGameThreadLookup, TEXT("Snapshot postures must be resolved on the game thread"))
    { return ck_snapshot_posture::Resolve(InStructType, DidEnsure); }

    static TMap<TWeakObjectPtr<const UScriptStruct>, FCk_Snapshot_PostureResolution> Cache;

    if (const auto* Found = Cache.Find(InStructType))
    { return *Found; }

    auto Resolution = ck_snapshot_posture::Resolve(InStructType, DidEnsure);

    // A resolution that ENSURED is deliberately NOT cached: it is an author error, and it must stay loud on every
    // later resolution rather than going quiet after the first one. Derived "split me" reds do not ensure, so a
    // project's whole outstanding split list is cached and costs nothing per capture.
    if (NOT DidEnsure)
    { Cache.Add(InStructType, Resolution); }

    return Resolution;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::
    Get_FragmentPosture(
        const UScriptStruct* InStructType)
    -> ECk_Snapshot_Posture
{
    return Resolve_FragmentPosture(InStructType).Posture;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::
    Get_DurablePayloadObjectRefPolicy()
    -> FCk_UntracedStructSafety_Policy
{
    return FCk_UntracedStructSafety_Policy
    {
        .ClassRefs = ECk_UntracedStructSafety_ClassRefs::Accept,
        .GcTracedStructs = ECk_UntracedStructSafety_GcTracedStructs::TreatAsBoundary,
    };
}
