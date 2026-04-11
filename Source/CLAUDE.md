// ============================================================================
// COMPLETE DEVELOPMENT GUIDELINES - READ ALL BEFORE CODING
// ============================================================================

// CRITICAL: Read and understand ALL sections before writing any code.
// This framework has specific patterns and conventions that must be followed.

// ============================================================================
// 1. DEVELOPMENT PARTNERSHIP
// ============================================================================

// We're building production-quality code together
// Lead (Human) + Principal Programmer (Claude) partnership
// Challenge directives if they don't make sense, are risky, or wrong
// DO NOT be a sycophant - be as concise as possible while clear

// CRITICAL WORKFLOW - ALWAYS FOLLOW THIS!
// Research → Plan → Implement (NEVER JUMP STRAIGHT TO CODING!)
// 1. Research: Explore codebase, understand existing patterns
// 2. Plan: Create detailed implementation plan and verify with me
// 3. Implement: Execute plan with validation checkpoints

// CRITICAL ENVIRONMENT DETECTION:
// - File paths present in conversation = Claude Desktop → Modify files directly
// - No file paths = Web version → Use artifacts for ALL code, NEVER in chat
// - Make iterative changes when possible to avoid message length limits
// - Avoid full file rewrites unless absolutely necessary

// CRITICAL: DO NOT ASSUME ANYTHING ABOUT CkFoundation FRAMEWORK
// - This is a custom framework with specific patterns
// - ASK for clarification rather than guessing
// - Request examples of existing code when unsure
// - Verify patterns before implementing

// Reality Checkpoints - Stop and validate at these moments:
// - After implementing complete feature
// - Before starting new major component
// - When something feels wrong
// - Before declaring "done"
// - If unsure of implementation

// Working Memory Management:
// Long context → Re-read guidelines, summarize progress, document current state
// Maintain TODO.md with Current Task, Completed, Next Steps

// ASK QUESTIONS if you are unsure at any point. Do NOT make assumptions, no matter how small.

// ============================================================================
// 2. OUR LINGO
// ============================================================================

// ECS = Entity Component System
// BPFL = Blueprint Function Library
// Entity = Entity from ECS
// Fragment = Component from ECS (because Component is overloaded in UE)
// Processor = System from ECS
// UHT = Unreal Header Tool

// ============================================================================
// 3. TECHNICAL STANDARDS
// ============================================================================

// Unreal Engine 5.6 (ask if this has changed)
// Do NOT create fallbacks that hide problems unless explicitly told
// Feature branch - no backwards compatibility needed
// Choose clarity over cleverness

// ============================================================================
// 4. FUNCTION FORMATTING STANDARDS
// ============================================================================

// UFUNCTION DECLARATIONS (headers):
UFUNCTION()
void
OnLifetimeExpired(
    FCk_Handle_Timer InTimer,
    FCk_Chrono InChrono,
    FCk_Time InDeltaT);

// C++ FUNCTION DECLARATIONS (headers):
auto
GetAssetRegistryTags(
    TArray<FAssetRegistryTag>& OutTags) const -> void override;

// Functions without parameters (headers):
auto
Foo() -> bool;

// FUNCTION DEFINITIONS (implementations):
auto
    UCk_CueBase_EntityScript::
    GetAssetRegistryTags(
        TArray<FAssetRegistryTag>& OutTags) const
    -> void
{
    // Implementation...
}

// Parameterless function definitions:
auto
    UCk_CueBase_EntityScript::
    Foo()
    -> bool
{
    return false;
}

// ============================================================================
// 5. C++ CODE STYLE & PATTERNS
// ============================================================================

// Use auto aggressively, even for nullptr pointers
auto IntPtr = static_cast<int*>(nullptr);
auto Handle = UCk_Utils_AudioTrack_UE::Cast(SomeHandle);

// Invert if statements to early out and reduce nesting
auto SomeFunction() -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Handle), TEXT("Handle is invalid"))
    { return; }

    // Continue with main logic...
}

// Validity checks and operators
if (ck::IsValid(Component)) { /* ... */ }
if (ck::Is_NOT_Valid(Component)) { return; }
if (NOT bSomeCondition) { /* ... */ }
if (ck::IsValid(Pin, ck::IsValid_Policy_NullptrOnly{})) { /* for pointers */ }

