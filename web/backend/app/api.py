import re
from typing import Optional
import fastapi
from fastapi import File, UploadFile, HTTPException
from fastapi.responses import FileResponse
import logging
import time
import os
import io
from pathlib import Path

import psycopg
import psycopg.rows

from models import InfoProps, LatestReadingProps, ReadingsProps, SensorProps, SensorSettingsProps

API_KEY = os.getenv("API_KEY")
MAX_LATENCY = 86400  # 24 hours in seconds
WARNING_CHARGE_VOLTAGE = 3.3  # Voltage below which battery is considered low
CRITICAL_CHARGE_VOLTAGE = 3.0  # Voltage below which battery is considered critical

UPLOADS_PATH = os.getenv("UPLOADS_PATH", "/uploads")
PHOTOS_DIR = Path(f"{UPLOADS_PATH}/photos")

logger = logging.getLogger(__name__)

# Ensure photos directory exists
PHOTOS_DIR.mkdir(exist_ok=True)

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

@api.post("/register", responses={200: {"description": "Success"}})
async def post_register(request: fastapi.Request, mac: str = fastapi.Header(..., description="The MAC address of the sensor"), authorization: str = fastapi.Header(..., description="Bearer token for authorization")):
    """
    Endpoint to register a new sensor node.
    Does nothing if the sensor is already registered.
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
    async with db.cursor() as cur:
        await cur.execute("INSERT INTO sensors (mac) VALUES (%s) ON CONFLICT (mac) DO NOTHING", (mac,))
        await db.commit()
    return fastapi.Response(status_code=200)

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
    try:
        async with db.cursor() as cur:
            # Make sure the sensor exists
            await cur.execute("INSERT INTO sensors (mac) VALUES (%s) ON CONFLICT (mac) DO NOTHING", (mac,))
            # Insert each reading
            for i in range(len(readings.timestamps)):
                delta_t = readings.timestamps[i] - readings.now
                absolute_timestamp = int(time.time()) + delta_t
                await cur.execute(
                    """INSERT INTO readings (mac, timestamp, humidity, temperature, battery)
                    VALUES (%s, to_timestamp(%s), %s, %s, %s)
                    ON CONFLICT (mac, timestamp) DO NOTHING""",
                    (mac, absolute_timestamp, readings.humidity[i], readings.temperature[i], readings.battery[i])
                )
        # Explicitly commit the changes
        await db.commit()
    except Exception as e:
        logger.error(f"Error inserting readings: {e}")
        # Rollback in case of error
        await db.rollback()
        raise fastapi.HTTPException(status_code=500, detail=f"Database error: {str(e)}")

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
                """WITH bounds AS (
                    SELECT
                        min(timestamp) AS start_time,
                        max(timestamp) AS end_time,
                        count(*) AS total_points
                    FROM readings
                    WHERE mac = %s
                ),
                bucket AS (
                    SELECT
                        GREATEST(
                            (EXTRACT(EPOCH FROM (end_time - start_time)) / 100)::int,
                            1
                        ) * INTERVAL '1 second' AS bucket_width,
                        start_time,
                        end_time
                    FROM bounds
                )
                SELECT
                    r.mac,
                    EXTRACT(EPOCH FROM time_bucket(bucket.bucket_width, r.timestamp, bucket.start_time)) AS timestamp,
                    avg(r.humidity)    AS humidity,
                    avg(r.temperature) AS temperature,
                    avg(r.battery)     AS battery
                FROM readings r
                CROSS JOIN bucket
                WHERE r.mac = %s
                AND r.timestamp BETWEEN bucket.start_time AND bucket.end_time
                GROUP BY r.mac, time_bucket(bucket.bucket_width, r.timestamp, bucket.start_time)
                ORDER BY timestamp DESC;""",
                (mac, mac)
            )
        else:
            await cur.execute(
                """WITH bounds AS (
                    SELECT
                        min(timestamp) AS start_time,
                        max(timestamp) AS end_time,
                        count(*) AS total_points
                    FROM readings
                    WHERE mac = %s
                    AND timestamp >= to_timestamp(%s)
                ),
                bucket AS (
                    SELECT
                        GREATEST(
                            (EXTRACT(EPOCH FROM (end_time - start_time)) / 100)::int,
                            1
                        ) * INTERVAL '1 second' AS bucket_width,
                        start_time,
                        end_time
                    FROM bounds
                )
                SELECT
                    r.mac,
                    EXTRACT(EPOCH FROM time_bucket(bucket.bucket_width, r.timestamp, bucket.start_time)) AS timestamp,
                    avg(r.humidity)    AS humidity,
                    avg(r.temperature) AS temperature,
                    avg(r.battery)     AS battery
                FROM readings r
                CROSS JOIN bucket
                WHERE r.mac = %s
                AND r.timestamp BETWEEN bucket.start_time AND bucket.end_time
                GROUP BY r.mac, time_bucket(bucket.bucket_width, r.timestamp, bucket.start_time)
                ORDER BY timestamp DESC;
                """,
                (mac, int(time.time()) - period, mac)
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
    
@api.delete("/readings", responses={
    200: {"description": "Readings deleted successfully"},
    400: {"description": "Invalid request"},
    404: {"description": "No readings found"}
})
async def delete_readings(
    request: fastapi.Request,
    mac: str = fastapi.Query(..., description="The MAC address of the sensor")
):
    """
    Delete all readings for a specific MAC address.
    """
    verify_mac(mac)
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor() as cur:
        await cur.execute("SELECT 1 FROM readings WHERE mac = %s LIMIT 1", (mac,))
        if await cur.fetchone() is None:
            raise fastapi.HTTPException(status_code=404, detail="No readings found for the specified MAC address")
        await cur.execute("DELETE FROM readings WHERE mac = %s", (mac,))
        await db.commit()
    return fastapi.Response(status_code=200)

@api.get(f"/sensors", response_model=list[SensorProps] | SensorProps)
async def get_sensors(
    request: fastapi.Request,
    mac: Optional[str] = fastapi.Query(None, description="The MAC address of the sensor to filter by")
):
    """
    Get all sensors with their latest readings (if available).
    """
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor(row_factory=psycopg.rows.dict_row) as cur:
        if mac:
            verify_mac(mac)
            await cur.execute("""
                SELECT s.mac, s.name, s.has_photo,
                       EXTRACT(EPOCH FROM r.timestamp) AS timestamp, 
                       r.humidity, r.temperature, r.battery
                FROM sensors s
                LEFT JOIN LATERAL (
                    SELECT * FROM readings 
                    WHERE readings.mac = s.mac 
                    ORDER BY readings.timestamp DESC 
                    LIMIT 1
                ) r ON true
                WHERE s.mac = %s
                ORDER BY s.mac
            """, (mac,))
        else:
            await cur.execute("""
                SELECT s.mac, s.name, s.has_photo,
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
                has_photo=row['has_photo'],
                online=(int(time.time()) - int(row['timestamp']) <= MAX_LATENCY) if row['timestamp'] is not None else False,
                latest_reading=latest_reading
            ))
        if mac is not None and len(result) == 0:
            return fastapi.Response(status_code=404, content="Sensor not found")
        elif mac is not None:
            return result[0]
        else:
            return result

