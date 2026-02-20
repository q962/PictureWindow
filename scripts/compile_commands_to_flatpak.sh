#!env bash

if [ -f /.flatpak-info ]; then
	sdk_root=`grep "runtime-path" /.flatpak-info | cut -d = -f 2`
else
	sdk_root=`flatpak info --user --installation=default org.gnome.Sdk --show-location`/files
fi

sed "s@\"-I\", \"/usr@\"-I\", \"${sdk_root}@g" .vscode/compile_commands.json > .vscode/compile_commands_flatpak.json