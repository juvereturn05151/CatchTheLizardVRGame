# Background music

All playback configuration is in `/Game/CatchTheLizard/Music/Playback/DA_MapMusic`.

| Map | Assigned music | Volume |
| --- | --- | --- |
| Map_MainMenu | TutorialBGM_Loop — Tiptoeing Curiosity | 0.16 |
| Map_LevelSelection | TutorialBGM_Loop — Tiptoeing Curiosity | 0.16 |
| Map_TrainingFacility | TutorialBGM_Loop — Tiptoeing Curiosity | 0.13 |
| Map_CatchTheLizard (Level 1) | GameBGM_Loop — Tiptoeing Through the Woods | 0.17 |

The Music folder originally contained two imported Sound Waves, GameBGM and TutorialBGM. Their import paths and source MP3 title metadata identify the two tracks above; both are instrumental. No menu-specific track was provided, so the menu provisionally reuses TutorialBGM. The original assets remain unchanged.

Edit each Maps entry's Track, Volume, Map and Continuity Group in DA_MapMusic. Master Volume scales all entries; Fade Out Seconds defaults to 0.45 and Fade In Seconds to 0.8. Menu and selection share the Menu group and track, preserving the exact audio component/playback through travel. Tutorial has a distinct group so entering it fades and restarts at the lower tutorial volume even though it shares the source track. Use a different group when a map should start a new piece. New tracks should have Looping enabled or be looping Sound Cues.

`BP_CTLGameInstance` references the library in Class Defaults → Music. Project Settings → Maps & Modes → Game Instance Class uses this Blueprint. Its native UCTLGameInstance owns one persistent, non-spatial 2D Audio Component, independent of VR pawn lifetime. Map-load hooks handle direct entry into all four maps, PIE, and shutdown. Main-menu travel, Exit, and tutorial completion call PrepareMusicTravel/FadeMusicForExit and wait for the outgoing fade before loading/quitting. Same-group menu travel continues without a fade or restart. Other custom future travel code should also call PrepareMusicTravel and wait the returned duration before OpenLevel; map-load hooks provide fallback cleanup for unannounced travel.

The playback copies are direct looping Sound Waves, using the audio renderer's loop rather than a game-thread restart timer. `Scripts/Audio/prepare_music_loops.py` creates their reimportable WAVs in SourceAudio/Music from the original MP3s. It removes trailing near-silence and applies a 250 ms circular equal-power seam crossfade to avoid the gameplay track's silent tail and hard loop seams. This requires ffmpeg on PATH. `Scripts/Editor/setup_background_music.py` imports/configures these assets and preserves existing library edits on reruns. Runtime code has no dependency on Python or ffmpeg.

The Playback folder is explicitly included in cooking. The Game Instance Blueprint also references the library and assigned tracks. Original gameplay sound effects, classes and volumes are untouched. Music is intentionally quiet; Quest 2 listening should determine the final balance with gameplay effects/dialogue.

Validation: the Win64 Development Editor target built successfully and all affected Blueprints compiled and saved. A short desktop PIE pass confirmed startup playback, non-spatial/loop settings, the same uninterrupted player through selection and Back, tutorial assignment, outgoing fade completion before replacement, and the Level 1 track with the normal VR pawn. No Android packaging or extensive tests were run. Logs and the result are in Saved/MusicCheckpoint. The existing GameFeatureData Asset Manager warning remains unrelated to music.
