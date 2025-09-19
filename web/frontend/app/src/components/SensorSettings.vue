<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import type { Sensor } from '@/types/sensor'

const route = useRoute()
const router = useRouter()

const sensorMac = route.query.mac as string | undefined
const sensor = ref<Sensor | null>(null)
const loading = ref(true)
const error = ref('')
const uploadingPhoto = ref(false)
const photoUploadError = ref('')
const photoFileInput = ref<HTMLInputElement | null>(null)
const deletingSensor = ref(false)
const deleteError = ref('')
const deletingHistory = ref(false)
const deleteHistoryError = ref('')

// Memoized display name for current sensor
const currentDisplayName = computed(() => {
  return sensor.value ? sensor.value.name || sensor.value.mac : ''
})

function goBack() {
  router.back()
}

const saveSensorName = async () => {
  if (!sensor.value) return

  try {
    await fetch(`/api/sensors/${sensor.value.mac}/settings`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ name: sensor.value.name }),
    })
  } catch (err) {
    error.value = 'Failed to save sensor name'
    console.error('Error saving sensor name:', err)
  }
}

const getPhotoUrl = (sensor: Sensor) => {
  return sensor.has_photo ? `/api/sensors/${sensor.mac}/photo?t=${Date.now()}` : undefined
}

const triggerPhotoUpload = () => {
  photoFileInput.value?.click()
}

const handlePhotoUpload = async (event: Event) => {
  const target = event.target as HTMLInputElement
  const file = target.files?.[0]

  if (!file || !sensor.value) return

  // Validate file type
  if (!file.type.startsWith('image/')) {
    photoUploadError.value = 'Please select a valid image file'
    return
  }

  try {
    uploadingPhoto.value = true
    photoUploadError.value = ''

    const formData = new FormData()
    formData.append('photo', file)

    const response = await fetch(`/api/sensors/${sensor.value.mac}/photo`, {
      method: 'POST',
      body: formData,
    })

    if (!response.ok) {
      throw new Error(`Upload failed: ${response.statusText}`)
    }

    // Update sensor to reflect photo upload
    sensor.value.has_photo = true

    // Clear the file input
    target.value = ''
  } catch (err) {
    photoUploadError.value = 'Failed to upload photo'
    console.error('Error uploading photo:', err)
  } finally {
    uploadingPhoto.value = false
  }
}

const deletePhoto = async () => {
  if (!sensor.value || !sensor.value.has_photo) return

  try {
    uploadingPhoto.value = true
    photoUploadError.value = ''

    const response = await fetch(`/api/sensors/${sensor.value.mac}/photo`, {
      method: 'DELETE',
    })

    if (!response.ok) {
      throw new Error(`Delete failed: ${response.statusText}`)
    }

    // Update sensor to reflect photo deletion
    sensor.value.has_photo = false
  } catch (err) {
    photoUploadError.value = 'Failed to delete photo'
    console.error('Error deleting photo:', err)
  } finally {
    uploadingPhoto.value = false
  }
}

const fetchSensorDetails = async (mac: string) => {
  try {
    loading.value = true
    const response = await fetch(`/api/sensors?mac=${mac}`)
    if (!response.ok) {
      throw new Error(`Failed to fetch sensor: ${response.statusText}`)
    }
    const data = await response.json()
    sensor.value = data
  } catch (err) {
    error.value = 'Failed to load sensor details'
    console.error('Error fetching sensor:', err)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  if (!sensorMac) {
    error.value = 'No sensor MAC address provided'
    loading.value = false
    return
  }

  fetchSensorDetails(sensorMac)
})

const deleteHistory = async () => {
  if (!sensor.value) return

  const confirmDelete = confirm(
    `Are you sure you want to delete all history for sensor "${currentDisplayName.value}"? This will permanently remove all readings but keep the sensor itself. This action cannot be undone.`,
  )

  if (!confirmDelete) return

  try {
    deletingHistory.value = true
    deleteHistoryError.value = ''

    const response = await fetch(`/api/readings?mac=${sensor.value.mac}`, {
      method: 'DELETE',
    })

    if (!response.ok) {
      throw new Error(`Delete failed: ${response.statusText}`)
    }

    // Update sensor to reflect no latest reading
    sensor.value.latest_reading = undefined
    sensor.value.online = false
  } catch (err) {
    deleteHistoryError.value = 'Failed to delete sensor history'
    console.error('Error deleting sensor history:', err)
  } finally {
    deletingHistory.value = false
  }
}

