import type { RouteRecordRaw } from 'vue-router'
import { EmailRoutes } from '@/router/modules/EmailRouter'
import { UserRoutes } from '@/router/modules/UserRouter'
import { LoginAndRegisterRoutes } from '@/router/modules/LoginAndRegisterRouter'
import { AgreementRoutes } from '@/router/modules/AgreementRouter'
import { SettingRouters } from '@/router/modules/SettingRouter'

export const MainRoutes: Array<RouteRecordRaw> = [
    { 
        path: '/',
        name: 'manager',  // 页面管理器
        component: () => import('@views/Manager.vue'),
        redirect: '/inbox', // 默认跳转收件箱
        meta: { requiresAuth: true },
        children: // 子路由
        [
            ...EmailRoutes,
            ...UserRoutes,
            ...SettingRouters
        ]
    },
    ...LoginAndRegisterRoutes, // 登录注册
    ...AgreementRoutes, // 协议
    {
        path: '/error',
        name: 'error', // 错误页面
        component: () => import('@views/Error.vue'),
        meta: { requiresAuth: false, title: '错误' },
    },
    {
        path: '/:pathMatch(.*)*', // 匹配所有未定义的路由
        redirect: '/error',         // 重定向到 404
    }
];
