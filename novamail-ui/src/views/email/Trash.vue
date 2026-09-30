<template>
    <div class="trash">
        <EmailList mailBox="trash" :title="'垃圾箱'" :mailBoxFolder="mailBoxFolder" :actionsMask="EmailActions.Delete | EmailActions.ReplyAll | EmailActions.Forward | EmailActions.NotSpam | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
import type { MailBoxFolder } from '@/api/email/Email';
import { ref } from 'vue';
export default {
    name: 'Trash',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
const mailBoxFolder = ref<MailBoxFolder>({
    folderId: "4",       // 文件夹ID
    floderName: "TRASH",     // 文件夹名称
    noReadCont: 0     // 未读邮件数量
});
</script>

<style scoped>
.trash {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
