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

@media (max-width: 768px) {
    .dashboard-title {
        font-size: 2rem;
    }
    
    .dashboard-subtitle {
        font-size: 1rem;
    }
}
</style>