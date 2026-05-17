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

  // Logarithmic High-Dynamic-Range Latency Histogram Processor
  const parseHistogram = (histStr) => {
    if (!histStr) return null;
    const buckets = histStr.split(',').map(Number);
    if (buckets.length !== 128) return null;
    
    const total = buckets.reduce((a, b) => a + b, 0);
    if (total === 0) return null;

    const getValueAtPercentile = (p) => {
      const target = Math.ceil(total * p);
      let runningSum = 0;
      for (let i = 0; i < 128; i++) {
        runningSum += buckets[i];
        if (runningSum >= target) {
          const logVal = i * (24.0 / 128.0);
          const us = Math.pow(2.0, logVal);
          return us / 1000.0;
        }
      }
      return 0;
    };

    const formatLatency = (ms) => {
      if (ms < 1.0) {
        return (ms * 1000).toFixed(0) + ' µs';
      } else if (ms < 1000.0) {
        return ms.toFixed(2) + ' ms';
      } else {
        return (ms / 1000.0).toFixed(2) + ' s';
      }
    };

    return {
      buckets,
      total,
      p50: formatLatency(getValueAtPercentile(0.50)),
      p90: formatLatency(getValueAtPercentile(0.90)),
      p95: formatLatency(getValueAtPercentile(0.95)),
      p99: formatLatency(getValueAtPercentile(0.99))
    };
  };

  const histData = parseHistogram(run.frame_time_hist);

  // Peak-preserving frame downsampling to keep Chart.js running beautifully
  const downsampleFrames = (rawFrames, maxPoints = 200) => {
    if (!rawFrames || rawFrames.length <= maxPoints) {
      return {
        labels: (rawFrames || []).map(f => `F${f.frame_number}`),
        renderDurations: (rawFrames || []).map(f => f.duration_ms),
        encodeDurations: (rawFrames || []).map(f => f.encode_time_ms),
        isDownsampled: false
      };
    }

    const step = rawFrames.length / maxPoints;
    const labels = [];
    const renderDurations = [];
    const encodeDurations = [];

    for (let i = 0; i < maxPoints; i++) {
      const start = Math.floor(i * step);
      const end = Math.min(Math.floor((i + 1) * step), rawFrames.length);
      if (start >= end) continue;

      // Select the maximum peak frame in the bucket to preserve latency spikes
      let peakIdx = start;
      let maxVal = rawFrames[start].duration_ms;
      for (let j = start + 1; j < end; j++) {
        if (rawFrames[j].duration_ms > maxVal) {
          maxVal = rawFrames[j].duration_ms;
          peakIdx = j;
        }
      }

      const peakFrame = rawFrames[peakIdx];
      labels.push(`F${peakFrame.frame_number}`);
      renderDurations.push(peakFrame.duration_ms);
      encodeDurations.push(peakFrame.encode_time_ms);
    }

    return { labels, renderDurations, encodeDurations, isDownsampled: true };
  };

  const { labels: frameLabels, renderDurations, encodeDurations, isDownsampled } = downsampleFrames(frames);

  // Mini metrics grid
  const totalSec = (run.wall_time_ms / 1000).toFixed(2) + 's';
  const renderSec = (run.render_ms / 1000).toFixed(2) + 's';
  const encodeSec = (run.encode_ms / 1000).toFixed(2) + 's';
  const fpsText = run.effective_fps ? run.effective_fps.toFixed(2) : '0.00';

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
        <div className="metrics-grid-2x2" style={{ gridTemplateColumns: 'repeat(3, 1fr)' }}>
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
          {run.time_to_first_frame_ms > 0 && (
            <div className="metric-mini-card">
              <span className="mini-label">Time to First Frame</span>
              <span className="mini-val" style={{ color: 'var(--accent-purple)' }}>{run.time_to_first_frame_ms.toFixed(1)}ms</span>
            </div>
          )}
          {run.ffmpeg_queue_depth > 0 && (
            <div className="metric-mini-card">
              <span className="mini-label">FFmpeg Queue</span>
              <span className="mini-val" style={{ color: 'var(--accent-amber)' }}>{run.ffmpeg_queue_depth}</span>
            </div>
          )}
        </div>

        {/* Frame Timeline Line Chart */}
        <div className="chart-box span-2">
          <div className="chart-title" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', width: '100%' }}>
            <span style={{ display: 'flex', alignItems: 'center', gap: '0.3rem' }}>
              <Zap size={14} style={{ color: 'var(--accent-cyan)' }} />
              Frame-by-Frame Duration Spikes (ms)
            </span>
            {isDownsampled && (
              <span style={{ fontSize: '0.7rem', color: 'var(--accent-amber)', opacity: 0.8, fontWeight: 'normal' }}>
                [Peak-Preserved Downsampling Active]
              </span>
            )}
          </div>
          <div style={{ height: '200px', position: 'relative' }}>
            <Line data={lineData} options={lineOptions} />
          </div>
        </div>

        {/* Logarithmic HDR Latency Distribution & Percentiles */}
        {histData && (
          <div className="chart-box span-2" style={{ minHeight: 'auto' }}>
            <div className="chart-title">
              <Zap size={14} style={{ color: 'var(--accent-cyan)' }} />
              High-Dynamic-Range Latency Distribution
            </div>
            
            <div style={{ position: 'relative', height: '65px', margin: '0.8rem 0', background: 'rgba(0,0,0,0.2)', borderRadius: '8px', padding: '4px', border: '1px solid rgba(255,255,255,0.03)', overflow: 'hidden' }}>
              <svg viewBox="0 0 380 60" width="100%" height="100%" preserveAspectRatio="none">
                <defs>
                  <linearGradient id="histGradient" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="0%" stopColor="rgba(6, 182, 212, 0.4)" />
                    <stop offset="100%" stopColor="rgba(168, 85, 247, 0.0)" />
                  </linearGradient>
                </defs>
                <path
                  d={`M 0,60 L ${histData.buckets.map((count, index) => {
                    const maxVal = Math.max(...histData.buckets, 1);
                    const x = (index / 127) * 380;
                    const y = 60 - (count / maxVal) * 55;
                    return `${x},${y}`;
                  }).join(' L ')} L 380,60 Z`}
                  fill="url(#histGradient)"
                  stroke="var(--accent-cyan)"
                  strokeWidth="1.5"
                />
              </svg>
              <div style={{ position: 'absolute', left: '6px', bottom: '4px', fontSize: '0.65rem', color: 'var(--text-secondary)' }}>1 µs</div>
              <div style={{ position: 'absolute', right: '6px', bottom: '4px', fontSize: '0.65rem', color: 'var(--text-secondary)' }}>10 s</div>
            </div>

            <div className="metrics-grid-2x2" style={{ marginTop: '0.8rem', gridTemplateColumns: 'repeat(4, 1fr)', gap: '0.4rem' }}>
              <div className="metric-mini-card" style={{ padding: '0.4rem' }}>
                <span className="mini-label" style={{ fontSize: '0.7rem' }}>p50 (Median)</span>
                <span className="mini-val" style={{ fontSize: '0.9rem', color: 'var(--accent-green)' }}>{histData.p50}</span>
              </div>
              <div className="metric-mini-card" style={{ padding: '0.4rem' }}>
                <span className="mini-label" style={{ fontSize: '0.7rem' }}>p90</span>
                <span className="mini-val" style={{ fontSize: '0.9rem', color: 'var(--accent-cyan)' }}>{histData.p90}</span>
              </div>
              <div className="metric-mini-card" style={{ padding: '0.4rem' }}>
                <span className="mini-label" style={{ fontSize: '0.7rem' }}>p95</span>
                <span className="mini-val" style={{ fontSize: '0.9rem', color: 'var(--accent-purple)' }}>{histData.p95}</span>
              </div>
              <div className="metric-mini-card" style={{ padding: '0.4rem' }}>
                <span className="mini-label" style={{ fontSize: '0.7rem' }}>p99 (Tail)</span>
                <span className="mini-val" style={{ fontSize: '0.9rem', color: 'var(--accent-amber)' }}>{histData.p99}</span>
              </div>
            </div>
          </div>
        )}

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

        {/* Distributed Tracing span metadata */}
        {run.trace_id && (
          <div className="chart-box" style={{ minHeight: 'auto' }}>
            <div className="chart-title">
              <Zap size={14} style={{ color: 'var(--accent-cyan)' }} />
              Distributed Tracing Context
            </div>
            <div className="meta-list">
              <div className="meta-item">
                <span className="meta-name">Trace ID</span>
                <span className="meta-value" style={{ fontFamily: 'monospace', color: 'var(--accent-cyan)', fontSize: '0.8rem' }}>
                  {run.trace_id}
                </span>
              </div>
              <div className="meta-item">
                <span className="meta-name">Parent Span ID</span>
                <span className="meta-value" style={{ fontFamily: 'monospace', color: 'var(--text-secondary)', fontSize: '0.8rem' }}>
                  {run.parent_span_id || 'root'}
                </span>
              </div>
            </div>
          </div>
        )}

        {/* Hardware Environment (item 1) */}
        <div className="chart-box" style={{ minHeight: 'auto' }}>
          <div className="chart-title">
            <Monitor size={14} style={{ color: 'var(--accent-green)' }} />
            Hardware Environment
          </div>
          <div className="meta-list">
            <div className="meta-item">
              <span className="meta-name">CPU</span>
              <span className="meta-value">{run.cpu_model || 'N/A'}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">Cores</span>
              <span className="meta-value">{run.physical_cores || '?'}P / {run.logical_cores || '?'}L @ {run.cpu_freq_mhz ? run.cpu_freq_mhz.toFixed(0) + ' MHz' : 'N/A'}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">GPU</span>
              <span className="meta-value">{run.gpu_vendor || 'N/A'} {run.gpu_driver ? '(' + run.gpu_driver + ')' : ''}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">RAM / VRAM</span>
              <span className="meta-value">{run.total_ram_bytes ? (run.total_ram_bytes / 1073741824).toFixed(1) + ' GB' : 'N/A'} / {run.total_vram_bytes ? (run.total_vram_bytes / 1073741824).toFixed(1) + ' GB' : 'N/A'}</span>
            </div>
          </div>
        </div>

        {/* Build Fingerprint (item 2) */}
        <div className="chart-box" style={{ minHeight: 'auto' }}>
          <div className="chart-title">
            <Zap size={14} style={{ color: 'var(--accent-purple)' }} />
            Build Fingerprint
          </div>
          <div className="meta-list">
            <div className="meta-item">
              <span className="meta-name">Git Commit</span>
              <span className="meta-value" style={{ fontFamily: 'monospace', color: 'var(--accent-cyan)', fontSize: '0.8rem' }}>{run.git_commit_short || 'N/A'}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">Build Type</span>
              <span className="meta-value">{run.build_type || 'N/A'}</span>
            </div>
            <div className="meta-item">
              <span className="meta-name">Compiler</span>
              <span className="meta-value">{run.compiler_info || 'N/A'}</span>
            </div>
          </div>
        </div>

        {/* Failure Analysis (item 3) — highlighted when failed */}
        {run.success === 0 && (
          <div className="chart-box" style={{ minHeight: 'auto', borderColor: 'rgba(239, 68, 68, 0.4)', background: 'rgba(239, 68, 68, 0.05)' }}>
            <div className="chart-title" style={{ color: 'var(--accent-red)' }}>
              <Zap size={14} />
              ⚠ Failure Analysis
            </div>
            <div className="meta-list">
              <div className="meta-item">
                <span className="meta-name">Exit Code</span>
                <span className="meta-value" style={{ color: 'var(--accent-red)' }}>{run.exit_code ?? 'N/A'}</span>
              </div>
              <div className="meta-item">
                <span className="meta-name">Error Category</span>
                <span className="meta-value" style={{ color: 'var(--accent-amber)' }}>{run.error_category || 'unknown'}</span>
              </div>
              <div className="meta-item">
                <span className="meta-name">Error Message</span>
                <span className="meta-value" style={{ fontSize: '0.75rem', color: 'var(--accent-red)', wordBreak: 'break-word', maxWidth: '250px', textAlign: 'right' }}>{run.error_message || 'N/A'}</span>
              </div>
            </div>
          </div>
        )}

        {/* Memory Timeline (item 5) */}
        {run.memory_samples && (
          <div className="chart-box span-2" style={{ minHeight: 'auto' }}>
            <div className="chart-title">
              <Zap size={14} style={{ color: 'var(--accent-amber)' }} />
              Memory Timeline (RSS MB)
            </div>
            {(() => {
              const samples = run.memory_samples.split(',').map(Number).filter(v => !isNaN(v));
              if (samples.length < 2) return <span style={{ color: 'var(--text-secondary)' }}>Insufficient samples</span>;
              const labels = samples.map((_, i) => `${(i * 0.1).toFixed(1)}s`);
              const memData = {
                labels,
                datasets: [{
                  label: 'RSS MB',
                  data: samples,
                  borderColor: '#f59e0b',
                  backgroundColor: 'rgba(245, 158, 11, 0.05)',
                  fill: true,
                  tension: 0.3,
                  borderWidth: 2
                }]
              };
              const memOptions = {
                responsive: true,
                maintainAspectRatio: false,
                plugins: { legend: { labels: { color: '#f8fafc', font: { family: 'Outfit' } } } },
                scales: {
                  x: { grid: { color: 'rgba(255,255,255,0.04)' }, ticks: { color: '#94a3b8', font: { family: 'Outfit' } } },
                  y: { grid: { color: 'rgba(255,255,255,0.04)' }, ticks: { color: '#94a3b8', font: { family: 'Outfit' } } }
                }
              };
              return <div style={{ height: '150px', position: 'relative' }}><Line data={memData} options={memOptions} /></div>;
            })()}
          </div>
        )}

        {/* Preset Details (item 6) */}
        {run.preset_json && (
          <div className="chart-box" style={{ minHeight: 'auto' }}>
            <div className="chart-title">
              <Monitor size={14} style={{ color: 'var(--accent-cyan)' }} />
              Preset Configuration
            </div>
            <div className="meta-list">
              {(() => {
                try {
                  const preset = JSON.parse(run.preset_json);
                  return Object.entries(preset).map(([k, v]) => (
                    <div className="meta-item" key={k}>
                      <span className="meta-name">{k}</span>
                      <span className="meta-value">{String(v)}</span>
                    </div>
                  ));
                } catch {
                  return <span className="meta-value">{run.preset_json}</span>;
                }
              })()}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
