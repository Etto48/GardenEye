<script setup lang="ts">
import { ref, onMounted } from 'vue'
import SensorCard from './SensorCard.vue'
import type { Sensor } from '@/types/sensor'

const sensors = ref<Sensor[]>([])
const loading = ref(true)
const error = ref<string | null>(null)

const fetchSensors = async () => {
    try {
        loading.value = true
        error.value = null
        
        const response = await fetch('/api/sensors')
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`)
        }
        
        const data = await response.json()
        sensors.value = data
    } catch (err) {
        error.value = err instanceof Error ? err.message : 'Failed to fetch sensors'
        console.error('Error fetching sensors:', err)
    } finally {
        loading.value = false
    }
}

onMounted(() => {
    fetchSensors()
})

const refreshSensors = () => {
    fetchSensors()
}
</script>

<template>
    <div class="sensor-list">
        <div class="section-header mb-3">
            <h2 class="m-0">Sensors</h2>
        </div>
        
        <div v-if="loading && sensors.length === 0" class="loading">
            Loading sensors...
        </div>
        
        <div v-else-if="error" class="error">
            <p>{{ error }}</p>
        </div>
        
        <div v-else-if="sensors.length === 0" class="empty">
            No sensors found
        </div>
        
        <div v-else class="grid grid-auto-fill mobile-grid-cols-1 mobile-gap-md">
            <SensorCard v-for="sensor in sensors" :key="sensor.mac" :sensor="sensor" />
        </div>
        
        <!-- Floating reload button in bottom right corner -->
        <button 
            @click="refreshSensors" 
            :disabled="loading" 
            class="reload-button accent-bg"
            :title="loading ? 'Loading...' : 'Refresh sensors'"
        >
            <i :class="loading ? 'bi bi-hourglass-split' : 'bi bi-arrow-clockwise'"></i>
        </button>
    </div>
</template>

<style scoped>
.sensor-list {
    padding: 0;
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
</style>