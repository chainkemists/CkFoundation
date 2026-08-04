# CkVoiceChat — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-04 (CkFoundation `feature/voice-chat` @ `cc95c7760`; CkTests
`feature/voice-chat-wip` @ `52a4d9f` — branch renamed upstream from `feature/voice-chat`):**
**Gates 0, 1, 2, and 3 (machine) ALL CLOSED AND AUDITED.** Gate-3 verdict **GO WITH CONDITIONS**;
**all three conditions implemented AND re-gated green 2026-08-04** — resolution table + run
evidence in [Gate_3.md](Gate_3.md) § "Audit-condition re-gate". C3's code (N2 throttled per-talker
drop Warning) compiled for the first time in that run.
**Baseline (current, post-rebase, BusterBlock host):** VoiceChat **30/30**
(Test-VoiceChatP3-postrebase2.log) + RenderTarget **22/22** delta-zero
(Test-P3-RenderTarget-postrebase2.log), both EXIT 0, 0 `Angelscript: Error`, same binary, no
rebuild between runs; freshness chain verified monotonic (sources 02:14:27 → DLL 02:21:14 → run
02:27:13).
**Next action:** **Gate 4 (P4) OPENED 2026-08-04** — [Gate_4.md](Gate_4.md) committed with entry
criteria ticked against the baseline above. Scope: HybridRadio near/far playback, per-channel
attenuation + source-effect chain (ADR-5 config is currently declared but has ZERO consumers),
moderation matrix, roger-beep recipe (docs+gym, no CkCue dep per Ruling Q4). Carried audit items
recorded there: **F5** (prune unbounded `ServeHistory`/`ListenerMuteMatrix` maps) is work item 6;
**F4** (strip campaign breadcrumb comments) is restated as a P5 obligation. First P4 work item is
CkResourceLoader soft-ref resolution — start there.
**Carried blocker (branch hygiene, not code):** CkTests `feature/voice-chat-wip` is 1 commit behind
`origin/dev` and needs `e5bb948b` merged/rebased in — upstream CkFoundation `dd3632bd7` dropped a
C++ default on `Request_ClearAllModifiers`, so without it the branch cannot AngelScript-compile
standalone. Applied as a working-tree edit for the 2026-08-04 run only; NOT committed.
The two Gate-2 HUMAN items (mic `[EDITOR-VERIFY]`, N5 packaged smoke) remain open as
P2-verification obligations gating P5 ship — both need `[Voice] bEnabled=true` in BB's
DefaultEngine.ini (a BB-repo decision; superproject untouched).
**Blocked on:** nothing machine-side. Nothing pushed by this session; superproject gitlink
untouched. NOTE: both feature tips were found already present on `origin` at session start —
not pushed by the campaign sessions.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-02 | ADR-4 blessed (paced budgeted relay-actor streams) with clause (a)–(g); UChannel = named fallback only | CTO adjudication | P0 spike shows pathological drop/starvation, or P3 profile shows relay overhead |
| 2026-08-02 | Dep budget: minimal earned deps at P0 (`CkCore/Ecs/EcsExt/Label/Log/Record/Settings` + engine `Core/CoreUObject/Engine/GameplayTags/DeveloperSettings`); no Relationship ever; Timer only if earned; ActorRelay/SpatialQuery/ResourceLoader/Profile added at the phase whose code consumes them | Review N4 ("every dep must be earned") | Each phase's Build.cs review |
| 2026-08-03 | P0 skeleton carries NO requests/signals/Codec/Net/Playback files — deferred to the phase that implements them; VoiceListener has no Processor files until P3 | Skeleton = compiling topology, not speculative surface; empty files are dead weight | P1 (Codec), P2 (Talker requests/signals/Playback), P3 (Net, Listener processors) |

## Dated entries (append-only, newest first)

### 2026-08-04 (later still) — P4 item 4 landed: HybridRadio near/far render decision
- **CkF `1d8d06033`:** per-recipient near/far decided at PLAYBACK per drain — listener position
  (`GetAudioListenerPosition`) vs synth location against `AudibleRange`, Route's hysteresis
  asymmetry mirrored (near inside range, held to range+margin). Near = spatialized, no radio
  filter; far = flat + the channel's effect chain. Routing untouched (one wire copy — the gate's
  no-fork STOP condition holds structurally). All fallbacks collapse to radio. `_HybridRenderNear`
  (TOptional) resets when the config channel is not HybridRadio.
- Ran: **30 total — 19/19 local, 11 env-fails name-identical, 0 AS errors**
  (Test-VoiceChatP4-item4b.log). Same deferred-verification ledger as items 2/3: near/far state
  spec via the debug seam + BB, audible difference via `[EDITOR-VERIFY]`.
- **Runbook refinement (model corrected):** the CkPlugins-host layout fatal recurs after some
  number of NORMAL headless runs, not only after crashes — items 1e+2 ran full suites
  post-ritual, then item 4's first run fataled at runner boot (0 tests). Treat the ritual (GUI
  boot → settle → graceful close) as the standing response to any `GetRestoredDimensions` fatal;
  ~2 min, automated. P4 items 1-4 each cost one ritual on average.

