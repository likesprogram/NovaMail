<template>
    <div class="content">
        <div class="background">
            <div class="backgroundImage"></div>
            <div class="backgroundOverlay"></div>
        </div>
        <router-view/>
    </div>
</template>

<script lang="ts">
export default {
    name: 'App',
}
</script>

<script setup lang="ts">
</script>

<style scoped>
.content {
    width: 100%;
    height: 100%;
    position: relative;
}

/* 背景 */
.content .background {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: -1;
}
.content .background .backgroundImage {
    position: absolute;
    top: 0;
    left: 0;
    z-index: -1;
    width: 100%;
    height: 100%;
    background-image: url('@assets/background.svg');
    background-size: cover;
    background-repeat: no-repeat;
}
.content .background .backgroundOverlay {
    position: absolute;
    top: 0;
    width: 100%;
    height: 100%;
    left: 0;
    z-index: 0;
    background-color: rgba(255, 255, 255, 0.3);
}
</style>