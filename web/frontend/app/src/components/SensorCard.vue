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

const viewSettings = (sensor: Sensor) => {
    router.push({ name: 'sensor-settings', query: { mac: sensor.mac } })
}

const getPhotoUrl = (sensor: Sensor) => {
    return sensor.has_photo ? `/api/sensors/${sensor.mac}/photo` : undefined
}

const handleImageError = (event: Event) => {
    const target = event.target as HTMLImageElement
    if (target) {
        target.style.display = 'none'
    }
}
</script>

<template>
    <div class="sensor-card card hover-lift animate-fade-in">
        <div v-if="sensor.has_photo" class="card-photo">
            <img 
                :src="getPhotoUrl(sensor)" 
                :alt="`Photo of ${displayName(sensor)}`"
                class="sensor-photo"
                @error="handleImageError"
            />
        </div>
        <div v-else class="card-photo card-photo-placeholder">
            <div class="photo-placeholder">
                <i class="bi bi-camera"></i>
            </div>
        </div>
        
        <div class="card-header">
            <div v-if="sensor.online" class="status-indicator online animate-pulse" title="Online"></div>
            <div v-else class="status-indicator offline animate-pulse" title="Offline"></div>
            <div class="header-content">
                <h3 class="card-title">{{ displayName(sensor) }}</h3>
                <span class="chip">{{ sensor.mac }}</span>
            </div>
        </div>
        <div class="card-content">
            <div v-if="sensor.latest_reading" class="readings flex flex-col gap-sm">
                <div class="detail-item">
                    <span class="detail-label">
                        <i class="bi bi-droplet-half"></i>
                        Humidity:
                    </span>
                    <span class="detail-value">{{ sensor.latest_reading.humidity.toFixed(1) }}%</span>
                </div>
                <div class="detail-item">
                    <span class="detail-label">
                        <i class="bi bi-thermometer-half"></i>
                        Temperature:
                    </span>
                    <span class="detail-value">{{ sensor.latest_reading.temperature.toFixed(1) }}°C</span>
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
            <div class="button-group">
                <button 
                    @click="viewHistory(sensor)" 
                    class="btn btn-accent btn-small btn-with-icon"
                    :title="`View history for ${displayName(sensor)}`"
                >
                    <i class="bi bi-graph-up"></i>
                    View History
                </button>
                <button 
                    @click="viewSettings(sensor)" 
                    class="btn btn-accent btn-small btn-with-icon"
                    :title="`Settings for ${displayName(sensor)}`"
                >
                    <i class="bi bi-gear"></i>
                    Settings
                </button>
            </div>
        </div>
        <div v-else class="card-footer">
            <button 
                @click="viewSettings(sensor)" 
                class="btn btn-accent btn-small btn-full-width btn-with-icon"
                :title="`Settings for ${displayName(sensor)}`"
            >
                <i class="bi bi-gear"></i>
                Settings
            </button>
        </div>
    </div>
</template>

<style scoped>
.detail-label i {
    margin-right: 0.5rem;
    color: inherit;
}

.card.sensor-card {
    padding: 0;
}

.card-photo {
    width: 100%;
    height: 200px;
    overflow: hidden;
    position: relative;
    border-radius: 0.5rem 0.5rem 0 0;
}

.card-photo.card-photo-placeholder {
    display: flex;
    align-items: center;
    justify-content: center;
    background: var(--color-background);
}

.sensor-photo {
    width: 100%;
    height: 100%;
    object-fit: cover;
    display: block;
}

.photo-placeholder {
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--color-text-secondary);
    font-size: 2rem;
}

.card-header {
    padding: 1rem;
    padding-bottom: 0;
}

.header-content {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
}

.card-content {
    padding: 0 1rem 1rem 1rem;
}

.card-footer {
    margin-top: 1rem;
    padding: 0 1rem 1rem 1rem;
    border-top: 1px solid var(--color-border);
    padding-top: 1rem;
}

.button-group {
    display: flex;
    gap: 0.5rem;
}

.button-group .btn {
    flex: 1;
}

@media (max-width: 768px) {
    .sensor-card {
        padding: 1rem;
    }
}
</style>
