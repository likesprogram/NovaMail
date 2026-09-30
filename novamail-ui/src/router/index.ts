import type { App } from 'vue';
// 引入 main_router.ts
import { setShellTitle } from '@utils/NovaShellSdk';
import { MainRoutes } from '@/router/modules/MainRouter';
import { createRouter, createWebHistory  } from 'vue-router';
import { getCurrentInstance } from 'vue'
import { userService } from '@service/UserService';

const instance = getCurrentInstance();
const title = instance?.appContext.config.globalProperties.$title || '新星邮箱';


const router = createRouter({
	// 哈希工作模式有#不美观
	// history: createWebHashHistory(),
	//history工作模式无#,但部署服务器要处理路径问题
	history: createWebHistory(),
	routes: MainRoutes
});

/* 初始化路由表 */
export function resetRouter() {
	router.getRoutes().forEach((route) => {
		const { name } = route;
		if (name) {
			router.hasRoute(name) && router.removeRoute(name);
		}
	});
}

// 路由前置守卫
router.beforeEach(async (to: any, from, next) => {
	from; // 避免 ESLint 报错
	// 判断是否需要登录后才可访问
	if (to.meta.requiresAuth) {  // 如果页面需要先登录才能访问
		if (userService.isLogin()) {
			if (to.meta.requiresAdmin) {
				if ((await userService.getUserInfo())?.isAdmin) next();  // 权限足够，放行
				else next(false); // 权限不足，停留在当前页面
			} else next();  // 没有权限要求，放行
		} else {
		    next({name:'login'}); // 未登录，跳到登录页
		}
	} else {
		next();
	}
});

// 路由后置守卫
router.afterEach((to) => {
	// 定义页面标题
	const pageTitle = to.matched.filter(r => r.meta && r.meta.title).map(r => r.meta.title as string).pop();
	if (pageTitle) document.title = `${title}-${pageTitle}`;
	else document.title = title;
	setShellTitle(document.title);
});

/* 导出 setupRouter */
export const setupRouter = (app: App<Element>) => {
	app.use(router);
};
