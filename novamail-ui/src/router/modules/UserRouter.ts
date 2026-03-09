import type { RouteRecordRaw } from 'vue-router';

export const UserRoutes: RouteRecordRaw[] = [
    {
        path: '/users',
        name: 'users',  // 用户信息
        component: () => import('@views/userInformation/UserInformation.vue'),
        meta: { requiresAuth: false, title: '用户信息' },
    },
    {
        path: '/settings',
        name: 'settings',  // 设置
        component: () => import('@views/userInformation/Settings.vue'),
        meta: { requiresAuth: true, title: '设置' },
    },
    {
        path: '/address',
        name: 'address',  // 通讯录
        component: () => import('@views/userInformation/AddressBook.vue'),
        meta: { requiresAuth: false, title: '通讯录' },
    },
    {
        path: '/files',
        name: 'files',  // 文件中转站
        component: () => import('@views/userInformation/FileBox.vue'),
        meta: { requiresAuth: false, title: '文件中转站' },
    },
];