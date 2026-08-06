//============================================================================
// ANGELSCRIPT GUIDELINES — CkFoundation framework reference
//============================================================================
//
// AngelScript (.as) development against CkFoundation: language deltas from
// C++, the utils_* layer, dynamic handles, generated-script hygiene. Style,
// naming, macros, lingo, and the non-negotiables live in the root doctrine
// (Plugins/CkFoundation/CLAUDE.md) and are NOT restated here. C++ module
// topology: Source/CLAUDE.md. Engine: UnrealEngine-Angelscript 5.7.x
// (Hazelight fork) — see root CLAUDE.md "Identity" (verified 2026-07-02).
// Do NOT assume framework behavior — verify against existing .as files, or
// ask. Research → Plan → Implement (root doctrine, Collaboration protocol).

//============================================================================
// 1. LANGUAGE DIFFERENCES FROM C++
//============================================================================
//
// - No lambdas.
// - No public:/private: sections. Functions are public by default; mark
//   individual functions `private` (e.g. `private void Foo()`).
// - No `NOT` macro — use `!` or `== false` (prefer `== false` for readability).
// - No `FMath::` — use `Math::` (from FMath bindings; UKismetMathLibrary is
//   skipped).
// - No static member functions. Use global functions instead.
// - No static_cast. Use direct casting: `uint8(value)`.
// - No `->` arrow operator. All UObjects are references; always use `.` dot.
// - No pointers. UObject vars are automatic references. `UPROPERTY()` is NOT
//   needed for GC protection (unlike C++).
// - No constructors in classes. Use `default Prop = value;` and
//   `UPROPERTY(DefaultComponent) ...` (see §8). Structs MAY have constructors.
// - No `ck::IsValid_Policy_NullptrOnly{}` — only basic `ck::IsValid(x)` and
//   `ck::Is_NOT_Valid(x)`.
// - `float` is 64-bit double (UE5 large-world coords). Use `float32` when you
//   explicitly need 32-bit.
// - `UPROPERTY()` defaults to `EditAnywhere + BlueprintReadWrite`. Restrict
//   with `NotEditable`, `EditDefaultsOnly`, `VisibleAnywhere`,
//   `BlueprintReadOnly`, `BlueprintHidden`, etc.
// - `UFUNCTION()` defaults to `BlueprintCallable`. Use `NotBlueprintCallable`
//   to hide.
// - RPCs are RELIABLE by default (opposite of C++). Add `Unreliable` if
//   needed.
// - `UFUNCTION()` on struct methods is not supported — structs get plain
//   methods only.

//============================================================================
// 2. VARIABLES, CASTING, STRINGS
//============================================================================

// auto everywhere except UPROPERTY
auto MyVar = SomeFunction();
auto Handle = ck::ToEntity(this);   // see §5 for the ToEntity overloads

UPROPERTY()
UCk_IsmRenderer_Data SomeRendererAsset;

// Float precision
float ValueDouble = 1.0;         // 64-bit
float32 ValueSingle = 1.f;       // 32-bit when you need it
float64 ValueAlsoDouble = 1.0;   // explicit 64-bit

// Type casting — direct, NOT static_cast
auto ArmorValue = uint8(Math::Clamp(V * 2.0f, 0.0f, 255.0f));

// String formatting — f"" interpolation; NO `+=` or `+` for strings.
// NEVER put a raw FCk_Handle in an f-string — runtime throw, see §22.1.
auto DisplayText = "=== ATTRIBUTES ===\n";
DisplayText = f"{DisplayText}Health: {HP} (Base: {Base})\n";

// Advanced format specifiers
auto Debug     = f"{DeltaSeconds =}";              // "DeltaSeconds = 0.01"
auto Precise   = f"{Value :.3}";                   // 3 decimals ( :010d pad, :#x hex, :>40 right-align)
auto EnumName  = f"{ESlateVisibility::Collapsed :n}";  // "Collapsed"

//============================================================================
// 3. VALIDITY & BOOLEAN LOGIC
//============================================================================

if (ck::IsValid(SomePointer)) { /* ... */ }
if (ck::Is_NOT_Valid(SomePointer)) { return; }
if (IsEnabled == false) { return; }   // preferred
if (!IsEnabled) { return; }           // OK
bool IsAlive = true;                  // NO `b` prefix (not bIsAlive)

//============================================================================
// 4. ENTITY SCRIPT LIFECYCLE
//============================================================================
//
// C++ hooks are BlueprintImplementableEvents on UCk_GenericEntityScript_UE
// (Source/CkEcs/Public/CkEcs/EntityScript/CkGenericEntityScript.h:48,55,62,69);
// AS subclasses override them with UFUNCTION(BlueprintOverride).

UFUNCTION(BlueprintOverride)
ECk_EntityScript_ConstructionFlow DoConstruct(FCk_Handle& InHandle)
{
    // Add fragments / child entities here.
    return ECk_EntityScript_ConstructionFlow::Finished;
    // Return ::Continue instead to defer BeginPlay — you must then call
    // DoFinishConstruction() when ready (enum: CkEntityScript.h), and
    // DoContinueConstruction(FCk_Handle) is the hook for that deferred path.
}

