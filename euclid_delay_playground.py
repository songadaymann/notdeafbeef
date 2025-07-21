import math
import argparse
import colorsys
import threading
import numpy as np
import pygame
import random

"""
euclid_delay_playground.py  – Sparse Euclidean grooves + stereo delay

Seed-driven:
  • Euclidean pulse counts for kick/snare/hat
  • BPM 50–120 (slower, roomy)
  • Root key + major/minor pentatonic
  • Delay time & feedback

Visuals: slow orbiting coloured circles whose size follows the running
audio level; delayed echoes leave fading rings.
"""

# ---------- CONSTANTS ----------
WIDTH, HEIGHT = 512, 512
FPS = 30
SR = 44_100
STEPS_PER_BEAT = 4   # 16th notes
BARS_PER_SEG = 8
CROSS_MS = 300

MIX_EVENT = pygame.USEREVENT + 1
NEXT_EVENT = pygame.USEREVENT + 2

# ---------- SEED ----------
parser = argparse.ArgumentParser()
parser.add_argument('--seed', type=str, default='0x42')
args,_ = parser.parse_known_args()
SEED = int(args.seed,0)

grng = np.random.default_rng(SEED)

# ---------- MUSICAL PARAMS ----------
BPM = int(grng.integers(50,121))
BEAT_SEC = 60/BPM
step_sec = BEAT_SEC/STEPS_PER_BEAT
steps_per_seg = int(BARS_PER_SEG*4*STEPS_PER_BEAT)
seg_dur = step_sec*steps_per_seg

root_freq = float(grng.choice([220,233.08,246.94,261.63,293.66]))
scale_type = str(grng.choice(['major','minor']))

print(f"Seed {SEED}: {BPM} BPM, {scale_type}, root {root_freq}Hz")

pent_major=[0,2,4,7,9]
pent_minor=[0,3,5,7,10]
scale_int = pent_major if scale_type=='major' else pent_minor

# ---------- BASS PROFILES ----------

bass_profiles = {
    'default': {'ratio':2.0,'index':5.0,'decay':10,'amp':0.4},
    'quantum': {'ratio':1.5,'index':8.0,'decay':8,'amp':0.45},
    'plucky' : {'ratio':3.0,'index':2.5,'decay':14,'amp':0.35},
}

bass_choice = grng.choice(list(bass_profiles.keys()))
bp = bass_profiles[bass_choice]
print(f"Bass profile: {bass_choice}")

# ---------- DENSITY (always sparse) ----------
DENSITY_FACTOR=0.5  # fixed sparse

# ---------- DELAY PARAMS ----------
# Delay varies: choose factor 2,1,0.5,0.25 beats (half, quarter, eighth, sixteenth)
delay_factors = [2.0,1.0,0.5,0.25]
delay_factor = float(grng.choice(delay_factors))
delay_ms = int(BEAT_SEC * delay_factor * 1000)
feedback = 0.45
print(f"Delay {delay_factor:g} beats ({delay_ms} ms), feedback {feedback:.2f}")

delay_samples = int(SR*delay_ms/1000)

# ---------- EUCLIDEAN RHYTHMS ----------

def euclidean(pulses:int, steps:int):
    """Return list of 0/1 per step using the Bjorklund algorithm (bucket method)."""
    pattern=[]
    bucket=0
    for _ in range(steps):
        bucket+=pulses
        if bucket>=steps:
            bucket-=steps
            pattern.append(1)
        else:
            pattern.append(0)
    return pattern

step_count_bar = 4*STEPS_PER_BEAT  # 16 steps

kick_pulses  = int(grng.integers(1,4))  # 1-3 hits per bar
snare_pulses = int(grng.integers(0,3))  # maybe 0-2 hits
hat_pulses   = int(grng.integers(2,5))  # 2-4 ticks

kick_pattern  = euclidean(kick_pulses, step_count_bar)
snare_pattern = euclidean(snare_pulses, step_count_bar)
hat_pattern   = euclidean(hat_pulses, step_count_bar)

