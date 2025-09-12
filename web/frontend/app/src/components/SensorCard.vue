<script setup lang="ts">
import { useRouter } from 'vue-router'
import type { Sensor } from '@/types/sensor'

defineProps<{
    sensor: Sensor
}>()

const router = useRouter()

const displayName = (sensor: Sensor) => {
    return sensor.name || sensor.mac
}

const formatTimestamp = (timestamp: number) => {
    const now = Date.now()
    const readingTime = timestamp * 1000
    const diffMs = now - readingTime
    
    const seconds = Math.floor(diffMs / 1000)
    const minutes = Math.floor(seconds / 60)
    const hours = Math.floor(minutes / 60)
    const days = Math.floor(hours / 24)
    const weeks = Math.floor(days / 7)
    const months = Math.floor(days / 30)
    const years = Math.floor(days / 365)
    
    if (years > 0) {
        return years === 1 ? '1 year ago' : `${years} years ago`
    } else if (months > 0) {
        return months === 1 ? '1 month ago' : `${months} months ago`
    } else if (weeks > 0) {
        return weeks === 1 ? '1 week ago' : `${weeks} weeks ago`
    } else if (days > 0) {
        return days === 1 ? '1 day ago' : `${days} days ago`
    } else if (hours > 0) {
        return hours === 1 ? '1 hour ago' : `${hours} hours ago`
    } else if (minutes > 0) {
        return minutes === 1 ? '1 minute ago' : `${minutes} minutes ago`
    } else if (seconds > 0) {
        return seconds === 1 ? '1 second ago' : `${seconds} seconds ago`
    } else {
        return 'just now'
    }
}

const formatBattery = (battery: number) => {
    return `${(battery).toFixed(1)}V`
}

const viewHistory = (sensor: Sensor) => {
    router.push({ name: 'history', query: { mac: sensor.mac } })
}
</script>

<template>
    <div class="sensor-card card hover-lift animate-fade-in">
        <div class="card-header">
            <span v-if="sensor.online" class="status-indicator online animate-pulse" title="Online">●</span>
            <span v-else class="status-indicator offline animate-pulse" title="Offline">●</span>
            <h3 class="card-title">{{ displayName(sensor) }}</h3>
            <span class="chip">{{ sensor.mac }}</span>
        </div>
        <div class="card-content">
            <div v-if="sensor.latest_reading" class="readings flex flex-col gap-sm">
                <div class="detail-item">
                    <span class="detail-label">
                        <i class="bi bi-thermometer-half"></i>
                        Temperature:
                    </span>
                    <span class="detail-value">{{ sensor.latest_reading.temperature.toFixed(1) }}°C</span>
                </div>
                <div class="detail-item">
                    <span class="detail-label">
                        <i class="bi bi-droplet-half"></i>
                        Humidity:
                    </span>
                    <span class="detail-value">{{ sensor.latest_reading.humidity.toFixed(1) }}%</span>
                </div>
                <div class="detail-item">
                    <span class="detail-label">
                        <i class="bi bi-battery-half"></i>
                        Battery:
                    </span>
                    <span class="detail-value">{{ formatBattery(sensor.latest_reading.battery) }}</span>
                </div>
                <div class="detail-item">
                    <span class="detail-label">Last Reading:</span>
                    <span class="detail-value timestamp">{{ formatTimestamp(sensor.latest_reading.timestamp) }}</span>
                </div>
            </div>
            <div v-else class="empty-state">
                <div class="empty-icon">
                    <i class="bi bi-database-x"></i>
                </div>
                <p class="empty-text">No readings available</p>
            </div>
        </div>
        <div v-if="sensor.latest_reading" class="card-footer">
            <button 
                @click="viewHistory(sensor)" 
                class="btn btn-accent btn-small btn-full-width btn-with-icon"
                :title="`View history for ${displayName(sensor)}`"
            >
                <i class="bi bi-graph-up"></i>
                View History
            </button>
        </div>
    </div>
</template>

<style scoped>
.detail-label i {
    margin-right: 0.5rem;
    color: inherit;
}

.card-footer {
    margin-top: 1rem;
    padding-top: 1rem;
    border-top: 1px solid var(--color-border);
}

@media (max-width: 768px) {
    .sensor-card {
        padding: 1rem;
    }
}
</style>
