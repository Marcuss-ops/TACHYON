import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import path from 'path'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    watch: {
      // Exclude large build directories from file watching to avoid ENOSPC
      ignored: [
        '**/build/**',
        '**/node_modules/**',
        '**/.git/**',
        '**/__pycache__/**',
        '**/*.sqlite',
      ],
    },
    proxy: {
      // Proxy API requests to Python backend during dev
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
    },
  },
})