const deleteSensor = async () => {
  if (!sensor.value) return

  const confirmDelete = confirm(
    `Are you sure you want to delete sensor "${currentDisplayName.value}"? This will permanently remove all data associated with this sensor and cannot be undone.`,
  )

  if (!confirmDelete) return

  try {
    deletingSensor.value = true
    deleteError.value = ''

    const response = await fetch(`/api/sensors/${sensor.value.mac}`, {
      method: 'DELETE',
    })

    if (!response.ok) {
      throw new Error(`Delete failed: ${response.statusText}`)
    }

    // Redirect back to dashboard after successful deletion
    router.push({ name: 'home' })
  } catch (err) {
    deleteError.value = 'Failed to delete sensor'
    console.error('Error deleting sensor:', err)
  } finally {
    deletingSensor.value = false
  }
}
</script>

<template>
  <div class="settings py-4 max-w-6xl mx-auto">
    <div class="header-section mb-4">
      <div class="text-center">
        <h1 class="text-3xl font-semibold text-heading m-0 mb-1">Sensor Settings</h1>
        <p class="text-base font-medium text-soft m-0">{{ currentDisplayName }}</p>
      </div>
    </div>

    <button
      @click="goBack"
      class="btn btn-outline btn-small btn-with-icon mb-3"
      title="Back to Dashboard"
    >
      <i class="bi bi-arrow-left"></i>
      Back
    </button>

    <div v-if="loading" class="loading-state">
      <div class="spinner"></div>
      <p>Loading sensor details...</p>
    </div>

    <div v-else-if="error" class="error-state">
      <div class="error-icon">
        <i class="bi bi-exclamation-triangle"></i>
      </div>
      <p class="error-text">{{ error }}</p>
    </div>

    <div v-else-if="sensor" class="settings-content">
      <div class="card">
        <div class="card-header">
          <h2 class="card-title">Basic Information</h2>
        </div>
        <div class="card-content">
          <div class="form-group">
            <label for="sensor-mac" class="form-label">MAC Address</label>
            <input id="sensor-mac" type="text" :value="sensor.mac" readonly class="form-input" />
            <p class="form-help">This is the unique identifier for your sensor</p>
          </div>

          <div class="form-group">
            <label for="sensor-name" class="form-label">Display Name</label>
            <input
              ref="nameInputRef"
              id="sensor-name"
              v-model="sensor.name"
              type="text"
              placeholder="Enter a friendly name for this sensor"
              class="form-input"
              autocomplete="off"
              spellcheck="false"
            />
            <p class="form-help">
              Give your sensor a memorable name (e.g., "Living Room Plant", "Garden Tomatoes")
            </p>
          </div>
        </div>
        <div class="card-footer">
          <button @click="saveSensorName" class="btn btn-primary btn-with-icon">
            <i class="bi bi-save"></i>
            Save Changes
          </button>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <h2 class="card-title">Photo</h2>
        </div>
        <div class="card-content">
          <div class="photo-section">
            <div class="photo-preview">
              <img
                v-if="sensor.has_photo"
                :src="getPhotoUrl(sensor)"
                :alt="`Photo of ${currentDisplayName}`"
                class="preview-image"
              />
              <div v-else class="photo-placeholder">
                <i class="bi bi-camera"></i>
                <p>No photo uploaded</p>
              </div>
            </div>

            <div class="photo-actions">
              <input
                ref="photoFileInput"
                type="file"
                accept="image/*"
                @change="handlePhotoUpload"
                style="display: none"
              />

              <button
                @click="triggerPhotoUpload"
                :disabled="uploadingPhoto"
                class="btn btn-primary btn-with-icon"
              >
                <i v-if="uploadingPhoto" class="bi bi-arrow-clockwise spinner"></i>
                <i v-else class="bi bi-upload"></i>
                {{
                  uploadingPhoto
                    ? 'Uploading...'
                    : sensor.has_photo
                      ? 'Replace Photo'
                      : 'Upload Photo'
                }}
              </button>

              <button
                v-if="sensor.has_photo"
                @click="deletePhoto"
                :disabled="uploadingPhoto"
                class="btn btn-secondary btn-with-icon"
              >
                <i class="bi bi-trash"></i>
                Delete Photo
              </button>
            </div>

            <div v-if="photoUploadError" class="error-message">
              <i class="bi bi-exclamation-triangle"></i>
              {{ photoUploadError }}
            </div>

            <p class="form-help">
              Upload a photo to help identify your sensor. Supported formats: JPEG, PNG, WebP, GIF.
            </p>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <h2 class="card-title">Status</h2>
        </div>
        <div class="card-content">
          <div class="status-item">
            <span class="status-label">Connection Status:</span>
            <span v-if="sensor.online" class="status-value online">
              <i class="bi bi-wifi"></i>
              Online
            </span>
            <span v-else class="status-value offline">
              <i class="bi bi-wifi-off"></i>
              Offline
            </span>
          </div>
          <div v-if="sensor.latest_reading" class="status-item">
            <span class="status-label">Last Reading:</span>
            <span class="status-value">
              {{ new Date(sensor.latest_reading.timestamp * 1000).toLocaleString() }}
            </span>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="card-header">
          <h2 class="card-title">Actions</h2>
        </div>
        <div class="card-content">
          <div class="action-buttons">
            <button
              @click="router.push({ name: 'history', query: { mac: sensor.mac } })"
              class="btn btn-accent btn-with-icon"
              :disabled="!sensor.latest_reading"
            >
              <i class="bi bi-graph-up"></i>
              View History
            </button>
          </div>
        </div>
      </div>

      <div class="card danger-zone">
        <div class="card-header">
          <h2 class="card-title danger-title">
            <i class="bi bi-exclamation-triangle"></i>
            Danger Zone
          </h2>
        </div>
        <div class="card-content">
          <div class="danger-content">
            <div class="danger-section">
              <div class="danger-description">
                <h3>Delete Sensor History</h3>
                <p>
                  Permanently delete all readings and data history for this sensor. The sensor
                  itself will remain configured, but all past measurements will be lost forever.
                </p>
              </div>

              <div class="danger-actions">
                <button
                  @click="deleteHistory"
                  :disabled="deletingHistory"
                  class="btn btn-danger btn-with-icon"
                >
                  <i v-if="deletingHistory" class="bi bi-arrow-clockwise spinner"></i>
                  <i v-else class="bi bi-clock-history"></i>
                  {{ deletingHistory ? 'Deleting...' : 'Delete History' }}
                </button>
              </div>

              <div v-if="deleteHistoryError" class="error-message">
                <i class="bi bi-exclamation-triangle"></i>
                {{ deleteHistoryError }}
              </div>
            </div>

            <div class="danger-section">
              <div class="danger-description">
                <h3>Delete Sensor</h3>
                <p>
                  Permanently delete this sensor and all its associated data. This action cannot be
                  undone. All readings, photos, and settings will be lost forever.
                </p>
              </div>

              <div class="danger-actions">
                <button
                  @click="deleteSensor"
                  :disabled="deletingSensor"
                  class="btn btn-danger btn-with-icon"
                >
                  <i v-if="deletingSensor" class="bi bi-arrow-clockwise spinner"></i>
                  <i v-else class="bi bi-trash"></i>
                  {{ deletingSensor ? 'Deleting...' : 'Delete Sensor' }}
                </button>
              </div>

              <div v-if="deleteError" class="error-message">
                <i class="bi bi-exclamation-triangle"></i>
                {{ deleteError }}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.page-header {
  display: flex;
  align-items: center;
  gap: 1rem;
  margin-bottom: 1.5rem;
}

