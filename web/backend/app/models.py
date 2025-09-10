from typing import Optional
import pydantic

class ReadingsProps(pydantic.BaseModel):
    timestamps: list[int]
    humidity: list[float]
    temperature: list[float]
    battery: list[float]
    now: int

    @pydantic.model_validator(mode="after")
    def check_list_lengths(self):
        lists = [self.timestamps, self.humidity, self.temperature, self.battery]
        lengths = [len(lst) for lst in lists]
        if len(set(lengths)) > 1:
            raise ValueError('All list fields (timestamps, humidity, temperature, battery) must have the same length')
        return self

class SensorProps(pydantic.BaseModel):
    mac: str
    online: bool
    name: Optional[str] = None
    latest_reading: Optional['LatestReadingProps'] = None

class LatestReadingProps(pydantic.BaseModel):
    timestamp: int
    humidity: float
    temperature: float
    battery: float