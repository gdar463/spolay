if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CROSSCOMPILING)
    set(OPENSSL_ZST_URL "https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-openssl-3.6.4-1-any.pkg.tar.zst")
    set(OPENSSL_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/openssl_mingw")

    if(NOT EXISTS "${OPENSSL_DOWNLOAD_DIR}/mingw64/lib/libcrypto.a")
        message(STATUS "Downloading OpenSSL MinGW binaries...")
        file(MAKE_DIRECTORY "${OPENSSL_DOWNLOAD_DIR}")

        file(DOWNLOAD "${OPENSSL_ZST_URL}"
            "${OPENSSL_DOWNLOAD_DIR}/openssl.pkg.tar.zst" SHOW_PROGRESS)

        message(STATUS "Extracting OpenSSL...")
        execute_process(
        COMMAND tar --zstd -xf openssl.pkg.tar.zst
        WORKING_DIRECTORY "${OPENSSL_DOWNLOAD_DIR}"
        RESULT_VARIABLE extract_result
    )

        if(NOT extract_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract OpenSSL. Ensure 'zstd' is installed on your Linux host.")
        endif()
    endif()

    set(OPENSSL_ROOT_DIR "${OPENSSL_DOWNLOAD_DIR}/mingw64")
    set(OPENSSL_USE_STATIC_LIBS ON)
endif()
