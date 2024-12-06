#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

rm -rf ./build
mkdir build

dotnet publish -c {{BUILD_TYPE}} -o ./build/bin {{ESTATE_PLATFORM_DIR}}/dotnet/Jayne/Jayne.csproj
dotnet publish -c {{BUILD_TYPE}} -o ./obj/test_data_gen {{ESTATE_PLATFORM_DIR}}/dotnet/Jayne.TestDataGenerator/Jayne.TestDataGenerator.csproj

export DOTNET_ROOT=/usr/lib/dotnet

./obj/test_data_gen/Jayne.TestDataGenerator {{ESTATE_PLATFORM_DIR}}/test_data ./build/generated_test_data

rm -rf ./obj