// Naming conventions
bool IsEnabled = true;        // Good - no 'b' prefix
auto Get_TrackName() const -> FGameplayTag;   // Get_ prefix for getters
auto Request_StartTrack() -> void;            // Request_ prefix for mutating functions

// Function signatures
// UFUNCTION declarations: NO trailing return type
UFUNCTION(BlueprintCallable)
static FCk_Handle_AudioTrack SomeFunction();

// UFUNCTION definitions: USE trailing return type
auto UCk_Utils_AudioTrack_UE::SomeFunction() -> FCk_Handle_AudioTrack { /* ... */ }

// All other functions: USE trailing return type
auto DoSomething() -> void;

// Construction syntax
auto MyStruct = MyStructType{};              // Use {} for construction
UFUNCTION(BlueprintCallable)
static void Func(float Value = 1.0f);       // Use () for UFUNCTION defaults (UHT limitation)

// Default initialization in UFUNCTIONs is DISALLOWED - remove = {}

// Unreal UFUNCTION overloading: Add suffix (cannot overload)
UFUNCTION(BlueprintCallable)
static void DoAction();
UFUNCTION(BlueprintCallable)
static void DoAction_Advanced();

// ============================================================================
// CRITICAL: COMMENTS AND CODE CLARITY
// ============================================================================

// Do NOT have unnecessary comments. If the code is not clear, use other means:
// - Better variable names
// - Named lambdas to clarify intent
// - Extract bool parameters into constexpr variables (see below)

// BAD - Comment explains unclear code:
NiagaraComponent->Activate(true);  // true = reset on activate

// GOOD - Self-documenting code:
constexpr auto ResetOnActivate = true;
NiagaraComponent->Activate(ResetOnActivate);

// Extract ALL bool parameters in function calls into constexpr variables:
// BAD:
auto Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    World, Effect, Location, Rotation, Scale,
    false,  // What does this mean?
    true,   // What does this mean?
    ENCPoolMethod::None,
    true    // What does this mean?
);

// GOOD:
constexpr auto AutoDestroy = false;
constexpr auto AutoActivate = true;
constexpr auto PreCullCheck = true;
auto Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    World, Effect, Location, Rotation, Scale,
    AutoDestroy,
    AutoActivate,
    ENCPoolMethod::None,
    PreCullCheck
);

// This pattern applies to ALL function calls with bool parameters
// The variable name documents what the parameter does

// ============================================================================
// CK_PROPERTY MACRO USAGE & ENCAPSULATION PATTERNS
// ============================================================================

// CK_PROPERTY_GET(_PrivateMember) automatically generates:
// - A public getter: Get_PrivateMember() that returns const reference
// - NO setter

// CK_PROPERTY(_PrivateMember) automatically generates:
// - A public getter: Get_PrivateMember() that returns const reference
// - A public setter: Set_PrivateMember(value)

// ALWAYS use the generated getter/setter methods for read-only access:

// CORRECT - Read-only access using generated getter:
for (const auto& Intent : InCurrent.Get_ActiveIntents())
{
    // Process intent - this works from anywhere
}

// WRONG - Direct private member access from non-friend:
for (const auto& Intent : InCurrent._ActiveIntents)  // Won't compile
{
    // This violates encapsulation
}

// Friend classes (Processors, Utils) can access private members directly for modification:
// This is ONLY allowed for friend classes that need to modify state:

// CORRECT - Friend class modifying state:
InCurrent._ActiveIntents.Add(Intent);           // Direct modification by friend
InCurrent._CachedBestTargets.Remove(Intent);    // Direct modification by friend

// WRONG - Trying to modify through getter:
InCurrent.Get_ActiveIntents().Add(Intent);      // Won't compile - getter returns const reference

// Pattern Summary:
// 1. Use getters (Get_XXX()) for ALL read-only access
// 2. Use direct private access (_XXX) ONLY in friend classes for modification
// 3. Declare friend relationships in Fragment headers when modification needed
// 4. The macros handle const-correctness and proper access patterns automatically

// Example Fragment with proper friend declarations:
struct CKMODULE_API FFragment_Example_Current
{
    CK_GENERATED_BODY(FFragment_Example_Current);

