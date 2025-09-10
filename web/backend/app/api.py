import re
from typing import Optional
import fastapi
import logging
import time
import os

import psycopg

from models import LatestReadingProps, ReadingsProps, SensorProps

API_KEY = os.getenv("API_KEY")
MAX_LATENCY = 86400  # 24 hours in seconds

logger = logging.getLogger(__name__)

api = fastapi.APIRouter(prefix="/api")

def verify_token(token: str):
    if token != API_KEY:
        raise fastapi.HTTPException(status_code=401, detail="Invalid or missing authorization token")

def verify_mac(mac: str):
    mac_regex = r"^[0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5}$"
    if not re.match(mac_regex, mac):
        raise fastapi.HTTPException(status_code=400, detail="Invalid MAC address format")

def verify_period(period: Optional[int]):
    if period is not None and (period <= 0 or period > time.time()):
        raise fastapi.HTTPException(status_code=400, detail="Invalid period value")

@api.get("/ping", responses={200: {"description": "Pong"}})
async def get_ping():
    return {"message": "Pong"}

@api.post("/readings", responses={
    200: {"description": "Readings accepted"},
    400: {"description": "Invalid request"},
    401: {"description": "Unauthorized"}
})
async def post_readings(
    request: fastapi.Request, 
    readings: ReadingsProps, 
    mac: str = fastapi.Header(..., description="The MAC address of the sensor"), 
    authorization: str = fastapi.Header(..., description="Bearer token for authorization")
):
    """
    Endpoint to receive sensor readings from sensor nodes.
    """
    try:
        scheme, token = authorization.split()
    except ValueError:
        raise fastapi.HTTPException(status_code=401, detail="Malformed header")
    if scheme.lower() != "bearer":
        raise fastapi.HTTPException(status_code=401, detail="Invalid auth scheme")
    verify_token(token)

    db: psycopg.AsyncConnection = request.app.state.db
    if mac is None:
        return fastapi.Response(status_code=400, content="Missing X-MAC-Address header")
    # Check if MAC address is valid (basic check)
    verify_mac(mac)
    # Insert readings into the database
    async with db.transaction():
        try:
            async with db.cursor() as cur:
                # Make sure the sensor exists
                await cur.execute("INSERT INTO sensors (mac) VALUES (%s) ON CONFLICT (mac) DO NOTHING", (mac,))
                # Insert each reading
                for i in range(len(readings.timestamps)):
                    delta_t = readings.timestamps[i] - readings.now
                    now = int(time.time())
                    absolute_timestamp = now + delta_t
                    await cur.execute(
                        """INSERT INTO readings (mac, timestamp, humidity, temperature, battery)
                        VALUES (%s, to_timestamp(%s), %s, %s, %s)
                        ON CONFLICT (mac, timestamp) DO NOTHING""",
                        (mac, absolute_timestamp, readings.humidity[i], readings.temperature[i], readings.battery[i])
                    )
        except Exception as e:
            logger.error(f"Error inserting readings: {e}")
            raise e

    return fastapi.Response(status_code=200)

@api.get("/readings", responses={
    200: {"model": ReadingsProps}, 
    400: {"description": "Invalid request"}, 
    404: {"description": "No readings found"}
})
async def get_readings(
    request: fastapi.Request, 
    mac: str = fastapi.Query(..., description="The MAC address of the sensor"),
    period: int = fastapi.Query(None, description="The time period in seconds to look back from the current time, omitting this retrieves all available data")
):
    """
    Get sensor readings for a specific MAC address and time period.
    """
    verify_mac(mac)
    verify_period(period)
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor(row_factory=psycopg.rows.dict_row) as cur:  
        if period is None:
            await cur.execute(
                """SELECT mac, EXTRACT(EPOCH FROM timestamp) as timestamp, humidity, temperature, battery 
                   FROM readings 
                   WHERE mac = %s
                   ORDER BY timestamp DESC""",
                (mac,)
            )
        else:
            await cur.execute(
                """SELECT mac, EXTRACT(EPOCH FROM timestamp) as timestamp, humidity, temperature, battery 
                   FROM readings 
                   WHERE mac = %s AND timestamp >= to_timestamp(%s)
                   ORDER BY timestamp DESC""",
                (mac, int(time.time()) - period)
            )

        rows = await cur.fetchall()
        if not rows:
            raise fastapi.HTTPException(status_code=404, detail="No readings found for the specified MAC address and period")
        timestamps = [int(row['timestamp']) for row in rows]
        humidity = [row['humidity'] for row in rows]
        temperature = [row['temperature'] for row in rows]
        battery = [row['battery'] for row in rows]
        now = int(time.time())
        return {
            "timestamps": timestamps,
            "humidity": humidity,
            "temperature": temperature,
            "battery": battery,
            "now": now
        }
    
@api.get(f"/sensors", response_model=list[SensorProps])
async def get_sensors(request: fastapi.Request):
    """
    Get all sensors with their latest readings (if available).
    """
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor(row_factory=psycopg.rows.dict_row) as cur:
        await cur.execute("""
            SELECT s.mac, s.name, 
                   EXTRACT(EPOCH FROM r.timestamp) AS timestamp, 
                   r.humidity, r.temperature, r.battery
            FROM sensors s
            LEFT JOIN LATERAL (
                SELECT * FROM readings 
                WHERE readings.mac = s.mac 
                ORDER BY readings.timestamp DESC 
                LIMIT 1
            ) r ON true
            ORDER BY s.mac
        """)
        rows = await cur.fetchall()
        result = []
        for row in rows:
            latest_reading = None
            if row['timestamp'] is not None:
                latest_reading = LatestReadingProps(
                    timestamp=int(row['timestamp']),
                    humidity=row['humidity'],
                    temperature=row['temperature'],
                    battery=row['battery']
                )
            
            result.append(SensorProps(
                mac=row['mac'],
                name=row['name'],
                online=(int(time.time()) - int(row['timestamp']) <= MAX_LATENCY) if row['timestamp'] is not None else False,
                latest_reading=latest_reading
            ))
        return result