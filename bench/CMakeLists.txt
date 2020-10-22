add_executable(mustache_exe
        main.cpp
        component_data_storage_bench.cpp
)

target_link_libraries(mustache_exe mustache)
