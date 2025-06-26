import wave, array, sys
import numpy as np

ref_path='kick.wav'
asm_path='C-version/kick.wav'

def load(path):
    with wave.open(path,'rb') as w:
        frames=w.getnframes()
        data=array.array('h', w.readframes(frames))
        return np.frombuffer(data, dtype=np.int16)

ref=load(ref_path)
asm=load(asm_path)
print('Total samples', len(ref))
print('Max diff', np.max(np.abs(ref-asm)))
print('Mean diff', np.mean(np.abs(ref-asm)))
# print first 20 sample pairs
for i in range(0,40,2):
    print(i//2, ref[i], asm[i]) 