import type { RouteRecordRaw } from 'vue-router';

export const UserRoutes: RouteRecordRaw[] = [
    {
        path: '/address',
        name: 'address',  // 通讯录
        component: () => import('@views/userProfile/AddressBook.vue'),
        meta: { requiresAuth: true, title: '通讯录' },
    },
    {
        path: '/files',
        name: 'files',  // 文件中转站
        component: () => import('@views/userProfile/FileBox.vue'),
        meta: { requiresAuth: true, title: '文件中转站' },
    },
];