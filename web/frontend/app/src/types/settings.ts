export interface GlobalSettings {
  sync_time: [number, number] // [hour, minute]
  battery_warning_threshold: number
  battery_critical_threshold: number
  max_latency: number
}