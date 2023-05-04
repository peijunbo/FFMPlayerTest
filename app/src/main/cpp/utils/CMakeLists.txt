aux_source_directory(. UTILS_SRC_LIST)
add_library(
        utils
        SHARED
        ${UTILS_SRC_LIST}
)