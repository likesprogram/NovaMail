import { createApp } from 'vue'
import './style.css'
import App from './App.vue'

// 设置窗口标题和尺寸
import { notifyShellReady } from '@utils/NovaShellSdk';
notifyShellReady({
    title: 'Nova Mails',
    window: {
        initialWidth: 1420,
        initialHeight: 860,
        minWidth: 1420,
        minHeight: 860,
    },
});

// 导入图标
import '@assets/icons/iconfont/iconfont.css'
// 导入通用样式
import '@assets/public.css';
// 路由
import { setupRouter } from '@router/index';
// 导入 favicon 图片
import faviconUrl from '@assets/icons/favicon.svg';
// Pinia
import { createPinia } from 'pinia'
// UserStore
import { UserStore } from '@store/UserStore'

// 将全局变量绑定到 window，方便类型推断
declare global {
  interface Window {
    tinymce: any;
  }
}

// 设置 vh 变量，解决移动端 100vh 不准确的问题
function setVh() {
    const vh = window.innerHeight * 0.01; // 1% 的可视高度
    document.documentElement.style.setProperty('--vh', `${vh}px`);
}
// 初始化
setVh();
// 当窗口大小变化时也更新（例如 iOS 地址栏显示/隐藏）
window.addEventListener('resize', setVh);

// 创建 VUEX 应用
const app = createApp(App);

// 注册 Pinia
const pinia = createPinia();
app.use(pinia);

// 初始化 UserStore
const userStore = UserStore();
userStore.init();

// 添加全局变量 title
app.config.globalProperties.$title = '新星邮箱';
app.config.globalProperties.$subtitle = 'NovaMail';
app.config.globalProperties.$version = '1.0.0';

// 设置网页 logo（favicon）
const setFavicon = (url: string) => {
    // 查找已有的 favicon 标签
    let link = document.querySelector<HTMLLinkElement>("link[rel~='icon']");
    
    // 如果不存在，就创建新的 <link>
    if (!link) {
        link = document.createElement('link');
        link.rel = 'icon';
        document.head.appendChild(link);
    }

    // 判断文件类型，自动设置 type
    if (url.endsWith('.svg')) {
        link.type = 'image/svg+xml';
    } else if (url.endsWith('.png')) {
        link.type = 'image/png';
    } else {
        link.removeAttribute('type');
    }

    // 更新路径
    link.href = url;
}
// 设置 favicon
setFavicon(faviconUrl);

// 挂载 Vue 路由
setupRouter(app);

// 挂载 Vue
app.mount('#app');