# Rotate patterns randomly
rot = int(grng.integers(0,step_count_bar))
kick_pattern  = kick_pattern[rot:]+kick_pattern[:rot]
snare_pattern = snare_pattern[rot:]+snare_pattern[:rot]
hat_pattern   = hat_pattern[rot:]+hat_pattern[:rot]

# ---------- AUDIO SEGMENT ----------

def lfsr16(prev:int)->int:
    bit = ((prev>>0)^(prev>>2)^(prev>>3)^(prev>>5)) &1
    return ((prev>>1)| (bit<<15)) & 0xFFFF


def make_segment(sr:int, lfsr_state:int):
    N = int(seg_dur*sr)
    buf = np.zeros((N,2), dtype=np.float32)
    delay_buf = np.zeros((N+delay_samples,2), dtype=np.float32)

    # RMS levels per frame (30fps)
    samples_per_frame = sr // FPS
    num_frames = int(seg_dur * FPS)
    rms_levels = np.zeros(num_frames)

    for step in range(steps_per_seg):
        t0 = int(step*step_sec*sr)
        t1 = int((step+1)*step_sec*sr)
        n  = t1-t0
        sl = buf[t0:t1]

        bar_pos = step % step_count_bar
        # ----- drums -----
        if kick_pattern[bar_pos]==1:
            env = np.exp(-20*np.linspace(0,step_sec,n))
            tone = np.sin(2*math.pi*50*np.linspace(0,step_sec,n))
            sl += 0.8*env[:,None]*tone[:,None]
        if snare_pattern[bar_pos]==1:
            env=np.exp(-35*np.linspace(0,step_sec,n))
            noise=grng.uniform(-1,1,n)[:,None]
            sl += 0.4*env[:,None]*noise
        if hat_pattern[bar_pos]==1:
            env=np.exp(-120*np.linspace(0,step_sec,n))
            noise=grng.uniform(-1,1,n)[:,None]
            sl += 0.15*env[:,None]*noise

        # ----- deterministic saw melody hits on each beat -----
        freq = None
        step32 = step % 32  # 2-bar cycle (32 sixteenth-notes)
        if step32 in (0,8,16,24):
            if step32==0:      # high
                freq = root_freq*4
            elif step32==8:    # mid-high pentatonic
                candidate=[d for d in scale_int if 0<d<12*3]
                deg=grng.choice(candidate)
                freq = root_freq*2**(deg/12)
            elif step32==16:   # very high again
                freq = root_freq*4
            else:              # step32 24 -> octave down
                freq = root_freq * 2**(deg/12)  # root octave
        if freq is not None:
            t_arr_b = np.linspace(0, BEAT_SEC, int(BEAT_SEC*sr))
            env_b = np.exp(-10*t_arr_b)
            ratio = 2.0
            index = 5.0
            carrier_phase = 2*math.pi*freq*t_arr_b + index*np.sin(2*math.pi*freq*ratio*t_arr_b)
            wave = 0.6*env_b*np.sin(carrier_phase)
            phase = 2*math.pi*freq*np.linspace(0, BEAT_SEC, int(BEAT_SEC*sr))
            env = np.exp(-5*np.linspace(0, BEAT_SEC, phase.size))
            frac = np.modf(phase/(2*math.pi))[0]
            raw = 2*frac - 1
            driven = 1.2*raw
            soft = 1.5*driven - 0.5*(driven**3)
            saw = 0.25*env*soft
            end_idx = min(t0+saw.size, N)
            slice_segment = buf[t0:end_idx]
            saw = saw[:slice_segment.shape[0]]
            slice_segment[:,0]+=saw
            slice_segment[:,1]+=saw

        # ----- offbeat mid-range notes -----
        spawn=False
        if step % 4 == 2:
            spawn=True
        elif step % 4 in (1,3) and grng.random()<0.2*DENSITY_FACTOR:
            spawn=True

        if spawn:
            deg = grng.choice(scale_int)
            mid_freq = root_freq*2**((deg/12)+1)
            phase = 2*math.pi*mid_freq*np.linspace(0, step_sec, n)
            env_t = np.exp(-6*np.linspace(0, step_sec, n))
            wf = grng.choice(['tri','sine','square','fm_bells','fm_calm','fm_quantum','fm_pluck'])
            if wf=='tri':
                wave = (2/np.pi)*np.arcsin(np.sin(phase))
            elif wf=='sine':
                wave = np.sin(phase)
            elif wf=='square':
                wave = np.sign(np.sin(phase))
            elif wf=='fm_bells':
                # FM Bells: bright metallic sound
                mod_ratio = 3.5
                mod_index = 4.0
                mod_phase = 2*math.pi*mid_freq*mod_ratio*np.linspace(0, step_sec, n)
                carrier_phase = phase + mod_index*np.sin(mod_phase)
                wave = np.sin(carrier_phase)
            elif wf=='fm_calm':
                # Calming Bells: softer, more harmonic
                mod_ratio = 2.0
                mod_index = 2.5
                mod_phase = 2*math.pi*mid_freq*mod_ratio*np.linspace(0, step_sec, n)
                carrier_phase = phase + mod_index*np.sin(mod_phase)
                wave = np.sin(carrier_phase)
            elif wf=='fm_quantum':
                # Quantum Piano: detuned, ethereal
                mod_ratio = 1.5
                mod_index = 3.0
                mod_phase = 2*math.pi*mid_freq*mod_ratio*np.linspace(0, step_sec, n)
                carrier_phase = phase + mod_index*np.sin(mod_phase)
                wave = np.sin(carrier_phase)
            else:  # fm_pluck
                # Eastern Plucks: percussive, string-like
                mod_ratio = 1.0
                mod_index = 6.0 * env_t  # index decays with envelope for pluck effect
                mod_phase = 2*math.pi*mid_freq*mod_ratio*np.linspace(0, step_sec, n)
                carrier_phase = phase + mod_index*np.sin(mod_phase)
                wave = np.sin(carrier_phase)
            note = 0.2*env_t*wave
            sl[:,0]+=note
            sl[:,1]+=note

        # ----- bass FM every beat -----
        if step % (STEPS_PER_BEAT*8) == 0:  # once every 2 bars
            deg = grng.choice(scale_int)
            bass_freq = root_freq/4 * 2**(deg/12)  # two octaves below root
            tb = np.linspace(0, BEAT_SEC, int(BEAT_SEC*sr))
            envb = np.exp(-bp['decay']*tb)
            ratio_b = bp['ratio']
            index_b = bp['index']
            carrier_b = 2*math.pi*bass_freq*tb + index_b*np.sin(2*math.pi*bass_freq*ratio_b*tb)
            bass_wave = bp['amp']*envb*np.sin(carrier_b)
            end_b = min(t0 + bass_wave.size, N)
            slice_b = buf[t0:end_b]
            bass_wave = bass_wave[:slice_b.shape[0]]
            slice_b[:,0]+=bass_wave
            slice_b[:,1]+=bass_wave

    # ----- stereo delay -----
    delay_buf[:N]=buf
    for i in range(delay_samples,N):
        delay_buf[i]+=delay_buf[i-delay_samples]*feedback
    out = delay_buf[:N]
    out=np.clip(out,-1,1)

    # compute RMS per visual frame
    for f in range(num_frames):
        start = f * samples_per_frame
        end = min(start + samples_per_frame, N)
        chunk = out[start:end]
        rms_levels[f] = np.sqrt(np.mean(chunk**2))

    return (out*32767).astype(np.int16), lfsr_state, rms_levels

