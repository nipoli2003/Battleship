#!/usr/bin/env bash
set -e # Stop immediately if any step fails

# Usage: ./gitpush.sh "your commit message"

if [ -z "$1" ]; then
    echo "Error: no commit message provided."
    echo "Usage: ./gitpush.sh \"your commit message\""
    exit 1
fi

echo "==> Staging changes..."
git add .

echo "==> Committing..."
git commit -m "$1"

echo "==> Pushing to origin main..."
git push origin main

echo "==> Done!"