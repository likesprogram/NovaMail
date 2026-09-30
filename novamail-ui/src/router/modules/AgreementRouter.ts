import type { RouteRecordRaw } from 'vue-router';

export const AgreementRoutes: RouteRecordRaw[] = [
    {
        path: '/agreement',
        name: 'agreement',
        redirect: { name: 'userAgreement' },
        meta: { requiresAuth: false, title: '协议与政策' },
    },
    {
        path: '/agreement/user',
        name: 'userAgreement',
        component: () => import('@views/agreement/UserAgreement.vue'),
        meta: { requiresAuth: false, title: '用户协议' },
    },
    {
        path: '/agreement/privacy',
        name: 'privacyPolicy',
        component: () => import('@views/agreement/PrivacyPolicy.vue'),
        meta: { requiresAuth: false, title: '隐私政策' },
    },
];
