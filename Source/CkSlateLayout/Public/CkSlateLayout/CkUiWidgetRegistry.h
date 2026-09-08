#pragma once

#include "CkSlateLayout/CkUiDocument.h"

#include "Framework/SlateDelegates.h"
#include "Widgets/SWidget.h"

struct FSlateBrush;
class FCkUiCollection;

DECLARE_DELEGATE_OneParam(FCkUiOnBoolChanged, bool);
DECLARE_DELEGATE_OneParam(FCkUiOnNumberChanged, float);
DECLARE_DELEGATE_TwoParams(FCkUiOnNumberCommitted, float, ETextCommit::Type);
DECLARE_DELEGATE_OneParam(FCkUiOnStringChanged, const FString&);

/** A numeric interaction has an explicit end; capture loss must not masquerade as a commit. */
enum class ECkUiInteractionPhase : uint8 { Begin, Commit, Cancel };
enum class ECkUiInteractionSource : uint8 { Pointer, Keyboard, Controller };

struct FCkUiNumberInteraction
{
    ECkUiInteractionPhase Phase = ECkUiInteractionPhase::Begin;
    ECkUiInteractionSource Source = ECkUiInteractionSource::Pointer;
    float Value = 0.0f;
};

DECLARE_DELEGATE_OneParam(FCkUiOnNumberInteraction, const FCkUiNumberInteraction&);


struct FCkUiCustomPropertySchema
{
    /** Literal kinds and events use Name="..."; value bindings use Name-bind="binding". */
    FString Name;
    ECkUiCustomPropertyKind Kind = ECkUiCustomPropertyKind::Text;
    bool bRequired = true;
};

struct FCkUiCustomSlotSchema
{
    FString Name;
    bool bRequired = true;
};

struct FCkUiCustomWidgetSchema
{
    FString Tag;
    TArray<FCkUiCustomPropertySchema> Properties;
    /** Optional typed CSS properties. Names use the -ck- prefix; shared names must share a kind. */
    TMap<FString, ECkUiCustomStyleKind> StyleProperties;
    /** Optional required literal Text property used as the retained component compatibility key. */
    FString StateKeyProperty;
    TArray<FCkUiCustomSlotSchema> Slots;
};

struct FCkUiCustomWidgetArguments
{
    FString Id;
    FCkUiStyle Style;
    TMap<FString, FText> TextProperties;
    TMap<FString, float> NumberProperties;
    TMap<FString, bool> BoolProperties;
    TMap<FString, FLinearColor> ColorProperties;
    TMap<FString, TAttribute<FText>> TextBindings;
    TMap<FString, TAttribute<const FSlateBrush*>> ImageBindings;
    TMap<FString, TAttribute<float>> NumberBindings;
    TMap<FString, TAttribute<bool>> BoolBindings;
    TMap<FString, TAttribute<FString>> StringBindings;
    TMap<FString, TAttribute<FLinearColor>> ColorBindings;
    TMap<FString, TSharedPtr<FCkUiCollection>> Collections;
    TMap<FString, FSimpleDelegate> Actions;
    TMap<FString, FOnTextChanged> TextChanged;
    TMap<FString, FOnTextCommitted> TextCommitted;
    TMap<FString, FCkUiOnBoolChanged> BoolChanged;
    TMap<FString, FCkUiOnNumberChanged> NumberChanged;
    TMap<FString, FCkUiOnNumberCommitted> NumberCommitted;
    TMap<FString, FCkUiOnNumberInteraction> NumberInteraction;
    TMap<FString, FCkUiOnStringChanged> StringChanged;
    /** Authored source name for every value binding, event, and action property. */
    TMap<FString, FString> BindingNames;
    /** False while a view stages/commits a reload or after its owner has released. Editable controls must gate all consumer event dispatch through this attribute. */
    TAttribute<bool> CanDispatchEvents;
    /** Consumer-selected base font for standard Slate controls. */
    FSlateFontInfo BaseFont;
    /** Persistent opaque slot mounts. The factory mounts each supplied box once beneath its detached widget. */
    TMap<FString, TSharedPtr<SWidget>> Slots;
    /** Runtime/teardown only: release this host user's captures beneath a named slot. Never call from factory, PrepareReload, or Commit. */
    TFunction<void(const FString&)> ReleaseSlotPointerCaptures;
    /** Host-owned Slate user, never authored in markup. INDEX_NONE means no explicit owner. */
    int32 SlateUserIndex = INDEX_NONE;
};

