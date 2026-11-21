"""Atlas for granular resynthesis from the previously synthesized crowd examples.
No production, stadium-reference or announcer recording enters this atlas.
"""
from pathlib import Path
import sys,json
work=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(work/'pylibs'))
import numpy as np
import soundfile as sf
from scipy import signal
project=Path(__file__).resolve().parents[1]
sources=[work/'reference-ensemble-64s.wav',work/'huge-play-ensemble-64s.wav']
atlas=[];indices=[];offset=0
for source in sources:
    x,sr=sf.read(source)
    x=signal.resample_poly(x,24000,sr,axis=0)
    # Stay clear of the entry and exit fades in the source examples.
    x=x[24000:63*24000]
    x*=.15/np.sqrt(np.mean(x*x))
    atlas.append(x)
    for start in np.arange(0,57,1.5):
        indices.append((offset+int(start*24000),5*24000))
    offset+=len(x)
sf.write(project/'resources/phonemes.wav',np.concatenate(atlas),24000,subtype='PCM_16')
n=len(indices)//2
(project/'Source/BankIndex.h').write_text('#pragma once\n#include <array>\ninline constexpr int chatterRegionCount='+str(n)+';\nstruct GrainRegion { int start,length; };\ninline constexpr std::array<GrainRegion,'+str(len(indices))+'> regions{{\n'+''.join('{%d,%d},\n'%i for i in indices)+'}};\n')
(project/'resources/ATLAS-PROVENANCE.json').write_text(json.dumps({'method':'granular resynthesis of original synthesized ensemble renders','real_crowd_or_film_samples':False,'sources':[s.name for s in sources],'no_stock_tts_voice_fragments':True,'channels':2,'sample_rate':24000,'seconds':offset/24000},indent=2))
print({'regions':len(indices),'atlas_seconds':offset/24000})
