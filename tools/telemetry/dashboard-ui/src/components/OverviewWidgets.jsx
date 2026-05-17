import React from 'react';

export default function OverviewWidgets({ runs }) {
  const formatBytes = (bytes) => {
    if (bytes === 0 || !bytes) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  if (!runs || runs.length === 0) {
    return (
      <div className="widgets-grid">
        <div className="widget-card runs">
          <span className="widget-label">Total Renders</span>
          <span className="widget-value">0</span>
        </div>
        <div className="widget-card rate">
          <span className="widget-label">Success Rate</span>
          <span className="widget-value">0%</span>
        </div>
        <div className="widget-card fps">
          <span className="widget-label">Avg Engine FPS</span>
          <span className="widget-value">0.0</span>
        </div>
        <div className="widget-card memory">
          <span className="widget-label">Peak Memory (RAM)</span>
          <span className="widget-value">0 B</span>
        </div>
      </div>
    );
  }

  const total = runs.length;
  const successRuns = runs.filter(r => r.success === 1).length;
  const rate = ((successRuns / total) * 100).toFixed(0);

  let validFpsCount = 0;
  let fpsSum = 0;
  let maxMemory = 0;

  runs.forEach(r => {
    if (r.effective_fps > 0) {
      fpsSum += r.effective_fps;
      validFpsCount++;
    }
    if (r.peak_working_set_bytes > maxMemory) {
      maxMemory = r.peak_working_set_bytes;
    }
  });

  const avgFps = validFpsCount > 0 ? (fpsSum / validFpsCount).toFixed(2) : '0.0';

  return (
    <div className="widgets-grid">
      <div className="widget-card runs">
        <span className="widget-label">Total Renders</span>
        <span className="widget-value">{total}</span>
      </div>
      <div className="widget-card rate">
        <span className="widget-label">Success Rate</span>
        <span className="widget-value">{rate}%</span>
      </div>
      <div className="widget-card fps">
        <span className="widget-label">Avg Engine FPS</span>
        <span className="widget-value">{avgFps}</span>
      </div>
      <div className="widget-card memory">
        <span className="widget-label">Peak Memory (RAM)</span>
        <span className="widget-value">{formatBytes(maxMemory)}</span>
      </div>
    </div>
  );
}
