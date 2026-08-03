# Bind-before-broadcast, authority, and readiness gating

Reference for `ck-game-replication-patterns`: the rep-notify one-shot (§3), what runs where and how a client asks for a change (§4), and gating on late-arriving net entities (§5).

## 3. Bind before broadcast — the rep-notify one-shot

On a client, replicated carrier fragments' initial values are applied by the framework's
dispatch processor, which broadcasts their **one-shot** `OnRepNotify` at apply time. If your
client-side setup code binds that notify *after* the apply already ran, the values sit there
pre-filled, nothing changes again, and the notify never re-fires. Symptom: **blank panels /
default values on MP clients, server and listen-host fine.**

Corpus incident (verified): `d6f9785a4` "fix(storedriver): consume replicated carriers after
rep-notify bind on MP clients" — StoreDriver subordinate panels were blank on clients because
`FProcessor_ReplicatedFragments_Dispatch` applied and broadcast before the client's
`NeedsSetup` processor bound `BindTo_OnRepNotify`. Two valid fixes; the corpus uses (a):

- **(a) Bind, then reconcile once immediately** — after binding, run your reconcile function
  once to consume already-applied values; the notify handles subsequent updates. The shipped
  fix (`Script/StoreDriver/BB_StoreDriver_Processor_Setup.as:43-52`) does exactly this, with
  the why in a comment ending "replicated container values are applied AFTER OnConstructed".
