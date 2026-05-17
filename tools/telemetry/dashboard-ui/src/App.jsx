import React, { useState, useEffect } from 'react';
import OverviewWidgets from './components/OverviewWidgets';
import RunsTable from './components/RunsTable';
import RunDetails from './components/RunDetails';
import { Database, RefreshCw, Layers } from 'lucide-react';

export default function App() {
  const [runs, setRuns] = useState([]);
  const [selectedRunId, setSelectedRunId] = useState('');
  const [selectedRunDetails, setSelectedRunDetails] = useState(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [loadingRuns, setLoadingRuns] = useState(true);
  const [loadingDetails, setLoadingDetails] = useState(false);
  const [dbOnline, setDbOnline] = useState(false);
  const [dbPath, setDbPath] = useState('Resolving DB...');

  const getApiUrl = (path) => {
    if (window.location.port === '5173') {
      return `http://localhost:8080${path}`;
    }
    return path;
  };

  const loadAllRuns = async () => {
    setLoadingRuns(true);
    try {
      const response = await fetch(getApiUrl('/api/runs'));
      if (!response.ok) {
        const err = await response.json();
        setDbOnline(false);
        setDbPath(err.path || 'Error');
        setRuns([]);
        setLoadingRuns(false);
        return;
      }

      const data = await response.json();
      setRuns(data);
      setDbOnline(true);
      setDbPath('Connected: tachyon_render_history.sqlite');
      
      // Auto-select first run if none selected
      if (data.length > 0 && !selectedRunId) {
        handleSelectRun(data[0].run_id);
      }
    } catch (e) {
      setDbOnline(false);
      setDbPath('Offline / Failed to connect');
      setRuns([]);
    } finally {
      setLoadingRuns(false);
    }
  };

  const handleSelectRun = async (runId) => {
    setSelectedRunId(runId);
    setLoadingDetails(true);
    try {
      const res = await fetch(getApiUrl(`/api/runs/${runId}`));
      if (!res.ok) throw new Error('Failed to fetch detailed metrics');
      const data = await res.json();
      setSelectedRunDetails(data);
    } catch (err) {
      console.error(err);
      setSelectedRunDetails(null);
    } finally {
      setLoadingDetails(false);
    }
  };

  useEffect(() => {
    loadAllRuns();
  }, []);

  return (
    <div className="dashboard-container">
      <header>
        <div className="logo-section">
          <h1>Tachyon</h1>
        </div>
        <div className="db-info">
          <span className={`db-dot ${dbOnline ? '' : 'offline'}`}></span>
          <Database size={14} style={{ color: dbOnline ? 'var(--accent-green)' : 'var(--accent-red)' }} />
          <span>{dbPath}</span>
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
            <button
              className="refresh-btn"
              style={{ alignSelf: 'flex-start' }}
              onClick={loadAllRuns}
            >
              <RefreshCw size={14} />
              Sync DB Records
            </button>

            {/* Full-width Run Details panel below the table */}
            <RunDetails details={selectedRunDetails} loading={loadingDetails} />
          </div>
        </div>
      </main>
    </div>
  );
}