### 2026-08-04 (later) — P4 items 2+3 landed: spatialized playback + per-channel audio config
- **CkF `a2cbd385c`:** the receive drain selects `_PlaybackConfigChannel` per drain
  (highest-Priority delivering channel; **deviation from the gate doc's tie rule, recorded: ties
  keep the EARLIEST selection** — strict `>` comparison — stabler than most-recent, no config
  flapping between equal-priority channels). `Apply_SynthChannelConfig` re-applies on change via
  Stop→set→Start (verified against engine source: `USynthComponent::Start()` copies the fields,
  SynthComponent.cpp:465-479; the module's PCM queue survives the cycle by design).
  `TryCreate_PlaybackSynth` attaches at the owning actor's `PlaybackAttachSocketName` (P0 param,
  first consumer). Spatialize requires Positional3D AND an attach parent. Invalid config channel
  = flat (loopback/pre-control-plane behavior preserved). HybridRadio renders flat+chain until
  item 4. Channel Setup resolves the settings default attenuation through the same batch when a
  spatializing channel authors none. **Item 3 (effect chain) landed inside item 2's apply path**
  — never separable.
- Ran (CkPlugins host, iteration lane): **30 total — 19/19 local pass, 11 net fails
  name-identical to the env-fail set, 0 AS errors, compile clean first try**
  (Test-VoiceChatP4-item2.log).
- **Open before item 2/3 can be called machine-verified end-to-end:** (a) selection net spec —
  needs a `Debug_Get_PlaybackConfigChannel` seam (follow the `Debug_InjectInboundBundle`
  precedent) + a BusterBlock run; (b) the application half is audio-device-only →
  `[EDITOR-VERIFY]` audition steps to write into Gate_4.md with items 2-4 together; (c) item 1's
  positive-path resolution spec (real .uasset) still deferred.
