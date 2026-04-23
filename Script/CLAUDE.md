//============================================================================
// ANGELSCRIPT GUIDELINES — CkFoundation framework reference
//============================================================================
//
// For AngelScript (.as) development against CkFoundation. For C++ work, read
// Plugins/CkFoundation/Source/CLAUDE.md. File is in code-comment format for
// token-efficient loading into Claude context.
//
// CkFoundation is a custom ECS framework for UE 5.5+. Do NOT assume framework
// behavior — verify against existing .as files, or ask. Prefer
// Research → Plan → Implement over jumping to code.

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
//   `UPROPERTY(DefaultComponent) ...` (see §9).
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
auto Handle = ck::SelfEntity(this);

UPROPERTY()
UCk_MusicLibrary_Base SomeLibrary;

// Float precision
float ValueDouble = 1.0;         // 64-bit
float32 ValueSingle = 1.f;       // 32-bit when you need it
float64 ValueAlsoDouble = 1.0;   // explicit 64-bit

// Type casting — direct, NOT static_cast
auto ArmorValue = uint8(Math::Clamp(V * 2.0f, 0.0f, 255.0f));

// String formatting — f"" interpolation; NO `+=` or `+` for strings
auto DisplayText = "=== ATTRIBUTES ===\n";
DisplayText = f"{DisplayText}Health: {HP} (Base: {Base})\n";

// Advanced format specifiers
auto Debug     = f"{DeltaSeconds =}";              // "DeltaSeconds = 0.01"
auto Precise   = f"{Value :.3}";                   // 3 decimals
auto Padded    = f"{400 :010d}";                   // "0000000400"
auto Hex       = f"{20 :#x}";                      // "0x14"
auto RightAln  = f"{GetName() :>40}";              // right-align 40
auto EnumName  = f"{ESlateVisibility::Collapsed :n}";  // "Collapsed"

//============================================================================
// 3. VALIDITY & BOOLEAN LOGIC
//============================================================================

if (ck::IsValid(SomePointer)) { /* ... */ }
if (ck::Is_NOT_Valid(SomePointer)) { return; }
if (IsEnabled == false) { return; }   // preferred
if (!IsEnabled) { return; }           // OK

// Bool naming: NO `b` prefix
bool IsAlive = true;   // not bIsAlive

//============================================================================
// 4. ENTITY SCRIPT LIFECYCLE
//============================================================================

UFUNCTION(BlueprintOverride)
ECk_EntityScript_ConstructionFlow DoConstruct(FCk_Handle& InHandle)
{
    // Add fragments / child entities here.
    return ECk_EntityScript_ConstructionFlow::Finished;
}

UFUNCTION(BlueprintOverride)
void DoBeginPlay(FCk_Handle InHandle)
{
    auto SelfEntity = InHandle;   // use the param, not ck::SelfEntity(this)
}

UFUNCTION(BlueprintOverride)
void DoEndPlay(FCk_Handle InHandle)
{
    // Unbind signals (auto if PostFireBehavior::Unbind was used at bind time).
}

//============================================================================
// 5. FRAMEWORK API — UTILS SHORTCUTS
//============================================================================
//
// ALWAYS use utils_* shortcuts. NEVER the full UCk_Utils_X_UE:: names.

auto SelfEntity = ck::SelfEntity(this);

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
// Scopes: Game, Plugins, Engine, All

// BAD patterns (never use):
//   UCk_Utils_Probe_UE::Add(...)
//   UCk_Utils_AudioDirector_UE::Request_StartTrack(...)

//============================================================================
// 6. HANDLE CONVERSIONS
//============================================================================

auto SelfEntity         = ck::SelfEntity(this);
auto TransformHandle    = SelfEntity.To_FCk_Handle_Transform();
auto ProbeHandle        = SelfEntity.To_FCk_Handle_Probe();
auto AudioDirectorH     = SelfEntity.To_FCk_Handle_AudioDirector();

//============================================================================
// 7. DYNAMIC HANDLE REGISTRATION (CRITICAL GOTCHA)
//============================================================================
//
// Declaring a typesafe handle via `asset <X>Handle of UCkDynamic_HandleDefinition`
// is NOT sufficient on its own. A registry JSON at
//   <Project>/Script/Generated/DynamicHandleTypes.json
// (path from `UCk_Utils_Dynamic_Settings_UE::Get_DynamicHandleRegistryFilePath()`;
// default `FPaths::ProjectConfigDir()`) must contain a matching entry, or AS
// compilation fails at engine startup with:
//   Identifier 'FCk_Handle_<X>' is not a data type in global namespace
// across every file that references the type (feature, utils, processor, HFSM).

// Example declaration:
//   asset CheckoutCounterHandle of UCkDynamic_HandleDefinition
//   {
//       TypeName = "FCk_Handle_CheckoutCounter";
//       RequiredFragments.Add(FBb_Feature_CheckoutCounter);
//       Description = "...";
//   }

