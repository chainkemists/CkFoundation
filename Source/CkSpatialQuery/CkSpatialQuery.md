# CkSpatialQuery

Collision probes using Jolt Physics. Persistent shapes in the physics world that track overlaps each frame, plus one-shot line traces and shape casts.

## Key Concepts

- **Probe** — An ECS entity with a Jolt physics body. Tracks overlaps with other probes and fires signals on begin/update/end.
- **Motion Types** — Static, Kinematic, or Dynamic bodies. LinearCast (CCD) for fast-moving objects.
- **Response Policy** — `Notify` receives overlap signals; `Silent` receives none but remains a target for an admitting `Notify` probe.
- **Contact Participation** — `PhysicalContacts` allows normal Probe contact admission; `QueryOnly` remains visible to ProbeTrace but rejects physical Probe contacts in both directions.
- **Contact Admission** — Jolt creates a Probe/Probe contact only when at least one `Notify` side accepts the other probe name through its filter. `QueryOnly` rejects the pair before directional admission, while `Silent` deliberately preserves the targetable asymmetry.
- **Context Overlap Policy** — Controls whether probes overlap with same-context or different-context probes.
- **Persistent Traces** — Long-lived line/shape casts that update every frame.
- **Persistent Physics State Policy** — `CurrentSolved` waits for the just-dispatched batch before its
  Overlap-group query. `LatestCompleted` captures the latest completed scheduled pre-step state before Jolt
  Transform writers and reconciles those value hits in the same rendered frame.
- **Debug Draw** — Optional visualization with configurable colors per state.

## Example: NPC Overlap Detection

```mermaid
flowchart LR
    A["NPC with Capsule<br/>probe moves"] -->|"overlaps shelf"| B["OnBeginOverlap fires<br/>with contact info"]
    B -->|"NPC moves away"| C["OnEndOverlap fires"]
```

## Usage Examples

### Add a probe to an entity

```cpp
UCk_Utils_Probe_UE::Add(TransformEntity, ProbeParams, DebugInfo);
```

### Listen for overlaps

```cpp
UCk_Utils_Probe_UE::BindTo_OnBeginOverlap(ProbeHandle, OnBeginDelegate);
UCk_Utils_Probe_UE::BindTo_OnEndOverlap(ProbeHandle, OnEndDelegate);
```

### One-shot line trace

```cpp
auto Result = UCk_Utils_Probe_UE::Request_SingleLineTrace(ProbeHandle, TraceRequest);
```

### Enable/disable probe

```cpp
UCk_Utils_Probe_UE::Request_EnableDisable(ProbeHandle, false);
```

### Check overlap state

```cpp
bool Overlapping = UCk_Utils_Probe_UE::Get_IsOverlapping(ProbeHandle);
```

## Contact and transform performance contracts

- Probe contact signatures are immutable after publication to Jolt worker threads. The signature contains the probe name, response policy, contact participation, and tag filter; game-specific tags are not hard-coded in CkSpatialQuery.
- Contact admission preserves the existing directional callback contract: a pair is admitted when either notifying receiver accepts the other probe. `QueryOnly` is the explicit bilateral exception: it is traceable but excludes both directions before narrow phase. Context policy and overlap bookkeeping remain game-thread checks.
- Contact events must resolve both entities to the exact live Probe `BodyID`, including its sequence number. A sibling JoltBody on the same ECS entity must never masquerade as the Probe or end one of its overlaps.
- Probe pose changes are applied as one aligned Jolt batch per activation mode per processor tick. Body IDs, positions, and rotations must be appended in lockstep; stale body IDs are skipped, static probes move only during restore rebasing, and kinematic `QueryOnly` probes update with `DontActivate`.
- Persistent ProbeTrace defaults to `CurrentSolved`. Its completed just-dispatched batch intentionally does not
  include Probe transforms pushed later in PostPhysics. `LatestCompleted` does not wait for that just-dispatched
  batch: it reports the latest completed scheduled pre-step state, with its buffered results consumed only in the
  matching rendered frame.
- `ck.SpatialQuery.ProbePairAttributionFrames N` enables a bounded, temporary pair counter for diagnostics. It is off by default and must not be left armed during normal profiling.

## Tests

Probe coverage lives in `CkTests/Script/CkProbe` and includes contact routing, Begin/End behavior, enable/disable, LinearCast, and ProbeTrace scenarios.