- **Next: item 4 (HybridRadio near/far).** Sketch: per drain on the receiving client, when
  policy == HybridRadio compute listener↔talker distance (talker pos = synth component location;
  listener pos = the engine audio listener), compare vs channel AudibleRange with the module
  hysteresis margin (near→far at range+margin, far→near at range — mirror Route's asymmetry),
  flip via Apply on change. Bundle-arrival-coupled cadence (silent talkers never flip).

### 2026-08-04 — P4 opened (Gate_4.md); item 1 landed (CkResourceLoader resolution); host runbook
- **Gate_4.md committed** (b95e4eae1 + survey correction f75828ed8): scope grounded in code, not
  the spec — HybridRadio currently routes like Global2D; `Get_Attenuation`/`Get_SourceEffectChain`
  had zero consumers; **playback today is entirely non-spatialized** (bare synth, no attach, no
  attenuation), so item 2 lands spatialized playback itself. Decision recorded: one synth per
  talker per machine adopts the highest-`_Priority` delivering channel's audio config.
- **Entry blocker cleared:** CkTests `35fcde38` + merge of `e5bb948b` → `feature/voice-chat-wip`
  is 0 behind / 28 ahead of origin/dev and AS-compiles standalone.
- **Item 1 (CkF `c4e923a65`):** channel Setup resolves the ADR-5 audio soft refs through
  `RequestLoad_RootedBatch` (AudioTrack pattern), resolved objects on Current behind the batch GC
  root, `Get_ResolvedAttenuation`/`Get_ResolvedSourceEffectChain` utils as the item-2/3 seam.
  CkResourceLoader dep earned. Deliberately no EndPlay reset (AuthorityOnly EndPlay vs
  every-machine resolution; RAII at fragment destruction covers all machines). Fix cycle:
  TProcessor demands `TReadWrite<>` on the mutated fragment.
- Ran (CkPlugins host, iteration lane): **30 total — 19/19 local pass, 11 net fails
  name-identical to the host's recorded no-NetDriver env-fail set, 0 AS errors, no fatal**
  (Test-VoiceChatP4-item1e.log). Positive-path resolution spec deferred to item 2 (observable at
  the synth there).
- **CkPlugins-host runbook (proven twice tonight):** a headless editor crash poisons
  `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` (headless-written state re-fatals
  every later windowless boot at `FTabManager::SavePersistentLayout` →
  `GetRestoredDimensions`); recovery = one GUI editor boot + graceful close (regenerates the
  known-good ~105 KB state), then headless runs work. Iteration on this host = local specs only;
  gate runs stay on BusterBlock.

### 2026-08-04 — Gate-3 audit conditions re-gated GREEN post-rebase; Gate 3 machine portion CLOSED
- **VoiceChat 30/30 + RenderTarget 22/22, both EXIT 0, 0 AS errors, same binary, no rebuild
  between runs** (Test-VoiceChatP3-postrebase2.log / Test-P3-RenderTarget-postrebase2.log).
  Freshness chain verified monotonic: sources 02:14:27 → CkVoiceChat compiled+linked in-run
  (actions 43-72/117) → DLL 02:21:14 → run 02:27:13. **C3's code compiled for the first time
  here** — it was committed at authoring time but never built.
- Conditions C1 (mute-spec arrival-counter freeze), C2 (N1 never-stash arrival equality +
  FutureIdx-unresolvable assert), C3 (throttled per-talker N2 drop Warning on a ServerInbox
  cooldown) all confirmed green. Resolution table in [Gate_3.md](Gate_3.md).
- **Upstream break found, not ours:** CkFoundation `dd3632bd7` (delegate-on-every-request sweep)
  dropped the C++ default on `Request_ClearAllModifiers`' `InAttributeComponent`, breaking every
  AngelScript caller that omitted it — `CkAttributeGym_Integer_Modifiers_Steps.as:115` failed the
  whole AS compile. `origin/dev` already had the fix (CkTests `e5bb948b`); our branch was exactly
  1 commit behind and had never touched the file. Applied as a working-tree edit for the run,
  NOT committed — the branch still needs `e5bb948b` merged in. Recommend a repo-wide sweep for
  other AS callers stranded by that same default-drop.
- **Host lesson (recorded so it isn't re-litigated):** the CkPlugins host CANNOT run this
  campaign's net specs. `DisableEnginePluginsByDefault: true` + a slimmed plugin closure excludes
  OnlineSubsystem/OnlineSubsystemUtils and it defines no `NetDriverDefinitions`, so every PIE net
  world dies with `NetDriverCreateFailure ... Driver = NONE` — 11/11 net specs fail on
  environment, 19/19 non-net pass. Also needed a one-off GUI editor boot before headless runs
  work at all (virgin projects fatal in `FGenericWindow::GetRestoredDimensions` while persisting
  a fresh Slate layout). **BusterBlock remains the only net-capable host.**
- BusterBlock restored to `dev` on both plugin submodules afterward; superproject left to its
  owning session. One residue: the run auto-registered `ActorRelay.VoiceChat` +
  `ActorRelay.VoiceChatControl` into BB `Config/DefaultGameplayTags.ini` (ResolveGameplayTag
  self-registration) — left in place, revert is `git checkout -- Config/DefaultGameplayTags.ini`.

### 2026-08-03 — S5 drain-budget measured; default budget 4096 → 640 (numbers in Gate_3.md)
- **Instrumentation:** arrival totals now count at the client RPC boundary BEFORE the inbox cap
  (`FFragment_VoiceTalker_ReceiveInbox` totals + `Debug_Get_ReceiveArrivedBundles/Bytes`), read
  by the new `Ck.VoiceChat.Net.DrainBudgetMeasure` spec: one 2000 × 240 B burst through
  `Client_ReceiveVoiceBundle` (0xFF payloads — guaranteed unpack-reject, decoder untouched),
  then arrival slopes at tick marks. Sanity asserts only (delivered, monotone, ≤ sent) — the
  numbers are machine/config-dependent and the spec must not flake on a slower box; it stays in
  the suite as the re-measurement tool.
- **Measured: saturated drain = 708.0 B/tick (2.95 × 240 B bundles/tick), IDENTICAL across both
  60-tick windows; queue still draining at mark 3 (441/2000)** — a stable ceiling, not noise.
  The spike's ~850 B/tick estimate was high; 708 sits BELOW its "~716 B/tick needed for 8
  talkers."
- **Default `MaxVoiceBytesPerConnectionPerTick` 4096 → 640:** covers the 8-talker steady worst
  case (≈533 B/tick) with ~20% margin while staying ~10% under the measured drain, so sustained
  overage drops at the counted budget layer (S2), never queues silently. Full numbers +
  config caveat (editor net-PIE; re-measure packaged before ship-tuning) recorded in
  [Gate_3.md](Gate_3.md) §S5.
- One logging round-trip: AddInfo events don't reach the unattended runner's log — the spec
  prints via LogTemp Display (the [VoiceSpike] canary precedent) and AddInfo both.

### 2026-08-03 — P3 item 5: Positional3D proximity routing set (28/28)
- **CkF 383630b38 + CkTests b7b2a61:** ADR-6 lands. Every Positional3D member gets a probe pair
  reconciled by the new authority-only `FProcessor_VoiceChat_ProximityProbes` (dirty-tag driven
  off Join/Leave/channel-EndPlay — no per-tick scans): a presence dot (`Probe.VoiceChat.Presence`,
  empty filter, reports nothing) and a range probe (`Probe.VoiceChat.Range`, filter=Presence,
  Kinematic, PersistContacts, radius = max member-channel range + `Get_ProximityHysteresisMarginCm`,
  resized via ShapeSphere UpdateDimensions on membership change). Route intersects CanHear members
  with the probe's candidate set and applies per-(recipient, channel-idx) hysteresis: audible from
  the channel's range, held to range + margin. **Fails CLOSED** with no Transform/probes (new
  stats: `RouteDroppedNoProximity`, `RouteDroppedOutOfRange`) — never falls back to
  membership-wide sends.
- **Deviation 5 (recorded): ADR-6's letter says `BindTo_OnBeginOverlap/OnEndOverlap` maintain the
  candidate set.** As built, Route READS the probe's event-maintained `_CurrentOverlaps` set
  (only when bundles flow). Why: the Bind surface is a dynamic delegate needing a UObject
  listener — NO C++ adopter binds it; the house pattern for a processor consuming overlaps is
  the direct read (`CkCrowdAgent_Neighbors_Processor.cpp:60`). The mechanism ADR-6 mandates —
  incremental event-maintained set, no polling queries, no world scans in the packet path — is
  preserved; the rejected alternative (per-frame re-query) is not what a fragment read is.
- `Ck.VoiceChat.Net.ProximityRouting` (3 worlds) proves the matrix: in-range (500cm/range 4000)
  receives while far (9000cm) NEVER does; moved into the margin band (4050) the stream HOLDS;
  beyond it (4300) it stops arriving; back into the band from outside it does NOT resume — the
  hysteresis asymmetry. Phase windows via decoded-PCM buffer resets (the buffer is ring-capped;
  count deltas would saturate). A diagnostic assert separates transform-sync failure from
  routing failure. First-run green after two compile fixes (missing NativeGameplayTags.h;
  typesafe-handle CastChecked at Transform/member call sites).
- Post-run cleanup: one unused accessor removed (`CK_PROPERTY_GET(_PresenceProbeChild)`) —
  behavior-neutral, re-covered by the S5 run and the exit sweep on the final binary.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **28/28, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-proximity3.log).

### 2026-08-03 — P3 items 8 + host-asymmetry + N7 complete (27/27); FCk_Time sweep
- **CkTests 99010c5 + CkF a0d72258c:** the asymmetry matrix is fully tested —
  `Ck.VoiceChat.Net.HostHearsClient` (the true client-origin path: client-side encoder, relay
  resolve, spoof guard passing; the host decodes), `Ck.VoiceChat.Net.ClientToClient` (3 worlds:
  client 0 → client 1 through the server; the speaker's own world decodes NOTHING — no echo),
  and `Ck.VoiceChat.Net.TeardownMidTransmit` (S4/§7.8: whole speaker subject destroyed
  mid-stream while voice provably flowed; replicas vanish cleanly, the listener side survives).
  **N7 routing-policy matrix** landed in the module Claude.md (incl. the known v1 NPC-sender
  gap, stated not hidden).
- Fix cycle (both test-side, routing untouched): client-local PlayerState absent at PIE-ready
  in a 3-world boot (settle ticks); a bystander world sees NULL owner on another client's
  subject — PlayerControllers replicate only to their owning client — so third-party worlds
  identify by not-locally-owned.
- Earlier same day: **FCk_Time sweep** (maintainer feedback; CkF 2905dcf79 + CkTests 56a9dc7) —
  all time-typed fields/params across the module (incl. the P2 capture-seam API) converted;
  24/24 + byte-identical pipeline decode proved it behavior-preserving.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **27/27, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-asymmetry3.log).

### 2026-08-03 — P3 item 4b (fairness half): audible-speaker cap + envelope + LRS (24/24)
- **CkF a1a844a1f + CkTests ed3b340:** routing split into stage/flush — Route (per-talker)
  stages authorized forwards and slews the fairness envelope (rise 2.0/s, fall 4.0/s: a spoofed
  instant-max claim takes ~0.5 s to reach the top bucket and cannot jump the queue);
  `FlushForwards` (AuthorityOnly, RunAfter Route) applies the MaxAudibleSpeakers cap per
  recipient with the PURE `Select_TopNTalkers` policy (8 loudness buckets, ties + the
  sustained-max spoof rotate least-recently-served), then the per-connection byte budget, then
  the sends + serve-history update. This lands the CTO amplitude-fairness ruling in full.
- Unit tests pin the policy where a 2-talker net spec cannot: cap, cross-bucket loudness wins,
  LRS rotation flips after serving, degenerate inputs.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **24/24, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-fairness.log); existing net specs unchanged (≤2 talkers → cap
  no-op; budget same value, applied at flush).
- Still open in 4b: the **S5 drain-budget measurement** (instrumented run + honest numbers in
  Gate_3.md, default budget justified against them).

### 2026-08-03 — P3 item 7 complete: listener mute is a routing exclusion (23/23)
- **CkF 3f84feadf + CkTests b397076:** mute sync is stateful must-apply, so it rides a SECOND
  reliable control relay (`ActorRelay.VoiceChatControl`) — S1 bans reliable RPCs on the voice
  STREAM actor; a separate Iris object cannot HoL-block it. Payload = full-set replace
  (idempotent, ordering-proof). VoiceListener grew its three requests (completion contract +
  cancel processor), `SyncMutes` (host applies directly into the routing matrix; client retries
  the control relay until resolved), `ApplyControl` (AuthorityOnly) draining reports into the
  per-player mute matrix the Route processor consults — **muted audio is never SENT**. Local
  defense-in-depth gate in ReceivePlayback covers the in-flight race (drops bundles, still
  drains the jitter tail so unmute never replays stale audio — caught in self-review).
- **Net test** (`Ck.VoiceChat.Net.ListenerMute_StopsForwarding`): three transmit cycles —
  baseline decodes; muted spurt leaves decoded bytes FROZEN (never arrived); unmute restores
  growth (the exclusion, not a broken pipe). First-run green.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **23/23, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-mute.log).

### 2026-08-03 — P3 item 6 complete: remote voice decodes cross-world (22/22)
- **CkF fcd8576f9 + CkTests 938ccf4:** `FProcessor_VoiceTalker_ReceivePlayback` (all net modes —
  a listen host hears clients too; time-stepping, no MarkedDirtyBy) drains forwarded bundles
  into the talker's jitter buffer (frame i = header seq + i — valid because pacing selection is
  provably contiguous: staleness drops a prefix, stop-at-first-misfit cuts a suffix), mirrors
  header amplitude for remote `Get_CurrentAmplitude` parity, creates decoder/synth on demand.
- **Consolidation, not duplication:** the P2 `_Loopback*` machinery is now the shared playout
  chain — `Drain_Playout` + `TryCreate_PlaybackSynth` are single implementations used by both
  Capture-loopback and remote receive (mutually exclusive per entity per machine); Capture's
  block and StartTransmit's synth block collapsed into calls. Pipeline decode byte-identical
  post-refactor (1.20 s / 115200 B) — behavior-preserving, proven.
