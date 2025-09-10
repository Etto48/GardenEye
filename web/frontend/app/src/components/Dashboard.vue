<script setup lang="ts">
import SensorList from './SensorList.vue';

</script>

<template>
    <div class="dashboard p-4 max-w-xl mx-auto">
        <div class="text-center mb-4">
            <h1 class="dashboard-title">Dashboard</h1>
            <p class="dashboard-subtitle">Monitor your garden sensors in real-time</p>
        </div>
        <SensorList />
    </div>
</template>

<style scoped>
.dashboard-title {
    color: var(--color-heading);
    margin: 0 0 0.5rem 0;
    font-size: 2.5rem;
}

.dashboard-subtitle {
    color: var(--color-text);
    margin: 0;
    font-size: 1.1rem;
    opacity: 0.8;
}

@media (max-width: 768px) {
    .dashboard-title {
        font-size: 2rem;
    }
    
    .dashboard-subtitle {
        font-size: 1rem;
    }
}
</style>