<script setup lang="ts">
import { ref, onMounted, computed, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import {
  Chart,
  type ChartConfiguration,
  type ChartData,
  LineElement,
  PointElement,
  LinearScale,
  TimeScale,
  Title,
  Tooltip,
  Legend,
  LineController,
  Filler,
} from 'chart.js'
import 'chartjs-adapter-date-fns'
import type { Sensor } from '@/types/sensor'
import type { SensorReadings } from '@/types/readings'

// Register Chart.js components
Chart.register(
  LineController,
  LineElement,
  PointElement,
  LinearScale,
  TimeScale,
  Title,
  Tooltip,
  Legend,
  Filler,
)

// Time periods in seconds
const TIME_PERIODS = {
  '1 Day': 24 * 60 * 60,
  '3 Days': 3 * 24 * 60 * 60,
  '1 Week': 7 * 24 * 60 * 60,
  '1 Month': 30 * 24 * 60 * 60,
  '3 Months': 3 * 30 * 24 * 60 * 60,
  '6 Months': 6 * 30 * 24 * 60 * 60,
  '1 Year': 365 * 24 * 60 * 60,
  Max: null,
} as const

type TimePeriodKey = keyof typeof TIME_PERIODS

// Router and route
const route = useRoute()
const router = useRouter()

// Reactive state
const sensor = ref<Sensor | null>(null)
const readings = ref<SensorReadings | null>(null)
const selectedPeriod = ref<TimePeriodKey>('1 Day')
const loading = ref(true)
const error = ref<string | null>(null)
const chartInstances = ref<{ [key: string]: Chart }>({})

// Chart canvas refs
const temperatureCanvas = ref<HTMLCanvasElement | null>(null)
const humidityCanvas = ref<HTMLCanvasElement | null>(null)
const batteryCanvas = ref<HTMLCanvasElement | null>(null)

// Get computed CSS color values for current theme
const getThemeColors = () => {
  if (typeof window === 'undefined') {
    // Default to light theme colors if window is not available
    return {
      text: '#2c3e50',
      border: 'rgba(60, 60, 60, 0.12)',
      background: '#f8f8f8',
    }
  }

  // Check if we're in dark mode
  const isDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches

  return {
    text: isDark ? '#ffffff' : '#2c3e50',
    border: isDark ? 'rgba(84, 84, 84, 0.48)' : 'rgba(60, 60, 60, 0.12)',
    background: isDark ? '#222222' : '#f8f8f8',
  }
}

// Chart colors for different metrics (consistent across views)
const METRIC_COLORS = {
  temperature: {
    border: 'rgb(255, 99, 132)', // Red
    background: 'rgba(255, 99, 132, 0.3)',
  },
  humidity: {
    border: 'rgb(54, 162, 235)', // Blue
    background: 'rgba(54, 162, 235, 0.3)',
  },
  battery: {
    border: 'rgb(75, 192, 192)', // Teal
    background: 'rgba(75, 192, 192, 0.3)',
  },
}

// Get sensor MAC from route query
const sensorMac = computed(() => route.query.mac as string)

// Display name for sensor
const displayName = computed(() => {
  if (!sensor.value) return sensorMac.value
  return sensor.value.name || sensor.value.mac
})

// Fetch sensor info
const fetchSensor = async (mac: string) => {
  try {
    const response = await fetch('/api/sensors')
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`)
    }
    const sensors = await response.json()
    sensor.value = sensors.find((s: Sensor) => s.mac === mac) || null
  } catch (err) {
    console.error('Error fetching sensor info:', err)
    // Continue without sensor info - we can still show readings using MAC
  }
}

// Fetch readings for the sensor
const fetchReadings = async (mac: string): Promise<SensorReadings> => {
  const period = TIME_PERIODS[selectedPeriod.value]
  const url = new URL('/api/readings', window.location.origin)
  url.searchParams.append('mac', mac)
  if (period !== null) {
    url.searchParams.append('period', period.toString())
  }

  const response = await fetch(url.toString())
  if (!response.ok) {
    if (response.status === 404) {
      // No readings for this sensor
      return {
        mac,
        timestamps: [],
        humidity: [],
        temperature: [],
        battery: [],
        now: Date.now() / 1000,
      }
    }
    throw new Error(`HTTP error! status: ${response.status}`)
  }

  const data = await response.json()
  return {
    ...data,
    mac,
    name: sensor.value?.name,
  }
}

// Fetch all data
const fetchData = async () => {
  if (!sensorMac.value) {
    error.value = 'No sensor specified'
    loading.value = false
    return
  }

  try {
    loading.value = true
    error.value = null

    await fetchSensor(sensorMac.value)
    const sensorReadings = await fetchReadings(sensorMac.value)
    readings.value = sensorReadings
  } catch (err) {
    error.value = err instanceof Error ? err.message : 'Failed to fetch readings'
    console.error('Error fetching data:', err)
  } finally {
    loading.value = false
  }
}

// Prepare chart data for a specific metric
const prepareChartData = (metric: 'temperature' | 'humidity' | 'battery'): ChartData<'line'> => {
  if (!readings.value || readings.value.timestamps.length === 0) {
    return { datasets: [] }
  }

  const data = readings.value.timestamps.map((timestamp, i) => ({
    x: timestamp * 1000, // Convert to milliseconds for Chart.js
    y: readings.value![metric][i],
  }))

  // // If metric is battery, scale voltage to %
  // if (metric === 'battery') {
  //     const minVoltage = 4.3 // Minimum expected voltage
  //     const maxVoltage = 7.6 // Maximum expected voltage
  //     data.forEach(point => {
  //         point.y = ((point.y - minVoltage) / (maxVoltage - minVoltage)) * 100
  //         if (point.y < 0) point.y = 0
  //         if (point.y > 100) point.y = 100
  //     })
  // }

  // Sort data by timestamp
  data.sort((a, b) => a.x - b.x)

  return {
    datasets: [
      {
        label: metric.charAt(0).toUpperCase() + metric.slice(1),
        data,
        borderColor: METRIC_COLORS[metric].border,
        backgroundColor: METRIC_COLORS[metric].background,
        borderWidth: 2,
        tension: 0.1,
        fill: 'origin', // Fill area under the line to the x-axis
        pointRadius: 3,
        pointHoverRadius: 5,
      },
    ],
  }
}

// Chart configuration for individual charts
const getChartConfig = (
  metric: 'temperature' | 'humidity' | 'battery',
): ChartConfiguration<'line'> => {
  const units = {
    temperature: '°C',
    humidity: '%',
    battery: 'V',
  }

  const themeColors = getThemeColors()

  return {
    type: 'line',
    data: prepareChartData(metric),
    options: {
      responsive: true,
      maintainAspectRatio: false,
      layout: {
        padding: 0,
      },
      plugins: {
        legend: {
          display: false, // Hide legend for single sensor
        },
      },
      scales: {
        x: {
          type: 'time',
          time: {
            displayFormats: {
              hour: 'MMM dd HH:mm',
              day: 'MMM dd',
              week: 'MMM dd',
              month: 'MMM yyyy',
            },
          },
          ticks: {
            color: themeColors.text,
          },
          grid: {
            color: themeColors.border,
          },
        },
        y: {
          beginAtZero: false,
          title: {
            display: true,
            text: units[metric],
            color: themeColors.text,
          },
          ticks: {
            color: themeColors.text,
          },
          grid: {
            color: themeColors.border,
          },
        },
      },
      interaction: {
        intersect: false,
        mode: 'index',
      },
    },
  }
}

// Create/update charts
const updateCharts = () => {
  // Destroy existing charts
  Object.values(chartInstances.value).forEach((chart) => chart.destroy())
  chartInstances.value = {}

  if (!readings.value || readings.value.timestamps.length === 0) {
    return
  }

  // Separate views - three charts
  if (!temperatureCanvas.value || !humidityCanvas.value || !batteryCanvas.value) {
    return
  }
  chartInstances.value.temperature = new Chart(
    temperatureCanvas.value,
    getChartConfig('temperature'),
  )
  chartInstances.value.humidity = new Chart(humidityCanvas.value, getChartConfig('humidity'))
  chartInstances.value.battery = new Chart(batteryCanvas.value, getChartConfig('battery'))
}

// Handle period change
const changePeriod = async (period: TimePeriodKey) => {
  selectedPeriod.value = period
  await fetchData()
  updateCharts()
}

// Go back to dashboard
const goBack = () => {
  router.push('/')
}

// Download CSV
const downloadCSV = async () => {
  if (!sensorMac.value) return

  try {
    const period = TIME_PERIODS[selectedPeriod.value]
    const url = new URL('/api/readings/download', window.location.origin)
    url.searchParams.append('mac', sensorMac.value)
    if (period !== null) {
      url.searchParams.append('period', period.toString())
    }

    const response = await fetch(url.toString())

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`)
    }

    // Get the blob data
    const blob = await response.blob()

    // Create download link
    const downloadUrl = window.URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = downloadUrl

    // Generate filename
    const filename = `${sensorMac.value.replace(/:/g, '').toLowerCase()}_readings.csv`
    link.download = filename

    // Trigger download
    document.body.appendChild(link)
    link.click()

    // Cleanup
    document.body.removeChild(link)
    window.URL.revokeObjectURL(downloadUrl)
  } catch (err) {
    error.value = err instanceof Error ? err.message : 'Failed to download CSV'
    console.error('Error downloading CSV:', err)
  }
}

