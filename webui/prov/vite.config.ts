import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

export default defineConfig({
  plugins: [svelte()],
  base: './',
  build: {
    target: 'es2020',
    minify: 'terser',
    cssMinify: 'lightningcss',
    rollupOptions: {
      output: {
        entryFileNames: 'assets/index.js',
        chunkFileNames: 'assets/index.js',
        assetFileNames: (info) => {
          if (info.name?.endsWith('.css')) return 'assets/index.css'
          return 'assets/[name][extname]'
        }
      }
    },
    terserOptions: {
      compress: { drop_console: true, passes: 3 },
      mangle: { toplevel: true },
      format: { comments: false }
    }
  },
  server: {
    port: 5176,
    proxy: {
      '/api': { target: process.env.VITE_MINER_URL || 'http://bitaxe-403-1.local', changeOrigin: true },
      '/save': { target: process.env.VITE_MINER_URL || 'http://bitaxe-403-1.local', changeOrigin: true },
      '/logo.svg': { target: process.env.VITE_MINER_URL || 'http://bitaxe-403-1.local', changeOrigin: true }
    }
  }
})