class CKSLATELAYOUT_API ICkUiPreparedWidgetUpdate
{
public:
    virtual ~ICkUiPreparedWidgetUpdate() = default;
    /** Non-failing publication only: use prepared local configuration; do not emit edits/actions, reenter the view, change focus, or mutate models/timers. */
    virtual void Commit() noexcept = 0;
};

/** An active pointer owned by a retained component; reports never acquire capture. */
struct FCkUiPointerCapture
{
    int32 UserIndex = INDEX_NONE;
    uint32 PointerIndex = 0;
    TWeakPtr<SWidget> Widget;
};

/**
 * Retained components must keep external captures weak where appropriate. PrepareReload must not mutate mounted Slate,
 * focus, callbacks, models, or timers; its returned update is published once through non-failing Commit().
 * Destruction releases ownership only and must not act as an external removal callback.
 */
class CKSLATELAYOUT_API ICkUiRetainedWidget
{
public:
    virtual ~ICkUiRetainedWidget() = default;
    virtual auto GetWidget() const -> TSharedRef<SWidget> = 0;
    /** Optional ancestor of an owned focused leaf used to refresh ancestry without leaving its popup.
     * Read-only; the view validates mounted ancestry and respects callback focus redirects. */
    virtual auto GetFocusTransferTarget() const -> TSharedPtr<SWidget> { return {}; }
    /**
     * Opt in to capture ancestry repair during publication. Read-only snapshot of active owned pointers.
     * Removed components lose capture without consumer notification; native local state must reset.
     */
    virtual auto GetPointerCaptures() const -> TArray<FCkUiPointerCapture> { return {}; }
    /** Release owned transient UI when an ancestor becomes inactive. Preserve persistent model/draft state. */
    virtual void ReleaseTransientInteraction() {}
    /** Local-state-only hooks: suppress this exact synthetic capture loss, then retain or reset the
     * interaction according to InRestored. No events, model/focus/capture mutation or view reentry. */
    virtual void BeginPointerCaptureTransfer(const FCkUiPointerCapture&) noexcept {}
    virtual void EndPointerCaptureTransfer(const FCkUiPointerCapture&, bool InRestored) noexcept {}

    virtual auto PrepareReload(const FCkUiCustomWidgetArguments&, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> = 0;
};

/**
 * Trusted factory contract: create a new detached widget without external side effects. A non-empty OutFailure rejects the output.
 * Factory outputs are recreated; use RetainedFactory for stateful controls.
 */
using FCkUiCustomWidgetFactory = TFunction<TSharedPtr<SWidget>(const FCkUiCustomWidgetArguments&, FString& OutFailure)>;
using FCkUiRetainedWidgetFactory = TFunction<TSharedPtr<ICkUiRetainedWidget>(const FCkUiCustomWidgetArguments&, FString& OutFailure)>;

struct FCkUiCustomWidgetRegistration
{
    FCkUiCustomWidgetSchema Schema;
    FCkUiCustomWidgetFactory Factory;
    FCkUiRetainedWidgetFactory RetainedFactory;
};

/** Immutable copy of registered custom leaf definitions and trusted factories. */
class CKSLATELAYOUT_API FCkUiWidgetRegistrySnapshot final
{
public:
    auto Find(const FString& InTag) const -> const FCkUiCustomWidgetRegistration*;
    auto FindStyleProperty(const FString& InName) const -> const ECkUiCustomStyleKind*;

private:
    friend class FCkUiWidgetRegistry;
    TMap<FString, FCkUiCustomWidgetRegistration> _Registrations;
    TMap<FString, ECkUiCustomStyleKind> _StyleProperties;
};

/** Mutable setup registry. Views receive only CreateSnapshot() results. */
class CKSLATELAYOUT_API FCkUiWidgetRegistry final
{
public:
    auto Register(FCkUiCustomWidgetRegistration InRegistration) -> FCkUiLoadResult;
    auto CreateSnapshot() const -> TSharedRef<const FCkUiWidgetRegistrySnapshot>;

private:
    TMap<FString, FCkUiCustomWidgetRegistration> _Registrations;
};
