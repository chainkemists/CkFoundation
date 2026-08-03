# Worked example — a complete minimal feature

Reference for `ck-game-feature-recipe`: the full file set for one minimal feature, in build order.

## 7. Worked example — a complete minimal feature

**Corpus example (generalized from BusterBlock Entryway — commits `582bddd26`, `345903a10`,
`287ee6601` — and Trashcan — `4713da549`; every shape below matches code read in the working
tree 2026-07-03).** Replace `<Prefix>`/`<prefix>` with your project prefix (`Bb`/`bb`, `Vns`,
...) and `<Game>` with your game name. The feature: **Openable** — anything with an open/closed
state, a request to change it, and a signal when it changes.

### `Script/ECS/Openable/<Prefix>_Openable_Feature.as`

```angelscript
//--------------------------------------------------------------------------------------------------------------------------
// Dynamic Handle Definition
//--------------------------------------------------------------------------------------------------------------------------

asset OpenableHandle of UCkDynamic_HandleDefinition
{
    TypeName = "FCk_Handle_Openable";
    RequiredFragments.Add(F<Prefix>_Feature_Openable);
    Description = "Anything with an open/closed state: request a state change, get a signal when it lands.";
}
struct F<Prefix>_Feature_Openable {}

//--------------------------------------------------------------------------------------------------------------------------
// Params — gameplay-only. Visuals (mesh, animation, sounds) belong to the
// spawn vehicle (U<Prefix>_Openable_EntityScript), not the feature. Tests can
// compose utils_openable::Add directly with no asset dependencies.
//--------------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct F<Prefix>_Fragment_Openable_Params
{
    UPROPERTY()
    bool StartOpen = false;
}

//--------------------------------------------------------------------------------------------------------------------------
// State (mutable, owned by the processors)
//--------------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct F<Prefix>_Fragment_Openable_State
{
    UPROPERTY()
    bool IsOpen = false;
}

//--------------------------------------------------------------------------------------------------------------------------
// Requests — payload structs wrapped in TOptional. Keep even an empty payload
// as a struct (not a bare tag) so future fields don't churn callers.
//--------------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct F<Prefix>_Request_Openable_SetOpen
{
    UPROPERTY()
    bool Open = true;
}

USTRUCT()
struct F<Prefix>_Fragment_Openable_Requests
{
    UPROPERTY()
    TOptional<F<Prefix>_Request_Openable_SetOpen> SetOpenRequest;
}

//--------------------------------------------------------------------------------------------------------------------------
// Signals — delegate (bindable) + event `_MC` (multicast storage) pair per
// signal, gathered into a Signals fragment.
//--------------------------------------------------------------------------------------------------------------------------

delegate void F<Prefix>_Delegate_Openable_OnOpenStateChanged(FCk_Handle_Openable InOpenable, bool InIsOpen);
event void F<Prefix>_Delegate_Openable_OnOpenStateChanged_MC(FCk_Handle_Openable InOpenable, bool InIsOpen);

USTRUCT()
struct F<Prefix>_Fragment_Openable_Signals
{
    F<Prefix>_Delegate_Openable_OnOpenStateChanged_MC OnOpenStateChanged;
}
```

New handle checklist (mechanics in `ck-angelscript-interop`): land the asset + marker + first
consumer in one edit, boot the editor, expect first-pass "not a data type" transients, gate on
the post-regen clean reload, commit the regenerated `Script/Generated/DynamicHandleTypes.json`.

### `Script/ECS/Openable/<Prefix>_Openable_Utils.as`

