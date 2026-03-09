import type { RouteRecordRaw } from 'vue-router';

export const LoginAndRegisterRoutes: RouteRecordRaw[] = [
    {
        path: '/loginAndRegister',
        name: 'loginAndRegister',  // 登录与注册
        component: () => import('@views/loginAndRegister/LoginAndRegister.vue'),
        redirect: 'login', // 默认跳转登录
        meta: { requiresAuth: false },
        children: [
            {
                path: 'login',
                name: 'login',  // 登录
                component: () => import('@views/loginAndRegister/Login.vue'),
                meta: { requiresAuth: false, title: '登录' },
            },
            {
                path: 'register',
                name: 'register',  // 注册
                component: () => import('@views/loginAndRegister/Register.vue'),
                meta: { requiresAuth: false, title: '注册' },
            },
            {
                path: 'forget',
                name: 'forget',  // 忘记密码
                component: () => import('@views/email/Star.vue'),
                meta: { requiresAuth: false, title: '忘记密码' },
            },
        ],
    },
];