@api.delete("/sensors/{mac}", responses={
    200: {"description": "Sensor deleted successfully"},
    404: {"description": "Sensor not found"}
})
async def delete_sensor(
    request: fastapi.Request,
    mac: str = fastapi.Path(..., description="The MAC address of the sensor to delete")
):
    """
    Delete a specific sensor and all its associated readings.
    """
    verify_mac(mac)
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor() as cur:
        await cur.execute("SELECT 1 FROM sensors WHERE mac = %s LIMIT 1", (mac,))
        if await cur.fetchone() is None:
            raise fastapi.HTTPException(status_code=404, detail="Sensor not found")
        # Delete associated photo if it exists
        mac_clean = mac.replace(":", "").lower()
        for photo_file in PHOTOS_DIR.glob(f"{mac_clean}.*"):
            photo_file.unlink()
        await cur.execute("DELETE FROM sensors WHERE mac = %s", (mac,))
        # Cascade delete will remove associated readings
        await db.commit()
    return fastapi.Response(status_code=200)

@api.post(
    "/sensors/{mac}/settings",      
)
async def post_sensor_settings(
    request: fastapi.Request,
    mac: str = fastapi.Path(..., description="The MAC address of the sensor"),
    settings: SensorSettingsProps = fastapi.Body(..., description="The sensor settings to update")
):
    """
    Update settings for a specific sensor.
    """
    verify_mac(mac)
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor() as cur:
        # Check if sensor exists
        await cur.execute("SELECT mac FROM sensors WHERE mac = %s", (mac,))
        if await cur.fetchone() is None:
            raise HTTPException(status_code=404, detail="Sensor not found")
        
        # Update sensor settings
        await cur.execute("UPDATE sensors SET name = %s WHERE mac = %s", (settings.name, mac))
        
        await db.commit()

    return fastapi.Response(status_code=200)

