export interface Readings {
  timestamps: number[]
  humidity: number[]
  temperature: number[]
  battery: number[]
  now: number
}

export interface SensorReadings extends Readings {
  mac: string
  name?: string
}
