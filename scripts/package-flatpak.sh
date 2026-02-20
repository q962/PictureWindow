#!/usr/bin/bash

set -e

cd `dirname $0`

git ls-files --recurse-submodules .. | tar caf archive.tar -T-

flatpak-builder --state-dir=../build/.flatpak-builder ../build/flatpak-build-dir flatpak.yml  --force-clean --user --install --disable-updates --repo=../build/.flatpak-repo "$@"
flatpak install -u -y --reinstall `flatpak info -o io.github.q962.PictureWindow` io.github.q962.PictureWindow.Locale//test

flatpak build-bundle ../build/.flatpak-repo io.github.q962.PictureWindow.flatpak  io.github.q962.PictureWindow test