# ---------- VISUALS ----------

hsv = lambda h,s,v: tuple(int(c*255) for c in colorsys.hsv_to_rgb(h,s,v))
circle_color = hsv
base_hue = grng.random()
orbit_radius = min(WIDTH,HEIGHT)//3

# Metroid floor tiles
TILE_SIZE = 32
SCROLL_SPEED = 2  # pixels per frame
floor_rows = 2
tile_color = circle_color((base_hue+0.3)%1,1,0.8)

# build procedural 32x32 rock tile (deterministic)
def build_tile(base_col):
    arr = np.zeros((TILE_SIZE, TILE_SIZE, 3), dtype=np.uint8)
    dark = np.array(base_col, dtype=np.uint8)
    mid  = (np.array(base_col)*1.3).clip(0,255).astype(np.uint8)
    hi   = (np.array(base_col)*1.8).clip(0,255).astype(np.uint8)
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            h = ((x*13 + y*7) ^ (x>>3)) & 0xFF
            if h < 40:
                arr[y,x]=hi
            elif h < 120:
                arr[y,x]=mid
            else:
                arr[y,x]=dark
    return pygame.surfarray.make_surface(arr.swapaxes(0,1))

TILE_SURF = build_tile(tile_color)

# build slope tile (45 degree angle)
def build_slope_tile(base_col, direction='up'):
    arr = np.zeros((TILE_SIZE, TILE_SIZE, 3), dtype=np.uint8)
    dark = np.array(base_col, dtype=np.uint8)
    mid  = (np.array(base_col)*1.3).clip(0,255).astype(np.uint8)
    hi   = (np.array(base_col)*1.8).clip(0,255).astype(np.uint8)
    
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            # slope threshold
            if direction == 'up':
                threshold = x
            else:  # down
                threshold = TILE_SIZE - x
                
            if y > threshold:
                # below slope - solid
                h = ((x*13 + y*7) ^ (x>>3)) & 0xFF
                if h < 40:
                    arr[y,x]=hi
                elif h < 120:
                    arr[y,x]=mid
                else:
                    arr[y,x]=dark
            # above slope is transparent (black)
    return pygame.surfarray.make_surface(arr.swapaxes(0,1))