.loading-state {
  text-align: center;
  padding: 3rem 1rem;
}

.spinner {
  width: 40px;
  height: 40px;
  border: 4px solid var(--color-border);
  border-top: 4px solid var(--color-primary);
  border-radius: 50%;
  animation: spin 1s linear infinite;
  margin: 0 auto 1rem;
}

@keyframes spin {
  0% {
    transform: rotate(0deg);
  }
  100% {
    transform: rotate(360deg);
  }
}

.error-state {
  text-align: center;
  padding: 3rem 1rem;
}

.error-icon {
  font-size: 3rem;
  color: var(--color-error);
  margin-bottom: 1rem;
}

.error-text {
  color: var(--color-error);
  margin-bottom: 2rem;
}

.settings-content {
  display: flex;
  flex-direction: column;
  gap: 2rem;
}

.form-group {
  margin-bottom: 1.5rem;
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
  transition: border-color 0.2s ease;
}

.form-input:focus {
  outline: none;
  border-color: var(--color-primary);
  box-shadow: 0 0 0 3px var(--color-primary-alpha);
}

.form-input:read-only {
  background: var(--color-surface-variant);
  cursor: not-allowed;
}

.form-help {
  margin-top: 0.5rem;
  font-size: 0.875rem;
  color: var(--color-text-secondary);
}

