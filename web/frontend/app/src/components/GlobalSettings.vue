<script setup lang="ts">
import { ref, onMounted, computed, watch } from 'vue'
import type { GlobalSettings } from '@/types/settings'
import { isNullOrUndef } from 'chart.js/helpers'

const loading = ref(true)
const saving = ref(false)
const error = ref('')
const success = ref('')

// Form data
const settings = ref<GlobalSettings>({
  sync_time: [12, 0],
  battery_warning_threshold: 3.3,
  battery_critical_threshold: 3.0,
  max_latency: 86400, // default 24 hours
})

// Computed properties for form inputs
const syncTimeFormatted = computed({
  get: () => {
    const [hour, minute] = settings.value.sync_time
    return `${hour.toString().padStart(2, '0')}:${minute.toString().padStart(2, '0')}`
  },
  set: (value: string) => {
    const [hour, minute] = value.split(':').map(Number)
    if (isNullOrUndef(hour) || isNullOrUndef(minute)) return
    settings.value.sync_time = [hour, minute]
  }
})

// max_latency with value and unit selector
const latencyValue = ref(1)
const latencyUnit = ref('hours')

const maxLatencySeconds = computed({
  get: () => {
    const multipliers = {
      'minutes': 60,
      'hours': 3600,
      'days': 86400
    }
    return latencyValue.value * multipliers[latencyUnit.value as keyof typeof multipliers]
  },
  set: (seconds: number) => {
    settings.value.max_latency = seconds
    // Update the display values based on the most appropriate unit
    if (seconds >= 86400 && seconds % 86400 === 0) {
      latencyValue.value = seconds / 86400
      latencyUnit.value = 'days'
    } else if (seconds >= 3600 && seconds % 3600 === 0) {
      latencyValue.value = seconds / 3600
      latencyUnit.value = 'hours'
    } else {
      latencyValue.value = seconds / 60
      latencyUnit.value = 'minutes'
    }
  }
})

// Watch for changes to sync with settings
watch([latencyValue, latencyUnit], () => {
  settings.value.max_latency = maxLatencySeconds.value
})

// Initialize from settings when loaded
const initializeLatencyFromSettings = () => {
  const seconds = settings.value.max_latency
  if (seconds >= 86400 && seconds % 86400 === 0) {
    latencyValue.value = seconds / 86400
    latencyUnit.value = 'days'
  } else if (seconds >= 3600 && seconds % 3600 === 0) {
    latencyValue.value = seconds / 3600
    latencyUnit.value = 'hours'
  } else {
    latencyValue.value = Math.max(1, seconds / 60)
    latencyUnit.value = 'minutes'
  }
}

const loadSettings = async () => {
  try {
    loading.value = true
    error.value = ''
    
    const response = await fetch('/api/settings')
    if (!response.ok) {
      throw new Error(`Failed to load settings: ${response.statusText}`)
    }
    
    const data = await response.json()
    settings.value = data
    initializeLatencyFromSettings()
  } catch (err) {
    error.value = 'Failed to load settings'
    console.error('Error loading settings:', err)
  } finally {
    loading.value = false
  }
}

const saveSettings = async () => {
  try {
    saving.value = true
    error.value = ''
    success.value = ''
    
    const response = await fetch('/api/settings', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(settings.value),
    })
    
    if (!response.ok) {
      throw new Error(`Failed to save settings: ${response.statusText}`)
    }
    
    success.value = 'Settings saved successfully!'
    setTimeout(() => {
      success.value = ''
    }, 3000)
  } catch (err) {
    error.value = 'Failed to save settings'
    console.error('Error saving settings:', err)
  } finally {
    saving.value = false
  }
}

onMounted(loadSettings)
</script>

<template>
  <div class="py-4 max-w-6xl mx-auto">
    <div class="header-section mb-4">
      <div class="text-center">
        <h1 class="text-3xl font-semibold text-heading m-0 mb-1">Global Settings</h1>
      </div>
    </div>

    <div v-if="loading" class="loading-state">
      <div class="loading-spinner"></div>
      <p>Loading settings...</p>
    </div>
    
    <div v-else class="settings-form">
      <!-- Error Message -->
      <div v-if="error" class="alert alert-error">
        <i class="bi bi-exclamation-triangle"></i>
        {{ error }}
      </div>
      
      <!-- Success Message -->
      <div v-if="success" class="alert alert-success">
        <i class="bi bi-check-circle"></i>
        {{ success }}
      </div>

      <!-- Sync Settings Card -->
      <div class="card">
        <div class="card-header">
          <h2 class="card-title">
            <i class="bi bi-clock"></i>
            Sync Settings
          </h2>
        </div>
        <div class="card-content">
          <div class="form-group">
            <label for="sync-time" class="form-label">Daily Sync Time</label>
            <input
              id="sync-time"
              v-model="syncTimeFormatted"
              type="time"
              class="form-input"
            />
            <p class="form-help">
              The time when sensors should synchronize their data daily. Sensors will wake up at this time to send readings.
            </p>
          </div>

          <div class="form-group">
            <label class="form-label">Maximum Latency</label>
            <div class="latency-selector">
              <input
                v-model.number="latencyValue"
                type="number"
                min="1"
                max="168"
                class="form-input latency-value"
              />
              <select
                v-model="latencyUnit"
                class="form-input latency-unit"
              >
                <option value="minutes">minutes</option>
                <option value="hours">hours</option>
                <option value="days">days</option>
              </select>
            </div>
            <p class="form-help">
              How long to wait before considering a sensor offline. It should be &geq; to the sync interval.
            </p>
          </div>
        </div>
      </div>

      <!-- Battery Settings Card -->
      <div class="card">
        <div class="card-header">
          <h2 class="card-title">
            <i class="bi bi-battery-half"></i>
            Battery Thresholds
          </h2>
        </div>
        <div class="card-content">
          <div class="form-group">
            <label for="battery-warning" class="form-label">Warning Threshold (V)</label>
            <input
              id="battery-warning"
              v-model.number="settings.battery_warning_threshold"
              type="number"
              min="2.5"
              max="4.2"
              step="0.1"
              class="form-input"
            />
            <p class="form-help">
              Battery voltage level that triggers a low battery warning. Typical range: 3.0-3.5V.
            </p>
          </div>

          <div class="form-group">
            <label for="battery-critical" class="form-label">Critical Threshold (V)</label>
            <input
              id="battery-critical"
              v-model.number="settings.battery_critical_threshold"
              type="number"
              min="2.0"
              max="4.0"
              step="0.1"
              class="form-input"
            />
            <p class="form-help">
              Battery voltage level that triggers a critical battery alert. Should be lower than warning threshold. Typical range: 2.8-3.2V.
            </p>
          </div>
          
          <!-- Validation warning -->
          <div v-if="settings.battery_critical_threshold >= settings.battery_warning_threshold" class="alert alert-warning">
            <i class="bi bi-exclamation-triangle"></i>
            Critical threshold should be lower than warning threshold
          </div>
        </div>
      </div>

      <!-- Save Button -->
      <div class="save-section">
        <button 
          @click="saveSettings" 
          :disabled="saving || settings.battery_critical_threshold >= settings.battery_warning_threshold"
          class="btn btn-primary btn-with-icon btn-large"
        >
          <i class="bi bi-save"></i>
          <span v-if="saving">Saving...</span>
          <span v-else>Save Settings</span>
        </button>
      </div>
    </div>
  </div>