// Watch for route changes
watch(
  () => route.query.mac,
  async (newMac) => {
    if (newMac) {
      await fetchData()
      updateCharts()
    }
  },
  { immediate: false },
)

// Lifecycle
onMounted(async () => {
  // Set up theme change listener
  if (window.matchMedia) {
    const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')
    mediaQuery.addEventListener('change', () => {
      updateCharts()
    })
  }

  await fetchData()
  // Use nextTick to ensure canvas elements are rendered
  await new Promise((resolve) => setTimeout(resolve, 0))
  updateCharts()
})

// Computed properties
const hasData = computed(() => readings.value && readings.value.timestamps.length > 0)
</script>

<template>
  <div class="py-4 max-w-6xl mx-auto">
    <div class="header-section mb-4">
      <div class="text-center">
        <h1 class="text-3xl font-semibold text-heading m-0 mb-1">Sensor History</h1>
        <p class="text-base font-medium text-soft m-0">{{ displayName }}</p>
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

    <!-- Time period selector -->
    <div class="card time-selector mb-3">
      <h3 class="text-xl font-medium text-heading m-0 mb-3">Time Range</h3>
      <div class="button-group">
        <button
          v-for="(period, key) in TIME_PERIODS"
          :key="key"
          @click="changePeriod(key as TimePeriodKey)"
          :class="['btn', 'btn-outline', 'btn-small', { 'btn-accent': selectedPeriod === key }]"
          :disabled="loading"
        >
          {{ key }}
        </button>
      </div>
    </div>

    <!-- Loading state -->
    <div v-if="loading" class="loading-container">
      <div class="loading-spinner">
        <i class="bi bi-hourglass-split"></i>
      </div>
      <p>Loading sensor data...</p>
    </div>

    <!-- Error state -->
    <div v-else-if="error" class="error-container">
      <div class="error-icon">
        <i class="bi bi-exclamation-triangle"></i>
      </div>
      <p>{{ error }}</p>
      <button @click="fetchData" class="btn btn-accent btn-small">Retry</button>
    </div>

    <!-- No data state -->
    <div v-else-if="!hasData" class="no-data-container">
      <div class="no-data-icon">
        <i class="bi bi-graph-down"></i>
      </div>
      <p>No sensor data available for the selected time period</p>
    </div>

    <!-- Charts -->
    <div v-else class="card charts-container">
      <!-- Humidity Chart -->
      <div class="chart-container">
        <div class="chart-header">
          <i class="bi bi-droplet-half chart-icon"></i>
          <h3 class="chart-title">Humidity</h3>
        </div>
        <div class="canvas-container"><canvas ref="humidityCanvas"></canvas></div>
      </div>

      <!-- Temperature Chart -->
      <div class="chart-container">
        <div class="chart-header">
          <i class="bi bi-thermometer-half chart-icon"></i>
          <h3 class="chart-title">Temperature</h3>
        </div>
        <div class="canvas-container"><canvas ref="temperatureCanvas"></canvas></div>
      </div>

      <!-- Battery Chart -->
      <div class="chart-container">
        <div class="chart-header">
          <i class="bi bi-battery-half chart-icon"></i>
          <h3 class="chart-title">Battery Voltage</h3>
        </div>
        <div class="canvas-container"><canvas ref="batteryCanvas"></canvas></div>
      </div>
    </div>

    <div class="flex flex-wrap mt-3 grid-gap-lg">
      <!-- Download CSV button -->
      <button
        @click="downloadCSV"
        :disabled="loading || !hasData"
        class="btn btn-outline btn-small btn-with-icon"
        title="Download sensor data as CSV"
      >
        <i class="bi bi-download"></i>
        Download CSV
      </button>
    </div>
    <!-- Refresh button -->
    <button
      @click="fetchData().then(() => updateCharts())"
      :disabled="loading"
      class="reload-button accent-bg"
      :title="loading ? 'Loading...' : 'Refresh data'"
    >
      <i :class="loading ? 'bi bi-hourglass-split' : 'bi bi-arrow-clockwise'"></i>
    </button>
  </div>
