#!/bin/sh

if [ "$MODE" = "production" ]; then
  npm install
  npm run build
  npm install -g serve
  serve -s dist -l ${FRONTEND_PORT}
else
  npm run dev -- --host --port ${FRONTEND_PORT}
fi