- **(b) Bind with a replay-capable policy.** Ck signal binds take a binding policy; verbatim
  names from the root doctrine (`Plugins/CkFoundation/CLAUDE.md`, "Signals" — quoted verbatim
  because a past doc typo'd them): `ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame`
  (replay same-frame payload), `FireIfPayloadInFlight` (replay last payload from any frame),
  `IgnorePayloadInFlight` (future fires only). If the bind surface you're using exposes the
  policy, `FireIfPayloadInFlight` makes a late bind safe. Semantics, replay limits ("only the
  last payload replays"), and the unbind interaction: `ckecs-architecture-contract` §5.

The general law for any client-side one-shot (rep-notify, `Promise_OnConstructed`,
`OnReplicationComplete`): **either bind before the broadcast can happen, or use a
replay-capable bind, or reconcile manually after binding.** Never assume "it will fire
eventually" — on a client that joined with state already in flight, it already fired.

---

## 4. Authority — what runs where, and how a client asks for a change

### 4.1 Server-only logic is gated, not assumed

Anything that *decides* runs on the authority. Two corpus forms (verified):

- **Server-only spawn**: `if (System::IsServer() == false) { return; }` before spawning drivers
  (`BB_Gameplay_GameState.as:74-75`). Server-authoritative drivers with no client mirror
  (corpus: CollectiblesDriver, ItemOverflowDriver) are `DoesNotReplicate` + server-only spawn —
  clients never even have the entity.
- **Authority gate inside shared code**: `utils_net::Get_HasAuthority(Handle)` — the NPC's AI
  tier (`BB_Npc_EntityScript.as:625-628`) and every cue body that writes state (below).

### 4.2 Client → server requests route through cues

Framework `Request_*` mutations on replicated state are authority-gated and **silently dropped
when called without authority** — a client-side call no-ops with no error. Symptom: a button /
interaction works on the listen server, does nothing on a remote client.

The established corpus pattern (two verified incidents that converged on it):

- `a421b2ce2` "fix(OpenSign): client→server toggle cue for the replicated IsOpen attribute" —
  the interaction SM ran on the initiating machine and wrote the replicated attribute locally:
  client toggles no-op'd, and a locally-diverged client later *fought* server values. Fix
  established the pattern: the initiating machine fires a **Reliable cue owned by the
  replicated entity**; the cue body **authority-gates and only the server writes**; the
  attribute's own replication converges every client.
- `e140915f7` "fix: Route LootAll button through server cue for client authority" — a widget
  click handler called an authority-gated mass transfer directly; remote clients silently
  no-op'd. Same fix shape.

The pattern, generalized (fire side from `Script/ECS/OpenSign/BB_OpenSign_Hfsm.as:57-62`, body
from `BB_OpenSign_Cues.as:41-57` — both verified):

```angelscript
// Initiating machine (any): fire the cue AT the replicated entity.
utils_cue_generic::Request_ExecuteCue(
    ReplicatedOwnerEntity,
    utils_gameplay_tag::ResolveGameplayTag(n"Cue.<Game>.<Feature>.<Action>"),
    U<Prefix>_Cue_<Feature>_<Action>::Params(DesiredState),
    ECk_Cue_ReliabilityPolicy::Reliable,
    ECk_Cue_MulticastPolicy::ServerAndAllClients);

// Cue body: everyone may run it; ONLY the authority writes.
UFUNCTION(BlueprintOverride)
void DoBeginPlay(FCk_Handle InHandle)
{
    auto _CkPerfScope = ck::ScopedStat();
    const auto Owner = ck::OwnerEntity(InHandle);
    if (utils_net::Get_HasAuthority(Owner) == false)
    { return; }
    // ... the single authoritative write; replication converges the clients.
}
```

Design details that made the pattern robust (from the `a421b2ce2` commit body, verified):
carry the **desired state**, not "toggle", so double-fires converge instead of double-flipping;
make the durable state a replicated attribute and keep the cue a one-shot event
(`_LifetimeBehavior = AfterOneFrame`); handles ride the RPC and re-resolve server-side (safe
for already-replicated entities — no spawn race). A `ServerOnly` multicast policy is the
variant when only the server needs to run the body at all (`Script/Cues/BB_Cue_InventoryLootAll.as`).

### 4.3 Never finish construction lazily on a replicated entity

`FProcessor_EntityScript_Replicate` gates on the construction-finish tag. If your entity script
returns `Continue` from `DoConstruct` and finishes only after a long async cascade, it can
finish **after** the Replicate processor's window — and then it never replicates to clients at
all. Corpus incident (verified): `1b0665a3f` "fix(storedriver): finish driver construction
eagerly so it replicates to clients" — an added async subordinate pushed the finish past the
window; clients got blank panels because the driver entity itself never arrived. Fix: **return
`Finished` from `DoConstruct` on the server path; track subordinate/async readiness separately
via your own Ready tag/signal** (§5). Rule of thumb: deferred construction-finish is for
"downstream `Promise_OnConstructed` consumers must see a fully-set-up entity", never for "wait
for my whole async world" — on a `Replicates` script the latter is a replication outage.

---

## 5. Readiness gating — net entities arrive late

On a client, replicated entities arrive whenever the net driver delivers them: after level
load, after your GameState's construction, in arbitrary order relative to each other, and their
replicated *values* arrive after the entities themselves (§1/§3). Gameplay code that reads a
net-delivered entity at its own construct time is a race you will lose on real clients.

Consumer patterns (all corpus-verified):

1. **Per-feature Acquire tickets with `Promise_OnReady`.** A singleton feature exposes
   `utils_<feature>::Acquire<Feature>(...)` returning a pending handle whose
   `Promise_OnReady(Delegate)` resolves whether the singleton already exists or shows up later
   (`Script/ECS/DayCycle/BB_DayCycle_Utils.as:72-98`; same shape for
   `AcquireMissionDriver`, `AcquireEventFeed`). This is the polite form of discovery — no
   polling, no ordering assumption. Teach this shape for any world-singleton your game adds.
2. **An explicit Ready predicate for compound readiness.** When "the entity exists" is not
   enough — discovery finished, subordinates bound, listeners live — expose a single accurate
   gate. Corpus example (BusterBlock Gauntlet helpers,
   `Plugins/BusterBlockTests/Script/Gauntlet/BB_GauntletStoreHelpers.as:36-48`):
   `Is_StoreReady()` returns false until the driver entity exists **and** has stamped its Ready
   tag — the in-code comment is the lesson: "An order broadcast before this reaches no
   listener." Generalize: any request/broadcast aimed at a driver must be gated on the driver's
   Ready contract, not on the entity handle being valid. Driver-side Ready mechanics (readiness
   counters, Ready-tag stamping) belong to `ck-game-driver-architecture`.
3. **Read replicated values only after `Promise_OnReplicationComplete`** — the framework-level
   gate for "the values are actually applied on this client"
   (`ckecs-architecture-contract` §7).

One-liner on environments: single-player PIE and listen-server PIE hide almost every bug in
this skill — timing collapses and the local machine has authority. Anything replication-touching
must be exercised with a real client (multi-PIE net autotest or packaged client) before it is
called done; for the triage of "works in PIE, broken packaged/net", see
`ck-game-debugging-playbook` and `ck-debugging-playbook` §6.

---