@api.get(f"/info", response_model=list[InfoProps])
async def get_info(request: fastapi.Request):
    """
    Get information about the system.
    """
    result = []
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor(row_factory=psycopg.rows.dict_row) as cur:
        online_sensors = 0
        offline_sensors = 0
        low_battery_nodes = 0
        critical_battery_nodes = 0
        
        # Get latest info for each sensor
        await cur.execute("""
            SELECT s.mac, s.name, s.has_photo,
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
        latest_readings = await cur.fetchall()
        for row in latest_readings:
            if row['timestamp'] is not None:
                if int(time.time()) - int(row['timestamp']) <= MAX_LATENCY:
                    online_sensors += 1
                else:
                    offline_sensors += 1
                
                if row['battery'] < CRITICAL_CHARGE_VOLTAGE:
                    critical_battery_nodes += 1
                elif row['battery'] < WARNING_CHARGE_VOLTAGE:
                    low_battery_nodes += 1
            else:
                offline_sensors += 1  # No readings means offline
        if online_sensors > 0:
            result.append(InfoProps(
                title="Online Sensors",
                content=str(online_sensors),
                level="info"))
        if offline_sensors > 0:
            result.append(InfoProps(
                title="Offline Sensors",
                content=str(offline_sensors),
                level="error"))
        if low_battery_nodes > 0:
            result.append(InfoProps(
                title="Low Battery Sensors",
                content=str(low_battery_nodes),
                level="warning"))
        if critical_battery_nodes > 0:
            result.append(InfoProps(
                title="Critical Battery Sensors",
                content=str(critical_battery_nodes),
                level="error"))
        return result

@api.post("/sensors/{mac}/photo", responses={
    200: {"description": "Photo uploaded successfully"},
    400: {"description": "Invalid request or file format"},
    404: {"description": "Sensor not found"}
})
async def upload_sensor_photo(
    request: fastapi.Request,
    mac: str = fastapi.Path(..., description="The MAC address of the sensor"),
    photo: UploadFile = File(..., description="The photo file to upload"),
):
    """
    Upload a photo for a specific sensor.
    """
    verify_mac(mac)
    og_mac = mac
    mac = mac.replace(":", "").lower()

    # Check if sensor exists
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor() as cur:
        await cur.execute("SELECT mac FROM sensors WHERE mac = %s", (og_mac,))
        if await cur.fetchone() is None:
            raise HTTPException(status_code=404, detail="Sensor not found")

    # Validate file type
    if not photo.content_type or not photo.content_type.startswith('image/'):
        raise HTTPException(status_code=400, detail="File must be an image")

    # Get file extension from content type
    content_type_to_ext = {
        'image/jpeg': '.jpg',
        'image/jpg': '.jpg',
        'image/png': '.png',
        'image/webp': '.webp',
        'image/gif': '.gif'
    }
    
    file_ext = content_type_to_ext.get(photo.content_type)
    if not file_ext:
        raise HTTPException(status_code=400, detail="Unsupported image format")

    # Save file with MAC address as filename
    filename = f"{mac}{file_ext}"
    file_path = PHOTOS_DIR / filename

    try:
        # Remove existing photo if it exists
        for existing_file in PHOTOS_DIR.glob(f"{mac}.*"):
            existing_file.unlink()

        # Save new photo
        with open(file_path, "wb") as buffer:
            content = await photo.read()
            buffer.write(content)

        # Update database to set has_photo = true
        async with db.cursor() as cur:
            await cur.execute("UPDATE sensors SET has_photo = true WHERE mac = %s", (og_mac,))
            await db.commit()

        return {"message": "Photo uploaded successfully", "filename": filename}

    except Exception as e:
        logger.error(f"Error uploading photo for sensor {og_mac}: {e}")
        raise HTTPException(status_code=500, detail="Failed to upload photo")

@api.get("/sensors/{mac}/photo", responses={
    200: {"description": "Photo file", "content": {"image/*": {}}},
    404: {"description": "Photo not found"}
})
async def get_sensor_photo(mac: str):
    """
    Get the photo for a specific sensor.
    """
    verify_mac(mac)
    mac = mac.replace(":", "").lower()

    # Find photo file for this MAC address
    photo_files = list(PHOTOS_DIR.glob(f"{mac}.*"))
    
    if not photo_files:
        raise HTTPException(status_code=404, detail="Photo not found")

    photo_path = photo_files[0]  # Take the first match
    
    if not photo_path.exists():
        raise HTTPException(status_code=404, detail="Photo file not found")

    # Determine media type from file extension
    ext_to_media_type = {
        '.jpg': 'image/jpeg',
        '.jpeg': 'image/jpeg', 
        '.png': 'image/png',
        '.webp': 'image/webp',
        '.gif': 'image/gif'
    }
    
    media_type = ext_to_media_type.get(photo_path.suffix.lower(), 'image/jpeg')
    
    return FileResponse(photo_path, media_type=media_type)

@api.delete("/sensors/{mac}/photo", responses={
    200: {"description": "Photo deleted successfully"},
    404: {"description": "Photo not found"}
})
async def delete_sensor_photo(
    request: fastapi.Request,
    mac: str,
):
    """
    Delete the photo for a specific sensor.
    """
    verify_mac(mac)
    og_mac = mac
    mac = mac.replace(":", "").lower()

    # Find and delete photo file
    photo_files = list(PHOTOS_DIR.glob(f"{mac}.*"))
    
    if not photo_files:
        raise HTTPException(status_code=404, detail="Photo not found")

    try:
        # Delete all photo files for this MAC (in case there are multiple)
        for photo_path in photo_files:
            photo_path.unlink()

        # Update database to set has_photo = false
        db: psycopg.AsyncConnection = request.app.state.db
        async with db.cursor() as cur:
            await cur.execute("UPDATE sensors SET has_photo = false WHERE mac = %s", (og_mac,))
            await db.commit()

        return {"message": "Photo deleted successfully"}

    except Exception as e:
        logger.error(f"Error deleting photo for sensor {og_mac}: {e}")
        raise HTTPException(status_code=500, detail="Failed to delete photo")
    
@api.get("/readings/download", responses={
    200: {"description": "CSV file"},
    400: {"description": "Invalid request"},
    404: {"description": "No readings found"}})
async def get_readings_download(
    request: fastapi.Request,
    mac: str = fastapi.Query(..., description="The MAC address of the sensor"),
    period: int = fastapi.Query(None, description="The time period in seconds to look back from the current time, omitting this retrieves all available data")
):
    """
    Download sensor readings as a CSV file for a specific MAC address and time period.
    """
    verify_mac(mac)
    verify_period(period)
    db: psycopg.AsyncConnection = request.app.state.db
    async with db.cursor(row_factory=psycopg.rows.dict_row) as cur:  
        if period is None:
            await cur.execute(
                """SELECT mac, to_char(timestamp, 'YYYY-MM-DD HH24:MI:SS') as timestamp, humidity, temperature, battery 
                   FROM readings 
                   WHERE mac = %s
                   ORDER BY timestamp DESC""",
                (mac,)
            )
        else:
            await cur.execute(
                """SELECT mac, to_char(timestamp, 'YYYY-MM-DD HH24:MI:SS') as timestamp, humidity, temperature, battery 
                   FROM readings 
                   WHERE mac = %s AND timestamp >= to_timestamp(%s)
                   ORDER BY timestamp DESC""",
                (mac, int(time.time()) - period)
            )

        rows = await cur.fetchall()
        if not rows:
            raise fastapi.HTTPException(status_code=404, detail="No readings found for the specified MAC address and period")
        
        # Create CSV content
        csv_content = "mac,timestamp,humidity,temperature,battery\n"
        for row in rows:
            csv_content += f"{row['mac']},{row['timestamp']},{row['humidity']},{row['temperature']},{row['battery']}\n"
        
        io_stream = io.StringIO(csv_content)
        headers = {
            'Content-Disposition': f'attachment; filename="{mac.replace(":", "").lower()}_readings.csv"'
        }
        return fastapi.responses.StreamingResponse(io_stream, media_type="text/csv", headers=headers)