```angelscript
namespace utils_openable
{
    // Composes Openable onto a transform-bearing entity.
    FCk_Handle_Openable Add(FCk_Handle_Transform& InTransform, F<Prefix>_Fragment_Openable_Params InParams,
                            ECk_Replication InReplication = ECk_Replication::Replicates)
    {
        auto _CkPerfScope = ck::ScopedStat();

        InTransform.Add_Fragment(F<Prefix>_Feature_Openable());
        InTransform.Add_Fragment(InParams);

        auto State = F<Prefix>_Fragment_Openable_State();
        State.IsOpen = InParams.StartOpen;
        InTransform.Add_Fragment(State);

        // Discovery contract: <name the consumer here — e.g. "the WorldDriver
        // scans for TAG_<Prefix>Openable and binds OnOpenStateChanged on each
        // match">. Composition inside this Add is synchronous, so stamping
        // here is safe; if you ever defer composition, move this stamp to the
        // finalize step — the tag is a promise the feature contract holds.
        utils_entity_tag::Add(InTransform, n"TAG_<Prefix>Openable");

        return InTransform.As_Openable();
    }
}

//--------------------------------------------------------------------------------------------------------------------------
// Getters
//--------------------------------------------------------------------------------------------------------------------------

mixin bool Get_IsOpen(const FCk_Handle_Openable& Self)
{
    auto _CkPerfScope = ck::ScopedStat();
    return Self.Get_Fragment(F<Prefix>_Fragment_Openable_State).IsOpen;
}

//--------------------------------------------------------------------------------------------------------------------------
// Operations — enqueue only; the processor mutates next tick (root doctrine #5)
//--------------------------------------------------------------------------------------------------------------------------

mixin void Request_SetOpen(FCk_Handle_Openable& Self, bool InOpen)
{
    auto _CkPerfScope = ck::ScopedStat();
    auto& Requests = Self.AddOrGet_Fragment(F<Prefix>_Fragment_Openable_Requests);
    auto Request = F<Prefix>_Request_Openable_SetOpen();
    Request.Open = InOpen;
    Requests.SetOpenRequest = Request;
}

//--------------------------------------------------------------------------------------------------------------------------
// Signal Binding (lazy — Signals fragment is added on first BindTo_*)
//--------------------------------------------------------------------------------------------------------------------------

mixin void BindTo_OnOpenStateChanged(FCk_Handle_Openable& Self, F<Prefix>_Delegate_Openable_OnOpenStateChanged InDelegate)
{
    auto _CkPerfScope = ck::ScopedStat();
    auto& Signals = Self.AddOrGet_Fragment(F<Prefix>_Fragment_Openable_Signals);
    Signals.OnOpenStateChanged.AddUFunction(InDelegate.GetUObject(), InDelegate.GetFunctionName());
}

mixin void UnbindFrom_OnOpenStateChanged(FCk_Handle_Openable& Self, F<Prefix>_Delegate_Openable_OnOpenStateChanged InDelegate)
{
    auto _CkPerfScope = ck::ScopedStat();
    if (Self.Has_Fragment(F<Prefix>_Fragment_Openable_Signals) == false)
    { return; }

    auto& Signals = Self.Get_Fragment(F<Prefix>_Fragment_Openable_Signals);
    Signals.OnOpenStateChanged.Unbind(InDelegate.GetUObject(), InDelegate.GetFunctionName());
}
```

(The `auto _CkPerfScope = ck::ScopedStat();` opener on every function body is project-wide
instrumentation in the corpus — adopt `ck::ScopedStat()` project-wide or not at all; the corpus
does. `[UNDER ADJUDICATION — see CkFoundation .claude/reports/ADJUDICATIONS.md A6]`)

### `Script/ECS/Openable/<Prefix>_Openable_Processor_HandleRequest.as`

```angelscript
// Drains pending SetOpen requests. Clears the request fragment BEFORE
// broadcasting so re-entrant Request_SetOpen calls from signal handlers
// survive.

class U<Prefix>_Processor_Openable_HandleRequest : UCk_Processor_Script_Base_UE
{
    default _Group = n"FGroup_Gameplay_Script";
    default _MarkedDirtyBy = F<Prefix>_Fragment_Openable_Requests;

    UFUNCTION(BlueprintOverride)
    void Tick(FCk_Time DeltaSeconds)
    {
        auto _CkPerfScope = ck::ScopedStat();
        _Handle.ForEach_EntityWithTwoFragments(
            F<Prefix>_Fragment_Openable_Requests, F<Prefix>_Feature_Openable,
            FCk_DynamicFragment_ForEachEntity(this, n"ForEach_Requests"));
    }

    UFUNCTION()
    private void ForEach_Requests(FCk_Handle& InHandle)
    {
        auto _CkPerfScope = ck::ScopedStat();
        const auto& Requests = InHandle.Get_Fragment(F<Prefix>_Fragment_Openable_Requests);
        const auto Pending = Requests.SetOpenRequest;

        // Clear BEFORE broadcasting so re-entrant requests survive.
        InHandle.Request_TryRemove(F<Prefix>_Fragment_Openable_Requests);

        if (Pending.IsSet() == false)
        { return; }

        auto Self = InHandle.As_Openable();
        auto& State = Self.Get_Fragment(F<Prefix>_Fragment_Openable_State);

        if (State.IsOpen == Pending.Get().Open)
        { return; }

        State.IsOpen = Pending.Get().Open;

        if (Self.Has_Fragment(F<Prefix>_Fragment_Openable_Signals) == false)
        { return; }

        auto& Signals = Self.Get_Fragment(F<Prefix>_Fragment_Openable_Signals);
        Signals.OnOpenStateChanged.Broadcast(Self, State.IsOpen);
    }
}
```

