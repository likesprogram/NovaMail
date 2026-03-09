<template>
    <div class="compose">
        <WriteMail></WriteMail>
    </div>
</template>

<script lang="ts">
export default {
    name: 'Compose',
}
</script>

<script setup lang="ts">
import WriteMail from '@components/WriteMail.vue';
</script>

<style scoped>
.compose {
    width: 100%;
    height: 100%;
    position: relative;
    z-index: 999;
    top: 0;
    left: 0;
}
</style>