SLOPE_UP_SURF = build_slope_tile(tile_color, 'up')
SLOPE_DOWN_SURF = build_slope_tile(tile_color, 'down')

# Generate terrain pattern
TERRAIN_LENGTH = 64  # tiles
terrain_pattern = []
terrain_rng = np.random.default_rng(SEED ^ 0x7E44A1)

i = 0
while i < TERRAIN_LENGTH:
    feature = terrain_rng.choice(['flat', 'wall', 'slope_up', 'slope_down', 'gap'])
    
    if feature == 'flat':
        # flat ground 2-6 tiles
        length = terrain_rng.integers(2, 7)
        for _ in range(min(length, TERRAIN_LENGTH - i)):
            terrain_pattern.append({'type': 'flat', 'height': 2})
            i += 1
            
    elif feature == 'wall':
        # 2x2 or 3x3 wall
        wall_height = terrain_rng.choice([4, 6])
        wall_width = terrain_rng.integers(2, 5)
        for _ in range(min(wall_width, TERRAIN_LENGTH - i)):
            terrain_pattern.append({'type': 'wall', 'height': wall_height})
            i += 1
            
    elif feature == 'slope_up':
        # upward slope
        terrain_pattern.append({'type': 'slope_up', 'height': 2})
        i += 1
        # followed by elevated platform
        length = terrain_rng.integers(2, 5)
        for _ in range(min(length, TERRAIN_LENGTH - i)):
            terrain_pattern.append({'type': 'flat', 'height': 3})
            i += 1
            
    elif feature == 'slope_down':
        # downward slope back to ground
        terrain_pattern.append({'type': 'slope_down', 'height': 3})
        i += 1
        
    else:  # gap
        # empty space 1-2 tiles
        length = terrain_rng.integers(1, 3)
        for _ in range(min(length, TERRAIN_LENGTH - i)):
            terrain_pattern.append({'type': 'gap', 'height': 0})
            i += 1

# Samus placeholder removed

# ensure VIS_MODE defined globally before draw()
if BPM < 70:
    VIS_MODE = 'thick'
elif BPM < 100:
    VIS_MODE = 'rings'
