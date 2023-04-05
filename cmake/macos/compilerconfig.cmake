# OBS CMake macOS compiler configuration module

# Enable distinction between Clang and AppleClang
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

# Honor visibility presets for all target types (executable, shared, module, static)
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()

include(ccache)
include(compiler_common)
include(simd)

# Set C17 / C++17 standards as required and disable extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set symbols to be hidden by default for C and C++
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

# Add default C and C++ compiler options if Xcode generator is not used
if(NOT XCODE)
  set(_obs_c_options
      # cmake-format: sortable
      -fno-strict-aliasing
      -Wbool-conversion
      -Wcomma
      -Wconstant-conversion
      -Wdeprecated-declarations
      -Wempty-body
      -Wenum-conversion
      -Werror
      -Werror=block-capture-autoreleasing
      -Werror=return-type
      -Wextra
      -Wformat
      -Wformat-security
      -Wfour-char-constants
      -Winfinite-recursion
      -Wint-conversion
      -Wnewline-eof
      -Wno-conversion
      -Wno-float-conversion
      -Wno-implicit-fallthrough
      -Wno-missing-braces
      -Wno-missing-field-initializers
      -Wno-missing-prototypes
      -Wno-semicolon-before-method-body
      -Wno-shadow
      -Wno-sign-conversion
      -Wno-trigraphs
      -Wno-unknown-pragmas
      -Wno-unused-function
      -Wno-unused-label
      -Wnon-literal-null-conversion
      -Wobjc-literal-conversion
      -Wparentheses
      -Wpointer-sign
      -Wquoted-include-in-framework-header
      -Wshorten-64-to-32
      -Wsign-compare
      -Wstrict-prototypes
      -Wswitch
      -Wuninitialized
      -Wunreachable-code
      -Wunused-parameter
      -Wunused-value
      -Wunused-variable
      -Wvla)

  set(_obs_cxx_options
      # cmake-format: sortable
      -Warc-repeated-use-of-weak
      -Wconversion
      -Wdeprecated-implementations
      -Wduplicate-method-match
      -Wfloat-conversion
      -Wfour-char-constants
      -Wimplicit-retain-self
      -Winvalid-offsetof
      -Wmove
      -Wno-arc-maybe-repeated-use-of-weak
      -Wno-exit-time-destructors
      -Wno-implicit-atomic-properties
      -Wno-non-virtual-dtor
      -Wno-objc-interface-ivars
      -Wno-overloaded-virtual
      -Wno-selector
      -Wno-strict-selector-match
      -Wprotocol
      -Wrange-loop-analysis
      -Wshadow
      -Wundeclared-selector)

  # Enable stripping of dead symbols when not building for Debug configuration
  set(_release_configs RelWithDebInfo Release MinSizeRel)
  if(CMAKE_BUILD_TYPE IN_LIST _release_configs)
    add_link_options(LINKER:-dead_strip)
  endif()

  add_compile_options("$<$<COMPILE_LANGUAGE:C>:${_obs_c_options}>"
                      "$<$<COMPILE_LANGUAGE:CXX>:${_obs_c_options} ${_obs_cxx_options}>")

  option(ENABLE_COMPILER_TRACE "Enable clang time-trace (requires Ninja)" OFF)
  mark_as_advanced(ENABLE_COMPILER_TRACE)

  # Add time trace option to compiler, if enabled.
  if(ENABLE_COMPILER_TRACE AND CMAKE_GENERATOR STREQUAL "Ninja")
    add_compile_options($<$<NOT:$<COMPILE_LANGUAGE:Swift>>:-ftime-trace>)
  else()
    set(ENABLE_COMPILER_TRACE
        OFF
        CACHE BOOL "Enable clang time-trace (requires Ninja)" FORCE)
  endif()

  # Enable color diagnostics for AppleClang
  set(CMAKE_COLOR_DIAGNOSTICS ON)
endif()

add_compile_definitions("$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:DEBUG>>:DEBUG;_DEBUG>")
