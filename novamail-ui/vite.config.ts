import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'

// https://vite.dev/config/
export default defineConfig({
    base: '/', // 类似publicPath，'./'避免打包访问后空白页面，不加线上也访问不了
    plugins: [vue()],
	define: {
    	global: 'globalThis' // 解决 ReferenceError: global is not defined
	},
	resolve: {
		alias: {
            '@': fileURLToPath(new URL('./src', import.meta.url)),
            '@assets': fileURLToPath(new URL('./src/assets', import.meta.url)),
            '@components': fileURLToPath(new URL('./src/components', import.meta.url)),
            '@views': fileURLToPath(new URL('./src/views', import.meta.url)),
            '@api': fileURLToPath(new URL('./src/api', import.meta.url)),
            '@router': fileURLToPath(new URL('./src/router', import.meta.url)),
            '@store': fileURLToPath(new URL('./src/store', import.meta.url)),
            '@utils': fileURLToPath(new URL('./src/utils', import.meta.url)),
            '@typings': fileURLToPath(new URL('./src/types', import.meta.url)),
            '@tinymce': fileURLToPath(new URL('./src/assets/tinymce', import.meta.url)),
		}
	},
    server: {
        port: 3000,
        host: '0.0.0.0',
        open: true, //服务器运行后自动打开网页
        cors: true, // 允许跨域
    }
})