elif BPM < 130:
    VIS_MODE = 'poly'
else:
    VIS_MODE = 'lissa'

# ---------- DEGRADATION PARAMS ----------
# Randomize "worn" effect levels based on seed
degrade_rng = np.random.default_rng(SEED ^ 0xDE5A7)

# Persistence (ghost trails): 0.3 (heavy trails) to 0.9 (minimal trails)
PERSISTENCE = float(degrade_rng.uniform(0.3, 0.9))

# Scanline intensity: 0 (none) to 200 (heavy)
SCANLINE_ALPHA = int(degrade_rng.uniform(0, 200))

# Chromatic aberration: 0 (none) to 5 (heavy RGB shift)
CHROMA_SHIFT = int(degrade_rng.uniform(0, 5))

# Noise amount: 0 (clean) to 300 (very noisy)
NOISE_PIXELS = int(degrade_rng.uniform(0, 300))

# Jitter: random position offset 0 (stable) to 3 (shaky)
JITTER_AMOUNT = float(degrade_rng.uniform(0, 3))

# Frame drops: 0 (smooth) to 0.1 (10% chance of repeat frame)
FRAME_DROP_CHANCE = float(degrade_rng.uniform(0, 0.1))

# Color bleed: 0 (sharp) to 0.3 (blurry colors)
COLOR_BLEED = float(degrade_rng.uniform(0, 0.3))

print(f"Degradation: persist={PERSISTENCE:.2f}, scan={SCANLINE_ALPHA}, chroma={CHROMA_SHIFT}, noise={NOISE_PIXELS}")
print(f"Jitter={JITTER_AMOUNT:.1f}, drops={FRAME_DROP_CHANCE:.2f}, bleed={COLOR_BLEED:.2f}")

def draw(surface:pygame.Surface, frame:int, level:float):
    surface.fill((0,0,0))
    t=frame/ FPS
    # main orbiting circle
    ang=t*0.2
    cx=int(WIDTH/2+math.cos(ang)*orbit_radius)
    cy=int(HEIGHT/2+math.sin(ang)*orbit_radius)
    base_r=int(30+80*level)

    if VIS_MODE=='thick':
        pygame.draw.circle(surface,circle_color(base_hue,1,1),(cx,cy),base_r+10,6)

    elif VIS_MODE=='rings':
        for k in range(3):
            rr=base_r+k*15+10*math.sin(t+k)
            col=circle_color((base_hue+0.05*k)%1,1,1)
            pygame.draw.circle(surface,col,(cx,cy),int(rr),2)

    elif VIS_MODE=='poly':
        n=4+int(BPM/30)  # 4–6 sides
        pts=[(cx+math.cos(t+i*2*math.pi/n)*base_r,
               cy+math.sin(t+i*2*math.pi/n)*base_r) for i in range(n)]
        pygame.draw.polygon(surface,circle_color(base_hue,1,1),pts,2)

    else: # lissa figure-8 style
        for phi in np.linspace(0,2*math.pi,120):
            x=cx+base_r*math.sin(2*phi+t)
            y=cy+base_r*math.sin(3*phi)
            surface.set_at((int(x)%WIDTH,int(y)%HEIGHT),circle_color(base_hue,1,1))