    friend class FProcessor_Example_HandleRequests;  // Needs to modify state
    friend class UCk_Utils_Example_UE;               // Needs to modify state

private:
    TSet<FGameplayTag> _ActiveItems;

public:
    CK_PROPERTY_GET(_ActiveItems);  // Generates Get_ActiveItems() const
}

// ============================================================================
// 6. INTERFACE DESIGN PRINCIPLES
// ============================================================================

// Avoid TOptional in UFUNCTION and UPROPERTY - doesn't work with Blueprints/Angelscript
// BAD:
UFUNCTION(BlueprintCallable)
static void SomeFunction(TOptional<int32> InValue);

UPROPERTY(EditAnywhere)
TOptional<int32> _SomeValue;

// GOOD: Use enum + value pattern
UENUM(BlueprintType)
enum class ECk_ValueMode : uint8
{
    UseDefault,
    Override
};

// Prefer enums over bool options - more self-documenting and extensible
// BAD:
UPROPERTY(EditAnywhere)
bool _AllowSomething = true;

// GOOD:
UENUM(BlueprintType)
enum class ECk_SomethingBehavior : uint8
{
    Block,
    Allow
};

// Design for THREE environments: C++, Blueprints, AND Angelscript

// ============================================================================
// 7. REQUEST STRUCT PATTERNS
// ============================================================================

// Use request structs following established pattern (like Probe system)
// Single struct per request type - don't create separate variants

USTRUCT(BlueprintType)
struct MYMODULE_API FCk_Request_SomeAction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_SomeAction);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_SomeAction);

private:
    // Essential parameters - cannot have meaningful defaults, go in constructor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _RequiredTag;

    // Optional parameters - have meaningful defaults, use setters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_SomeMode _Mode = ECk_SomeMode::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                     EditCondition = "_Mode == ECk_SomeMode::Override"))
    int32 _OverrideValue = 50;

public:
    // Essential parameters use CK_PROPERTY_GET (no setters)
    CK_PROPERTY_GET(_RequiredTag);

    // Optional parameters use CK_PROPERTY (with setters)
    CK_PROPERTY(_Mode);
    CK_PROPERTY(_OverrideValue);

public:
    // Constructor takes only essential parameters
    CK_DEFINE_CONSTRUCTORS(FCk_Request_SomeAction, _RequiredTag);
};

// UFUNCTION should take request struct
UFUNCTION(BlueprintCallable, Category = "Ck|Utils|MyFeature")
static FCk_Handle_MyFeature
Request_SomeAction(
    UPARAM(ref) FCk_Handle_MyFeature& InHandle,
    const FCk_Request_SomeAction& InRequest);

// ============================================================================
// 8. ENUM + VALUE PATTERN FOR OPTIONAL OVERRIDES
// ============================================================================

// When replacing TOptional<T>, use enum mode + value:
if (InRequest.Get_PriorityOverrideMode() == ECk_PriorityOverride::Override)
{
    auto Priority = InRequest.Get_PriorityOverrideValue();
    // Use override value
}

// ============================================================================
// 9. ECS FRAMEWORK PATTERNS
// ============================================================================

// Naming conventions:
struct FCk_Fragment_AudioTrack_Params { /* ... */ };     // Fragment_[Feature]_[Type]
struct FTag_AudioTrack_NeedsSetup { /* ... */ };         // Tag_[Feature]_[Purpose]
struct FCk_Handle_AudioTrack : public FCk_Handle_TypeSafe { /* ... */ };  // Handle_[Feature]
class UCk_Utils_AudioTrack_UE : public UBlueprintFunctionLibrary { /* ... */ };  // Utils_[Feature]_UE
class FProcessor_AudioTrack_Setup { /* ... */ };         // Processor_[Feature]_[Purpose]

// IMPORTANT: TypeSafe handles go in _Fragment_Data.h, never in _Fragment.h

// Handle patterns:
auto SelfHandle = ck::SelfEntity(this);
auto OwnerHandle = ck::GetOwnerEntity(SelfHandle);

// Probe/Signal binding patterns:
UFUNCTION(BlueprintCallable,
	Category = "Ck|Utils|Probe",
	DisplayName = "[Ck][ProbeTrace] Bind To OnEndOverlap")
