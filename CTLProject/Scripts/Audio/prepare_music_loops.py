"""Prepare circular seam-crossfaded WAV imports; leave the original music assets intact.

Usage: python prepare_music_loops.py [folder containing the two original MP3s]
Requires ffmpeg on PATH. Outputs editable/reimportable source WAVs in SourceAudio/Music.
"""
from array import array
from pathlib import Path
import math
import subprocess
import sys
import wave

source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.home() / 'Downloads'
destination = Path(__file__).resolve().parents[2] / 'SourceAudio' / 'Music'
destination.mkdir(parents=True, exist_ok=True)
for filename, name in [('Tiptoeing Curiosity.mp3', 'TutorialBGM_Loop'),
                       ('Tiptoeing Through the Woods.mp3', 'GameBGM_Loop')]:
    pcm = subprocess.run(['ffmpeg', '-v', 'error', '-i', str(source / filename),
                          '-vn', '-ar', '48000', '-ac', '2', '-f', 's16le', '-'],
                         check=True, stdout=subprocess.PIPE).stdout
    samples = array('h', pcm)
    frames = len(samples) // 2
    # Remove only trailing near-silence (-50 dB), including the gameplay MP3's silent tail.
    while frames > 48000 and max(abs(samples[2 * (frames - 1)]), abs(samples[2 * (frames - 1) + 1])) < 104:
        frames -= 1
    seam = 12000  # 250 ms circular equal-power crossfade, baked once instead of two runtime voices.
    result = samples[2 * seam:2 * (frames - seam)]
    for i in range(seam):
        angle = (i + .5) / seam * math.pi / 2
        for channel in range(2):
            value = samples[2 * (frames - seam + i) + channel] * math.cos(angle)
            value += samples[2 * i + channel] * math.sin(angle)
            result.append(max(-32768, min(32767, round(value))))
    output = destination / (name + '.wav')
    with wave.open(str(output), 'wb') as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(48000)
        wav.writeframes(result.tobytes())
    print(f'{name}: {len(result) / 96000:.3f}s, seam discontinuity '
          f'{max(abs(result[-2 + c] - result[c]) for c in range(2))}/32768')