def draw_floor(surface:pygame.Surface, frame:int):
    # ---- scrolling floor ----
    offset = (frame*SCROLL_SPEED) % TILE_SIZE
    
    # Calculate which terrain tiles are visible
    tiles_per_screen = (WIDTH // TILE_SIZE) + 2
    scroll_tiles = (frame * SCROLL_SPEED) // TILE_SIZE
    
    for i in range(tiles_per_screen):
        terrain_idx = (scroll_tiles + i) % len(terrain_pattern)
        terrain = terrain_pattern[terrain_idx]
        
        x0 = i * TILE_SIZE - offset
        
        if terrain['type'] == 'gap':
            continue  # skip drawing for gaps
            
        # Draw tiles based on height
        for row in range(terrain['height']):
            y0 = HEIGHT - (row+1)*TILE_SIZE
            
            if terrain['type'] == 'slope_up' and row == terrain['height']-1:
                surface.blit(SLOPE_UP_SURF, (x0, y0))
            elif terrain['type'] == 'slope_down' and row == terrain['height']-1:
                surface.blit(SLOPE_DOWN_SURF, (x0, y0))
            else:
                surface.blit(TILE_SURF, (x0, y0))

# ---------- MAIN ----------

def main():
    pygame.init(); pygame.display.set_caption("Euclid Delay Playground")
    pygame.mixer.init(frequency=SR,size=-16,channels=2)

    current={'chan':None,'rms':None}; next_buf={'data':None,'state':12345,'rms':None}; ready=threading.Event(); seg_start_ms=pygame.time.get_ticks()

    def producer(state:int):
        buf,st,rms=make_segment(SR,state)
        next_buf['data']=buf; next_buf['state']=st; next_buf['rms']=rms; ready.set()

    # first segment
    buf0,state,rms0=make_segment(SR,0xACE1)
    ch0=pygame.mixer.Sound(buf0).play()
    duration_ms=int(seg_dur*1000)
    pygame.time.set_timer(NEXT_EVENT,duration_ms-CROSS_MS,loops=1)
    current['chan']=ch0
    current['rms']=rms0
    threading.Thread(target=producer,args=(state,),daemon=True).start()

    screen=pygame.display.set_mode((WIDTH,HEIGHT))
    pygame.font.init(); FONT=pygame.font.SysFont('Courier', 24)
    clock=pygame.time.Clock(); frame=0; level=0.0
    particles=[]
    last_step=-1
    bass_hits=[]  # track bass hit visuals
    running=True

    # --- post-process surfaces ---
    prev_frame = pygame.Surface((WIDTH, HEIGHT)).convert_alpha()
    prev_frame.fill((0,0,0))

    # Scanline mask precompute
    scan_mask = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    if SCANLINE_ALPHA > 0:
        for y in range(0, HEIGHT, 2):
            pygame.draw.line(scan_mask, (0,0,0,SCANLINE_ALPHA), (0,y), (WIDTH,y))

    while running:
        for e in pygame.event.get():
            if e.type==pygame.QUIT: running=False
            if e.type==NEXT_EVENT:
                if ready.is_set():
                    buf=next_buf['data']; state=next_buf['state']; rms=next_buf['rms']; ready.clear()
                else:
                    buf,state,rms=make_segment(SR,state)
                threading.Thread(target=producer,args=(state,),daemon=True).start()
                ch=pygame.mixer.Sound(buf).play(fade_ms=CROSS_MS)
                if current['chan'] and current['chan'].get_busy():
                    current['chan'].fadeout(CROSS_MS)
                current['chan']=ch
                current['rms']=rms
                pygame.time.set_timer(NEXT_EVENT,duration_ms-CROSS_MS,loops=1)
                seg_start_ms=pygame.time.get_ticks()
            if e.type==pygame.KEYDOWN and e.key==pygame.K_ESCAPE: running=False

        # calculate elapsed time in segment
        elapsed_ms = pygame.time.get_ticks() - seg_start_ms
        
        # use pre-computed RMS level
        if current['rms'] is not None:
            frame_idx = int((elapsed_ms/1000)*FPS) % len(current['rms'])
            level = float(current['rms'][frame_idx])
        else:
            level = 0.1

        # explosions sync based on audio position for tight sync
        if elapsed_ms < seg_dur*1000:
            current_step = int((elapsed_ms/1000)/step_sec) % 32

            # spawn burst
            if ((current_step+2) % 32) in SAW_STEPS and current_step!=last_step:
                last_step=current_step
                num_p=20
                cx=random.uniform(WIDTH*0.3, WIDTH*0.7)
                cy=random.uniform(HEIGHT*0.2, HEIGHT*0.5)
                for i in range(num_p):
                    angle=2*math.pi*i/num_p
                    speed=random.uniform(2,4)
                    vx=math.cos(angle)*speed
                    vy=math.sin(angle)*speed
                    char=random.choice(glyph_set)
                    col=hsv((base_hue + random.uniform(-0.1,0.1))%1,1,1)
                    surf=FONT.render(char, True, col)
                    life=random.choice([30,60,90])
                    particles.append(Particle(surf,cx,cy,vx,vy,life))

            # spawn bass hit visual every 8 beats (2 bars)
            if current_step % (STEPS_PER_BEAT*8) == 0 and current_step != last_step:
                shape_type = grng.choice(['triangle', 'diamond', 'hexagon', 'star', 'square'])
                hue_shift = grng.uniform(-0.2, 0.2)
                bass_hits.append(BassHitShape(shape_type, (base_hue + hue_shift) % 1))

            # update particles
            particles=[p for p in particles if p.life>0]
            for p in particles:
                p.update()

            # update bass hits
            bass_hits=[b for b in bass_hits if b.alpha>0]
            for b in bass_hits:
                b.update()

            # persistence: start with faded previous frame
            screen.blit(prev_frame, (0,0))
            alpha = int(255 * PERSISTENCE)
            prev_frame.fill((0,0,0,alpha), special_flags=pygame.BLEND_RGBA_MULT)

            # Draw centerpiece first
            draw(screen, frame, level)
            
            # Then bass hits (behind floor but in front of centerpiece)
            for b in bass_hits:
                b.draw(screen)
            
            # Then floor on top
            draw_floor(screen, frame)
            
            # Finally particles on top
            for p in particles:
                p.draw(screen)

            # scanline mask
            if SCANLINE_ALPHA > 0:
                screen.blit(scan_mask, (0,0))

            # chroma jitter: shift red + blue channels slightly alternating
            if CHROMA_SHIFT > 0:
                if frame%15<7:
                    offset=CHROMA_SHIFT
                else:
                    offset=-CHROMA_SHIFT
                temp=screen.copy()
                tinted=temp.copy(); tinted.fill((255,0,0), special_flags=pygame.BLEND_MULT)
                screen.blit(tinted, (offset,0), special_flags=pygame.BLEND_ADD)
                tinted=temp.copy(); tinted.fill((0,0,255), special_flags=pygame.BLEND_MULT)
                screen.blit(tinted, (-offset,0), special_flags=pygame.BLEND_ADD)

            # color bleed (horizontal blur)
            if COLOR_BLEED > 0:
                temp = screen.copy()
                temp.set_alpha(int(255 * COLOR_BLEED))
                screen.blit(temp, (1, 0))
                screen.blit(temp, (-1, 0))

            # store for next decay
            prev_frame.blit(screen, (0,0))

            # jitter effect (screen shake)
            if JITTER_AMOUNT > 0 and random.random() < 0.3:
                jx = int(random.uniform(-JITTER_AMOUNT, JITTER_AMOUNT))
                jy = int(random.uniform(-JITTER_AMOUNT, JITTER_AMOUNT))
                temp = screen.copy()
                screen.fill((0,0,0))
                screen.blit(temp, (jx, jy))

            # random pixel noise
            for _ in range(NOISE_PIXELS):
                rx=random.randint(0,WIDTH-1); ry=random.randint(0,HEIGHT-1)
                screen.set_at((rx,ry), (random.randint(0,255),)*3)

            # frame drop effect (repeat previous frame occasionally)
            if FRAME_DROP_CHANCE > 0 and random.random() > FRAME_DROP_CHANCE:
                pygame.display.flip()
            clock.tick(FPS); frame+=1

    pygame.mixer.stop(); pygame.quit()

# glyphs and saw-step list for explosions
glyph_set = list("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*+-=?")
SAW_STEPS=[0,8,16,24]

# ---------- Explosion Particles ----------
class Particle:
    def __init__(self, char_surf, x, y, vx, vy, life):
        self.base=char_surf.convert_alpha()
        self.x=x; self.y=y
        self.vx=vx; self.vy=vy
        self.life=life
        self.max_life=life
    def update(self):
        self.x+=self.vx; self.y+=self.vy
        self.vy+=0.1  # gravity
        self.life-=1
    def draw(self, surface):
        if self.life>0:
            alpha=int(255*self.life/self.max_life)
            self.base.set_alpha(alpha)
            surface.blit(self.base, (int(self.x), int(self.y)))

# ---------- Bass Hit Shapes ----------
class BassHitShape:
    def __init__(self, shape_type, hue):
        self.shape_type = shape_type
        self.color = hsv(hue, 1, 1)
        self.alpha = 255
        self.scale = 0.1  # starts small, grows quickly
        self.max_size = min(WIDTH, HEIGHT) * 0.6  # huge!
        self.rotation = 0
        self.rot_speed = grng.uniform(-0.05, 0.05)
        
    def update(self):
        # grow rapidly then fade
        if self.scale < 1.0:
            self.scale += 0.15  # fast growth
        self.alpha = max(0, self.alpha - 8)  # fade out
        self.rotation += self.rot_speed
        
    def draw(self, surface):
        if self.alpha <= 0:
            return
            
        size = int(self.max_size * min(self.scale, 1.0))
        cx, cy = WIDTH//2, HEIGHT//2
        
        # Create temporary surface for shape with alpha
        temp = pygame.Surface((size*2, size*2), pygame.SRCALPHA)
        temp.fill((0,0,0,0))
        
        # Draw shape centered in temp surface
        center = size
        
        if self.shape_type == 'triangle':
            pts = []
            for i in range(3):
                angle = self.rotation + i * 2 * math.pi / 3
                x = center + size * 0.8 * math.cos(angle)
                y = center + size * 0.8 * math.sin(angle)
                pts.append((x, y))
            pygame.draw.polygon(temp, (*self.color, self.alpha), pts, 3)
            
        elif self.shape_type == 'diamond':
            pts = [
                (center, center - size*0.8),  # top
                (center + size*0.6, center),   # right
                (center, center + size*0.8),   # bottom
                (center - size*0.6, center)    # left
            ]
            # rotate points
            rot_pts = []
            for x, y in pts:
                dx, dy = x - center, y - center
                rx = dx * math.cos(self.rotation) - dy * math.sin(self.rotation)
                ry = dx * math.sin(self.rotation) + dy * math.cos(self.rotation)
                rot_pts.append((center + rx, center + ry))
            pygame.draw.polygon(temp, (*self.color, self.alpha), rot_pts, 3)
            
        elif self.shape_type == 'hexagon':
            pts = []
            for i in range(6):
                angle = self.rotation + i * math.pi / 3
                x = center + size * 0.7 * math.cos(angle)
                y = center + size * 0.7 * math.sin(angle)
                pts.append((x, y))
            pygame.draw.polygon(temp, (*self.color, self.alpha), pts, 3)
            
        elif self.shape_type == 'star':
            pts = []
            for i in range(10):
                angle = self.rotation + i * math.pi / 5
                if i % 2 == 0:
                    r = size * 0.8
                else:
                    r = size * 0.4
                x = center + r * math.cos(angle)
                y = center + r * math.sin(angle)
                pts.append((x, y))
            pygame.draw.polygon(temp, (*self.color, self.alpha), pts, 3)
            
        else:  # square
            half = size * 0.6
            pts = [
                (center - half, center - half),
                (center + half, center - half),
                (center + half, center + half),
                (center - half, center + half)
            ]
            # rotate points
            rot_pts = []
            for x, y in pts:
                dx, dy = x - center, y - center
                rx = dx * math.cos(self.rotation) - dy * math.sin(self.rotation)
                ry = dx * math.sin(self.rotation) + dy * math.cos(self.rotation)
                rot_pts.append((center + rx, center + ry))
            pygame.draw.polygon(temp, (*self.color, self.alpha), rot_pts, 3)
        
        # Blit centered on screen
        surface.blit(temp, (cx - size, cy - size))

if __name__=='__main__':
    main() 