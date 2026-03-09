import type { App } from 'vue';
// 引入 main_router.ts
import { MainRoutes } from '@/router/modules/MainRouter';
import { createRouter, createWebHistory  } from 'vue-router';
import { getCurrentInstance } from 'vue'

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
router.beforeEach((to: any, from, next) => {
  // 判断是否需要登录后才可访问
  //console.log('to.meta.requiresAuth:'+to.meta.requiresAuth)
  if (to.meta.requiresAuth) {  // 如果页面需要先登录才能访问
    if (true) { // 没有登录
		console.log(from.path+'to:'+to.path+'   not Authen')
        next({name:'login'})
    } else if (to.path=='/'){ // 已经登录
		console.log('to:'+to.path+'    Authen')
		// let currentRole = SignUtil.getUserDetail()?.role
		// if (currentRole=='admin'){
		// 	next({name:'chief_index'})
		// }
    }else{
		next()
	}
  }else{
	  next()
  }
});

// 路由后置守卫
router.afterEach((to) => {
	// 定义页面标题
	const pageTitle = to.matched.filter(r => r.meta && r.meta.title).map(r => r.meta.title as string).pop();
	if (pageTitle) document.title = `${title}-${pageTitle}`;
	else document.title = title;
});

/* 导出 setupRouter */
export const setupRouter = (app: App<Element>) => {
	app.use(router);
};