// Matching JSON entry:
//   {
//     "TypeName": "FCk_Handle_CheckoutCounter",
//     "ShortName": "CheckoutCounter",
//     "Description": "...",
//     "SourceAsset": "/Script/AngelscriptAssets.CheckoutCounterHandle",
//     "RequiredFragments": ["Bb_Feature_CheckoutCounter"]
//   }

// Gotchas:
// - RequiredFragments drops the `F` prefix — `FBb_Feature_X` → `Bb_Feature_X`.
// - SourceAsset follows `/Script/AngelscriptAssets.<Name>Handle`.
// - Editor restart is required after regenerating the registry — hot reload
//   does not pick it up (see ForceRefreshDynamicHandleBindings below, which
//   attempts hot rebinding without restart).

// HOW TO REGENERATE (don't hand-edit unless you know why):
// `UCkDynamicHandleSubsystem` (editor subsystem) exposes two CallInEditor
// buttons:
//   - GenerateHandleTypeRegistry()        — discovers all
//     UCkDynamic_HandleDefinition assets, writes JSON sorted by TypeName.
//   - ForceRefreshDynamicHandleBindings() — regenerates + resets registry
//     flags + re-registers AS bindings without an editor restart (dev-only).
// Locate under Editor Subsystems or call from a Blueprint. Source:
//   Source/CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.{h,cpp}

// Runtime consumers:
// - Path resolution:
//   Source/CkDynamic/Public/CkDynamic/Settings/CkDynamic_Settings.cpp
// - Registry load:
//   Source/CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.cpp

//============================================================================
// 8. ACTORS & COMPONENTS
//============================================================================
//
// NO constructors. Use `UPROPERTY(DefaultComponent)` + `default` keyword.

class AMyActor : AActor
{
    UPROPERTY(DefaultComponent, RootComponent)
    USceneComponent SceneRoot;

    UPROPERTY(DefaultComponent, Attach = SceneRoot)
    UStaticMeshComponent Mesh;

    UPROPERTY(DefaultComponent, Attach = CharacterMesh, AttachSocket = RightHand)
    UStaticMeshComponent WeaponMesh;

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

// Spawn actors
auto Spawned = SpawnActor(AMyActor, SpawnLocation, SpawnRotation);
UPROPERTY() TSubclassOf<AMyActor> ActorClass;
auto Spawned2 = SpawnActor(ActorClass, SpawnLocation, SpawnRotation);

// Query
TArray<UStaticMeshComponent> Meshes;
Actor.GetComponentsByClass(Meshes);
TArray<ANiagaraActor> Niagaras;
GetAllActorsOfClass(Niagaras);

// Construction script
UFUNCTION(BlueprintOverride)
void ConstructionScript()
{
    for (int i = 0; i < Count; ++i)
    {
        auto M = UStaticMeshComponent::Create(this);
        M.SetStaticMesh(MeshAsset);
    }
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

StoredDelegate.BindUFunction(this, n"OnDelegateExecuted");
StoredDelegate = FMyDelegate(this, n"OnDelegateExecuted");
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
    if (ProbeParams.LocalOffset.Equals(FTransform::Identity))
    { ProbeParams.LocalOffset = FTransform(FVector(0, -75, 0)); }

    auto Resolved = FSomeParams();
    Resolved.Probe      = ProbeParams;
    Resolved.OtherField = InParams.OtherField;   // copy anything else needed
    // use Resolved from here on
}

// ✓ Option B: take the param by reference — caller needs a mutable var
void Foo(FSomeParams& InParams) { InParams.Probe = SomeValue; }

//============================================================================
// 10. SPAWN PARAMS PATTERN
//============================================================================
//
// Struct must match UPROPERTY(ExposeOnSpawn) names exactly on the target.

USTRUCT()
struct FMyEntitySpawnParams
{
    UPROPERTY()
    FTransform InitialTransform = FTransform::Identity;

