<template>
    <div class="star">
        <EmailList mailBox="star" :title="'星标邮件'" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Star',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
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