UFUNCTION(BlueprintOverride)
void DoBeginPlay(FCk_Handle InHandle) { /* use the param, not ck::ToEntity(this) */ }

UFUNCTION(BlueprintOverride)
void DoEndPlay(FCk_Handle InHandle) { /* unbind signals (auto if bound with PostFireBehavior::Unbind) */ }

//============================================================================
// 5. FRAMEWORK API — UTILS SHORTCUTS
//============================================================================
//
// ALWAYS use utils_* shortcuts. NEVER the full UCk_Utils_X_UE:: names.
// Grounding: FCkAngelscriptWrapperGenerator emits 268 generated namespaces at
// editor boot (Script/Generated/utils_<feature>.as, one per UCk_Utils_<Feature>_UE);
// hand-written Script/CkUtils_*.as files merge extra sugar into the same
// namespaces. Not just style: a function whose first param type matches its
// class's ScriptMixin target binds as a HANDLE MEMBER only — the static form
// does not even resolve for it; the utils_* wrapper works uniformly (it
// forwards to the member form when needed, e.g. Generated/utils_timer.as:199).

auto SelfEntity = ck::ToEntity(this);
// Two overloads (Script/CkUtils_Common.as:5,10): ck::ToEntity(const AActor) /
// ck::ToEntity(const UCk_EntityScript_UE). Replaces the REMOVED ck::SelfEntity
// — the old name fails to compile (§21).

// Transform
utils_transform::Add(Handle, Transform, ReplicationMode);
auto Xf = utils_transform::Get_EntityCurrentTransform(TransformHandle);
// NOT utils_transform::Get_WorldTransform(...)

// Entity tags — find entities later
utils_entity_tag::Add(InHandle, n"TAG_MyEntity");
auto Entities = utils_entity_tag::ForEach_Entity(SelfEntity, n"TAG_MyEntity");

// Debug draw — NO World param in AS
utils_debug_draw::DrawDebugString(Pos, Text, nullptr, FLinearColor::White, 0.0f);

// Entity script retrieval + typed cast
auto Script = utils_entity_script::TryGet_EntityScript(EntityHandle);
auto Typed  = Cast<UMyEntityScript>(Script);

// Asset loading
utils_i_o::LoadAssetByName(
    "/CkTests/CkAudio/SFX/Ambient_Edm_SFX.Ambient_Edm_SFX",
    ECk_AssetSearchScope::Plugins);
// Scopes: Game (default), Plugins, Engine, All. Optional 3rd param
// ECk_AssetSearchStrategy (default ExactThenFuzzy) — Generated/utils_i_o.as:32.

// BAD (never): UCk_Utils_Probe_UE::Add(...), UCk_Utils_AudioDirector_UE::Request_StartTrack(...)

//============================================================================
// 6. HANDLE CONVERSIONS
//============================================================================

auto SelfEntity         = ck::ToEntity(this);
auto TransformHandle    = SelfEntity.To_FCk_Handle_Transform();
auto ProbeHandle        = SelfEntity.To_FCk_Handle_Probe();
auto AudioDirectorH     = SelfEntity.To_FCk_Handle_AudioDirector();

// Parent-chain implicit conversion (typesafe handle hierarchies):
//   FCk_Handle_Inventory_DataOnly  -->  FCk_Handle_Inventory  -->  FCk_Handle
//   A derived typesafe handle implicitly converts to ANY parent typesafe handle in
//   its inheritance chain — pass a _DataOnly where FCk_Handle_Inventory& is expected
//   without an explicit As_Inventory(...). Parent-utility methods (Get_*, Request_*)
//   are also propagated onto the derived handle in AS via the mixin pass.
//
// Validation note: implicit parent conversion is UNCHECKED — it forwards the
// bytes as-is; no CastChecked / fragment-presence ensure runs at the call
// boundary (the bound opImplConv is a pass-through:
// CkHandle_TypeSafe_AngelScript.h:48-54). The downstream util ensures when it
// touches state. Use As_Parent() explicitly when you want the boundary
// diagnostic (e.g. when the source handle's fragment-presence is uncertain).

//============================================================================
// 7. DYNAMIC HANDLE REGISTRATION (CRITICAL GOTCHA)
//============================================================================
//
// Declaring a typesafe handle via `asset <X>Handle of UCkDynamic_HandleDefinition`
// is NOT sufficient on its own. A registry JSON — DynamicHandleTypes.json —
// must contain a matching entry, or AS compilation fails at engine startup —
//   Identifier 'FCk_Handle_<X>' is not a data type in global namespace
// — in every file referencing the type (feature, utils, processor, HFSM).
//
// Registry path: UCk_Utils_Dynamic_Settings_UE::Get_DynamicHandleRegistryFilePath()
// (Source/CkDynamic/Public/CkDynamic/Settings/CkDynamic_Settings.cpp; default
// dir = ProjectConfigDir). Superprojects usually override — BusterBlock,
// Config/DefaultCkFoundation.ini:118:
//   [/Script/CkDynamic.Ck_Dynamic_ProjectSettings_UE]
//   _DynamicHandleRegistryDirectory=(Path="../../Script/Generated")
// → <Project>/Script/Generated/DynamicHandleTypes.json.

