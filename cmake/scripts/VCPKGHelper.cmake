# Check if the requested sanitizers are supported, and set the appropriated
# flags.
function(config_msvc)
    if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
    endif()

    if(NOT DEFINED VCPKG_TARGET_TRIPLET)

        string(FIND ${CMAKE_GENERATOR} "Visual Studio" VISUAL_STUDIO_IN_CMAKE_GENERATOR)

        if(VISUAL_STUDIO_IN_CMAKE_GENERATOR GREATER -1)

            if (DEFINED CMAKE_GENERATOR_PLATFORM AND
                CMAKE_GENERATOR_PLATFORM STREQUAL "" AND
                DEFINED ENV{VCPKG_DEFAULT_TRIPLET})

                set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET}
                    CACHE STRING "")

                string(FIND ${VCPKG_TARGET_TRIPLET} "x64-" X64_IN_VCPKG_TARGET_TRIPLET)
                if(X64_IN_VCPKG_TARGET_TRIPLET GREATER -1)
                    set(CMAKE_GENERATOR_PLATFORM "x64"
                        CACHE STRING "" FORCE)
                else()
                    string(FIND ${VCPKG_TARGET_TRIPLET} "x86-" X86_IN_VCPKG_TARGET_TRIPLET)
                    if(X86_IN_VCPKG_TARGET_TRIPLET GREATER -1)
                        set(CMAKE_GENERATOR_PLATFORM "Win32"
                            CACHE STRING "" FORCE)
                    endif()
                endif()

            else()

                if(DEFINED CMAKE_GENERATOR_PLATFORM)
                    if (${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64")
                        set(VCPKG_TARGET_TRIPLET "x64-windows"
                            CACHE STRING "")
                    elseif (${CMAKE_GENERATOR_PLATFORM} STREQUAL "Win32")
                        set(VCPKG_TARGET_TRIPLET "x86-windows"
                            CACHE STRING "")
                    endif()
                else()
                    string(FIND ${CMAKE_GENERATOR} "Win64" WIN64_IN_CMAKE_GENERATOR)
                    if (WIN64_IN_CMAKE_GENERATOR GREATER -1)
                        set(VCPKG_TARGET_TRIPLET "x64-windows"
                            CACHE STRING "")
                    else()
                        set(VCPKG_TARGET_TRIPLET "x86-windows"
                            CACHE STRING "")
                    endif()
                endif()

                if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
                    string(FIND $ENV{VCPKG_DEFAULT_TRIPLET} "-static" STATIC_IN_VCPKG_DEFAULT_TRIPLET)
                    if(STATIC_IN_VCPKG_DEFAULT_TRIPLET GREATER -1)
                        set(VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}-static"
                            CACHE STRING "" FORCE)
                    endif()
                endif()

            endif()

        elseif(DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
            set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET}
                CACHE STRING "")
        endif()

    endif()
endfunction()

