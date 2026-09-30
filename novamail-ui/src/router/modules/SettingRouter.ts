import type { RouteRecordRaw } from 'vue-router';

export const SettingRouters: RouteRecordRaw[] = [
    {
        path: '/settings',
        name: 'settings',  // 设置
        component: () => import('@views/settings/Settings.vue'),
        redirect: { name: 'usersSetting' }, // 默认跳转用户信息
        meta: { requiresAuth: true, title: '设置' },
        children: [
            {
                path: 'users',
                name: 'usersSetting',  // 用户信息
                component: () => import('@views/settings/UserInformation.vue'),
                meta: { requiresAuth: true, title: '用户信息' },
            },
            {
                path: 'mail',
                name: 'mailSetting',  // 邮箱管理
                component: () => import('@views/settings/MailSetting.vue'),
                meta: { requiresAuth: true, title: '邮箱管理' },
            },
            {
                path: 'system',
                name: 'systemSetting',  // 系统管理
                component: () => import('@views/settings/SystemSetting.vue'),
                meta: { requiresAuth: true, requiresAdmin: true, title: '系统管理' },
            },
            {
                path: 'services',
                name: 'serviceMonitorSetting',  // 服务监控
                component: () => import('@views/settings/ServiceMonitor.vue'),
                meta: { requiresAuth: true, requiresAdmin: true, title: '服务监控' },
            },
        ],
    },
];
