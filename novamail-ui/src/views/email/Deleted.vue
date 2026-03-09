<template>
    <div class="deleted">
        <EmailList mailBox="deleted" :title="'已删除'" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Deleted',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
</script>

<style scoped>
.deleted {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
