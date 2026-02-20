#!/usr/bin/bash

set -e

cd `dirname $0`

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest flatpak.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo ../build/.flatpak-repo