- **Routing spec upgraded** to the stronger property: the client world DECODED talker A's voice
  into PCM (none for B); N1 never-stash equality now compares decoded bytes. Longer settle so
  the jitter tail drains before the snapshot.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **22/22, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-receive.log). **The full mouth-to-ear machine path now runs
  cross-world: capture → VAD → encode → pace → relay → route → forward → jitter → decode.**
  Only the audible speaker output remains human-verifiable.

### 2026-08-03 — P3 item 4 core complete: Route processor + N1 enforcement (22/22)
- **CkF 86d857995 + CkTests 5c8930a:** `FProcessor_VoiceChat_Route` (AuthorityOnly, after
  Capture) — strict unpack (malformed = ensure-once + drop), **N1: unresolvable ChannelIdx =
  drop + count, never stash** (expected traffic, no ensure), clause-(c) spoof guard (stamped
  sender vs talker-owning player, skipped for player-less talkers), membership + CanTalk,
  server-mute privacy drop; forwards packed bytes untouched to CanHear members on OTHER
  connections under a per-connection per-tick byte budget (frame-reset world fragment). 7 route
  counters. Debug seams: `Debug_InjectInboundBundle` / `Debug_Get_ReceiveInboxNum`.
- **N1 net test landed** (`Ck.VoiceChat.Net.Routing_ForwardsAndNeverStashes`): the real e2e
  path — host-injected scripted sine from an unowned subject forwards to a client-PC-owned
  subject's connection (client ReceiveInbox fills); then a bundle naming the NEXT unallocated
  idx is dropped, that idx is allocated with would-be recipients, and the client count must not
  move. **The campaign's standing mandatory condition (N1) is now enforcement + test.**
- Fix cycle: run 1 red — the spec lacked the `[Voice] bEnabled` self-enable (encoder factory
  null → transmit never started; `CkEnsure: EncoderCreated` named it). Third test file bitten
  by the once-at-startup config gate — reinforces the BB DefaultEngine.ini line as a permanent
  production prerequisite.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **22/22, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-routing2.log).
- Deferred within item 4 (recorded): top-N fairness (envelope clamp + LRS tiebreak) and the S5
  production drain-budget measurement → item 4b before gate close; Positional3D range filter →
  item 5 (probe). NPC-talker sender binding is unguarded by design in v1 (player-less talkers
  skip the spoof check) — document in module Claude.md at N7 time.

### 2026-08-03 — P3 item 3 complete: talker outbound + audit F3/F6 (21/21)
- **CkF 5c8fc8cd6:** encoded frames → timestamped outbound queue → per-tick
  `Select_BundlesToSend` (stale-drop FIRST at 150 ms per S2, then oldest-first under
  `Get_MaxVoiceBytesPerConnectionPerTick`) → bundles ≤ FramesPerRpc (≤3) with an incremental
  <240 B packed-size guard (S3 headroom) → packed once per eligible CanTalk-member channel →
  host injects into own routing inbox (no self-RPC, host-player-stamped) / client resolves its
  per-player relay (resolve-or-retry, RenderTarget shape) → `Server_SendVoiceBundle`.
- Defensive inbox caps (256 bundles, drop+count) at all three enqueue sites — item 3 starts
  filling inboxes that only item 4's Route processor drains.
- Gap found + fixed while wiring channel eligibility: the client applier never mirrored the
  registry — `Apply_ReplicatedControlPlane` now upserts the transient-entity registry, giving
  clients idx→channel resolution and channel enumeration parity with the authority.
- Audit conditions landed: **F3** (loopback synth created only after capture `Start()`
  succeeds) + **F6** (`_AmplitudeQ8` holds on frameless ticks — the wire header and item 4's
  fairness clamp read it). StopTransmit clears the outbound queue.
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **21/21, EXIT 0, 0 AS
  errors**; pipeline decode byte-identical (1.20 s / 115200 B) — outbound insertion did not
  perturb the loopback path (Test-VoiceChatP3-outbound.log). Behavior note: with no channels
  composed (the pipeline test), frames age out quietly at 150 ms — bounded by design.
- Next: item 4 — `FProcessor_VoiceChat_Route` (AuthorityOnly): drain ServerInbox → N2-shaped
  validation → N1 drop+count on unresolvable ChannelIdx → send-set by policy → sender exclusion
  → top-N fairness → per-connection budget → `Client_ReceiveVoiceBundle`; with the N1
  voice-before-registry + routing net tests.

### 2026-08-03 — P3 item 2 complete: control plane replicates (21/21)
- **Replicated half (CkF 36ef940f7, CkTests d8d404f):** `FCk_RepData_VoiceChat` (registry
  entries tag→idx, memberships+flags, server-mute matrix) as ONE container fragment on the
  channel HOST; `Register_NetOnly` registrar delegating to
  `UCk_Utils_VoiceChannel_UE::Apply_ReplicatedControlPlane` (NotReady-before-any-mutation until
  every named channel composes; applies idx + clears client NeedsIdx; fires OnMemberJoined/Left
  from the state diff). Server pushes at mutation sites via host-gated
  TryAdd/TryUpdateContainerFragment (RenderTarget pattern) — local-only usage no-ops cleanly.
