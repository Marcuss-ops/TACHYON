import cv2
import numpy as np
import argparse
from pathlib import Path

def smoothstep(x):
    x = np.clip(x, 0, 1)
    return x * x * (3 - 2 * x)

def apply_light_leak(frame, t, color_a, color_b, intensity=1.0):
    h, w = frame.shape[:2]
    u, v = np.meshgrid(np.linspace(0, 1, w), np.linspace(0, 1, h))
    
    # Simple Sweep Math
    angle = np.radians(30)
    proj = u * np.cos(angle) + v * np.sin(angle)
    center = -0.5 + t * 2.0
    dist = np.abs(proj - center)
    
    mask = np.clip(1.0 - dist / 0.3, 0, 1)
    mask = mask ** 3.0 # Sharpen
    
    leak_color = np.zeros_like(frame, dtype=np.float32)
    leak_color[:, :, 0] = color_a[0] + (color_b[0] - color_a[0]) * mask
    leak_color[:, :, 1] = color_a[1] + (color_b[1] - color_a[1]) * mask
    leak_color[:, :, 2] = color_a[2] + (color_b[2] - color_a[2]) * mask
    
    # Screen blend: 1 - (1-a)*(1-b)
    frame_norm = frame.astype(np.float32) / 255.0
    leak_final = leak_color * mask[:, :, np.newaxis] * intensity
    
    result = 1.0 - (1.0 - frame_norm) * (1.0 - leak_final)
    return (np.clip(result, 0, 1) * 255).astype(np.uint8)

def main():
    parser = argparse.ArgumentParser(description="Tachyon Fast Transition Preview")
    parser.add_argument("--out", type=str, default="output/transition_preview.mp4")
    parser.add_argument("--duration", type=float, default=1.0)
    parser.add_argument("--fps", type=int, default=30)
    
    args = parser.parse_args()
    
    w, h = 1280, 720
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out = cv2.VideoWriter(str(out_path), cv2.VideoWriter_fourcc(*'mp4v'), args.fps, (w, h))
    
    print(f"Rendering preview to {out_path}...")
    
    num_frames = max(1, int(round(args.duration * args.fps)))
    for i in range(num_frames):
        t = 0.0 if num_frames == 1 else i / (num_frames - 1)
        
        # Base frame: Dark Grey
        frame = np.full((h, w, 3), 60, dtype=np.uint8)
        
        # Apply Solar Flare style (Orange/Gold)
        color_a = [0.0, 0.4, 1.0] # BGR
        color_b = [0.1, 0.8, 1.0]
        
        result = apply_light_leak(frame, t, color_a, color_b, intensity=2.5)
        out.write(result)
        
    out.release()
    print("Done!")

if __name__ == "__main__":
    main()
