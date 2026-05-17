#!/usr/bin/env python3
import os
import sys
import json
import sqlite3
import argparse
import urllib.request
import urllib.error
import pathlib

def get_default_db_path():
    home = pathlib.Path.home()
    return home / ".tachyon" / "telemetry" / "tachyon_render_history.sqlite"

def fetch_runs_from_server(server_url):
    """Fetch list of run IDs already stored on the central server."""
    url = f"{server_url.rstrip('/')}/api/runs"
    try:
        req = urllib.request.Request(url, method='GET')
        req.add_header('Accept', 'application/json')
        with urllib.request.urlopen(req, timeout=5) as response:
            if response.status == 200:
                runs = json.loads(response.read().decode('utf-8'))
                return {r['run_id'] for r in runs if 'run_id' in r}, True
    except urllib.error.URLError as e:
        return set(), False
    except Exception as e:
        return set(), False
    return set(), False

def upload_run_to_server(server_url, payload):
    """POST a complete run telemetry payload to the central server."""
    url = f"{server_url.rstrip('/')}/api/telemetry"
    try:
        data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(url, data=data, method='POST')
        req.add_header('Content-Type', 'application/json')
        req.add_header('Accept', 'application/json')
        with urllib.request.urlopen(req, timeout=5) as response:
            if response.status == 200:
                result = json.loads(response.read().decode('utf-8'))
                return result.get('success', False), ""
            else:
                return False, f"Server returned status code {response.status}"
    except urllib.error.HTTPError as e:
        try:
            err_data = json.loads(e.read().decode('utf-8'))
            return False, err_data.get('error', str(e))
        except Exception:
            return False, str(e)
    except Exception as e:
        return False, str(e)

def main():
    parser = argparse.ArgumentParser(description="Tachyon Telemetry SQLite Multi-Machine Sync Client")
    parser.add_argument("--server", type=str, default=os.environ.get("TACHYON_CENTRAL_DB_URL", "http://localhost:8080"),
                        help="Central telemetry server URL (default: http://localhost:8080 or TACHYON_CENTRAL_DB_URL)")
    parser.add_argument("--local-db", type=str, default=str(get_default_db_path()),
                        help="Path to the local SQLite database")
    parser.add_argument("--verbose", action="store_true", help="Print detailed logs during synchronization")
    args = parser.parse_args()

    print("=======================================================")
    print("🔄  Tachyon SQLite Telemetry Synchronization Hub")
    print(f"📂  Local Database: {args.local_db}")
    print(f"🌐  Target Server : {args.server}")
    print("=======================================================\n")

    # 1. Connect to Central Server
    print("📡 Connecting to Central Telemetry Server...")
    existing_run_ids, connected = fetch_runs_from_server(args.server)
    if not connected:
        print("⚠️  WARNING: Could not connect to the central server or server is offline.")
        print("💡 Your local telemetry remains 100% safe. Sync will retry when online. Exiting gracefully.")
        sys.exit(0)
    
    print(f"✅ Connected successfully! Server has {len(existing_run_ids)} historical run records.")

    # 2. Check local database
    if not os.path.exists(args.local_db):
        print(f"ℹ️  No local SQLite database found at {args.local_db}")
        print("🎉 All synced! Run a render session first to populate local records.")
        sys.exit(0)

    try:
        conn = sqlite3.connect(args.local_db)
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
    except Exception as e:
        print(f"❌ Error opening local database: {str(e)}")
        sys.exit(1)

    try:
        # Check if render_runs table exists
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='render_runs';")
        if not cursor.fetchone():
            print("ℹ️  Local database is empty (no render schema detected).")
            print("🎉 Nothing to sync. Run a render session first!")
            sys.exit(0)

        # 3. Find runs that are NOT present on the server
        cursor.execute("SELECT * FROM render_runs;")
        local_runs = [dict(r) for r in cursor.fetchall()]
        
        unsynced_runs = [r for r in local_runs if r['run_id'] not in existing_run_ids]

        if not unsynced_runs:
            print("🎉 ALL SYNCED! All local render records are already present in the central database.")
            sys.exit(0)

        print(f"🚀 Found {len(unsynced_runs)} unsynced render runs. Preparing batch sync...")

        success_count = 0
        for run in unsynced_runs:
            run_id = run['run_id']
            print(f"⏳ Syncing run {run_id[:8]} ({run.get('scene_id', 'N/A')})...", end="", flush=True)

            # Fetch frames
            cursor.execute("SELECT * FROM render_frames WHERE run_id = ? ORDER BY frame_number ASC;", (run_id,))
            frames = [dict(f) for f in cursor.fetchall()]

            # Fetch phases
            cursor.execute("SELECT * FROM render_phase_events WHERE run_id = ? ORDER BY duration_ms DESC;", (run_id,))
            phases = [dict(p) for p in cursor.fetchall()]

            # Fetch counters
            cursor.execute("SELECT * FROM render_counters WHERE run_id = ? ORDER BY counter_name ASC;", (run_id,))
            counters = [dict(c) for c in cursor.fetchall()]

            # Build payload
            payload = {
                "run": run,
                "frames": frames,
                "phases": phases,
                "counters": counters
            }

            # Upload
            success, err_msg = upload_run_to_server(args.server, payload)
            if success:
                print(" ✅ SUCCESS!")
                success_count += 1
            else:
                print(f" ❌ FAILED! ({err_msg})")

        print(f"\n=======================================================")
        print(f"🎉 Synchronization Complete: {success_count}/{len(unsynced_runs)} runs synced successfully!")
        print("=======================================================")

    except Exception as e:
        print(f"\n❌ Error during synchronization process: {str(e)}")
    finally:
        conn.close()

if __name__ == "__main__":
    main()
