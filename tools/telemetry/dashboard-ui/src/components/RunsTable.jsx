import React from 'react';
import { Search, ListChecks } from 'lucide-react';

export default function RunsTable({ runs, selectedRunId, onSelectRun, searchQuery, onSearchChange }) {
  const formatFinishedTime = (isoString) => {
    if (!isoString) return 'N/A';
    try {
      const date = new Date(isoString);
      return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    } catch {
      return 'N/A';
    }
  };

  const filteredRuns = runs.filter(r => {
    const query = searchQuery.toLowerCase().trim();
    if (!query) return true;
    const scene = (r.scene_id || '').toLowerCase();
    const preset = (r.preset_id || '').toLowerCase();
    const runId = r.run_id.toLowerCase();
    const job = (r.job_id || '').toLowerCase();
    return scene.includes(query) || preset.includes(query) || runId.includes(query) || job.includes(query);
  });

  return (
    <div className="content-panel">
      <div className="panel-header">
        <span className="panel-title">
          <ListChecks size={20} style={{ color: 'var(--accent-cyan)' }} />
          Render Sessions History
        </span>
        <div className="controls-row">
          <div style={{ position: 'relative', display: 'flex', alignItems: 'center' }}>
            <Search size={16} style={{ position: 'absolute', left: '10px', color: 'var(--text-secondary)' }} />
            <input
              type="text"
              className="search-input"
              style={{ paddingLeft: '2.2rem' }}
              value={searchQuery}
              onChange={(e) => onSearchChange(e.target.value)}
              placeholder="Filter by scene, preset, job..."
            />
          </div>
        </div>
      </div>

      <div className="table-container">
        <table>
          <thead>
            <tr>
              <th>Status</th>
              <th>Run / Job ID</th>
              <th>Scene</th>
              <th>Preset</th>
              <th>Frames</th>
              <th>Wall Time</th>
              <th>Finished (Local)</th>
            </tr>
          </thead>
          <tbody>
            {filteredRuns.length === 0 ? (
              <tr>
                <td colSpan="7" style={{ textAlign: 'center', color: 'var(--text-secondary)' }}>
                  No matching render session records found.
                </td>
              </tr>
            ) : (
              filteredRuns.map((run) => {
                const isActive = run.run_id === selectedRunId;
                return (
                  <tr
                    key={run.run_id}
                    className={`clickable ${isActive ? 'active' : ''}`}
                    onClick={() => onSelectRun(run.run_id)}
                  >
                    <td>
                      {run.success === 1 ? (
                        <span className="badge success">Success</span>
                      ) : (
                        <span className="badge failed">Failed</span>
                      )}
                    </td>
                    <td style={{ fontFamily: 'monospace', fontSize: '0.8rem', fontWeight: 600 }}>
                      {run.run_id.substring(0, 8)}...
                    </td>
                    <td style={{ fontWeight: 500 }}>{run.scene_id || 'N/A'}</td>
                    <td>
                      <span className="badge preset">{run.preset_id || 'N/A'}</span>
                    </td>
                    <td>
                      {run.frames_written} / {run.frames_total}
                    </td>
                    <td>{(run.wall_time_ms / 1000).toFixed(2)}s</td>
                    <td style={{ color: 'var(--text-secondary)', fontSize: '0.85rem' }}>
                      {formatFinishedTime(run.finished_at_iso)}
                    </td>
                  </tr>
                );
              })
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
}
