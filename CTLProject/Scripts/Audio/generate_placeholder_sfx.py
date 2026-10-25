"""Original synthesized game placeholders, not realistic recordings.

Pure Python, deterministic, 48 kHz mono PCM16. No external generator or samples.
Outputs source WAVs plus measured levels to SourceAudio/SFX. Run with Python 3.
"""
from array import array
from pathlib import Path
import json
import math
import random
import wave

RATE = 48000
OUT = Path(__file__).resolve().parents[2] / 'SourceAudio' / 'SFX'
OUT.mkdir(parents=True, exist_ok=True)
rng = random.Random(731904)
TAU = math.tau

def envelope(t, duration, attack=.012, release=.035):
    a = min(1., t / attack)
    b = min(1., (duration - t) / release)
    return math.sin(max(0., a) * math.pi / 2) ** 2 * math.sin(max(0., b) * math.pi / 2) ** 2

def noise(duration):
    # Warm filters for a stationary, gently band-limited breath/noise texture.
    low = slow = 0.
    result = []
    for i in range(int(duration * RATE) + 2400):
        low += .22 * (rng.uniform(-1, 1) - low)
        slow += .025 * (low - slow)
        if i >= 2400:
            result.append(low - slow)
    return result

def chirp(duration, start, end, wobble=0.):
    phase = 0.
    result = []
    for i in range(round(duration * RATE)):
        t = i / RATE
        frequency = start + (end - start) * t / duration + wobble * math.sin(TAU * 8 * t)
        phase += TAU * frequency / RATE
        result.append((math.sin(phase) + .12 * math.sin(2 * phase)) * envelope(t, duration))
    return result

def chime(duration, notes):
    result = [0.] * round(duration * RATE)
    for start, frequency in notes:
        for i in range(round(start * RATE), len(result)):
            t = i / RATE - start
            length = duration - start
            result[i] += (math.sin(TAU * frequency * t) + .08 * math.sin(TAU * frequency * 2 * t)) * math.exp(-7 * t) * envelope(t, length, .015, .06)
    return result

sounds = {}
sounds['SFX_UIHover'] = chirp(.065, 790, 640)
sounds['SFX_UIConfirm'] = chime(.12, [(0., 540), (.018, 810)])
sounds['SFX_ObjectPickup'] = chirp(.15, 240, 480)
broom = noise(.25)
sounds['SFX_BroomSwing'] = [x * math.sin(math.pi * i / (len(broom) - 1)) ** 2 for i, x in enumerate(broom)]
spray = noise(1.1)
seam = round(.1 * RATE)
loop = spray[seam:-seam]
for i in range(seam):
    angle = (i + .5) / seam * math.pi / 2
    loop.append(spray[-seam + i] * math.cos(angle) + spray[i] * math.sin(angle))
sounds['SFX_SprayLoop'] = loop
sounds['SFX_StunActivated'] = chirp(.48, 660, 210, 65)
sounds['SFX_ContainerCapture'] = chime(.46, [(0., 392), (.055, 587.33), (.12, 783.99)])
sounds['SFX_TutorialComplete'] = chime(.62, [(0., 523.25), (.10, 659.25), (.20, 880)])

report = {}
for name, values in sounds.items():
    # Soft overall mix, with headroom even if multiple different effects coincide.
    average = sum(values) / len(values)
    values = [x - average for x in values]
    rms = math.sqrt(sum(x * x for x in values) / len(values))
    target = .065 if name == 'SFX_SprayLoop' else .09
    gain = min(target / max(rms, 1e-9), .28 / max(abs(x) for x in values))
    values = [x * gain for x in values]
    if name != 'SFX_SprayLoop':
        values[0] = values[-1] = 0.
    pcm = array('h', [round(x * 32767) for x in values])
    with wave.open(str(OUT / (name + '.wav')), 'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(RATE)
        wav.writeframes(pcm.tobytes())
    report[name] = {'duration_s': len(values) / RATE,
                    'peak_dbfs': 20 * math.log10(max(abs(x) for x in values)),
                    'rms_dbfs': 20 * math.log10(math.sqrt(sum(x * x for x in values) / len(values))),
                    'loop': name == 'SFX_SprayLoop'}
(OUT / 'GenerationReport.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
