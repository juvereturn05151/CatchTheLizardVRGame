"""Original synthetic success jingle; no recordings, samples, services or credentials.
Deterministic standard-library synthesis, 48 kHz mono PCM16, 2.4 seconds.
"""
from pathlib import Path
from array import array
import math,wave,json

rate=48000
duration=2.4
out=Path(__file__).resolve().parents[2]/'SourceAudio'/'Feedback'
out.mkdir(parents=True,exist_ok=True)
audio=[0.]*round(duration*rate)
def note(start,length,midi,gain):
 frequency=440*2**((midi-69)/12)
 for n in range(round(length*rate)):
  i=round(start*rate)+n
  if i>=len(audio):break
  t=n/rate
  attack=math.sin(min(1.,t/.018)*math.pi/2)**2
  release=math.sin(min(1.,max(0.,length-t)/.11)*math.pi/2)**2
  envelope=attack*release*math.exp(-2.6*t/max(.25,length))
  tone=math.sin(math.tau*frequency*t)+.09*math.sin(math.tau*2*frequency*t)+.025*math.sin(math.tau*3*frequency*t)
  audio[i]+=gain*tone*envelope

for event in [(0.,.5,72,.6),(.2,.5,76,.6),(.4,.55,79,.6),(.65,.7,84,.65),(.98,.5,79,.4),
              (1.25,1.1,72,.42),(1.25,1.1,76,.35),(1.25,1.1,79,.35),
              (0.,.65,60,.17),(.65,.65,67,.17),(1.25,1.1,60,.2)]:note(*event)
peak=max(map(abs,audio))
rms=math.sqrt(sum(x*x for x in audio)/len(audio))
gain=min(.075/rms,.26/peak)
audio=[x*gain for x in audio];audio[0]=audio[-1]=0.
with wave.open(str(out/'SFX_MissionComplete.wav'),'wb') as w:
 w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate)
 w.writeframes(array('h',[round(x*32767) for x in audio]).tobytes())
report={'type':'original procedurally synthesized effect, not a recording','duration_seconds':duration,'sample_rate':rate,'channels':1,'peak_dbfs':20*math.log10(max(map(abs,audio))),'rms_dbfs':20*math.log10(math.sqrt(sum(x*x for x in audio)/len(audio)))}
(out/'GenerationReport.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
