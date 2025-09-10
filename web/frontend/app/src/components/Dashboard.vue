<script setup lang="ts">
import { ref, computed } from 'vue';
import SensorList from './SensorList.vue';
import SystemSummary from './SystemSummary.vue';

// Component refs to access child component methods
const systemSummaryRef = ref<InstanceType<typeof SystemSummary> | null>(null);
const sensorListRef = ref<InstanceType<typeof SensorList> | null>(null);

// Loading state computed from both child components
const loading = computed(() => {
    return (systemSummaryRef.value?.loading ?? true) || (sensorListRef.value?.loading ?? true);
});

// Function to refresh both components
const refreshDash = async () => {
    if (systemSummaryRef.value && sensorListRef.value) {
        // Call both fetch functions in parallel
        await Promise.all([
            systemSummaryRef.value.fetchInfo(),
            sensorListRef.value.fetchSensors()
        ]);
    }
};
</script>

<template>
    <div class="dashboard p-4 max-w-xl mx-auto">
        <div class="text-center mb-4">
            <h1 class="dashboard-title">Dashboard</h1>
            <p class="dashboard-subtitle">Monitor your garden sensors in real-time</p>
        </div>
        <SystemSummary ref="systemSummaryRef" />
        <SensorList ref="sensorListRef" />
        <!-- Floating reload button in bottom right corner -->
        <button 
            @click="refreshDash" 
            :disabled="loading" 
            class="reload-button accent-bg"
            :title="loading ? 'Loading...' : 'Refresh sensors'"
        >
            <i :class="loading ? 'bi bi-hourglass-split' : 'bi bi-arrow-clockwise'"></i>
        </button>
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

.reload-button {
    position: fixed;
    bottom: 20px;
    right: 20px;
    width: 56px;
    height: 56px;
    border-radius: 50%;
    color: var(--color-background);
    border: none;
    font-size: 24px;
    font-weight: bold;
    cursor: pointer;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
    z-index: 1000;
    transition: all 0.3s ease;
    display: flex;
    align-items: center;
    justify-content: center;
}

.reload-button:hover:not(:disabled) {
    background-color: var(--color-accent-hover);
    transform: scale(1.05);
    box-shadow: 0 6px 16px rgba(0, 0, 0, 0.2);
}

.reload-button:disabled {
    background-color: var(--color-accent-active);
    cursor: wait;
    transform: none;
}

.reload-button:active:not(:disabled) {
    background-color: var(--color-accent-active);
    transform: scale(0.95);
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