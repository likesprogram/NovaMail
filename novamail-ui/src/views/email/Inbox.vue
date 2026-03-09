<template>
    <div class="inbox">
        <EmailList mailBox="inbox" :title="'收件箱'" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Inbox',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';

</script>

<style scoped>
.inbox {
    width: 100%;
    height: 100%;
    position: relative;
    overflow: hidden;
    overflow-y: auto;
}
</style>