static FCk_Handle_ProbeTrace
BindTo_OnEndOverlap_ProbeTrace(
	UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
	const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate,
	ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightthisFrame,
	ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

UFUNCTION(BlueprintCallable,
		  Category = "Ck|Utils|Probe",
		  DisplayName = "[Ck][ProbeTrace] Unbind From OnEndOverlap")
static FCk_Handle_ProbeTrace
UnbindFrom_OnEndOverlap_ProbeTrace(
	UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
	const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate);

// definition of the bind
auto
    UCk_Utils_Probe_UE::
    BindTo_OnEndOverlap_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceEndOverlap, InProbeTraceEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEndOverlap_ProbeTrace(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate)
        -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceEndOverlap, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

// ============================================================================
// 10. UNREAL ENGINE SPECIFICS
// ============================================================================

// Common operations:
auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InEntity);
auto Context = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InEntity);
GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(TArray<UObject*>);

// World time retrieval pattern:
const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
const auto CurrentTime = TimeResult.Get_WorldTime().Get_Time();

// Common includes:
#include "CkCore/Chrono/CkChrono.h"  // For FCk_Chrono
#include "CkCore/Time/CkTime_Utils.h"  // For world time utilities

// ============================================================================
// STANDALONE COMPONENT REGISTRATION (NO ACTOR OWNER)
// ============================================================================

// When creating UActorComponent instances without an Actor owner (e.g., in ECS):
// DO NOT use NewObject + manual RegisterComponent - this causes registration issues

// BAD - Manual component creation and registration:
auto NiagaraComponent = NewObject<UNiagaraComponent>(World);
NiagaraComponent->SetWorldTransform(Transform);
NiagaraComponent->RegisterComponent();  // ❌ Will fail or cause ensures

// GOOD - Use factory functions that handle registration:
constexpr auto AutoDestroy = false;
constexpr auto AutoActivate = false;
constexpr auto PreCullCheck = true;
auto NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    World,
    Effect,
    Location,
    Rotation,
    Scale,
    AutoDestroy,
    AutoActivate,
    ENCPoolMethod::None,
    PreCullCheck
);

// Factory functions like SpawnSystemAtLocation properly register components with the world
// Use similar factory patterns for other component types (Audio, etc.)

// UObjects cannot use CK_DEFINE_CONSTRUCTORS - Unreal generates its own

// UFUNCTION patterns:
UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AudioTrack")
static FCk_Handle_AudioTrack // UFUNCTIONs CANNOT have trailing return types in headers
Request_Play(
    UPARAM(ref) FCk_Handle_AudioTrack& InTrack,
    const FCk_Request_AudioTrack_Play& InRequest);

// UPROPERTY patterns:
UPROPERTY(EditAnywhere, BlueprintReadWrite,
          Category = "Audio",
          meta = (AllowPrivateAccess = true, Categories = "Audio.Track"))
FGameplayTag _TrackName;

// Private members with public accessors:
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Volume = 1.0f;

public:
    CK_PROPERTY(_Volume); // Generates getter/setter

// ============================================================================
// 11. MEMORY MANAGEMENT
// ============================================================================

// Use TStrongObjectPtr for UObject references in ECS components
TStrongObjectPtr<UAudioComponent> _AudioComponent;

// Use TObjectPtr for UPROPERTY UObject references
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
TObjectPtr<USoundBase> _Sound;

// ============================================================================
// COMPONENT LIFETIME MANAGEMENT IN ECS
// ============================================================================

// Pattern: Components stored in fragments should be destroyed in EndPlay processor
// Do NOT destroy components before the entity is destroyed

// Fragment storage:
struct FFragment_VfxCue_Current
{
    friend class FProcessor_VfxCue_Setup;
    friend class FProcessor_VfxCue_EndPlay;

private:
    TStrongObjectPtr<UNiagaraComponent> _NiagaraComponent;

public:
    CK_PROPERTY_GET(_NiagaraComponent);
};

// Setup processor - Create component:
auto
    FProcessor_VfxCue_Setup::
    ForEachEntity(...)
    -> void
{
    constexpr auto AutoDestroy = false;
    constexpr auto AutoActivate = false;
    auto Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(...);
    InCurrent._NiagaraComponent = TStrongObjectPtr{Component};
}

