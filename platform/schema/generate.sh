#!/bin/bash
set -ex
# One generation script for all languages

FLATC="${ESTATE_NATIVE_DEPS_DIR}/flatbuffers/cmake-build-release/flatc"

generate() {
    local gen_dir=$1
    local schema_dir=$2
    local lang=$3
    local gen_file=$4
    local clean_first=$5
    shift 5
    local other_options=$@

    mkdir -p ${gen_dir}
    gen_dir=$(readlink -f ${gen_dir})

    if [ "${clean_first}" = true ]; then
        #cleanup
        pushd ${gen_dir}
        for f in `find . -name \*${gen_file}`
        do
            echo "removed ${f}"
            rm -f $f
        done
        popd >> /dev/null
    fi

    #generate
    # shellcheck disable=SC2045
    for f in `ls ${schema_dir}/*.fbs`
    do
        ${FLATC} ${other_options} --${lang} -o gen_temp $f
    done

    #place
    pushd gen_temp || exit
    for f in `find . -name \*${gen_file}`
    do
        echo "moved ${f}"
        mv ${f} ${gen_dir}/
    done
    popd >> /dev/null
    rm -rf gen_temp
}


# C++
generate "../native/lib/runtime/include/estate/runtime/protocol" "runtime/client" "cpp" "_generated.h" true --cpp-std 'c++17'
generate "../native/lib/runtime/include/estate/runtime/protocol" "runtime/internal" "cpp" "_generated.h" false --cpp-std 'c++17'
generate "../native/lib/runtime/include/estate/runtime/protocol" "runtime" "cpp" "_generated.h" false --cpp-std 'c++17'

# C#
generate "../dotnet/Jayne.Protocol" "runtime/internal" "csharp" ".cs" true
generate "../dotnet/Jayne.Protocol" "runtime/client" "csharp" ".cs" false
generate "../dotnet/Jayne.Protocol" "runtime" "csharp" ".cs" false

# TypeScript (Tools)
generate "${ESTATE_FRAMEWORK_DIR}/tools/src/protocol" "runtime" "ts" ".ts" true
pushd "${ESTATE_FRAMEWORK_DIR}/tools/src/protocol"
sed -i "s/proto';/proto\.js';/g" *.ts
popd

# TypeScript (Client)
generate "${ESTATE_FRAMEWORK_DIR}/client/src/internal/protocol" "runtime/client" "ts" ".ts" true
generate "${ESTATE_FRAMEWORK_DIR}/client/src/internal/protocol" "runtime" "ts" ".ts" false
pushd "${ESTATE_FRAMEWORK_DIR}/client/src/internal/protocol"
sed -i "s/proto';/proto\.js';/g" *.ts
popd