### `Script/ECS/Openable/<Prefix>_Openable_EntityScript.as`

```angelscript
// Spawn vehicle for a placed Openable. Composes the gameplay feature via
// utils_openable::Add and — separately — wires any visuals. Tests/headless
// callers compose the feature directly instead and never touch this class.

class U<Prefix>_Openable_EntityScript : UCk_GenericEntityScript_UE
{
    default _Replication = ECk_Replication::Replicates;

    UPROPERTY(ExposeOnSpawn)
    FTransform SpawnTransform = FTransform::Identity;

    UPROPERTY(ExposeOnSpawn)
    F<Prefix>_Fragment_Openable_Params Params;

    UFUNCTION(BlueprintOverride)
    ECk_EntityScript_ConstructionFlow DoConstruct(FCk_Handle& InHandle)
    {
        auto _CkPerfScope = ck::ScopedStat();
        auto TransformHandle = utils_transform::Add(InHandle, SpawnTransform, ECk_Replication::Replicates);
        utils_openable::Add(TransformHandle, Params);

        // Visuals go HERE (ISM proxy / mesh / world-space widget), never in
        // the feature. World-dependent bring-up (local player, day cycle)
        // belongs in DoBeginPlay behind a readiness promise, not here.

        return ECk_EntityScript_ConstructionFlow::Finished;
    }
}
```

`ExposeOnSpawn` fields feed the auto-generated positional `::Params(...)` constructor —
**append-only**: adding/removing/reordering fields breaks every positional caller
(`ck-game-angelscript-gameplay` owns the detail).

### `<TestPlugin>/Script/Tests/Openable/<Prefix>_AutoTest_Openable_OpenCloseSignal.as`

```angelscript
// Verifies:
//   1. utils_openable::Add composes State from Params (StartOpen honored).
//   2. Request_SetOpen(true) -> OnOpenStateChanged(true) fires next tick.
// Out of scope (separate test files): driver discovery/binding; replication.

class U<Prefix>_AutoTest_Openable_OpenCloseSignal : UCk_AutoTest_Base
{
    private FCk_Handle_Openable OpenableHandle;

    UFUNCTION(BlueprintOverride)
    void DoBeginPlay(FCk_Handle InHandle)
    {
        auto _CkPerfScope = ck::ScopedStat();
        auto LocalHandle = InHandle;

        auto TransformHandle = utils_transform::Add(
            LocalHandle, FTransform::Identity, ECk_Replication::DoesNotReplicate);

        auto Params = F<Prefix>_Fragment_Openable_Params();
        Params.StartOpen = false;

        OpenableHandle = utils_openable::Add(
            TransformHandle, Params, ECk_Replication::DoesNotReplicate);

        Assert_True(OpenableHandle.Get_IsOpen() == false, "StartOpen=false honored");

        OpenableHandle.BindTo_OnOpenStateChanged(
            F<Prefix>_Delegate_Openable_OnOpenStateChanged(this, n"OnOpenStateChanged"));

        // Deferred-request discipline: do NOT assert Get_IsOpen() here — the
        // mutation lands when the processor runs. The signal IS the read-back.
        OpenableHandle.Request_SetOpen(true);
    }

    UFUNCTION()
    private void OnOpenStateChanged(FCk_Handle_Openable InOpenable, bool InIsOpen)
    {
        auto _CkPerfScope = ck::ScopedStat();
        if (IsFinished()) { return; }

        Assert_True(InIsOpen, "Signal reports the new state");
        Assert_True(OpenableHandle.Get_IsOpen(), "State fragment updated before broadcast");
        FinishSuccess();
    }
}
```

Ship it in the §3 order: Feature.as + registry JSON → Utils + this test (same commit) →
processor + EntityScript → driver integration + e2e test → chore(generated) → Content BP last.

---

