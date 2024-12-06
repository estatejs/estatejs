#!/usr/bin/env bash
set -e
rm -f {{ESTATE_PLATFORM_DIR}}/dotnet/Jayne/Version_Generated.cs
cp {{RENDER_AREA_DIR}}/Version_Generated.cs {{ESTATE_PLATFORM_DIR}}/dotnet/Jayne/Version_Generated.cs