// Lifetime processor - Monitor state, fire signals, but DON'T destroy:
auto
    FProcessor_VfxCue_LifetimeMonitor::
    ForEachEntity(...)
    -> void
{
    auto Component = InCurrent._NiagaraComponent.Get();
    if (Component->IsActive() == false)
    {
        // Fire completion signals, remove tags
        // But DON'T call DestroyComponent() here
        UUtils_Signal_OnFinished::Broadcast(InHandle, ...);
    }
}

// EndPlay processor - Destroy component when entity is destroyed:
auto
    FProcessor_VfxCue_EndPlay::
    ForEachEntity(...)
    -> void
{
    auto Component = InCurrent._NiagaraComponent.Get();
    if (ck::IsValid(Component))
    {
        Component->DestroyComponent();  // ✓ Destroy here, during entity cleanup
    }
    InCurrent._NiagaraComponent.Reset();
}

// This ensures proper cleanup order:
// 1. Signal fires → EntityScript receives OnFinished
// 2. EntityScript destroys entity (if AutoDestroy behavior)
// 3. EndPlay processor destroys component
// Component lifetime is tied to entity lifetime

// ============================================================================
// 12. ERROR HANDLING & LOGGING
// ============================================================================

// Validation with early returns:
CK_ENSURE_IF_NOT(ck::IsValid(Handle),
    TEXT("Invalid handle in function [{}]"), __FUNCTION__)
{ return; }

// Logging levels:
ck::audio::Verbose(TEXT("Starting track [{}]"), TrackName);
ck::audio::VeryVerbose(TEXT("Debug info: [{}]"), DebugValue);
ck::audio::Warning(TEXT("Potential issue: [{}]"), Issue);

// ============================================================================
// 13. ARTIFACT FORMATTING GUIDELINES
// ============================================================================

// When providing code artifacts:

// 1. BE SPECIFIC ABOUT LOCATIONS
//    ✅ "In CkAudioCue_Utils.cpp, in UCk_Utils_AudioCue_UE::Add() method, line ~25"
//    ❌ "Update director params creation in Add() method"

// 2. USE CLEAR SECTION HEADERS for each file/section

// 3. INDICATE NON-SEQUENTIAL CODE:
// ============================================================================
// [ELSEWHERE IN SAME FILE] - CkSomeFile.cpp
// ============================================================================

// 4. ALWAYS INCLUDE HEADERS - don't assume they'll be adjusted automatically

// 5. PROVIDE COMPLETE CONTEXT - show surrounding code for unambiguous location

// EXAMPLE STRUCTURE:

// ============================================================================
// CkAudioTrack_Fragment_Data.h - Add enum after ECk_AudioTrack_State (line ~45)
// ============================================================================

// UENUM(BlueprintType)
// enum class ECk_LoopBehavior : uint8
// {
//     PlayOnce,
//     Loop
// };

// ============================================================================
// [ELSEWHERE IN SAME FILE] - CkAudioTrack_Fragment_Data.h - Update struct (line ~120)
// ============================================================================

// // Replace _Loop property with:
// UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
// ECk_LoopBehavior _LoopBehavior = ECk_LoopBehavior::Loop;

// ============================================================================
// 14. ANGELSCRIPT COMPATIBILITY
// ============================================================================

// Same patterns as C++ but ensure UFUNCTION compatibility:
// - Use auto aggressively
// - Invert if statements for early out
// - No 'b' prefix for booleans
// - Get_ prefix for getters, Request_ prefix for mutating functions
// - Add suffix for function overloads (UFUNCTION limitation)

// ============================================================================
// 15. ANGELSCRIPT ASSET CREATION ('asset ... of ...' SYNTAX)
// ============================================================================

