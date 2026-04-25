# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-src")
  file(MAKE_DIRECTORY "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-build"
  "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix"
  "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix/tmp"
  "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix/src/harfbuzz-populate-stamp"
  "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix/src"
  "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix/src/harfbuzz-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix/src/harfbuzz-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/pater/Pyt/Tachyon/build-ninja/_deps/harfbuzz-subbuild/harfbuzz-populate-prefix/src/harfbuzz-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
