#!/bin/bash

source_path="./src"

reaperapi_header_path=$(find . -name reaper_plugin_functions.h | head -n 1)

function_names=$(grep -o 'REAPERAPI_WANT_[a-zA-Z0-9_]*' "$reaperapi_header_path")
function_names=${function_names//REAPERAPI_WANT_REAPERAPI_MINIMAL/}

for function in $function_names; do
    function=${function//REAPERAPI_WANT_/}
    if [ -z "$function" ]; then
        continue
    fi
    if grep -q -r -h  "$function" --exclude=reaper_plugin\*.h --include=\*.cpp --include=\*.h "$source_path"; then
        echo "#define REAPERAPI_WANT_${function}"
    fi
done