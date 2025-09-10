export interface LatestReading {
  timestamp: number
  humidity: number
  temperature: number
  battery: number
}

export interface Sensor {
  mac: string
  name?: string
  online: boolean
  latest_reading?: LatestReading
}
