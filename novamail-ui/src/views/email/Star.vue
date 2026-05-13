<template>
    <div class="star">
        <EmailList mailBox="star" :title="'星标邮件'" :mailBoxFolder="mailBoxFolder" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
import type { MailBoxFolder } from '@/api/email/Email';
import { ref } from 'vue';
export default {
    name: 'Star',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
const mailBoxFolder = ref<MailBoxFolder>({
    folderId: "-1",       // 文件夹ID
    floderName: "STAR",     // 文件夹名称
    noReadCont: 0     // 未读邮件数量
});
</script>

<style scoped>
.star {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