// Example declaration:
//   asset CheckoutCounterHandle of UCkDynamic_HandleDefinition
//   {
//       TypeName = "FCk_Handle_CheckoutCounter";
//       RequiredFragments.Add(FBb_Feature_CheckoutCounter);
//       Description = "...";
//   }
// Matching JSON entry:
//   { "TypeName": "FCk_Handle_CheckoutCounter", "ShortName": "CheckoutCounter",
//     "Description": "...",
//     "SourceAsset": "/Script/AngelscriptAssets.CheckoutCounterHandle",
//     "RequiredFragments": ["Bb_Feature_CheckoutCounter"] }
// Gotchas: RequiredFragments drops the `F` prefix (FBb_Feature_X → Bb_Feature_X);
// SourceAsset follows `/Script/AngelscriptAssets.<Name>Handle`; the file is
// UTF-16 LE — preserve the encoding if you ever touch it by hand.

// ADDING A NEW HANDLE — ONE EDIT (self-heal is default-on): land declaration
// + empty feature struct (FBb_Feature_<X> {}) + first consumer (e.g. a utils
// Add returning As_<X>()) in ONE edit, then boot the editor. Observed cycle
// (canonical: Source/CkAngelscriptGenerator/Claude.md, DynamicHandle row):
//   1. First-pass compile FAILS: "not a data type" + consumer no-match +
//      "Hot reload failed ... Keeping all old script code".
//   2. Self-heal writes Script/Generated/_StubRecovery_DynamicHandleTypes.json
//      + a permissive validator; logs "Self-heal recovered: FCk_Handle_<X>".
//   3. Deferred regen (OnPostEngineInit) writes the REAL JSON entry (sorted by
//      TypeName, UTF-16 LE, F-prefix stripped), removes the stub, goes strict.
//   4. The recompile pass compiles the consumer clean. No restart needed.
// The first-pass errors are EXPECTED TRANSIENTS of the cycle, not a failure.
// Success gate = a clean reload AFTER the deferred regen + the entry present
// in DynamicHandleTypes.json — NOT the absence of first-pass errors.

// MANUAL REGEN: UCkDynamicHandleSubsystem (editor subsystem) CallInEditor buttons:
//   GenerateHandleTypeRegistry()        — rewrite JSON from all definitions
//   ForceRefreshDynamicHandleBindings() — regen + re-register live AS bindings
//                                         without an editor restart (dev-only)
// Source: Source/CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.{h,cpp}

// EDGE CASE — self-heal DISABLED (`-NoCkAsRegen`, or
// `_EnableAsBootstrapSelfHeal = false` in CkFoundation.ini): with the editor
// CLOSED, hand-add the JSON entry (keep UTF-16 LE), or once AS is healthy run
// GenerateHandleTypeRegistry() + restart (or ForceRefreshDynamicHandleBindings()).
// HISTORY: the old "unrecoverable lockup" that mandated a two-phase split only
// meant unrecoverable via in-editor hot reload — closing the editor and
// hand-adding the JSON always recovered; self-heal now automates exactly that,
// so the one-edit path is the normal path.

//============================================================================
// 8. ACTORS & COMPONENTS
//============================================================================
//
// NO constructors. Use `UPROPERTY(DefaultComponent)` + `default` keyword.

class AMyActor : AActor
{
    UPROPERTY(DefaultComponent, RootComponent)
    USceneComponent SceneRoot;

    UPROPERTY(DefaultComponent, Attach = SceneRoot)   // AttachSocket = <Name> also supported
    UStaticMeshComponent Mesh;

    // Defaults — NOT constructors
    default SceneRoot.bGenerateOverlapEvents = true;
    default Mesh.bHiddenInGame = false;

    UPROPERTY()
    float ConfigurableValue = 5.0;
}

// Override parent components
class AChildActor : ABaseActor
{
    UPROPERTY(OverrideComponent = SceneRoot)
    UStaticMeshComponent RootStaticMesh;
}

// Retrieve components
auto Skel    = USkeletalMeshComponent::Get(Actor);                  // null if not found
auto Named   = USkeletalMeshComponent::Get(Actor, n"WeaponMesh");   // by name
auto Interact= UInteractionComponent::GetOrCreate(Actor);
auto New     = UStaticMeshComponent::Create(Character);             // always new

// Spawn actors — class literal or a TSubclassOf<...> UPROPERTY both work
auto Spawned = SpawnActor(AMyActor, SpawnLocation, SpawnRotation);

// Query
TArray<UStaticMeshComponent> Meshes;   Actor.GetComponentsByClass(Meshes);
TArray<ANiagaraActor> Niagaras;        GetAllActorsOfClass(Niagaras);

// Construction script
UFUNCTION(BlueprintOverride)
void ConstructionScript()
{
    auto M = UStaticMeshComponent::Create(this);
    M.SetStaticMesh(MeshAsset);
}

//============================================================================
// 9. STRUCTS, DELEGATES, EVENTS
//============================================================================

// Structs — value types. Plain methods only (no UFUNCTION on struct methods).
struct FMyStruct
{
    UPROPERTY() float Value = 4.0;
    UPROPERTY() FString Name = "Default";

    void Reset() { Value = 0.0; Name = ""; }
}

