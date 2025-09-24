import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  server: {
    proxy: {
      '/api': {
        target: `http://${process.env.BACKEND_HOST || 'localhost'}:${process.env.BACKEND_PORT || '8000'}`,
        changeOrigin: true,
        secure: false,
      },
    },
    allowedHosts: process.env.MODE || 'development' === 'development' ? true : ['localhost', process.env.PROXY_HOST || 'gardeneye-proxy'],
  },
})