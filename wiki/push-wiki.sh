#!/bin/bash
# Push the wiki/ directory to the GitHub wiki git repo.
# Run this once after creating the first wiki page via the GitHub web UI.
#
# Usage:
#   ./wiki/push-wiki.sh [github-token]
#
# The GitHub token needs 'repo' scope.

set -euo pipefail

REPO="2E0LXY/wsjt-zii"
TOKEN="${1:-${GH_TOKEN:-}}"

if [[ -z "$TOKEN" ]]; then
  echo "Usage: $0 <github-token>"
  echo "Or set GH_TOKEN environment variable"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

TMPDIR="$(mktemp -d)"
trap "rm -rf $TMPDIR" EXIT

echo "Cloning wiki repo..."
git clone "https://2E0LXY:${TOKEN}@github.com/${REPO}.wiki.git" "$TMPDIR/wiki"

echo "Copying wiki pages..."
cp "$SCRIPT_DIR"/*.md "$TMPDIR/wiki/"

cd "$TMPDIR/wiki"
git config user.email "2e0lxy@aprsnet.uk"
git config user.name "2E0LXY"

git add -A
if git diff --cached --quiet; then
  echo "No changes to push."
  exit 0
fi

git commit -m "Update wiki from source repo"
git push origin master

echo "Wiki updated successfully."
echo "View at: https://github.com/${REPO}/wiki"