// Struct params are implicitly const& (read-only).
UFUNCTION() float GetValue(FMyStruct Struct) { return Struct.Value; }
// `&` for mutable, `&out` for output (creates BP output pin)
UFUNCTION() void Randomize(FMyStruct& S)               { S.Value = Math::RandRange(0.0, 1.0); }
UFUNCTION() void Build(FMyStruct&out O, bool&out bOK)  { O.Value = 42.0; bOK = true; }

// delegate = single-cast; event = multicast. Bound funcs need UFUNCTION().
delegate void FMyDelegate(UObject Object, float Value);
event    void FMyEvent(int Counter);

StoredDelegate = FMyDelegate(this, n"OnDelegateExecuted");  // or .BindUFunction(this, n"...")
StoredDelegate.ExecuteIfBound(this, DeltaSeconds);

UPROPERTY() FMyEvent OnSomethingHappened;
OnSomethingHappened.AddUFunction(this, n"HandleEvent");
OnSomethingHappened.Broadcast(CallCounter);

//============================================================================
// 9.1 BY-VALUE STRUCT PARAMS ARE READ-ONLY (GOTCHA)
//============================================================================
//
// AS refuses to assign to any member of a by-value struct parameter, at any
// depth. Treat by-value struct params as read-only. Symptom:
//   Cannot assign, variable is const or is not a valid l-value

// ❌ Won't compile — direct OR nested member assignment both fail
void Foo(FSomeParams InParams)
{
    InParams.Probe.LocalOffset = FTransform(FVector(0, -75, 0)); // error
    InParams.Probe             = SomeValue;                      // also error
}

// ✓ Option A: build a fresh local with resolved values, use that
void Foo(FSomeParams InParams)
{
    auto ProbeParams = InParams.Probe;
    ProbeParams.LocalOffset = FTransform(FVector(0, -75, 0));

    auto Resolved = FSomeParams();
    Resolved.Probe = ProbeParams;   // copy anything else needed too
}

// ✓ Option B: take the param by reference — caller needs a mutable var
void Foo(FSomeParams& InParams) { InParams.Probe = SomeValue; }

//============================================================================
// 9.2 CONST PROPAGATION IN AS — RULES THAT DIFFER FROM C++
//============================================================================
//
// AS const-correctness is stricter than C++'s. Knowing the rules saves you
// from chasing "I can't assign X to Y" / "no matching ctor signature" errors.
//
// (1) `Cast<T>()` PRESERVES const. Contrary to typical UE C++ where Cast<>
//     returns a non-const pointer, AS's Cast<T> mirrors the input's const-ness.
//     Use it for type narrowing, not for stripping const.
//     const auto Frozen = Cast<UMyThing>(SomeConstValue);   // STILL const
//
// (2) `auto X = constSource` PRESERVES const — you get a const local. There
//     is no in-AS way to launder const away: no const_cast, no routing
//     through a base type, and Cast<> doesn't strip it either.
//     auto Def2 = Item.Get_Definition();    // STILL const (getter is const)
//     auto Def3 = Cast<UCk_InventoryItem_Definition>(Item.Get_Definition()); // STILL const
//
// (3) AS REJECTS const → non-const VALUE-PARAM conversion. C++ silently
//     copies; AS treats it as a type mismatch.
//     void TakesNonConst(UMyThing X);
//     const auto Frozen = Item.Get_Definition();
//     TakesNonConst(Frozen);  // ❌ compile error
//     The fix is at the receiving function — declare the param `const`:
//     void TakesConst(const UMyThing X);
//
// (4) USTRUCT FIELDS can be declared `const UObject` to receive const values.
//     Load-bearing when threading a const pointer (e.g. `Item.Get_Definition()`
//     result) through a struct.
//     USTRUCT()
//     struct FMyEntry
//     {
//         UPROPERTY() const UCk_InventoryItem_Definition Def;  // accepts const
//         UPROPERTY() int32 Count = 1;
//     }
//
// (5) `CK_DEFINE_CONSTRUCTORS` C++ macro: when a USTRUCT field is
//     `const UObject*` on the C++ side, the AS-exposed ctor also takes
//     `const UObject` for that param (post the CkCore reflection fix), so
//     building a request from a const Definition just works:
//         const auto Def = Item.Get_Definition();
//         auto Req = FCk_Request_Inventory_AddItemByDefinition(Def, 1); // ✓
//     If you spot the old asymmetry elsewhere (AS ctor non-const, C++ field
//     const), that's a `Get_RuntimeTypeToString_AngelScript` bug — fix it
//     there, don't work around it at the caller.
//
// (6) TArray::Add of const elements is rejected — TArray<T>::Add takes a
//     non-const ref (AddUnique too). For dedup, store a projected key:
//     TArray<UCk_InventoryItem_Definition> Defs;
//     const auto Def = Item.Get_Definition();
//     Defs.Add(Def);                        // ❌ const → non-const ref
//     TArray<FName> SeenNames;
//     SeenNames.AddUnique(Def.GetName());   // ✓

