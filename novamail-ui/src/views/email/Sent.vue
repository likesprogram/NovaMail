<template>
    <div class="sent">
        <EmailList mailBox="sent" :title="'已发送'" :actionsMask="EmailActions.Edit | EmailActions.Revoke | EmailActions.Delete | EmailActions.ReplyAll | EmailActions.Forward | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Sent',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
</script>

<style scoped>
.sent {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