// Angelscript can create UDataAsset instances directly from script using the
// 'asset' keyword. The resulting asset is registered with the engine as if it
// were authored in the editor - no .uasset file to manage, no manual editor
// steps required. This is the preferred way to define data assets that are
// conceptually owned by a particular .as file (e.g. a gym's gameplay tags,
// asset-registry configs, etc).
//
// SYNTAX:
//   namespace Ck  // or any namespace, including none
//   {
//       asset <AssetName> of <UDataAssetSubclass>
//       {
//           // Initializer body - set public UPROPERTY fields directly,
//           // or call methods on them (e.g. GameplayTags.Add(...))
//           Field1 = "some value";
//           Field2.Add(n"Some.Tag");
//       }
//   }
//
// EXAMPLE 1 - Gameplay tags asset (defines tags without editing .ini files):
//   namespace Ck
//   {
//       asset Asset_Tags of UCk_GameplayTags
//       {
//           GameplayTags.Add(n"MyFeature.Category.SomeTag");
//           GameplayTags.Add(n"MyFeature.Category.AnotherTag");
//       }
//   }
//
// EXAMPLE 2 - Asset registry config (scans a folder and auto-generates a .as
// file with typesafe accessors for assets found there):
//   #if EDITOR
//       asset MyFeature_AssetRegistryConfig of UCkAssetRegistryConfig
//       {
//           AssetDiscoveryRoot = "/MyPlugin/MyFolder";
//           OutputFileName     = "my_feature_assets.as";
//           Namespace          = "my_feature_assets";
//       }
//   #endif
//
// #if EDITOR IS REQUIRED for editor-only types. UCkAssetRegistryConfig is
// editor-only, so any 'asset ... of UCkAssetRegistryConfig' definition MUST be
// wrapped in '#if EDITOR' / '#endif' - otherwise the script fails to compile
// at engine startup with "Cannot use editor-only type ... outside of an EDITOR
// block".
//
// PROPERTY ACCESS IN INITIALIZER:
// - Public UPROPERTY fields are assigned directly: 'Field = value;'
// - Can also call methods on fields: 'TagContainer.Add(n"Some.Tag");'
// - Private UPROPERTY fields with BlueprintReadWrite are accessible the same way
// - Order doesn't matter - it's just an initializer block
//
// WHEN TO USE:
// - Feature-specific data assets that belong with the script that uses them
// - Gameplay tags that are only relevant to one feature/gym
// - Asset registry configs pointing at generated-code folders
// - Any place you'd otherwise have to manually create a .uasset in the editor
//
// WHEN NOT TO USE:
// - Assets that need hand-authored content (meshes, textures, blueprints)
// - Assets that will be edited frequently by non-programmers
// - Shared assets that live in engine Content folders
//
// GAMEPLAY TAGS — DECLARE WHERE USED:
// Tags (UCk_GameplayTags assets) should be declared in the same file that
// primarily uses them — typically the _Shared.as file for a feature, or
// directly in a station file if the tags are only used there. Do NOT create
// a separate _Assets.as file just for tags. Tags are lightweight metadata
// that belong alongside the code that references them.
//   GOOD: tags in CkInteractionGym_Shared.as (shared by all stations)
//   GOOD: tags in CkInventoryGym_Spatial.as (only used by that station)
//   BAD:  tags in CkInteractionGym_Assets.as (separate file just for tags)
//
// FILE NAMING CONVENTION FOR HEAVYWEIGHT ASSETS:
// Files containing heavyweight `asset ... of ...` declarations (item
// definitions, ability configs, etc. — anything beyond simple tags) must use
// the `_Assets.as` suffix. This makes asset-containing files easy to find
// and keeps them separate from logic files.
//   GOOD: CkInventoryGym_Assets.as (item definitions with traits)
//   BAD:  CkInventoryGym_Shared.as (item definitions mixed with helpers)

// ============================================================================
// 16. PROBLEM-SOLVING PROTOCOL
// ============================================================================

// When stuck or confused:
// 1. STOP - Don't spiral into complex solutions
// 2. Delegate - Consider spawning agents for parallel investigation
// 3. Step back - Re-read requirements
// 4. Simplify - Simple solution is usually correct
// 5. Ask - "I see two approaches: [A] vs [B]. Which do you prefer?"

// Communication: "The current approach works, but I notice [observation].
//                 Would you like me to [specific improvement]?"

// ============================================================================
// 17. TESTING & VALIDATION
// ============================================================================

// Always test in all environments: C++, Blueprints, Angelscript
// Measure first - no premature optimization
// Benchmark before claiming performance improvements
// Ask for benchmark runs when needed

// Test request structs in all contexts:
// 1. Pure C++ usage
// 2. Blueprint node usage
// 3. Angelscript usage

// Ensure request structs work properly in Blueprint editors
// Verify EditCondition metadata works correctly
// Test enum dropdowns appear correctly in all editors

// ============================================================================
// REMINDER: If this file hasn't been referenced in 30+ minutes, RE-READ IT!
// ============================================================================