# Nova Mail Vue 内嵌内容

这是 Nova Mail 原生桌面壳子的 Vue 3 内容层。窗口圆角、阴影、标题栏、图标、窗口按钮和通知外壳都由 C++ 绘制；Vue 只负责渲染内嵌页面内容，并通过 `src/nova-shell-sdk.ts` 与原生壳子通信。

## 常用命令

```bash
npm install
npm run dev
npm run build
npm run build:embedded
```

## 壳子 SDK

`src/nova-shell-sdk.ts` 是对外开发 SDK，只封装和原生壳子的通信，不包含页面数据读取或示例内容逻辑。

```ts
import {
  getNovaShellTransportStatus,
  getNovaShellRuntime,
  isRunningInNovaShell,
  notifyShellReady,
  requestShellWindowControl,
  setShellTitle,
  setShellWindowMetrics,
  showShellNotification,
} from './nova-shell-sdk';
```

### 判断是否运行在壳子中

```ts
if (isRunningInNovaShell()) {
  console.log('当前运行在 Nova Mail 桌面壳子中。');
}

console.log(getNovaShellRuntime());
```

在 Nova Mail 桌面壳子中，`getNovaShellRuntime()` 会返回：

```ts
{
  name: 'NovaMailDesktop',
  version: '1.0.0',
  bridge: 'webview2',
}
```

在普通浏览器中运行时，`getNovaShellRuntime()` 返回 `undefined`，`isRunningInNovaShell()` 返回 `false`。

### 初始化壳子

Vue 页面挂载后调用一次，用来同步标题、初始窗口尺寸和最小窗口尺寸：

```ts
notifyShellReady({
  title: 'Nova Mail',
  window: {
    initialWidth: 1044,
    initialHeight: 744,
    minWidth: 560,
    minHeight: 440,
  },
});
```

### 设置标题

```ts
setShellTitle('Nova Mail - 收件箱');
```

### 设置窗口尺寸

```ts
setShellWindowMetrics({
  initialWidth: 960,
  initialHeight: 680,
  minWidth: 640,
  minHeight: 480,
});
```

### 显示通知

```ts
showShellNotification({
  title: '新邮件通知',
  body: '张三给你发来一封新邮件。',
  unreadCount: 3,
});
```

### 请求窗口控制

```ts
requestShellWindowControl('minimize');
requestShellWindowControl('maximize');
requestShellWindowControl('close');
```

SDK 会优先向 WebView2 的 `window.chrome.webview.postMessage` 发送 JSON 字符串消息；同时也会调用 `window.NovaMailShell.postMessage(message)`，并派发同名 `CustomEvent`，方便本地浏览器调试。

如果调用后没有效果，先检查当前页面是否真的运行在壳子通信环境中：

```ts
console.log(getNovaShellTransportStatus());
```

正常情况下至少应有一个值为 `true`：

```text
hasWebViewBridge: true
hasHostBridge: true
```

如果两个都是 `false`，说明当前页面没有连接到原生壳子的通信通道。普通浏览器里打开页面时就是这种情况；原生壳子如果没有注册 WebView2 的 `WebMessageReceived` 回调，也会出现这种情况。

C++ 壳子必须在 WebView2 初始化后注册消息回调，并把收到的 JSON 转给 `HandleVueBridgeMessage()`：

```cpp
webview->add_WebMessageReceived(
    Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2*,
               ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
          wil::unique_cotaskmem_string message;
          if (SUCCEEDED(args->TryGetWebMessageAsString(&message)) && message) {
            HandleVueBridgeMessage(message.get());
          }
          return S_OK;
        })
        .Get(),
    &webMessageReceivedToken_);
```

如果没有这段原生回调，`notifyShellReady()`、`setShellTitle()`、`setShellWindowMetrics()` 都只是在前端发送消息，C++ 标题栏和窗口尺寸不会变化。

## 其他 Vue 项目接入步骤

### 1. 复制 SDK 文件

把本项目的 `src/nova-shell-sdk.ts` 复制到你的 Vue 项目中，例如：

```text
your-vue-project/src/sdk/nova-shell-sdk.ts
```

这个文件没有依赖 Vue，也不依赖本示例项目的其它文件，可以直接复制使用。

### 2. 在 main.ts 中初始化壳子

推荐在 `main.ts` 中完成 Vue 应用挂载后调用 `notifyShellReady()`，让壳子在页面启动时同步标题、初始窗口尺寸和最小窗口尺寸：

```ts
import { createApp } from 'vue';
import App from './App.vue';
import {
  getNovaShellTransportStatus,
  isRunningInNovaShell,
  notifyShellReady,
} from './sdk/nova-shell-sdk';

createApp(App).mount('#app');

if (!isRunningInNovaShell()) {
  console.info('当前页面运行在普通浏览器中，壳子能力不会生效。');
}

const shellTransport = getNovaShellTransportStatus();
if (!shellTransport.hasWebViewBridge && !shellTransport.hasHostBridge) {
  console.warn('当前页面没有检测到 Nova Mail 壳子通信通道。');
}

notifyShellReady({
  title: 'Nova Mail',
  window: {
    initialWidth: 1044,
    initialHeight: 744,
    minWidth: 560,
    minHeight: 440,
  },
});
```

`initialWidth` / `initialHeight` 是窗口首次显示或初始化后的尺寸；`minWidth` / `minHeight` 是用户拖拽缩放时允许的最小尺寸。

注意：`notifyShellReady()` 需要在壳子的 WebView2 页面里运行才会影响 C++ 标题栏和窗口尺寸。仅在普通浏览器里运行时，它只会派发本地调试事件，不会改变原生窗口。

### 3. 页面切换时同步标题

如果你的项目使用 Vue Router，可以在路由变化后更新原生标题栏：

