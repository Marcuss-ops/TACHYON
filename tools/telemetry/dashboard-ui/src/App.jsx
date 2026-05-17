import React, { useState, useEffect, useRef } from 'react';
import OverviewWidgets from './components/OverviewWidgets';
import RunsTable from './components/RunsTable';
import RunDetails from './components/RunDetails';
import { Database, Download } from 'lucide-react';

export default function App() {
  const [runs, setRuns] = useState([]);
  const [selectedRunId, setSelectedRunId] = useState('');
  const [selectedRunDetails, setSelectedRunDetails] = useState(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [loadingRuns, setLoadingRuns] = useState(true);
  const [loadingDetails, setLoadingDetails] = useState(false);
  const [dbOnline, setDbOnline] = useState(false);
  const [dbPath, setDbPath] = useState('Resolving DB...');

  // Use a ref to prevent closure staleness in setInterval
  const selectedRunIdRef = useRef(selectedRunId);
  useEffect(() => {
    selectedRunIdRef.current = selectedRunId;
  }, [selectedRunId]);

  const getApiUrl = (path) => {
    if (window.location.port === '5173') {
      return `http://localhost:8080${path}`;
    }
    return path;
  };

  const loadAllRuns = async (isSilent = false) => {
    if (!isSilent) setLoadingRuns(true);
    try {
      const response = await fetch(getApiUrl('/api/runs'));
      if (!response.ok) {
        const err = await response.json();
        setDbOnline(false);
        setDbPath(err.path || 'Error');
        setRuns([]);
        return;
      }

      const data = await response.json();
      setRuns(data);
      setDbOnline(true);
      setDbPath('Connected: tachyon_render_history.sqlite');
      
      // Auto-select first run if none selected
      if (data.length > 0 && !selectedRunIdRef.current) {
        handleSelectRun(data[0].run_id, isSilent);
      }
    } catch (e) {
      setDbOnline(false);
      setDbPath('Offline / Failed to connect');
      setRuns([]);
    } finally {
      if (!isSilent) setLoadingRuns(false);
    }
  };

  const handleSelectRun = async (runId, isSilent = false) => {
    setSelectedRunId(runId);
    if (!isSilent) setLoadingDetails(true);
    try {
      const res = await fetch(getApiUrl(`/api/runs/${runId}`));
      if (!res.ok) throw new Error('Failed to fetch detailed metrics');
      const data = await res.json();
      setSelectedRunDetails(data);
    } catch (err) {
      console.error(err);
      setSelectedRunDetails(null);
    } finally {
      if (!isSilent) setLoadingDetails(false);
    }
  };

  const handleDownloadLast100 = async () => {
    try {
      const response = await fetch(getApiUrl('/api/export-last-100'));
      if (!response.ok) throw new Error('Failed to export telemetry data');
      
      const data = await response.json();
      const jsonString = JSON.stringify(data, null, 2);
      const blob = new Blob([jsonString], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      
      const link = document.createElement('a');
      link.href = url;
      link.download = `tachyon_detailed_telemetry_${new Date().toISOString().slice(0, 10)}.json`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
    } catch (err) {
      console.error(err);
      alert('Error exporting telemetry data: ' + err.message);
    }
  };

  useEffect(() => {
    // Initial fetch with spinner
    loadAllRuns();

    // Auto-sync everything silently every 3 seconds
    const interval = setInterval(async () => {
      // 1. Silent sync of all runs
      await loadAllRuns(true);
      
      // 2. Silent sync of current selected run details
      if (selectedRunIdRef.current) {
        try {
          const res = await fetch(getApiUrl(`/api/runs/${selectedRunIdRef.current}`));
          if (res.ok) {
            const data = await res.json();
            setSelectedRunDetails(data);
          }
        } catch (err) {
          console.error('Auto-sync run details failure:', err);
        }
      }
    }, 3000);

    return () => clearInterval(interval);
  }, []);

  return (
    <div className="dashboard-container">
      <header>
        <div className="logo-section">
          <h1>Tachyon</h1>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
          <button 
            className="download-btn"
            onClick={handleDownloadLast100}
            disabled={runs.length === 0}
            style={{ opacity: runs.length === 0 ? 0.5 : 1 }}
            title="Download latest 100 render runs with complete details"
          >
            <Download size={14} style={{ color: 'var(--accent-cyan)' }} />
            Export Last 100 Renders
          </button>
          <div className="db-info">
            <span className={`db-dot ${dbOnline ? '' : 'offline'}`}></span>
            <Database size={14} style={{ color: dbOnline ? 'var(--accent-green)' : 'var(--accent-red)' }} />
            <span>{dbPath}</span>
          </div>
        </div>
      </header>

      <main>
        {/* Overview Widgets */}
        <OverviewWidgets runs={runs} />

        {/* Main Panels Workspace - stacked layout */}
        <div className="dashboard-body">
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
            {/* Runs Table */}
            <RunsTable
              runs={runs}
              selectedRunId={selectedRunId}
              onSelectRun={handleSelectRun}
              searchQuery={searchQuery}
              onSearchChange={setSearchQuery}
            />

            {/* Full-width Run Details panel below the table */}
            <RunDetails details={selectedRunDetails} loading={loadingDetails} />
          </div>
        </div>
      </main>
    </div>
  );
}
