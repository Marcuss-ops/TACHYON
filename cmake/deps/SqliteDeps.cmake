include_guard(GLOBAL)

if(TACHYON_ENABLE_SQLITE_TELEMETRY)
    if(TACHYON_USE_SYSTEM_DEPS)
        find_package(SQLite3 REQUIRED)
        # Wrap system SQLite3 inside the tachyon_sqlite interface target
        if(NOT TARGET tachyon_sqlite)
            add_library(tachyon_sqlite INTERFACE)
            target_link_libraries(tachyon_sqlite INTERFACE SQLite::SQLite3)
        endif()
    else()
        if(TACHYON_FETCH_DEPS)
            FetchContent_Declare(
                sqlite
                URL "https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip"
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            FetchContent_MakeAvailable(sqlite)
            
            if(NOT TARGET tachyon_sqlite)
                # Build SQLite static library from the fetched amalgamation source
                add_library(tachyon_sqlite STATIC
                    ${sqlite_SOURCE_DIR}/sqlite3.c
                )
                target_include_directories(tachyon_sqlite PUBLIC
                    ${sqlite_SOURCE_DIR}
                )
                target_compile_definitions(tachyon_sqlite PUBLIC
                    SQLITE_THREADSAFE=1
                    SQLITE_OMIT_LOAD_EXTENSION
                )
                # Keep compilation quiet
                if(MSVC)
                    target_compile_options(tachyon_sqlite PRIVATE /wd4244 /wd4100 /wd4018 /wd4090)
                else()
                    target_compile_options(tachyon_sqlite PRIVATE -w)
                endif()
            endif()
        endif()
    endif()
endif()
