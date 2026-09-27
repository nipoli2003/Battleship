#!/usr/bin/env bash
set -e # Stop immediately if any step fails

# Usage: ./gitpush.sh "your commit message"
# Stages, commits, and pushes the CURRENT branch (not a hardcoded "main").
# Refuses to push straight to main - everyone should work on a feature
# branch and merge into main via a Pull Request on GitHub.

if [ -z "$1" ]; then
    echo "Error: no commit message provided."
    echo "Usage: ./gitpush.sh \"your commit message\""
    exit 1
fi

branch=$(git rev-parse --abbrev-ref HEAD)

if [ "$branch" = "main" ] || [ "$branch" = "master" ]; then
    echo "Error: you're on '$branch'."
    echo "Don't commit/push directly to $branch in a shared repo."
    echo "Create a feature branch instead, e.g.:"
    echo "  git checkout -b yourname/short-description"
    exit 1
fi

echo "==> Staging changes on branch '$branch'..."
git add .

echo "==> Committing..."
git commit -m "$1"

echo "==> Syncing with origin/$branch before pushing..."
git pull --rebase origin "$branch" 2>/dev/null || echo "    (no remote branch yet, skipping pull)"

echo "==> Pushing to origin/$branch..."
git push -u origin "$branch"

echo "==> Done! Open a Pull Request on GitHub to merge '$branch' into main."
