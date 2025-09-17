#!/usr/bin/env bash

set -e

cd $(dirname "$0")
MODE=${1:-dev}
if [ "$MODE" = "dev" ]; then
    DOCKER_COMPOSE="-f docker-compose.yml"
elif [ "$MODE" = "prod" ]; then
    DOCKER_COMPOSE="-f docker-compose.yml -f docker-compose.prod.yml"
elif [ "$MODE" = "rm" ]; then
    DOCKER_COMPOSE="-f docker-compose.yml -f docker-compose.prod.yml"
else
    echo "Invalid MODE: $MODE. Use 'dev' or 'prod'."
    exit 1
fi
ENV_FILES="--env-file public.env --env-file private.env"

function stop() {
    if [ "$MODE" = "dev" ]; then
        docker compose $DOCKER_COMPOSE $ENV_FILES down
    elif [ "$MODE" = "prod" ]; then
        docker compose $DOCKER_COMPOSE $ENV_FILES down
    elif [ "$MODE" = "rm" ]; then
        docker compose $DOCKER_COMPOSE $ENV_FILES down -v --rmi all --remove-orphans
    fi
}

function start() {
    if [ "$MODE" = "dev" ]; then
        docker compose $DOCKER_COMPOSE $ENV_FILES up --build
    elif [ "$MODE" = "prod" ]; then
        docker compose $DOCKER_COMPOSE $ENV_FILES up --build
    elif [ "$MODE" = "rm" ]; then
        # do nothing
        echo "Removing all containers, images, volumes, and networks..."
    fi
}

trap stop EXIT

start