import fastapi
from fastapi.concurrency import asynccontextmanager
import psycopg
import os
import logging
from api import api

logger = logging.getLogger(__name__)

DB_HOST = os.getenv("DB_HOST", "localhost")
DB_PORT = os.getenv("DB_PORT", "5432")
DB_USER = os.getenv("DB_USER", "root")
DB_PASSWORD = os.getenv("DB_PASSWORD", "password")

@asynccontextmanager
async def lifespan(app: fastapi.FastAPI):
    app.state.db = await psycopg.AsyncConnection.connect(
        f"host={DB_HOST} port={DB_PORT} user={DB_USER} password={DB_PASSWORD}"
    )
    logger.info("Database connection established.")
    yield
    await app.state.db.close()
    logger.info("Database connection closed.")

app = fastapi.FastAPI(lifespan=lifespan)
app.include_router(api)