```ts
import router from './router';
import { setShellTitle } from './sdk/nova-shell-sdk';

router.afterEach((to) => {
  const pageTitle = typeof to.meta.title === 'string' ? to.meta.title : 'Nova Mail';
  setShellTitle(pageTitle);
});
```

也可以在任意组件中直接调用：

```ts
setShellTitle('Nova Mail - 设置');
```

### 4. 需要时发送通知

收到新邮件、同步完成或后台任务结束时，调用：

```ts
import { showShellNotification } from './sdk/nova-shell-sdk';

showShellNotification({
  title: '新邮件通知',
  body: '张三给你发来一封新邮件。',
  unreadCount: 3,
});
```

### 5. 需要时调整窗口约束

如果某个页面需要更大的最小可用区域，可以进入页面时更新窗口尺寸：

```ts
import { setShellWindowMetrics } from './sdk/nova-shell-sdk';

setShellWindowMetrics({
  minWidth: 900,
  minHeight: 640,
});
```

### 6. 正常打包给原生壳子加载

壳子支持普通 Vue 项目正常编译后的目录结构，不要求把所有资源内联到一个 HTML 文件里。你的项目可以直接按正常方式构建：

```bash
npm run build
```

构建完成后保留整个 `dist` 目录，例如：

```text
dist/
  index.html
  assets/
    index-xxxx.js
    index-xxxx.css
    ...
```

把这个 `dist` 目录交给 C++ 壳子加载 `dist/index.html` 即可。不要只复制 `index.html`，因为普通 Vue 构建会把 JS、CSS、图片、字体等资源放在 `assets` 目录里。

Vite 项目建议设置相对路径：

```ts
// vite.config.ts
import vue from '@vitejs/plugin-vue';
import { defineConfig } from 'vite';

export default defineConfig({
  base: './',
  plugins: [vue()],
});
```

`base: './'` 可以让 `index.html` 通过本地文件或 WebView2 虚拟主机加载时，正确找到 `assets` 下的资源。

最终把构建产物交给 C++ 壳子加载即可。当前壳子侧识别的消息类型是：

```text
nova-mail:ready
nova-mail:shell:set-title
nova-mail:shell:set-window-metrics
nova-mail:shell:notify
nova-mail:window-control
```

### 7. 将 dist 编入壳子 exe

普通 Vue 项目构建出来的 `dist` 不能直接当 C++ 源码编译，它需要先作为 Windows 资源打进壳子程序。流程是：

```text
Vue 源码
  -> npm run build
  -> dist/index.html + dist/assets/*
  -> 打包成 RCDATA 资源
  -> MSBuild 编译进 NovaMailDesktop.exe
  -> 运行时释放资源目录并让 WebView2 加载 index.html
```

本仓库已经提供了打包脚本：

```bash
node tools/pack-vue-dist.mjs VueShell/dist NovaMailDesktop/res/vue-dist
```

如果是其它 Vue 项目，把第一个参数换成它的 `dist` 路径即可：

```bash
node tools/pack-vue-dist.mjs C:/path/to/your-vue-project/dist NovaMailDesktop/res/vue-dist
```

脚本会生成或更新这些文件：

```text
NovaMailDesktop/res/vue-dist/
  index.html
  assets/
  manifest.json
NovaMailDesktop/res/vue-dist-generated.rc
NovaMailDesktop/res/vue-dist-resources.h
NovaMailDesktop/res/vue-dist-resource-map.h
```

`ShellResources.rc` 已经包含：

```rc
#include "res\\vue-dist-generated.rc"
```

所以执行打包脚本后，重新编译 C++ 项目，`dist` 内容就会进入最终 exe：

```bash
MSBuild NovaMailDesktop.sln /p:Configuration=Release /p:Platform=x64
```

完整更新流程：

```bash
cd VueShell
npm run build
cd ..
node tools/pack-vue-dist.mjs VueShell/dist NovaMailDesktop/res/vue-dist
MSBuild NovaMailDesktop.sln /p:Configuration=Release /p:Platform=x64
```

运行时建议壳子把内置资源释放到用户缓存目录，例如：

```text
%LOCALAPPDATA%/NovaMailDesktop/vue-dist/
```

然后让 WebView2 加载：

```text
%LOCALAPPDATA%/NovaMailDesktop/vue-dist/index.html
```

不要直接拼接 HTML 字符串加载普通构建产物，因为普通构建依赖 `assets` 目录里的 JS、CSS、字体和图片。

## 示例页面嵌入数据

`src/embedded-payload.ts` 只服务于当前 Vue 示例页面，用来读取宿主注入的演示内容，不属于对外 SDK。

宿主可以在 Vue 挂载前替换 `index.html` 中的 JSON：

```html
<script id="nova-mail-embedded-data" type="application/json">
  {
    "title": "Nova Mail",
    "contentHtml": "<div>Embedded content</div>",
    "accentColor": "#2457d6",
    "window": {
      "initialWidth": 1044,
      "initialHeight": 744,
      "minWidth": 560,
      "minHeight": 440
    }
  }
</script>
```

也可以在应用脚本运行前设置全局变量：

```html
<script>
  window.__NOVA_MAIL_EMBEDDED_DATA__ = {
    title: 'Nova Mail',
    contentText: 'Embedded content',
    window: {
      initialWidth: 960,
      initialHeight: 680,
      minWidth: 640,
      minHeight: 480
    }
  };
</script>
```

## 构建产物

默认推荐普通构建：

```bash
npm run build
```

产物结构：

```text
dist/
  index.html
  assets/
```

如果确实需要单文件内嵌模式，可以使用：

```bash
npm run build:embedded
```

单文件产物会输出到：

```text
dist-embedded/index.html
```