//============================================================================
// 10. SPAWN PARAMS PATTERN
//============================================================================
//
// PREFERRED: the generated accessor. For every entity-script class the
// post-compile generator emits `F<Script>_SpawnParams` + `U<Script>::Params()`
// into Script/Generated/<Plugin>_EntitySpawnParams.as:
auto SpawnParams = UCk_EntityScript_EntityScriptGym_Spawn::Params();
// (set fields, then spawn — real call sites in Plugins/CkTests/Script/CkEntityScript/)

// Hand-rolled alternative: struct field names must match the target script's
// UPROPERTY(ExposeOnSpawn) names exactly. Structs may have constructors.
USTRUCT()
struct FMyEntitySpawnParams
{
    UPROPERTY()
    FTransform InitialTransform = FTransform::Identity;

    FMyEntitySpawnParams(FTransform InTransform) { InitialTransform = InTransform; }
}

auto Params  = FMyEntitySpawnParams(StationTransform);
auto Pending = utils_entity_script::Request_SpawnEntity(
    ck::ToEntity(this), UMyEntityScript, Params);
// Pass the struct directly — the hand-written overloads take
// FAngelscriptAnyStructParameter (Script/CkUtils_EntityScript.as), implicitly
// wrapping any AS struct. `FInstancedStruct::Make(Params)` also compiles
// (in-tree: Plugins/CkTests/Script/Common/CkGym_Utils.as:133). Returns
// FCk_Handle_PendingEntityScript — bind the constructed callback via:
//   Pending.Promise_OnConstructed(FCk_Delegate_EntityScript_Constructed(this, n"OnConstructed"));

//============================================================================
// 10.1 ACTOR ↔ ENTITY LINKUP (EntityScript_WithActor)
//============================================================================
//
// To give an Actor an ECS Entity, use the one-call helper — do NOT hand-roll
// the spawn-params + TransientEntity + HasAuthority boilerplate:

class AMyActor : AActor
{
    default bReplicates = true;

    UFUNCTION(BlueprintOverride)
    void BeginPlay()
    {
        // Authority-gated internally: on clients this returns an INVALID pending
        // handle and the Entity arrives via the EntityScript replication pipeline.
        auto PendingEntity = utils_entity_script_with_actor::Request_SpawnEntityScript_OnActor(
            this, UMyEntityScript);
    }
}

// Editor/designer path: add the "Ck Entity Script (With Actor)" component
// (UCk_EntityScript_WithActor_ActorComponent_UE) to the Actor and pick the
// EntityScript class — zero code, same pipeline.
//
// To know when the Actor is ECS ready ON EVERY WORLD (server, client, late
// join), bind the actor-side promise — works whether or not this world did
// the spawning:
//
//   utils_owning_actor::Promise_OnActorEcsReady(
//       this, FCk_Delegate_OwningActor_OnEcsReady(this, n"OnEcsReady"));
//
//   UFUNCTION()
//   private void OnEcsReady(AActor InActor, FCk_Handle InEntity)
//   { /* Entity linked; replicated values applied (default policy) */ }
//
// Policies (3rd param, optional): ValuesReplicated (default — safe to read
// replicated attributes/team/SM state) | LinkEstablished (earlier; replicated
// values may not be applied yet on clients).
// Fires immediately if the Actor is already ready; auto-discards if the Actor
// is destroyed before ever becoming ready.

//============================================================================
// 11. TIMERS / TICK PATTERN
//============================================================================

// In DoConstruct:
auto TimerParams = FCk_Timer_Spec(FCk_Time(0.0f));
TimerParams.Set_StartingState(ECk_Timer_State::Running)
           .Set_Behavior(ECk_Timer_Behavior::ResetOnDone);
auto Timer = utils_timer::Add(InHandle, TimerParams);
// Binding: delegate FIRST; policy + postfire are optional trailing params with
// defaults (FireIfPayloadInFlightThisFrame + DoNothing) — omit unless overriding.
// Real signature: CkTimer_Utils.h:315-319 / Generated/utils_timer.as:196.
Timer.BindTo_OnUpdate(FCk_Delegate_Timer(this, n"Tick"));

// One-liner sugar for a running per-frame tick (Script/CkUtils_Timer.as):
auto T = utils_timer::Create_Tick(InHandle, FCk_Delegate_Timer(this, n"Tick"));

UFUNCTION()
private void Tick(FCk_Handle_Timer InHandle, FCk_Chrono InChrono, FCk_Time InDeltaT)
{
    // per-frame logic
}

//============================================================================
// 12. COMMON FEATURE PATTERNS
//============================================================================

// Attribute
auto Attr = FCk_FloatAttribute_Spec(
    utils_gameplay_tag::ResolveGameplayTag(n"Attribute.Health"), 100.0f);
Attr.Set_MinMax(ECk_MinMax::MinMax).Set_MinValue(0.0f).Set_MaxValue(100.0f);
auto Health = utils_float_attribute::Add(SelfEntity, Attr);

// Find tagged entities + typed script access
for (auto E : utils_entity_tag::ForEach_Entity(SelfEntity, n"TAG_MyEntity"))
{
    auto S = utils_entity_script::TryGet_EntityScript(E);
    auto Typed = Cast<UMyEntityScript>(S);
    if (ck::IsValid(Typed)) { Typed.SomeFunction(); }
}

