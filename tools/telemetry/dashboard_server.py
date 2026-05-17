#!/usr/bin/env python3
import os
import sys
import json
import sqlite3
import urllib.parse
import http.server
import socketserver
import pathlib

DEFAULT_PORT = 8000

def get_db_path():
    # Automatically resolve default SQLite database path
    home = pathlib.Path.home()
    return home / ".tachyon" / "telemetry" / "tachyon_render_history.sqlite"

class TelemetryAPIHandler(http.server.BaseHTTPRequestHandler):
    db_file_path = str(get_db_path())

    def log_message(self, format, *args):
        # Keep server stdout clean
        pass

    def send_json(self, data, status=200):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode("utf-8"))

    def send_html(self, content, status=200):
        self.send_response(status)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(content.encode("utf-8"))

    def get_db_connection(self):
        if not os.path.exists(self.db_file_path):
            return None
        try:
            conn = sqlite3.connect(self.db_file_path)
            conn.row_factory = sqlite3.Row
            return conn
        except Exception:
            return None

    def do_GET(self):
        parsed_url = urllib.parse.urlparse(self.path)
        path_parts = [p for p in parsed_url.path.split("/") if p]

        # API: Export Last 100 Render Runs with Complete Details
        if len(path_parts) >= 2 and path_parts[0] == "api" and path_parts[1] == "export-last-100":
            conn = self.get_db_connection()
            if not conn:
                self.send_json({"error": "Database not found or cannot be opened", "path": self.db_file_path}, 404)
                return
            try:
                cursor = conn.cursor()
                cursor.execute("SELECT * FROM render_runs ORDER BY finished_at_iso DESC LIMIT 100;")
                run_rows = cursor.fetchall()
                
                detailed_runs = []
                for run_row in run_rows:
                    run_id = run_row["run_id"]
                    
                    # Query Frame Info
                    cursor.execute("SELECT * FROM render_frames WHERE run_id = ? ORDER BY frame_number ASC;", (run_id,))
                    frames = [dict(r) for r in cursor.fetchall()]

                    # Query Phase Events Info
                    cursor.execute("SELECT * FROM render_phase_events WHERE run_id = ? ORDER BY duration_ms DESC;", (run_id,))
                    phases = [dict(r) for r in cursor.fetchall()]

                    # Query Counter Info
                    cursor.execute("SELECT * FROM render_counters WHERE run_id = ? ORDER BY counter_name ASC;", (run_id,))
                    counters = [dict(r) for r in cursor.fetchall()]
                    
                    detailed_runs.append({
                        "run": dict(run_row),
                        "frames": frames,
                        "phases": phases,
                        "counters": counters
                    })
                
                self.send_json(detailed_runs)
            except Exception as e:
                self.send_json({"error": str(e)}, 500)
            finally:
                conn.close()
            return

        # API: Get List of Render Runs
        if len(path_parts) >= 2 and path_parts[0] == "api" and path_parts[1] == "runs":
            if len(path_parts) == 2:
                conn = self.get_db_connection()
                if not conn:
                    self.send_json({"error": "Database not found or cannot be opened", "path": self.db_file_path}, 404)
                    return
                try:
                    cursor = conn.cursor()
                    cursor.execute("SELECT * FROM render_runs ORDER BY finished_at_iso DESC;")
                    rows = cursor.fetchall()
                    runs = [dict(r) for r in rows]
                    self.send_json(runs)
                except Exception as e:
                    self.send_json({"error": str(e)}, 500)
                finally:
                    conn.close()
                return

            # API: Get Detailed Render Run (by run_id)
            elif len(path_parts) == 3:
                run_id = path_parts[2]
                conn = self.get_db_connection()
                if not conn:
                    self.send_json({"error": "Database not found"}, 404)
                    return
                try:
                    cursor = conn.cursor()
                    # Query Run Info
                    cursor.execute("SELECT * FROM render_runs WHERE run_id = ?;", (run_id,))
                    run_row = cursor.fetchone()
                    if not run_row:
                        self.send_json({"error": "Run not found"}, 404)
                        return
                    
                    # Query Frame Info
                    cursor.execute("SELECT * FROM render_frames WHERE run_id = ? ORDER BY frame_number ASC;", (run_id,))
                    frames = [dict(r) for r in cursor.fetchall()]

                    # Query Phase Events Info
                    cursor.execute("SELECT * FROM render_phase_events WHERE run_id = ? ORDER BY duration_ms DESC;", (run_id,))
                    phases = [dict(r) for r in cursor.fetchall()]

                    # Query Counter Info
                    cursor.execute("SELECT * FROM render_counters WHERE run_id = ? ORDER BY counter_name ASC;", (run_id,))
                    counters = [dict(r) for r in cursor.fetchall()]

                    self.send_json({
                        "run": dict(run_row),
                        "frames": frames,
                        "phases": phases,
                        "counters": counters
                    })
                except Exception as e:
                    self.send_json({"error": str(e)}, 500)
                finally:
                    conn.close()
                return

        # Serve React Build Static Assets if present
        script_dir = pathlib.Path(__file__).parent.absolute()
        dist_dir = script_dir / "dashboard-ui" / "dist"

        if len(path_parts) >= 2 and path_parts[0] == "assets":
            asset_path = dist_dir / "/".join(path_parts)
            if asset_path.exists() and asset_path.is_file():
                mime_type = "application/octet-stream"
                if asset_path.suffix == ".js":
                    mime_type = "application/javascript"
                elif asset_path.suffix == ".css":
                    mime_type = "text/css"
                elif asset_path.suffix == ".png":
                    mime_type = "image/png"
                elif asset_path.suffix == ".svg":
                    mime_type = "image/svg+xml"
                elif asset_path.suffix == ".ico":
                    mime_type = "image/x-icon"
                
                try:
                    with open(asset_path, "rb") as f:
                        content = f.read()
                    self.send_response(200)
                    self.send_header("Content-Type", mime_type)
                    self.send_header("Access-Control-Allow-Origin", "*")
                    self.end_headers()
                    self.wfile.write(content)
                    return
                except Exception:
                    pass

        # Check if requesting a root static file (e.g. favicon.svg, etc.)
        if len(path_parts) == 1:
            root_file = dist_dir / path_parts[0]
            if root_file.exists() and root_file.is_file():
                mime_type = "application/octet-stream"
                if root_file.suffix == ".svg":
                    mime_type = "image/svg+xml"
                elif root_file.suffix == ".png":
                    mime_type = "image/png"
                
                try:
                    with open(root_file, "rb") as f:
                        content = f.read()
                    self.send_response(200)
                    self.send_header("Content-Type", mime_type)
                    self.send_header("Access-Control-Allow-Origin", "*")
                    self.end_headers()
                    self.wfile.write(content)
                    return
                except Exception:
                    pass

        # Serve SPA Dashboard Frontend
        if parsed_url.path == "/" or parsed_url.path == "/index.html":
            react_index = dist_dir / "index.html"
            if react_index.exists() and react_index.is_file():
                try:
                    with open(react_index, "r", encoding="utf-8") as f:
                        react_html = f.read()
                    self.send_html(react_html)
                    return
                except Exception:
                    pass
            
            # Fallback to embedded dashboard HTML if React app not compiled
            self.send_html(self.get_dashboard_html())
            return

        # Fallback 404
        self.send_response(404)
        self.end_headers()
        self.wfile.write(b"Not Found")

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, X-Tachyon-Token")
        self.end_headers()

    def do_POST(self):
        parsed_url = urllib.parse.urlparse(self.path)
        path_parts = [p for p in parsed_url.path.split("/") if p]

        # API: Ingest Telemetry Payload
        if len(path_parts) == 2 and path_parts[0] == "api" and path_parts[1] == "telemetry":
            # Security token check if environment TACHYON_TELEMETRY_TOKEN is configured
            auth_token = os.environ.get("TACHYON_TELEMETRY_TOKEN")
            if auth_token:
                request_token = self.headers.get("X-Tachyon-Token")
                auth_header = self.headers.get("Authorization", "")
                if auth_header.startswith("Bearer "):
                    request_token = auth_header[7:].strip()
                
                if request_token != auth_token:
                    self.send_json({"error": "Unauthorized: Invalid or missing telemetry token"}, 401)
                    return

            content_length = int(self.headers.get('Content-Length', 0))
            if content_length == 0:
                self.send_json({"error": "Empty payload"}, 400)
                return
            
            try:
                post_data = self.rfile.read(content_length)
                payload = json.loads(post_data.decode('utf-8'))
                
                success, error_msg = self.write_telemetry_to_db(payload)
                if success:
                    self.send_json({"success": True, "run_id": payload.get("run", {}).get("run_id")})
                else:
                    self.send_json({"error": error_msg}, 500)
            except Exception as e:
                self.send_json({"error": str(e)}, 500)
            return

        # Fallback 404
        self.send_response(404)
        self.end_headers()
        self.wfile.write(b"Not Found")

    def write_telemetry_to_db(self, payload):
        conn = None
        try:
            # Connect or dynamically create DB and Schema
            conn = sqlite3.connect(self.db_file_path)
            conn.execute("PRAGMA journal_mode=WAL;")
            conn.execute("PRAGMA synchronous=NORMAL;")
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            
            # Check schema version
            cursor.execute("PRAGMA user_version;")
            current_version = cursor.fetchone()[0]

            if current_version < 1:
                # Dynamically initialize canonical target schema
                cursor.execute("""
                    CREATE TABLE IF NOT EXISTS render_runs (
                        run_id TEXT PRIMARY KEY,
                        job_id TEXT,
                        scene_id TEXT,
                        preset_id TEXT,
                        machine_id TEXT,
                        success INTEGER,
                        error_code TEXT,
                        error_message TEXT,
                        frames_total INTEGER,
                        frames_written INTEGER,
                        wall_time_ms REAL,
                        render_ms REAL,
                        encode_ms REAL,
                        effective_fps REAL,
                        peak_working_set_bytes INTEGER,
                        avg_working_set_bytes INTEGER,
                        peak_private_bytes INTEGER,
                        avg_private_bytes INTEGER,
                        avg_cpu_percent_machine REAL,
                        avg_cpu_cores_used REAL,
                        output_path TEXT,
                        finished_at_iso TEXT
                    );
                """)
                cursor.execute("""
                    CREATE TABLE IF NOT EXISTS render_frames (
                        run_id TEXT,
                        frame_number INTEGER,
                        duration_ms REAL,
                        encode_time_ms REAL,
                        write_time_ms REAL,
                        cache_hit INTEGER,
                        PRIMARY KEY (run_id, frame_number)
                    );
                """)
                cursor.execute("""
                    CREATE TABLE IF NOT EXISTS render_phase_events (
                        run_id TEXT,
                        phase_name TEXT,
                        duration_ms REAL,
                        PRIMARY KEY (run_id, phase_name)
                    );
                """)
                cursor.execute("""
                    CREATE TABLE IF NOT EXISTS render_counters (
                        run_id TEXT,
                        counter_name TEXT,
                        counter_value INTEGER,
                        PRIMARY KEY (run_id, counter_name)
                    );
                """)
                cursor.execute("PRAGMA user_version = 1;")
                current_version = 1

            if current_version < 2:
                # Migrate to version 2 (Trace ID context + HDR Histogram columns)
                cursor.execute("ALTER TABLE render_runs ADD COLUMN trace_id TEXT NOT NULL DEFAULT '';")
                cursor.execute("ALTER TABLE render_runs ADD COLUMN parent_span_id TEXT NOT NULL DEFAULT '';")
                cursor.execute("ALTER TABLE render_runs ADD COLUMN frame_time_hist TEXT NOT NULL DEFAULT '';")
                cursor.execute("PRAGMA user_version = 2;")
            
            if current_version < 3:
                # Migrate to version 3: hardware env, build fingerprint, failure details, time series, preset, KPIs
                columns_to_add = [
                    ("physical_cores", "INTEGER NOT NULL DEFAULT 0"),
                    ("cpu_freq_mhz", "REAL NOT NULL DEFAULT 0.0"),
                    ("gpu_vendor", "TEXT NOT NULL DEFAULT ''"),
                    ("gpu_driver", "TEXT NOT NULL DEFAULT ''"),
                    ("total_ram_bytes", "INTEGER NOT NULL DEFAULT 0"),
                    ("total_vram_bytes", "INTEGER NOT NULL DEFAULT 0"),
                    ("git_commit_short", "TEXT NOT NULL DEFAULT ''"),
                    ("build_type", "TEXT NOT NULL DEFAULT ''"),
                    ("compiler_info", "TEXT NOT NULL DEFAULT ''"),
                    ("exit_code", "INTEGER NOT NULL DEFAULT 0"),
                    ("error_category", "TEXT NOT NULL DEFAULT ''"),
                    ("total_pixels_processed", "INTEGER NOT NULL DEFAULT 0"),
                    ("total_tiles", "INTEGER NOT NULL DEFAULT 0"),
                    ("memory_samples", "TEXT NOT NULL DEFAULT ''"),
                    ("cpu_util_samples", "TEXT NOT NULL DEFAULT ''"),
                    ("gpu_util_samples", "TEXT NOT NULL DEFAULT ''"),
                    ("preset_json", "TEXT NOT NULL DEFAULT ''"),
                    ("time_to_first_frame_ms", "REAL NOT NULL DEFAULT 0.0"),
                    ("ffmpeg_queue_depth", "INTEGER NOT NULL DEFAULT 0"),
                    ("hostname", "TEXT NOT NULL DEFAULT ''"),
                    ("os", "TEXT NOT NULL DEFAULT ''"),
                    ("cpu_model", "TEXT NOT NULL DEFAULT ''"),
                    ("logical_cores", "INTEGER NOT NULL DEFAULT 0"),
                    ("tachyon_version", "TEXT NOT NULL DEFAULT ''")
                ]
                for col_name, col_type in columns_to_add:
                    try:
                        cursor.execute(f"ALTER TABLE render_runs ADD COLUMN {col_name} {col_type};")
                    except sqlite3.OperationalError:
                        pass # Ignore if duplicate
                cursor.execute("PRAGMA user_version = 3;")

            cursor.execute("BEGIN TRANSACTION;")

            # 1. Insert Render Run
            run = payload.get("run", {})
            if not run or "run_id" not in run:
                return False, "Missing run info or run_id"
            
            run_keys = list(run.keys())
            placeholders = ", ".join(["?"] * len(run_keys))
            columns = ", ".join(run_keys)
            values = [run[k] for k in run_keys]
            cursor.execute(f"INSERT OR IGNORE INTO render_runs ({columns}) VALUES ({placeholders});", values)
            
            # 2. Insert Render Frames
            frames = payload.get("frames", [])
            for frame in frames:
                frame_keys = list(frame.keys())
                f_placeholders = ", ".join(["?"] * len(frame_keys))
                f_columns = ", ".join(frame_keys)
                f_values = [frame[k] for k in frame_keys]
                cursor.execute(f"INSERT OR IGNORE INTO render_frames ({f_columns}) VALUES ({f_placeholders});", f_values)
                
            # 3. Insert Phase Events
            phases = payload.get("phases", [])
            for phase in phases:
                phase_keys = list(phase.keys())
                p_placeholders = ", ".join(["?"] * len(phase_keys))
                p_columns = ", ".join(phase_keys)
                p_values = [phase[k] for k in phase_keys]
                cursor.execute(f"INSERT OR IGNORE INTO render_phase_events ({p_columns}) VALUES ({p_placeholders});", p_values)
                
            # 4. Insert Counters
            counters = payload.get("counters", [])
            for counter in counters:
                counter_keys = list(counter.keys())
                c_placeholders = ", ".join(["?"] * len(counter_keys))
                c_columns = ", ".join(counter_keys)
                c_values = [counter[k] for k in counter_keys]
                cursor.execute(f"INSERT OR IGNORE INTO render_counters ({c_columns}) VALUES ({c_placeholders});", c_values)
                
            conn.commit()
            return True, ""
        except Exception as e:
            if conn:
                try:
                    conn.rollback()
                except Exception:
                    pass
            return False, str(e)
        finally:
            if conn:
                conn.close()

    def get_dashboard_html(self):
        return r"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tachyon Engine — Telemetry Fallback</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Outfit', sans-serif;
            background-color: #080c14;
            color: #f8fafc;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background-image: radial-gradient(at 50% 50%, rgba(6, 182, 212, 0.15) 0px, transparent 50%);
        }
        .container {
            background: rgba(13, 20, 35, 0.75);
            border: 1px solid rgba(255, 255, 255, 0.08);
            backdrop-filter: blur(12px);
            padding: 3rem;
            border-radius: 16px;
            text-align: center;
            max-width: 500px;
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5);
        }
        h1 {
            background: linear-gradient(135deg, #06b6d4 0%, #a855f7 100%);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 1rem;
            font-size: 2rem;
        }
        p {
            color: #94a3b8;
            margin-bottom: 2rem;
            line-height: 1.6;
        }
        .code-box {
            background: rgba(0, 0, 0, 0.4);
            border: 1px solid rgba(255, 255, 255, 0.05);
            padding: 1rem;
            border-radius: 8px;
            font-family: monospace;
            font-size: 0.9rem;
            color: #06b6d4;
            text-align: left;
            margin-bottom: 2rem;
            line-height: 1.5;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Tachyon Telemetry Fallback</h1>
        <p>The compiled React Single-Page Application assets were not found in the <code>dashboard-ui/dist</code> directory.</p>
        <p>To run the high-performance React dashboard, please compile it using the following commands:</p>
        <div class="code-box">
            cd tools/telemetry/dashboard-ui<br>
            npm install<br>
            npm run build
        </div>
        <p style="font-size: 0.85rem; color: #64748b;">The REST APIs remain fully online and active at <code>/api/runs</code>.</p>
    </div>
</body>
</html>
"""

def run_server(port=DEFAULT_PORT):
    # Ensure standard socket binding works nicely and permits fast re-binding
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", port), TelemetryAPIHandler) as httpd:
        print(f"\n=======================================================")
        print(f"🚀  Tachyon Telemetry Web Server Dashboard is ONLINE!")
        print(f"🔗  URL: http://localhost:{port}/")
        print(f"📂  Reading SQLite DB: {TelemetryAPIHandler.db_file_path}")
        print(f"=======================================================\n")
        print("Press Ctrl+C to terminate...")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down Telemetry Dashboard server gracefully. Goodbye!")
            sys.exit(0)

if __name__ == "__main__":
    port = int(os.environ.get("PORT", DEFAULT_PORT))
    if len(sys.argv) > 1:
        custom_path = sys.argv[1]
        TelemetryAPIHandler.db_file_path = str(pathlib.Path(custom_path).absolute())
    run_server(port)
