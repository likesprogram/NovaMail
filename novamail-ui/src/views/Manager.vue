<template>
    <div class="manager">
        <Header></Header>
        <div class="body">
            <Sidebar></Sidebar>
            <div class="content">
                <router-view></router-view>
            </div>
        </div>
    </div>
</template>

<script lang="ts">
export default {
    name: 'Manager',
}
</script>

<script setup lang="ts">
import Sidebar from '@components/Sidebar.vue';
import Header from '@components/Header.vue'

</script>

<style scoped>
.manager {
    width: 100%;
    height: 100%;
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
}
.manager .body {
    flex: 1;
    min-height: 0; /* 允许子元素收缩 */
    width: 100%;
    display: flex;
    position: relative;
}

.manager .content {
    flex: 1;
    min-height: 0; /* 避免内容撑开 */
    width: 100%;
    position: relative;
    z-index: 1;
    overflow: hidden;
    overflow-y: auto;
    border-radius: 10px;
    margin-bottom: 15px;
    margin-right: 15px;
    background-color: rgba(255, 255, 255, .5);
	backdrop-filter: blur(2rem);
	-webkit-backdrop-filter: blur(2rem);
	-moz-backdrop-filter: blur(2rem);
	-ms-backdrop-filter: blur(2rem);
    box-shadow: var(--shadow-card);
}
</style>
