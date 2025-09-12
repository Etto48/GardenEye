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

// Expose the fetchSensors function to parent component
defineExpose({
    fetchSensors,
    loading
})
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
        
        <div v-else-if="sensors.length === 0" class="empty-state-large">
            <div class="empty-icon-large">
                <i class="bi bi-wifi-off"></i>
            </div>
            <p class="empty-text-large">No sensors found</p>
        </div>
        
        <div v-else class="grid grid-auto-fill mobile-grid-cols-1 mobile-gap-md">
            <SensorCard v-for="sensor in sensors" :key="sensor.mac" :sensor="sensor" />
        </div>
    </div>
</template>

<style scoped>
.sensor-list {
    padding: 0;
}
</style>