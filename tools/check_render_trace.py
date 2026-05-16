#!/usr/bin/env python3
import json
import sys
import argparse
from collections import defaultdict

def analyze_trace(trace_path):
    try:
        with open(trace_path, 'r') as f:
            events = json.load(f)
    except Exception as e:
        print(f"Error: Could not parse trace file: {e}")
        return False

    print(f"Analyzing {len(events)} trace events...")

    # Basic stats
    event_types = defaultdict(int)
    scopes = {}
    frame_times = []
    
    # Track open scopes for nesting validation
    open_scopes = []
    
    for event in events:
        etype = event.get('type')
        ename = event.get('name')
        ts = event.get('ts')
        
        event_types[etype] += 1
        
        if etype == 'scope_begin':
            open_scopes.append((ename, ts))
        elif etype == 'scope_end':
            if not open_scopes:
                print(f"Error: scope_end '{ename}' without matching scope_begin")
                return False
            
            begin_name, begin_ts = open_scopes.pop()
            if begin_name != ename:
                print(f"Error: scope mismatch! Got end '{ename}', expected end for '{begin_name}'")
                return False
            
            duration_ms = (ts - begin_ts) / 1000000.0
            if ename == 'render.frame':
                frame_times.append(duration_ms)
    
    if open_scopes:
        print(f"Error: {len(open_scopes)} scopes were never closed: {[s[0] for s in open_scopes]}")
        return False

    print("\nEvent Statistics:")
    for etype, count in event_types.items():
        print(f"  {etype}: {count}")

    if frame_times:
        avg_frame = sum(frame_times) / len(frame_times)
        max_frame = max(frame_times)
        print(f"\nFrame Timing Statistics:")
        print(f"  Frame count: {len(frame_times)}")
        print(f"  Avg frame:   {avg_frame:.2f}ms")
        print(f"  Max frame:   {max_frame:.2f}ms")
    else:
        print("\nNo 'render.frame' scopes found.")

    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tachyon Trace Validator")
    parser.add_argument("trace_file", help="Path to JSON trace file")
    args = parser.parse_args()
    
    success = analyze_trace(args.trace_file)
    sys.exit(0 if success else 1)
