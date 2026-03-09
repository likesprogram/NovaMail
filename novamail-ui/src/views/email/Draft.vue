<template>
    <div class="draft">
        <EmailList mailBox="draft" :title="'草稿箱'" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Draft',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
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