.status-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.75rem 0;
  border-bottom: 1px solid var(--color-border);
}

.status-item:last-child {
  border-bottom: none;
}

.status-label {
  font-weight: 500;
  color: var(--color-text);
}

.status-value {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-weight: 500;
}

.status-value.online {
  color: var(--color-success);
}

.status-value.offline {
  color: var(--color-error);
}

.action-buttons {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.photo-section {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

.photo-preview {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 200px;
}

.preview-image {
  max-width: 300px;
  max-height: 200px;
  width: auto;
  height: auto;
  border-radius: 0.5rem;
  border: 2px solid var(--color-border);
  object-fit: contain;
}

.photo-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  padding: 2rem;
  border: 2px dashed var(--color-border);
  border-radius: 0.5rem;
  color: var(--color-text-secondary);
  text-align: center;
  min-height: 200px;
  width: 100%;
  max-width: 300px;
}

.photo-placeholder i {
  font-size: 3rem;
}

.photo-placeholder p {
  margin: 0;
  font-size: 1rem;
}

.photo-actions {
  display: flex;
  flex-direction: column;
  gap: 1rem;
  align-items: center;
}

.error-message {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  color: var(--color-error);
  background: var(--color-error-background, rgba(239, 68, 68, 0.1));
  padding: 0.75rem;
  border-radius: 0.5rem;
  border: 1px solid var(--color-error);
}

.spinner {
  animation: spin 1s linear infinite;
}

.danger-zone {
  border: 2px solid var(--color-error);
  background: var(--color-error-background, rgba(239, 68, 68, 0.05));
}

.danger-title {
  color: var(--color-error);
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.danger-content {
  display: flex;
  flex-direction: column;
  gap: 2rem;
}

.danger-section {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
  padding-bottom: 1.5rem;
  border-bottom: 1px solid var(--color-border);
}

.danger-section:last-child {
  border-bottom: none;
  padding-bottom: 0;
}

.danger-description h3 {
  margin: 0 0 0.5rem 0;
  color: var(--color-error);
  font-size: 1.125rem;
  font-weight: 600;
}

.danger-description p {
  margin: 0;
  color: var(--color-text-secondary);
  line-height: 1.5;
}

.danger-actions {
  display: flex;
  justify-content: flex-start;
}

.btn-danger {
  background: var(--color-error-1);
  color: white;
  border: 1px solid var(--color-error-1);
}

.btn-danger:hover:not(:disabled) {
  background: var(--color-error-2);
  border-color: var(--color-error-2);
}

.btn-danger:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

@media (min-width: 640px) {
  .action-buttons {
    flex-direction: row;
    justify-content: flex-start;
  }

  .photo-actions {
    flex-direction: row;
    justify-content: center;
  }

  .danger-section {
    flex-direction: row;
    justify-content: space-between;
    align-items: flex-start;
  }

  .danger-description {
    flex: 1;
    margin-right: 2rem;
  }

  .danger-actions {
    flex-shrink: 0;
  }
}
</style>
