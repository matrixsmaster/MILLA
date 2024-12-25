#!/bin/bash
set -e

BUILDIR="build-Desktop-Release"

sudo mkdir -p /opt/milla/share/icons
sudo cp -av "$BUILDIR/bin" /opt/milla/
sudo cp -av "$BUILDIR/share" /opt/milla/
sudo cp -av milla-view/milla*.png /opt/milla/share/icons/
sudo cp 3rdparty/* /opt/milla/share/

[ "z$1" = "zupdate" ] && exit 0

cd /opt/milla/share
sudo cat colors_a* > colors.7z
sudo 7zr e colors.7z
sudo rm -vf colors_a* colors.7z

cd -
mkdir -p ~/.local/share/applications
cp -av milla-view/milla-image-viewer.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications
