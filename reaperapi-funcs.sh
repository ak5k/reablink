#!/bin/bash
source_path="$1"

reaperapi_header_path=$(find . -name reaper_plugin_functions.h | head -n 1)

function_names=$(grep -o 'REAPERAPI_WANT_[a-zA-Z0-9_]*' "$reaperapi_header_path" | sort | uniq)
function_names=${function_names//REAPERAPI_WANT_REAPERAPI_MINIMAL/}

res=""

echo "// working"

for function in $function_names; do
    function=${function//REAPERAPI_WANT_/}
    if [ -z "$function" ]; then
        continue
    fi
    if grep -q -r -h "$function" --exclude=reaper_plugin\*.h --include=\*.cpp --include=\*.h --include=\*.hpp  "$source_path"; then
        # res+="#define REAPERAPI_WANT_${function}\n"
        res+="REQUIRED_API(${function}),\n"
    fi
done

res=${res%,*}${res##*,}

echo -e "$res"
