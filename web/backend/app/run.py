import uvicorn
import logging
import os
import colorlog


PORT = int(os.getenv("BACKEND_PORT", "8000"))
MODE = os.getenv("MODE", "development")

if __name__ == "__main__":
    # Create custom logging config for uvicorn
    log_config = {
        "version": 1,
        "disable_existing_loggers": False,
        "formatters": {
            "colored": {
                "()": colorlog.ColoredFormatter,
                "format": "%(log_color)s%(levelname)-8s%(reset)s %(cyan)s%(name)s:%(reset)s %(message)s",
                "log_colors": {
                    'DEBUG': 'cyan',
                    'INFO': 'green',
                    'WARNING': 'yellow',
                    'ERROR': 'red',
                    'CRITICAL': 'red,bg_white',
                }
            },
        },
        "handlers": {
            "default": {
                "formatter": "colored",
                "class": "logging.StreamHandler",
                "stream": "ext://sys.stdout",
            },
        },
        "root": {
            "level": "INFO",
            "handlers": ["default"],
        },
        "loggers": {
            # Main uvicorn logger - for general server messages
            "uvicorn": {
                "handlers": ["default"], 
                "level": "INFO", 
                "propagate": False
            },
            # Uvicorn error logger - handles startup/shutdown messages (despite the name)
            "uvicorn.error": {
                "handlers": ["default"], 
                "level": "INFO", 
                "propagate": False
            },
            # Uvicorn access logger - handles HTTP request logs
            "uvicorn.access": {
                "handlers": ["default"], 
                "level": "INFO", 
                "propagate": False
            },
        },
    }
    
    uvicorn.run(
        "app:app", 
        host="0.0.0.0", 
        port=PORT, 
        reload=(MODE == "development"),
        reload_delay=1,
        log_config=log_config
    )