// Request structs — use fluent setter chains for optional params
auto Req = FCk_Request_AudioDirector_StartTrack(TrackName);
Req.Set_PriorityOverrideMode(ECk_PriorityOverride::Override)
   .Set_PriorityOverrideValue(75)
   .Set_FadeInTime(FCk_Time(2.0f));
utils_audio_director::Request_StartTrack(AudioDirector, Req);

// Signal binding — delegate FIRST, exactly as in §11 (a policy-first call does
// NOT compile; no such overload exists). Override defaults via trailing params:
FCk_Delegate_Timer TimerDelegate(this, n"Tick");
Timer.BindTo_OnUpdate(TimerDelegate);   // defaults: FireIfPayloadInFlightThisFrame + DoNothing
Timer.BindTo_OnUpdate(TimerDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);

//============================================================================
// 13. ASSET DEFINITIONS ('asset ... of ...') — canonical home of this topic
//============================================================================
//
// `asset <Name> of <UDataAssetSubclass>` creates a data-asset instance from
// script, registered with the engine as if authored in the editor — no
// .uasset to manage (object path: /Script/AngelscriptAssets.<Name>).
// Preferred for data assets conceptually owned by a specific .as file: a
// feature's gameplay tags, asset-registry configs, item/look definitions.
// NOT for hand-authored content (meshes/textures/BPs), assets edited
// frequently by non-programmers, or shared assets in engine Content folders.
//
// INITIALIZER RULES (it's an initializer block; statement order irrelevant):
// - Assign public UPROPERTY fields directly: `Field = value;` (private
//   BlueprintReadWrite fields work the same way). Call methods on fields
//   (`GameplayTags.Add(n"Some.Tag");`); local struct variables are allowed.
//   NO `default` keyword inside an asset block.
// - Functions cannot be DEFINED inside an asset block — define a global /
//   namespace function and call it from the initializer (real:
//   CkTests_AutoTestMapConfig.as calls assets::AutoTests_CkTests_Level()).

// Real example (Script/CkUsf/CkUsf_Looks_Assets.as:3 — trimmed):
namespace CkUsf
{
    asset Hologram of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/Hologram.ush";
        _UshFunctionName = n"CkUsf_Look_Hologram";

        FCk_Usf_ParamDesc Tint;          // locals + field method calls are fine
        Tint._Name = n"TintColor";
        _Parameters.Add(Tint);
    }
}

// GAMEPLAY TAGS — DECLARE WHERE USED. Tag assets (UCk_GameplayTags) belong in
// the file that primarily uses them — a feature's _Shared.as / _Common.as, or
// directly in a station/test file if only used there. Do NOT create a
// separate _Assets.as just for tags.
//   GOOD: tags in CkEqs_Shared.as / CkProbeGym_Common.as (real in-tree examples)
//   BAD:  tags in CkInteractionGym_Assets.as (separate file just for tags)
namespace Ck
{
    asset Asset_Tags of UCk_GameplayTags
    {
        GameplayTags.Add(n"MyFeature.Category.SomeTag");
    }
}

// #if EDITOR IS REQUIRED for editor-only asset types — otherwise the script
// fails to compile at engine startup with:
//   "Cannot use editor-only type ... outside of an EDITOR block"
// e.g. UCkAssetRegistryConfig (editor-only; scans a content folder and
// generates a .as file of typed asset accessors). Real gated example:
// Plugins/CkTests/Script/Common/CkTests_AutoTestMapConfig.as:27-28.
#if EDITOR
    asset MyFeature_AssetRegistryConfig of UCkAssetRegistryConfig
    {
        AssetDiscoveryRoot = "/MyPlugin/MyFolder";
        OutputFileName     = "my_feature_assets.as";
        Namespace          = "my_feature_assets";
    }
#endif

// FILE NAMING: files containing heavyweight `asset ... of ...` declarations
// (item definitions, look definitions, ability configs — anything beyond
// simple tags) must use the `_Assets.as` suffix (real: CkUsf_Looks_Assets.as).
// Tags alone do NOT warrant a dedicated _Assets.as file.
// Special case: `asset <X>Handle of UCkDynamic_HandleDefinition` also needs
// the registry entry — see §7.

//============================================================================
// 14. NETWORKING
//============================================================================

class ANetworkedActor : AActor
{
    default bReplicates = true;

    UPROPERTY(Replicated) bool bReplicatedBool = true;
    UPROPERTY(Replicated, ReplicationCondition = OwnerOnly) int ReplicatedInt = 0;

    UPROPERTY(Replicated, ReplicatedUsing = OnRep_Health)
    float Health = 100.0;
    UFUNCTION() void OnRep_Health() { /* called on clients */ }

    // RPCs — RELIABLE by default (opposite of C++)!
    UFUNCTION(NetMulticast)              void MulticastDoEffect() { }
    UFUNCTION(NetMulticast, Unreliable)  void MulticastDoCosmetic() { }
    UFUNCTION(Server)                    void ServerDoAction() { }
    UFUNCTION(Client)                    void ClientNotify() { }
}

//============================================================================
// 15. GAMEPLAY TAGS
//============================================================================
// Tags bound to global GameplayTags namespace. Dots → underscores.
// "UI.Action.Escape" → GameplayTags::UI_Action_Escape
auto T = GameplayTags::UI_Action_Escape;

