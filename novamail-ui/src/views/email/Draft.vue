<template>
    <div class="draft">
        <EmailList mailBox="draft" :title="'草稿箱'" :mailBoxFolder="mailBoxFolder" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
import type { MailBoxFolder } from '@/api/email/Email';
import { ref } from 'vue';
export default {
    name: 'Draft',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
const mailBoxFolder = ref<MailBoxFolder>({
    folderId: "1",       // 文件夹ID
    floderName: "DRAFT",     // 文件夹名称
    noReadCont: 0     // 未读邮件数量
});
</script>

<style scoped>
.draft {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