</template>

<style scoped>

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 3rem;
  text-align: center;
}

.loading-spinner {
  width: 2rem;
  height: 2rem;
  border: 3px solid var(--color-border);
  border-top: 3px solid var(--color-accent-1);
  border-radius: 50%;
  animation: spin 1s linear infinite;
  margin-bottom: 1rem;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.settings-form {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

/* Alert Styles */
.alert {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 1rem;
  border-radius: 0.5rem;
  font-weight: 500;
  margin-bottom: 1rem;
}

.alert-error {
  background-color: rgba(255, 45, 45, 0.1);
  color: var(--color-error-1);
  border: 1px solid rgba(255, 45, 45, 0.3);
}

.alert-success {
  background-color: rgba(95, 206, 93, 0.1);
  color: var(--color-info-1);
  border: 1px solid rgba(95, 206, 93, 0.3);
}

.alert-warning {
  background-color: rgba(255, 129, 40, 0.1);
  color: var(--color-warning-1);
  border: 1px solid rgba(255, 129, 40, 0.3);
}

/* Card Styles */
.card {
  background: var(--color-background);
  border: 1px solid var(--color-border);
  border-radius: 0.75rem;
  overflow: hidden;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  padding: 0;
}

.card-header {
  border-radius: 5px 5px 0 0;
  padding: 1.5rem;
  border-bottom: 1px solid var(--color-border);
  background: var(--color-background-soft);
}

.card-title {
  margin: 0;
  font-size: 1.25rem;
  font-weight: 600;
  color: var(--color-heading);
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.card-content {
  padding: 1.5rem;
}

/* Form Styles */
.form-group {
  margin-bottom: 1.5rem;
}

.form-group:last-child {
  margin-bottom: 0;
}

.form-label {
  display: block;
  margin-bottom: 0.5rem;
  font-weight: 500;
  color: var(--color-text);
}

.form-input {
  width: 100%;
  padding: 0.75rem;
  border: 1px solid var(--color-border);
  border-radius: 0.5rem;
  background: var(--color-background);
  color: var(--color-text);
  font-size: 1rem;
  transition: border-color 0.2s ease, box-shadow 0.2s ease;
}

.form-input:focus {
  outline: none;
  border-color: var(--color-accent-1);
  box-shadow: 0 0 0 3px rgba(95, 206, 93, 0.1);
}

.form-input:invalid {
  border-color: var(--color-error-1);
}

/* Number input spinner styles for dark theme */
.form-input[type="number"] {
  position: relative;
}

.form-input[type="number"]::-webkit-outer-spin-button,
.form-input[type="number"]::-webkit-inner-spin-button {
  -webkit-appearance: inner-spin-button;
  opacity: 1;
  height: 100%;
  cursor: pointer;
}

/* Dark theme specific styling */
@media (prefers-color-scheme: dark) {
  .form-input[type="number"]::-webkit-outer-spin-button,
  .form-input[type="number"]::-webkit-inner-spin-button {
    filter: invert(1) brightness(0.8);
  }
  
  .form-input[type="number"] {
    color-scheme: dark;
  }
}

.form-help {
  margin-top: 0.5rem;
  font-size: 0.875rem;
  color: var(--color-text);
  opacity: 0.7;
  line-height: 1.4;
}

/* Latency Selector Styles */
.latency-selector {
  display: flex;
  gap: 0.5rem;
  align-items: stretch;
}

.latency-value {
  flex: 1;
  min-width: 60px;
}

.latency-unit {
  flex: 1;
  min-width: 80px;
}

/* Button Styles */
.save-section {
  display: flex;
  justify-content: center;
  padding: 1rem 0;
}

.btn-large {
  padding: 1rem 2rem;
  font-size: 1.1rem;
  min-width: 200px;
}

.btn-with-icon {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

/* Responsive Design */
@media (max-width: 768px) {
  .card-header,
  .card-content {
    padding: 1rem;
  }

  .latency-selector {
    flex-direction: row;
    gap: 0.5rem;
  }

  .btn-large {
    width: 100%;
    min-width: auto;
  }
}
</style>