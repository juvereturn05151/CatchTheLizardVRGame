# Broom herding tool

Open `/Game/Maps/Map_CatchTheLizard`. This is now the game and editor startup map.
`Broom_HerdingTool` is beside PlayerStart at **(75, 75, 120) cm**, standing tip-down with its handle at a reachable height.

Overlap the handle end with either controller and press grip. Release grip to detach it. Swing the opposite tip through the lizard to move it to a reachable surface. The broom cannot catch the lizard; use the existing hand overlap plus grip interaction for the final catch.

## Designer settings

These variables are instance-editable under **Broom Herding**:

| Blueprint | Variable | Default | Meaning |
|---|---|---:|---|
| BP_Broom | RakeVelocityThreshold | 150 cm/s | Minimum tip speed; equality qualifies |
| BP_Lizard | MaxRakeHeight | 140 cm | Maximum relocated center height above the play-space supporting floor |
| BP_Lizard | RakeCooldown | 0.5 s | Minimum interval between accepted rakes |
| BP_Lizard | DazedDuration | 0.4 s | Movement pause after relocation |
| BP_Lizard | BroomDetectionRadiusBonus | 20 cm | Added to the existing DetectionRadius for held tips |
| BP_Lizard | ForcedFleeDuration | 1.0 s | Minimum flee period after daze before ordinary threat checks may resume wandering |

The existing DetectionRadius, AlertDuration and FleeSpeed still apply. Cooldown and daze use game-time deadlines. A continuous contact is separately latched until separation, so merely waiting out cooldown does not repeat a rake.

## Implementation

- Gameplay is entirely Blueprint assets: `BP_Broom`, `BPI_Rakeable`, and additions to `BP_Lizard`. No C++ or runtime Python dependency.
- The 135 cm scaled cube has a separate 45 cm handle Box collision/root, template `BP_GrabComponent`, and 12 cm tip Sphere. The existing XR pawn grip/release and template grab query are reused without edits.
- Tip velocity is sampled after physics. Pickup, release and controller transfer reset sampling. Tip overlap and a swept sphere handle sustained contact and fast between-frame swings. Only collision components tagged `RakeTarget` on interface receivers qualify; the lizard's existing CatchSphere has that tag.
- `BPI_Rakeable.Raked` returns whether relocation succeeded. The broom never writes Caught. Both original hand-catch function graphs are unchanged.
- The base prototype has local Visibility traces rather than a surface graph. New candidate-search functions therefore reuse the same trace channel, surface-normal alignment and 9 cm stand-off. They search nearby floor and walls, filter final center height, reject ceiling undersides, check travel obstruction and destination clearance, and choose the nearest valid candidate. No candidate means no transform, state, feedback or cooldown change.
- Floor height comes from a WorldStatic trace beneath VROrigin, with VROrigin Z as fallback. This accounts for this map's existing PlayerStart being 100 cm above the room floor. Custom moving-floor environments may need a broader floor object query.
- Feedback reuses the existing color/scale pulse and `RestoreCatchFeedback` timer, matching the prototype's existing implementation rather than adding a Timeline.
- A movement gate preserves the original surface-following Tick chain. Daze expiry enters Flee with the Alert delay already elapsed. Threat sensing cannot cancel the brief forced-flee period.

## Verification performed

- All three affected Blueprints compiled with warnings treated as errors, and were saved with the level.
- 26 scripted Play-in-Editor checks passed, including template pickup/release with both controller components, handle/tip separation, first-sample suppression, 200 cm/s measured motion, threshold values 149/150 cm/s, interface dispatch, cooldown/contact latching, swept-tip contact, ceiling-to-floor relocation, failed-search preservation, held/dropped threat sensing and daze/flee state behavior.
- A separate real-time PIE check confirmed no movement during daze, feedback scale restoration, automatic daze expiry, actual fleeing movement, and no Caught transition.
- A separate obstruction check rejected a candidate requiring travel through a solid room wall.
- Original left/right hand-catch graph descriptions were compared before and after integration and matched.

These are desktop PIE tests with scripted controller transforms. **Quest 2 testing is still required** for actual grip input, tracking/late-update behavior, comfortable reach, swing feel, visual feedback with the headset, and Android performance. No Quest build or device deployment was performed.

## Quest 2 smoke test

1. Launch Map_CatchTheLizard. Pick up and release the handle with each hand; touching the tip should not pick it up.
2. Approach the lizard with a held tip: it should Alert/Flee before contact. A dropped broom should not threaten it.
3. Rest or move the tip slowly against the lizard: no rake. Swing through it: relocate below the height cap, pulse, pause, then flee.
4. Keep contact or swing repeatedly within cooldown: no rapid repeated relocation. Separate and swing again after cooldown to rake again.
5. Catch it with hand collision plus grip. Verify the original catch behavior still works, including during daze.
