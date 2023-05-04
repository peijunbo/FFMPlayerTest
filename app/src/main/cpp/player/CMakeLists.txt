aux_source_directory(./src PLAYER_SRC_LIST)
add_library(
        player
        SHARED
        ${PLAYER_SRC_LIST}
)
target_include_directories(
        player
        PRIVATE
        ./include
)
target_link_libraries(
        player
        utils
)