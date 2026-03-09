<template>
    <div class="trash">
        <EmailList mailBox="trash" :title="'垃圾箱'" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.NotSpam | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Trash',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
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
