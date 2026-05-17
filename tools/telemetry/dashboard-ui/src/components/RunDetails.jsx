import React from 'react';
import { Line, Doughnut } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  ArcElement
} from 'chart.js';
import { BarChart2, Info, Zap, Monitor } from 'lucide-react';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  ArcElement
);

export default function RunDetails({ details, loading }) {
  if (loading) {
    return (
      <div className="content-panel">
        <div className="panel-header">
          <span className="panel-title">
            <BarChart2 size={20} style={{ color: 'var(--accent-cyan)' }} />
            Session Diagnostics
          </span>
        </div>
        <div className="spinner-container">
          <div className="spinner"></div>
          <span style={{ marginTop: '1rem', fontSize: '0.9rem', color: 'var(--text-secondary)' }}>
            Fetching full session details...
          </span>
        </div>
      </div>
    );
  }

  if (!details) {
    return (
      <div className="content-panel">
        <div className="panel-header">
          <span className="panel-title">
            <BarChart2 size={20} style={{ color: 'var(--accent-cyan)' }} />
            Session Diagnostics
          </span>
        </div>
        <div className="detail-placeholder">
          <BarChart2 />
          <span>Select a render session row to view high-performance graphs</span>
        </div>
      </div>
    );
  }

  const { run, frames, phases, counters } = details;

  // Mini metrics grid
  const totalSec = (run.wall_time_ms / 1000).toFixed(2) + 's';
  const renderSec = (run.render_ms / 1000).toFixed(2) + 's';
  const encodeSec = (run.encode_ms / 1000).toFixed(2) + 's';
  const fpsText = run.effective_fps ? run.effective_fps.toFixed(2) : '0.00';

  // 1. Line Chart: Frames Details
  const frameLabels = (frames || []).map(f => `F${f.frame_number}`);
  const renderDurations = (frames || []).map(f => f.duration_ms);
  const encodeDurations = (frames || []).map(f => f.encode_time_ms);

  const lineData = {
    labels: frameLabels,
    datasets: [
      {
        label: 'Render Duration',
        data: renderDurations,
        borderColor: '#06b6d4',
        backgroundColor: 'rgba(6, 182, 212, 0.05)',
        fill: true,
        tension: 0.1,
        borderWidth: 2
      },
      {
        label: 'Encode Time',
        data: encodeDurations,
        borderColor: '#a855f7',
        borderWidth: 1.5,
        borderDash: [5, 5],
        fill: false,
        tension: 0.1
      }
    ]
  };

  const lineOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        labels: { color: '#f8fafc', font: { family: 'Outfit' } }
      }
    },
    scales: {
      x: { grid: { color: 'rgba(255,255,255,0.04)' }, ticks: { color: '#94a3b8', font: { family: 'Outfit' } } },
      y: { grid: { color: 'rgba(255,255,255,0.04)' }, ticks: { color: '#94a3b8', font: { family: 'Outfit' } } }
    }
  };

  // 2. Doughnut Chart: Phases Details
  const phaseLabels = (phases || []).map(p => p.phase_name);
  const phaseDurations = (phases || []).map(p => p.duration_ms);

  const doughnutData = {
    labels: phaseLabels,
    datasets: [
      {
        data: phaseDurations,
        backgroundColor: [
          'rgba(6, 182, 212, 0.75)',
          'rgba(168, 85, 247, 0.75)',
          'rgba(245, 158, 11, 0.75)',
          'rgba(16, 185, 129, 0.75)',
          'rgba(239, 68, 68, 0.75)'
        ],
        borderColor: '#080c14',
        borderWidth: 2
      }
    ]
  };

  const doughnutOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'right',
        labels: { color: '#f8fafc', font: { family: 'Outfit', size: 11 } }
      }
    }
  };

  return (
    <div className="content-panel">
      <div className="panel-header">
        <span className="panel-title" style={{ gap: '0.4rem' }}>
          <Info size={20} style={{ color: 'var(--accent-cyan)' }} />
          Diagnostics for Run: {run.run_id.substring(0, 8)}
        </span>
      </div>

      <div className="detail-pane">
        {/* Mini Grid */}
        <div className="metrics-grid-2x2">
          <div className="metric-mini-card">
            <span className="mini-label">Wall Time</span>
            <span className="mini-val" style={{ color: 'var(--accent-cyan)' }}>{totalSec}</span>
          </div>
          <div className="metric-mini-card">
            <span className="mini-label">Pipeline Render</span>
            <span className="mini-val" style={{ color: 'var(--accent-purple)' }}>{renderSec}</span>
          </div>
          <div className="metric-mini-card">
            <span className="mini-label">FFmpeg Encoding</span>
            <span className="mini-val" style={{ color: 'var(--accent-amber)' }}>{encodeSec}</span>
          </div>
          <div className="metric-mini-card">
            <span className="mini-label">Effective FPS</span>
            <span className="mini-val" style={{ color: 'var(--accent-green)' }}>{fpsText}</span>
          </div>
        </div>

        {/* Frame Timeline Line Chart */}
        <div className="chart-box">
          <div className="chart-title">
            <Zap size={14} style={{ color: 'var(--accent-cyan)' }} />
            Frame-by-Frame Duration Spikes (ms)
          </div>
          <div style={{ height: '200px', position: 'relative' }}>
            <Line data={lineData} options={lineOptions} />
          </div>
        </div>

        {/* Engine Phases Latency Doughnut Chart */}
        <div className="chart-box">
          <div className="chart-title">
            <BarChart2 size={14} style={{ color: 'var(--accent-purple)' }} />
            Engine Phase Latencies
          </div>
          <div style={{ height: '200px', position: 'relative' }}>
            <Doughnut data={doughnutData} options={doughnutOptions} />
          </div>
        </div>

        {/* Engine Optimization Counters */}
        <div className="chart-box" style={{ minHeight: 'auto' }}>
          <div className="chart-title">
            <Zap size={14} style={{ color: 'var(--accent-amber)' }} />
            Engine Optimization Counters
          </div>
          <div className="meta-list">
            {!counters || counters.length === 0 ? (
              <span style={{ color: 'var(--text-secondary)', fontStyle: 'italic' }}>
                No counters recorded for this run.
              </span>
            ) : (
              counters.map((c) => (
                <div className="meta-item" key={c.counter_name}>
                  <span className="meta-name" style={{ color: 'var(--accent-cyan)' }}>
                    {c.counter_name}
                  </span>
                  <span className="meta-value">
                    {c.counter_value.toLocaleString()}
                  </span>
                </div>
              ))
            )}
          </div>
        </div>

        {/* Runtime Environment Specs */}
        <div className="chart-box" style={{ minHeight: 'auto' }}>
          <div className="chart-title">
            <Monitor size={14} style={{ color: 'var(--accent-green)' }} />
            Target OS & Environment
          </div>
          <div className="meta-list">
            <div className="meta-item">
              <span className="meta-name">Host Name</span>
              <span className="meta-value">{run.hostname || 'N/A'}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">Operating System</span>
              <span className="meta-value">{run.os || 'Linux'}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">CPU Architecture</span>
              <span className="meta-value">
                {run.cpu_model || 'N/A'} ({run.logical_cores || 0} cores)
              </span>
            </div>
            <div className="meta-item">
              <span className="meta-name">Tachyon Engine</span>
              <span className="meta-value">v{run.tachyon_version || '0.1.0'}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