    FMyEntitySpawnParams(FTransform InTransform) { InitialTransform = InTransform; }
}

auto Params  = FMyEntitySpawnParams(StationTransform);
auto Spawned = utils_entity_script::Request_SpawnEntity(
    ck::SelfEntity(this),
    UMyEntityScript,
    FInstancedStruct::Make(Params));

//============================================================================
// 11. TIMERS / TICK PATTERN
//============================================================================

// In DoConstruct:
auto TimerParams = FCk_Fragment_Timer_ParamsData(FCk_Time(0.0f));
TimerParams.Set_StartingState(ECk_Timer_State::Running)
           .Set_Behavior(ECk_Timer_Behavior::ResetOnDone);
auto Timer = utils_timer::Add(InHandle, TimerParams);
Timer.BindTo_OnUpdate(
    ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
    FCk_Delegate_Timer(this, n"Tick"));

UFUNCTION()
private void Tick(FCk_Handle_Timer InHandle, FCk_Chrono InChrono, FCk_Time InDeltaT)
{
    // per-frame logic
}

//============================================================================
// 12. COMMON FEATURE PATTERNS
//============================================================================

// Attribute
auto Attr = FCk_Fragment_FloatAttribute_ParamsData(
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

// Request structs — use setter chains for optional params
auto Req = FCk_Request_AudioDirector_StartTrack(TrackName);
Req.Set_PriorityOverrideMode(ECk_PriorityOverride::Override);
Req.Set_PriorityOverrideValue(75);
Req.Set_FadeInTime(FCk_Time(2.0f));
utils_audio_director::Request_StartTrack(AudioDirector, Req);

// Signal binding
FCk_Delegate_Timer TimerDelegate(this, n"Tick");
Timer.BindTo_OnUpdate(ECk_Signal_BindingPolicy::FireIfPayloadInFlight, TimerDelegate);
// Delegate function name uses n"FunctionName" (name literal).

//============================================================================
// 13. ASSET DEFINITIONS ('asset ... of ...')
//============================================================================
//
// Creates a UDataAsset instance from script, registered as if authored in the
// editor. Preferred when the asset belongs to a specific .as file.

// Gameplay tags — declare where used (usually <Feature>_Shared.as or a gym
// station file). Do NOT create a separate _Assets.as just for tags.
namespace Ck
{
    asset Asset_Tags of UCk_GameplayTags
    {
        GameplayTags.Add(n"MyFeature.Category.SomeTag");
        GameplayTags.Add(n"MyFeature.Category.AnotherTag");
    }
}

// Editor-only types MUST be inside `#if EDITOR` or compilation fails with
// "Cannot use editor-only type ... outside of an EDITOR block".
#if EDITOR
    asset MyFeature_AssetRegistryConfig of UCkAssetRegistryConfig
    {
        AssetDiscoveryRoot = "/MyPlugin/MyFolder";
        OutputFileName     = "my_feature_assets.as";
        Namespace          = "my_feature_assets";
    }
#endif

// Asset initializers: assign UPROPERTY fields directly, or call methods
// (e.g. `Container.Add(...)`). NO `default` keyword inside an asset.
// Functions cannot be members of assets — call global functions:
TArray<FCk_MusicTrackEntry> Get_MusicTracks()
{
    auto Tracks = TArray<FCk_MusicTrackEntry>();
    auto T = FCk_MusicTrackEntry();
    T.Set_TrackName(utils_gameplay_tag::ResolveGameplayTag(n"Music.Track1"));
    Tracks.Add(T);
    return Tracks;
}
asset MyMusicLibrary of UCk_MusicLibrary_Base
{
    _LibraryName = utils_gameplay_tag::ResolveGameplayTag(n"My.Music.Library");
    _Tracks      = Get_MusicTracks();
}

// File naming:
// - Heavyweight `asset ... of ...` declarations (item defs, ability configs,
//   etc.) go in `<Feature>_Assets.as`.
// - Tags alone do NOT require a dedicated _Assets.as file.

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
mixin void TeleportTo(AActor Self, FVector Loc) { Self.SetActorLocation(Loc); }
// Usage: MyActor.TeleportTo(FVector(0,0,100));
mixin void SetToZero(FVector& Self) { Self = FVector(0,0,0); }
// Usage: MyVector.SetToZero();

// Subsystems
auto LevelEditor = ULevelEditorSubsystem::Get();
auto Player      = Gameplay::GetPlayerController(0).LocalPlayer;
auto PSub        = UMyPlayerSubsystem::Get(Player);
// Custom subsystems inherit from UScriptWorldSubsystem,
// UScriptGameInstanceSubsystem, UScriptLocalPlayerSubsystem,
// UScriptEditorSubsystem, UScriptEngineSubsystem.

// BP Function Libraries are exposed as namespaces; common prefixes stripped:
//   UGameplayStatics        → Gameplay::
//   UKismetSystemLibrary    → System::
//   UKismetMathLibrary      → SKIPPED (use Math:: from FMath)
//   UNiagaraFunctionLibrary → Niagara::
//   UWidgetBlueprintLibrary → Widget::
System::SetTimer(this, n"OnTimer", 2.0, bLooping = false);
Gameplay::GetPlayerController(0);

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

void Test_MyFeature(FUnitTest& T)
{
    T.AssertTrue(Condition);
    T.AssertEquals(Expected, Actual);
    T.AssertNotNull(SomePtr);
}

// Integration tests need a matching level: Content/Testing/IntegrationTest_X.umap
void IntegrationTest_MyTest(FIntegrationTest& T) { /* latent cmds OK */ }

// Convention: File_Test.as alongside File.as.

//============================================================================
// 20. NAMING
//============================================================================
// Get_ prefix for getters, Request_ prefix for mutating functions.
// No `b` prefix for bools. Descriptive names.
auto Get_CurrentHealth() -> float;
auto Request_TakeDamage(float Damage) -> void;

//============================================================================
// 21. COMMON MISTAKES — QUICK REFERENCE
//============================================================================
/*
❌ void BeginPlay()                                → void DoBeginPlay(FCk_Handle InHandle)
❌ FMath::Sin(x)                                   → Math::Sin(x)
❌ if (NOT Valid)                                  → if (Valid == false)
❌ static_cast<uint8>(x)                           → uint8(x)
❌ Text += "More"                                  → Text = f"{Text}More"
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
// REMINDER: This file is for AngelScript. For C++, see
// Plugins/CkFoundation/Source/CLAUDE.md.
//============================================================================