//============================================================================
// 16. MIXINS, SUBSYSTEMS, LIBRARIES
//============================================================================

// Mixins — add methods to existing types. First param is `self`.
mixin void TeleportTo(AActor Self, FVector Loc) { Self.SetActorLocation(Loc); }  // MyActor.TeleportTo(Loc)
mixin void SetToZero(FVector& Self) { Self = FVector(0,0,0); }                   // MyVector.SetToZero()

// Subsystems
auto LevelEditor = ULevelEditorSubsystem::Get();
auto Player      = Gameplay::GetPlayerController(0).LocalPlayer;
auto PSub        = UMyPlayerSubsystem::Get(Player);
// Custom subsystems inherit UScriptWorldSubsystem, UScriptGameInstanceSubsystem,
// UScriptLocalPlayerSubsystem, UScriptEditorSubsystem, UScriptEngineSubsystem.

// BP Function Libraries are exposed as namespaces; common prefixes stripped:
//   UGameplayStatics        → Gameplay::
//   UKismetSystemLibrary    → System::
//   UKismetMathLibrary      → SKIPPED (use Math:: from FMath)
//   UNiagaraFunctionLibrary → Niagara::
//   UWidgetBlueprintLibrary → Widget::
System::SetTimer(this, n"OnTimer", 2.0, bLooping = false);
Gameplay::GetPlayerController(0);

// 16.1 NAMING YOUR OWN BFLs — AVOID SUFFIX-STRIP COLLISIONS (GOTCHA)
//
// The AS plugin auto-strips a default suffix list from UBlueprintFunctionLibrary
// class names when building the AS namespace. Defaults (Hazelight fork,
// AngelscriptSettings.h:126-139):
//   suffixes: "Statics", "Library", "BlueprintLibrary",
//             "BlueprintFunctionLibrary", "FunctionLibrary"
//   prefixes: "UKismet", "UBlueprint"
// If your BFL ends in any of these, AS silently rewrites the namespace and
// callsites using the C++ name fail with the MISLEADING error:
//   "No matching signatures to 'UMyClass_FunctionLibrary::Foo()'"
// — looks like a parameter mismatch; actually the class name was rewritten.
//
// ❌ class UMyFeature_FunctionLibrary : UBlueprintFunctionLibrary
//      → AS namespace becomes "UMyFeature_" (mangled, won't match callsites)
// ✓ class UCk_Utils_MyFeature_UE : UBlueprintFunctionLibrary
//      → "_UE" isn't on the strip list, namespace round-trips unchanged
//
// This is why every Ck BFL ends `_UE` — stick to it for any new BFL exposed
// to AS. If you must use a stripped suffix for a non-AS reason, override the
// namespace explicitly via `UCLASS(meta = (ScriptName = "MyChosenName"))`.

//============================================================================
// 17. EDITOR-ONLY CODE & BP OVERRIDES
//============================================================================

#if EDITOR
    SetActorLabel("Debug Name");
#endif
// #if EDITOR, #if EDITORONLY_DATA, #if RELEASE, #if TEST
// Editor-only dirs (auto-excluded from cooked): Editor/, Examples/, Dev/

// BlueprintOverride — override C++/parent events
UFUNCTION(BlueprintOverride) void BeginPlay() { }
UFUNCTION(BlueprintOverride) void Tick(float DeltaSeconds) { }

// BlueprintEvent — allow child BPs to override (must have base impl)
UFUNCTION(BlueprintEvent) void OnCountdownFinished() { }

// C++ prefixes Receive, BP_, K2_, Received_ are auto-stripped — use the
// simplified name. Script methods are virtual by default; override directly
// and call `Super::...` to invoke parent.

//============================================================================
// 18. PROPERTY ACCESSORS
//============================================================================
property float GetHealth() const { return _Health; }
property void  SetHealth(float Value) { _Health = Value; }
// Usage: Actor.Health = 50.0; auto H = Actor.Health;
// C++ `Get...()` bindings auto-work as property accessors.

//============================================================================
// 19. TESTING
//============================================================================
// House testing goes through CkTests (gyms / AutoTests / Gauntlet) — see the
// root CLAUDE.md skill index (`ck-tests-authoring-and-running`, CkTests) and
// the specs in Plugins/CkTests/Script/Common/. The Hazelight fork also ships
// a native harness (FUnitTest / FIntegrationTest); it has ZERO in-tree usage
// in the Ck plugins (verified 2026-07-02) — do not reach for it by default.

//============================================================================
// 20. NAMING
//============================================================================
// All naming rules (Get_/TryGet_/Request_ prefixes, no `b` bool prefix,
// member/param conventions) live in root CLAUDE.md "Code style" and apply
// unchanged in AS.

