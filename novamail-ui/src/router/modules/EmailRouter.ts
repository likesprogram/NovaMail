import type { RouteRecordRaw } from 'vue-router';
import Compose from '@views/email/Compose.vue';
import Inbox from '@views/email/Inbox.vue';
import Sent from '@views/email/Sent.vue';
import Start from '@views/email/Star.vue';
import Draft from '@views/email/Draft.vue';
import Deleted from '@views/email/Deleted.vue';
import Trash from '@views/email/Trash.vue';
import Advertisement from '@views/email/Advertisement.vue';

const mailBoxes = [
    { name: 'compose', title: '写信', component: Compose },
    { name: 'inbox', title: '收件箱', component: Inbox },
    { name: 'sent', title: '已发送', component: Sent },
    { name: 'star', title: '星标件', component: Start },
    { name: 'draft', title: '草稿箱', component: Draft },
    { name: 'deleted', title: '已删除', component: Deleted },
    { name: 'trash', title: '垃圾箱', component: Trash },
    { name: 'advertisement', title: '广告箱', component: Advertisement }
]

export const EmailRoutes: RouteRecordRaw[] = mailBoxes.map(mailBox => ({
    path: `/${mailBox.name}`,
    name: mailBox.name,
    component: () => import('@views/email/EmailManager.vue'),
    meta: { requiresAuth: false, title: mailBox.title },
    children: [
        {
            path: '',
            name: `${mailBox.name}Box`,  // 邮件
            component: mailBox.component,
            meta: { requiresAuth: false, title: '邮件' },
        },
        {
            path: 'read/:id',
            name: `${mailBox.name}Read`,  // 邮件
            component: () => import('@views/email/Read.vue'),
            meta: { requiresAuth: false, title: '邮件' },
        }
    ]
}));
