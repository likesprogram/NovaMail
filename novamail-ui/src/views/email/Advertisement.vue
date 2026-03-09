<template>
    <div class="advertisement">
        <EmailList mailBox="advertisement" :title="'广告箱'" :actionsMask="EmailActions.Delete | EmailActions.Forward | EmailActions.NotAd | EmailActions.MarkRead | EmailActions.Mark | EmailActions.MoveTo"></EmailList>
    </div>
</template>

<script lang="ts">
import { EmailActions } from '@typings/emailList/EmailActions';
export default {
    name: 'Advertisement',
}
</script>

<script setup lang="ts">
import EmailList from '@components/EmailList.vue';
</script>

<style scoped>
.advertisement {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