- Fix cycle: LNK2019 `UEPushModelPrivate::MarkPropertyDirty` from the container push template →
  `NetCore` dep earned (same path as CkRenderTarget carries it).
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` → **21/21, EXIT 0, 0 AS
  errors** (Test-VoiceChatP3-repdata2.log). The net spec ran in its own net lane and CONFIRMED
  the riskiest claim: a member handle serialized server→client resolves to the client's own
  bridged entity (cross-wire handle identity), making `Get_IsMember(clientChannel, clientTalker)`
  true. Leave replicates; server mute survives leave on the client too.
- Sequencing correction recorded: the N1 voice-before-registry net test belongs to item 4 (the
  Route processor owns the drop site); item 2's net contract (registry/membership/late-join via
  full-state container) is covered by `Ck.VoiceChat.Net.ControlPlane_Replicates`.

### 2026-08-03 — Gate 3 opened; P3 work items 1 + 2(local half) landed green
- Gate_3.md committed (94a7abcfb): contract carries N1–N3/N7 + S1–S5 + audit F3/F6; ten
  sequenced work items; the two Gate-2 HUMAN items reclassified P2-verification-gating-P5
  (decision + revert hook in its status header).
- **Item 1 — transport skeleton (bb0513333):** `ACk_VoiceChatRelay_UE` with
  `Server_SendVoiceBundle`/`Client_ReceiveVoiceBundle`, both Unreliable BY CONTRACT (S1),
  enqueue-only bodies into talker-entity inboxes, sender stamped server-side from the channel
  owner chain, drop+count on unresolvable (NO stash path — deliberate divergence from
  RenderTarget, per N1); `UCk_VoiceChatRelay_Subsystem_UE` on group `ActorRelay.VoiceChat`
  (ResolveGameplayTag self-registers missing tags — verified in CkGameplayTag_Utils.cpp:498);
  CkActorRelay dep earned. Gate: 18/18 (Test-VoiceChatP3-transportskeleton.log).
- **Item 2, local half — control plane (c01b43ef0):** `FProcessor_VoiceChannel_AssignIdx`
  (AuthorityOnly via `NetModeRequirement`, exemplar CkEntityScript_Processor.h:110) allocating
  session-append-only u8 indices from a transient-entity world registry, N3 ensure on
  exhaustion (255 = unassigned sentinel, 0–254 usable); membership requests
  Join/Leave/SetMemberFlags/ServerMute/ServerUnmute with completion contract + EndPlay cancel;
  non-authority callers rejected synchronously Failed_NotEnqueued; OnMemberJoined/Left signals;
  TryGet_ChannelByIdx quiet-on-miss (stale idx = expected N1 traffic); server mute survives
  leave/rejoin (moderation-safe default, pinned by test). Gate: 18/18
  (Test-VoiceChatP3-controlplane.log).
- **Item 2 tests (CkTests eef61de):** `Ck.VoiceChat.Channel.MembershipAndIdx` +
  `.InvalidInputs_Rejected` — **20/20, EXIT 0, 0 AS errors**
  (Test-VoiceChatP3-membership.log). Non-authority-rejection test deferred to the RepData
  slice (needs a genuinely non-authoritative replicated channel on a client world).
- Next: item 2 replicated half — `FCk_RepData_VoiceChat` (Register_NetOnly, NotReady until
  composed), late-join + voice-before-registry (N1) net tests, then item 3 (talker outbound +
  F3/F6).

### 2026-08-03 — Gate 2 audited GO WITH CONDITIONS; both conditions resolved + re-gated
- Audit (fresh top-tier session) appended to Gate_2.md: no blocker in the capture seam, talker
  request/signal surface, processor pipeline, synth component, or tests; evidence chain and
  binary freshness verified independently by the auditor.
- Condition 1 landed as code: four frame counters (Captured/Encoded/Concealed/Decoded) on
  `STATGROUP_CkVoiceChat`, wired in the Capture processor — commit 136ca780f.
- Condition 2 + finding 4 recorded as deviations 3/4 in Gate_2.md (Conceal zero-fill — the
  engine's `IVoiceDecoder` factory surface cannot express packet-level Opus PLC, verified by the
  auditor against `VoiceCodecOpus.cpp`; synth creation at the request boundary).
- Ran: `--build --test --test-pattern VoiceChat --discover-fresh` on the counters commit →
  **18/18, all 3 lanes EXIT 0, 0 fails, 0 `Angelscript: Error`**
  (Test-VoiceChatP2-statscounters.log); pipeline decode line unchanged (1.20 s). Freshness:
  source 03:36:52 → `BusterBlockEditor-CkVoiceChat.dll` 03:37:46 → run 03:38–03:40.
- Non-blocking findings routed: F3 (synth-before-capture-start residue) and F6 (amplitude
  flicker on frameless ticks) → P3 open items; F5 (capture hot-path allocations) → P5 perf
  ledger; F7 needs no action (the HUMAN items are the audible-path proof).

### 2026-08-03 — Gate 1 (P1) work complete: codec layer green, benchmark recorded
- Ran: `--build --test --test-pattern UnitTests.CkVoiceChat --discover-fresh` × 4 runs (fix cycle
  below). Final run 4: **10/10 passed, EXIT CODE: 0** (Test-VoiceChatP1-run4.log), 0
  `Angelscript: Error`.
- Benchmark (run 4): encode **61.3 µs/frame**, decode **26.9 µs/frame**, **47.7 B/frame** avg at
  24 kbps — per-talker game-thread encode ≈ 0.31% core; full details + P2 contract discoveries in
  Gate_1.md § Benchmark record.
- Audit condition C1 landed green: `CkTests.UnitTests.CkVoiceChat.Channel.AddInvalidNameRejected`
  (zero-partial-state rejection on a bare slot-table registry).
- Fix cycle (all committed, honest history): run 1 — AdaptiveDepth test asserted the wrong
  contract (near-zero jitter EWMA correctly converges to the MinDepth floor, not
  InitialTargetDepth; float accumulation makes the estimate epsilon-positive), and the Opus
  factories returned null (`[Voice] bEnabled` read once at module startup; host project has no
  `[Voice]` section — benchmark now self-enables via the config cache + module reload fallback +
  key cleanup). Run 3 — all decodes were size 0: `FVoiceDecoderOpus` silently skips frames unless
  the OUTPUT buffer holds ≥ MAX_OPUS_FRAMES (6) frames (`VoiceCodecOpus.cpp:20`); root-caused by
  reading the engine source after two failed hypotheses (stuck-protocol stop), buffer sized to
  the floor.
- P2 obligations discovered here: host project needs `[Voice] bEnabled=true` for capture; synth
  decode target must honor the 6-frame capacity floor.
- Commits: CkFoundation 59a7de954 (codec layer) + comment/docs commits; CkTests 4d01366 → 0019a63
  (tests + benchmark + fixes; Build.cs earns Voice + CkVoiceChat).
- Canary + regression sweep on the run-4 binary: spike canaries **2/2**, RenderTarget **22/22**,
  both EXIT 0 (Test-P1-Canary.log / Test-P1-Regression.log) — delta-zero vs the Gate-0 baseline.

### 2026-08-03 — Gate 0 closed: build + AS + spike green; verdict PROCEED
- Ran: `UnrealToolbox --build --config=Development --target=Editor` → `Result: Succeeded`
  (Build-Editor-VoiceChatP0.log). Machine gotcha recorded: `--generate` fails here (no VS2022
  IDE — project-file generation needs it, UBT does not); a first failed toolbox invocation also
  sat on the machine-wide build lock without exiting and had to be killed (own process, verified
  by start time + already-failed log).
- Ran: spike suite three times (fix cycle recorded in the memo's Run record). Final:
  `--test --test-pattern VoiceChat.Spike --discover-fresh` → **2/2 passed, EXIT CODE: 0**
  (Test-VoiceChatSpike3.log). Numbers: Phase A 300/300, lag −2 const (zero added latency),
  ~214 B/RPC; Phase B 2400/2400 drained, lag max 298; Phase C (+60 KB/t churn) 2400/2400, lag
  37/484/641; unresolved-target burst 25/25 delivered, 0 net warnings.
- Ran: AS gate — 0 `Angelscript: Error` in the run-3 boot; 4 `utils_voice_*` wrappers emitted.
- Ran: adjacent regression — `--test --test-pattern RenderTarget` → **22/22 passed, EXIT 0**,
  2m48s (Test-RenderTargetRegression.log), on the final binary.
- Confirmed: the three load-bearing Iris claims re-read by hand (`NetRPCHandler.cpp:24-33`
  Ordered flag on all unicast RPCs; `AttachmentReplication.cpp:215-226` Ordered→reliable-queue
  routing; `PartialNetObjectAttachmentHandler.h:30` 256 B server unreliable split threshold).
- Spike harness defects found+fixed en route (in the committed history, not hidden): 65,536-byte
  pressure payload tripped Iris's 65,535-element array cap; send-window fencepost (299/300);
  AddInfo lines don't reach captured logs (→ UE_LOG); fixed-settle windows measure the window,
  not the transport (→ drain-to-completion + lag as the pressure metric).
- Inferred (unconfirmed, named for the auditor): BP-editor surface renders correctly
  (`[EDITOR-VERIFY]` steps in the review package); production-config drain budget (S5 makes it a
  P3 measurement); lag-tick→wall-clock conversion assumes ~60 fps PIE ticking.

## Decision log (Gate-0 close additions)
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-03 | Spike verdict PROCEED; latency (not loss) is the pressure response; S1–S5 bound it | Memo § Verdict — mechanism + measurement agree | P3 profile (S5 measurement) contradicts |
| 2026-08-03 | Spike tests stay committed + green under `Ck.VoiceChat.Spike.*`, marked THROWAWAY | Red-by-design tests would pollute the suite; green canaries are harmless | Campaign end (delete) or auditor keeps them |

### 2026-08-03 — skeleton + doctrine + spike code landed; build gate running
- Ran: scaffold committed as CkFoundation 9b0a55d22 (28 files: 3 quartets, Settings, Log/Stats/
  Module, uplugin entry, tier row, module Claude.md stub); doctrine amendment 1ff7decf9
  (CkActorRelay Anti-patterns split: event channels events-only verbatim; relay actors may carry
  paced budgeted streams under clause (a)-(g)); campaign docs ffa02f6c0.
- Ran: CkTests branch `feature/voice-chat` cut from origin/dev (417b062); spike committed 9d9b9d2
  (channel actor mirroring relay wire shape bReplicates+bAlwaysRelevant+Owner=PlayerState per
  CkActorRelay_Actor.cpp:23-24 / _GroupSubsystem.cpp:452; pressure actor churning a 64 KB
  replicated payload; specs Ck.VoiceChat.Spike.{UnreliableUnicast_DeliveryUnderLoad,
  UnresolvedTarget_SilentDrop}).
- Confirmed: Iris is ON for the host project (`net.Iris.UseIrisReplication=1`,
  BusterBlock/Config/DefaultEngine.ini:6).
- Confirmed: pre-commit sweep clean — no stock ensure/check in Source/CkVoiceChat (rg exit 1);
  every mimicked API verified at source (Request_CreateEntity_AsTypeSafe, Get_ValidEntry_ByTag,
  record policies, ck::IsValid(FGameplayTag) at CkIsValid_Defaults.h:94, FCk_Time ctor
  CkTime.h:85, GameplayLabel Add CkLabel_Utils.h:24-26).
- Decision (spike vehicle): plain replicated actor copying the relay channel's wire shape instead
  of subclassing abstract ACk_ActorRelay_UE — the base adds only group-subsystem registration
  bookkeeping (no netcode; verified by reading CkActorRelay_Actor.h in full), and a subsystem-less
  subclass would spin its registration retry loop. Transport composite is identical.
- Inferred (pending): Iris unreliable-attachment mechanics — Explore agent reading the fork's
  Iris source (queue/drop policy, unresolved-target path, per-RPC overhead, SaturateBandwidth
  cvar); build gate running in background (Build-Editor-VoiceChatP0.log).

### 2026-08-03 — Gate 0 opened: branch, docs, research
- Ran: `git checkout -b feature/voice-chat origin/dev` (base d02278cdd) → cherry-pick d6f6edd6c
  → 7a66ff11e (CTO review rides the branch); `git add -f docs/specs/2026-08-02-CkVoiceChat-technical-review.md`
  → 21bf89232. Superproject gitlink untouched.
- Confirmed: editor-lock probe found `BusterBlock_2.log` + `PickupGate-Snapshot.log` exclusively
  locked by a sibling session's `BusterBlockEditor-Cmd` (PID 22316) — branch ops touched docs
  only (safe); C++ build deferred until the sibling run ends.
- Research (exemplars read on HEAD, per non-negotiable #1). The neighboring features I will
  mimic are **CkTimer** (quartet), **CkRenderTarget** (module topology/Settings/relay), **CkCue**
  (unreliable relay RPCs), **CkAudio** (Stats.h; Director→Track child-entity Create):
  - `CkTimer/{CkTimer.Build.cs, CkTimer_Module.h/.cpp, CkTimer_Log.h/.cpp}`
  - `CkTimer/Public/CkTimer/{CkTimer_Fragment_Data.h, CkTimer_Fragment.h, CkTimer_Processor.h}`;
    `CkTimer_Utils.h:1-150`, `CkTimer_Utils.cpp:1-140` (Add at :38-78; HAS_CAST at :128)
  - `CkRenderTarget/{CkRenderTarget.Build.cs}`, `Settings/CkRenderTarget_Settings.h/.cpp`
    (constexpr-fallback getter shape), `Net/CkRenderTargetRelay_Actor.h` (enqueue-only +
    server-stamp contract comments :16-17, :58-59), `Net/CkRenderTargetRelay_Subsystem.h`
    (2-override), subsystem .cpp:14 (`ResolveGameplayTag("ActorRelay.RenderTarget")`)
  - `CkCue/Public/CkCue/CkCueRelay_Actor.h` (Unreliable Server :24 / Multicast :62 RPCs)
  - `CkActorRelay/Public/CkActorRelay/CkActorRelay_GroupSubsystem.h` + `CkActorRelay/CLAUDE.md`
    (anti-pattern text to amend, plus bNetStartup/Iris scar at "Channel pools")
  - `CkAudio/CkAudio_Stats.h`; `CkAudio/Public/CkAudio/AudioTrack/CkAudioTrack_Utils.cpp:17-46`
    (child-entity Create + record connect + HAS_CAST)
  - `CkFoundation.uplugin` entry shape (CkRenderTarget row, :898-908)
  - Native-tag exemplar: `CkRenderTarget_Fragment_Data.cpp:5` / `.h:20`
- Inferred (unconfirmed): unreliable unicast Client RPC behavior under the fork's net stack —
  exactly what the P0 spike exists to confirm.

## Open items  <!-- refreshed 2026-08-03 at the Gate-2 close; Gate-0-era rows retired (all ✅ audited) -->
| Item | Status | Next step |
|---|---|---|
| Gate 0 / 1 / 2(machine) audits | ✅ all GO (Gate 2: GO WITH CONDITIONS, resolved same session) | — |
| HUMAN: `[Voice] bEnabled=true` in BB `DefaultEngine.ini` | ⏳ Adam — BB-repo decision | prerequisite for both items below |
| HUMAN: mic loopback `[EDITOR-VERIFY]` (+ Gate-0 BP checklist folded in) | ⏳ human | exact steps in Gate_2.md |
| HUMAN: N5 packaged Opus smoke | ⏳ human | exact steps in Gate_2.md |
| P3 carry-forwards: N1 (mandatory pre-P3), N2, N3, N7, S1–S5 (PROMPT.md) + audit F3 (synth-after-capture-start reorder) + F6 (amplitude decay/hold) | 📋 queued | fold into the P3 gate doc at open |
| P5 perf-pass ledger: capture hot-path allocations (audit F5) | 📋 queued | profile first, then fix |
| Spike canaries (`CkAutoTest_VoiceSpike`, transport spec) | 📋 campaign end | delete unless P3 promotes them |
