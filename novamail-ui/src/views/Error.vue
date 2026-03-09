<template>
    <div class="error">
        <div class="errorImage">
            <img :src="notFoundImage" alt="404 Not Found" class="error-image" />
        </div>
        <div class="text">
            <span class="title">抱歉，你访问的页面不存在或已被删除。</span>
            <span class="description">请检查您输入的网址是否正确，或点击下面的按钮返回主页。</span>
            <button class="back-button" @click="$router.push('/')">返回主页</button>
        </div>
    </div>
</template>

<script lang="ts">
export default {
    name: 'Error',
}
</script>

<script setup lang="ts">
import notFoundImage from '@assets/illustration/404.svg'
</script>

<style scoped>
.error {
    width: 100%;
    height: 100%;
    min-width: 0px;
    min-height: 0px;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    padding: 20px;
}

.errorImage {
    flex: 1;
    width: 100%;
    max-width: 100%;
    max-height: 100%;
    min-width: 0px;
    min-height: 0px;
    display: flex;
    justify-content: center;
    align-items: center;
}.error-image {
    width: 100%;
    height: auto;           /* 保持比例自适应 */
    max-height: 100%;       /* 不超过父容器高度 */
    object-fit: contain;    /* 保持完整显示 */
}

.text {
    flex-shrink: 0px;
    width: 100%;
    max-width: 100%;
    min-width: 0px;
    min-height: 0px;
    display: flex;
    gap: 15px;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}

.title {
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
}

.description {
  color: #666;
}

.back-button {
  background-color: #42b983;
  color: white;
  border: none;
  padding: 10px 25px;
  font-size: 1rem;
  border-radius: 6px;
  cursor: pointer;
  transition: background-color 0.3s ease;
}

.back-button:hover {
  background-color: #36a174;
}
</style>