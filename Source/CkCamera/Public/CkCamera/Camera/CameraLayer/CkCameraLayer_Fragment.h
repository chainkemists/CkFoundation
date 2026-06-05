#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAttribute/CkAttribute_Fragment_Data.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment_Data.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------
// Camera "layer" fragments — the SmTask-style data side of UCk_CameraLayer_EntityScript. A layer is a child entity
// of the Camera director; it acquires attribute modifiers on the director's tuner attributes (named by the layer's
// auto-generated gameplay tag), and the framework auto-blends them in/out via FFragment_CameraLayer_Blend.
// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraLayer_EntityScript;

namespace ck
{
    // Per-layer identity/config: the script class (so RemoveLayer / OneOnly can match), the ordering group, and an
    // optional look-at target for auto-reorient.
    struct CKCAMERA_API FFragment_CameraLayer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraLayer_Params);

    public:
        friend class FProcessor_Camera_HandleRequests;

    private:
        TSubclassOf<UCk_CameraLayer_EntityScript> _LayerClass;
        int32                                     _Priority = 0;
        FCk_Handle_Transform                      _LookAtTarget;

        // The persistent base layer created by UCk_Utils_Camera_UE::Add (one per director). It represents the
        // resting profile, is pinned at full blend, and is never evicted (OneOnly), pruned, or removed — feature
        // layers always blend back to it. Lives at the lowest priority so any real layer dominates it.
        bool _IsDefault = false;

    public:
        CK_PROPERTY_GET(_LayerClass);
        CK_PROPERTY(_Priority);
        CK_PROPERTY(_LookAtTarget);
        CK_PROPERTY(_IsDefault);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_CameraLayer_Params, _LayerClass);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Per-layer blend weight in [0,1]. _Alpha interpolates toward _TargetAlpha at _BlendRate (units/sec). Blend-in:
    // target 1. Blend-out (removal / OneOnly eviction): target 0 → pruned when it reaches 0. The blend processor
    // advances _Alpha and rewrites each acquired modifier's effective delta from this weight — invisible to the layer.
    struct CKCAMERA_API FFragment_CameraLayer_Blend
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraLayer_Blend);

    public:
        friend class FProcessor_Camera_HandleRequests;
        friend class FProcessor_CameraLayer_Lifecycle;
        friend class FProcessor_CameraLayer_Blend;

    private:
        float _Alpha       = 0.0f;
        float _TargetAlpha = 1.0f;
        float _BlendRate   = 1000.0f; // ~instant unless overridden

        // Edge-detect guards so the blend processor fires FullyBlendedIn / FullyBlendedOut
        // exactly once per crossing. Each fire resets the opposite guard, so a layer that
        // blends in, out, then in again re-fires correctly.
        bool _FiredBlendedIn  = false;
        bool _FiredBlendedOut = false;

    public:
        CK_PROPERTY(_Alpha);
        CK_PROPERTY(_TargetAlpha);
        CK_PROPERTY(_BlendRate);
        CK_PROPERTY(_FiredBlendedIn);
        CK_PROPERTY(_FiredBlendedOut);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // One acquired-modifier record: the modifier entity, the tuner attribute it modifies, the layer-authored target
    // (the value at full blend), the user-facing operation, and the targeted component (Current for scalars; Min/Max
    // for FloatRange tuners). The blend processor scales target→effective each frame; teardown removes the modifier.
    template <typename T_ModHandle, typename T_AttrHandle, typename T_Delta>
    struct TCk_CameraLayer_AcquiredModifier
    {
        T_ModHandle                     _Modifier;
        T_AttrHandle                    _Attribute;
        T_Delta                         _Target    = T_Delta{};
        ECk_AttributeModifier_Operation _Op        = ECk_AttributeModifier_Operation::Add;
        ECk_MinMaxCurrent               _Component = ECk_MinMaxCurrent::Current;
    };

    using FCk_CameraLayer_FloatModifier   = TCk_CameraLayer_AcquiredModifier<FCk_Handle_FloatAttributeModifier,   FCk_Handle_FloatAttribute,   float>;
    using FCk_CameraLayer_VectorModifier  = TCk_CameraLayer_AcquiredModifier<FCk_Handle_VectorAttributeModifier,  FCk_Handle_VectorAttribute,  FVector>;
    using FCk_CameraLayer_RotatorModifier = TCk_CameraLayer_AcquiredModifier<FCk_Handle_RotatorAttributeModifier, FCk_Handle_RotatorAttribute, FRotator>;
    using FCk_CameraLayer_IntegerModifier = TCk_CameraLayer_AcquiredModifier<FCk_Handle_IntegerAttributeModifier, FCk_Handle_IntegerAttribute, int32>;

    // The attribute modifiers this layer has acquired, kept in per-type arrays so the blend processor iterates each
    // typed array directly (no per-entry type dispatch) and teardown removes them — they live under the tuner
    // ATTRIBUTE, not under the layer, so they are not auto-destroyed with the layer entity.
    struct CKCAMERA_API FFragment_CameraLayer_AcquiredModifiers
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraLayer_AcquiredModifiers);

    public:
        TArray<FCk_CameraLayer_FloatModifier>   _Float;
        TArray<FCk_CameraLayer_VectorModifier>  _Vector;
        TArray<FCk_CameraLayer_RotatorModifier> _Rotator;
        TArray<FCk_CameraLayer_IntegerModifier> _Integer;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Marks an active layer (admitted to the compose loop). Stamped by EnterLayer, cleared by ExitLayer.
    CK_DEFINE_ECS_TAG(FTag_CameraLayer_Active);

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_CameraLayer_OwningCamera, FFragment_CameraLayer_OwningCamera, FCk_Handle_Camera);

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfCameraLayers, FCk_Handle_CameraLayer);

    // Shared record-of-layers utility (used by both the processors and the Utils class).
    struct CKCAMERA_API FUtils_RecordOfCameraLayers : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfCameraLayers> {};
}

// --------------------------------------------------------------------------------------------------------------------