//============================================================================
// 21. COMMON MISTAKES — QUICK REFERENCE
//============================================================================
/*
❌ ck::SelfEntity(this)                            → ck::ToEntity(this)     // renamed — old name no longer exists (§5)
❌ Timer.BindTo_OnUpdate(Policy, Delegate)         → Timer.BindTo_OnUpdate(Delegate[, Policy][, PostFire])  // delegate first (§11)
❌ void BeginPlay()                                → void DoBeginPlay(FCk_Handle InHandle)
❌ FMath::Sin(x)                                   → Math::Sin(x)
❌ if (NOT Valid)                                  → if (Valid == false)
❌ static_cast<uint8>(x)                           → uint8(x)
❌ Text += "More"                                  → Text = f"{Text}More"
❌ f"{SomeHandle}"                                 → f"{SomeHandle.ToString()}"   // runtime throw otherwise (§22.1)
❌ ck::IsValid(P, ck::IsValid_Policy_NullptrOnly{})→ ck::IsValid(P)
❌ UCk_Utils_Probe_UE::Add(...)                    → utils_probe::Add(...)
❌ utils_transform::Get_WorldTransform(...)        → utils_transform::Get_EntityCurrentTransform(...)
❌ utils_debug_draw::DrawDebugString(World, ...)   → utils_debug_draw::DrawDebugString(Pos, ...)
❌ asset X of default UCk_Base                     → asset X of UCk_Base
❌ static void Foo()                               → regular member function
❌ Actor->SetLocation(Pos)                         → Actor.SetLocation(Pos)
❌ AMyActor() { Cap = 88; }                        → default Capsule.CapsuleHalfHeight = 88.0;
❌ UPROPERTY(EditAnywhere, BlueprintReadWrite)     → UPROPERTY()            // already default
❌ UFUNCTION(BlueprintCallable)                    → UFUNCTION()            // already default
❌ UFUNCTION(NetMulticast)                         // already RELIABLE — add Unreliable if intended
❌ struct S { UFUNCTION() void F(); }              → plain methods (no UFUNCTION on struct methods)
❌ UCLASS(BlueprintType) class AMyActor            → class AMyActor : AActor
❌ CreateDefaultSubobject in ctor                  → UPROPERTY(DefaultComponent) U...Component X;
*/

//============================================================================
// 22. WHAT BREAKS SILENTLY — TOP ITEMS
//============================================================================
// Full catalog (13 items) + recipes: `ck-angelscript-interop` skill (root CLAUDE.md skill index).
//
// 1. Raw FCk_Handle in an f-string → RUNTIME throw "Invalid type to append
//    to string." (engine Bind_FString.cpp:597) at PIE-start/exec time, NOT
//    compile time — invisible to `-skipcompile` headless boots. Every
//    typesafe handle has .ToString() bound — use it.
// 2. EntitySpawnParams phantom namespace: deleting an entity-script .as while
//    its block in <Plugin>_EntitySpawnParams.as survives leaves a phantom AS
//    namespace — re-adding a same-named class then SILENTLY fails to register
//    as a live UClass (UObjectIterator misses it; the autotest populator drops
//    the test). Recovery: revert ALL of Script/Generated/*.as atomically,
//    never AutoTestActors.as alone (Source/CkAngelscriptGenerator/Claude.md:170-176).
// 3. Stale/absent DynamicHandleTypes.json → project-wide "'FCk_Handle_X' is
//    not a data type" cascade. Self-heal (default-on) recovers in ONE boot;
//    the first-pass errors + "Hot reload failed ... Keeping all old script
//    code" are EXPECTED transients — gate on the post-regen clean reload and
//    the JSON entry, not first-pass silence (§7).
// 4. NEVER blanket-delete Script/Generated/. The Hazelight hot-reload watcher
//    is mtime-based: ANY mtime change there triggers a full AS reload sweep
//    (editor-freezing at scale). Generator cleanup is manifest-based via
//    _index.as for exactly this reason (generator Claude.md:216).
// 5. Two editor/headless instances of one project: a cross-process
//    single-writer lock (<Saved>/CkAngelscriptGenerator_RegenOwner.lock,
//    Rev 12) makes the second instance a READ-ONLY secondary — it compiles
//    against the owner's generated files and writes nothing to Generated
//    (generator Claude.md:28-32).

//============================================================================
// 23. PROVENANCE AND MAINTENANCE
//============================================================================
// Facts above were verified against code on 2026-07-02 (citations inline).
// Re-verify the volatile ones (run from the superproject root):
//   rg --no-ignore -n 'ToEntity' Plugins/CkFoundation/Script/CkUtils_Common.as    → overloads at :5 (AActor), :10 (EntityScript)
//   rg --no-ignore -n 'BindTo_OnUpdate' Plugins/CkFoundation/Script/Generated/utils_timer.as  → delegate first, policy/postfire defaulted
//   ls Plugins/CkFoundation/Script/Generated/utils_*.as | wc -l                   → generated-wrapper count (268 on 2026-07-02)
//   rg -n 'DynamicHandleRegistryDirectory' Config/DefaultCkFoundation.ini         → superproject registry-path override
// TOOLING (same caveat as root CLAUDE.md provenance): the agent Grep/Glob
// tools are silently blind under Script/ (superproject `.ignore`) — ALL .as
// searches must use `rg --no-ignore` in Bash, or Read with exact paths;
// re-check any zero-match with `rg --no-ignore --files`.

//============================================================================
// REMINDER: This file is for AngelScript. C++: Source/CLAUDE.md.
// Style/macros/non-negotiables: root Plugins/CkFoundation/CLAUDE.md.
//============================================================================