</template>

<style scoped>
.canvas-container {
  position: relative;
  width: 100%;
  height: 450px;
}

.time-selector {
  background: var(--color-background-soft);
  border: 1px solid var(--color-border);
  border-radius: 8px;
  padding: 1rem;
}

.button-group {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
  justify-content: center;
}

.loading-container,
.error-container,
.no-data-container {
  text-align: center;
  padding: 3rem 1rem;
  color: var(--color-text-soft);
}

.loading-spinner,
.error-icon,
.no-data-icon {
  font-size: 3rem;
  margin-bottom: 1rem;
  color: var(--color-accent);
}

.error-icon {
  color: var(--color-error);
}

.charts-container {
  display: flex;
  flex-direction: column;
  padding: 0;
}

.chart-container {
  padding: 10px;
}

.chart-header {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  margin-bottom: 1rem;
  padding-bottom: 0.5rem;
  padding: 1rem 1rem 0 1rem;
  border-top: 1px solid var(--color-border);
}

.chart-container:first-of-type .chart-header {
  border-top: none;
}

.chart-icon {
  font-size: 1.25rem;
  color: var(--color-accent);
}

.chart-title {
  margin: 0;
  font-size: 1.1rem;
  font-weight: 600;
  color: var(--color-heading);
}

/* Responsive design */
@media (max-width: 768px) {
  .canvas-container {
    height: 300px;
  }
}
</style>
