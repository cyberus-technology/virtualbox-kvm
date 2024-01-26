#/bin/bash -xe
kmk_sed \
    -e 's/CLANGXXMACHO/CLANGCCMACHO/g' \
    -e '/^TOOL_CLANGCCMACHO_LD/s/clang++/clang/' \
    -e 's/for building C++ code/for building C code/' \
    --output CLANGCCMACHO.kmk CLANGXXMACHO.kmk
