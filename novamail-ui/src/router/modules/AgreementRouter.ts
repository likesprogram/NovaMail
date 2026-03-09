import type { RouteRecordRaw } from 'vue-router';

export const AgreementRoutes: RouteRecordRaw[] = [
    {
        path: '/agreement',
        name: 'agreement',  // 协议
        component: () => import('@views/email/Sent.vue'),
        meta: { requiresAuth: false, title: '用户协议' },
        children: [
            {
                path: ':id',
                name: 'agreementDetail',
                component: () => import('@views/email/Star.vue'),
                meta: { requiresAuth: false },
            }
        ],
    },
];