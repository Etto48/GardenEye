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

