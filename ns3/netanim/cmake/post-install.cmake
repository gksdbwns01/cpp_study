# Run by CPack Bundle generator at install time.
# CMAKE_INSTALL_PREFIX during install is .../NetAnim.app/Contents/Resources
get_filename_component(APP_DIR "${CMAKE_INSTALL_PREFIX}/../.." ABSOLUTE)
set(MACOS_BIN "${APP_DIR}/Contents/Resources/bin/netanim")
set(FRAMEWORKS_DIR "${APP_DIR}/Contents/Frameworks")
set(PLUGINS_DIR "${APP_DIR}/Contents/Resources/bin/plugins")
set(QT_CONF "${APP_DIR}/Contents/Resources/bin/qt.conf")

if(NOT EXISTS "${MACOS_BIN}")
  message(FATAL_ERROR "Binary not found at install time: ${MACOS_BIN}")
endif()

file(MAKE_DIRECTORY "${FRAMEWORKS_DIR}")
file(MAKE_DIRECTORY "${PLUGINS_DIR}")

# Fallback prefixes searched when a framework is referenced via @rpath/ rather
# than an absolute path (Homebrew's Qt frameworks reference each other this way).
set(FALLBACK_QT_PREFIXES
    "/opt/homebrew/opt/qtbase/lib"
    "/opt/homebrew/opt/qtsvg/lib"
    "/opt/homebrew/opt/qttools/lib"
    "/opt/homebrew/opt/qt/lib"
    "/usr/local/opt/qtbase/lib"
    "/usr/local/opt/qtsvg/lib"
    "/usr/local/opt/qt/lib")

# Macros (not functions) so VISITED_FWS / QT_LIB_PREFIXES updates land in
# the caller's scope and persist across iterations. With functions, each call
# would re-read a stale snapshot and re-copy the same frameworks forever.

# Ensure FW_NAME.framework is copied into Contents/Frameworks (if not already)
# and its inner binary is appended to NEW_BINS_VAR for further processing.
macro(ensure_framework FW_NAME NEW_BINS_VAR)
  list(FIND VISITED_FWS "${FW_NAME}" _ef_idx)
  if(_ef_idx EQUAL -1)
    set(_ef_pfx "")
    foreach(_ef_p IN LISTS QT_LIB_PREFIXES FALLBACK_QT_PREFIXES)
      if(EXISTS "${_ef_p}/${FW_NAME}.framework")
        set(_ef_pfx "${_ef_p}")
        break()
      endif()
    endforeach()
    if(_ef_pfx)
      list(APPEND VISITED_FWS "${FW_NAME}")
      list(FIND QT_LIB_PREFIXES "${_ef_pfx}" _ef_pfx_idx)
      if(_ef_pfx_idx EQUAL -1)
        list(APPEND QT_LIB_PREFIXES "${_ef_pfx}")
      endif()
      message(STATUS "Copying framework ${FW_NAME} from ${_ef_pfx}")
      execute_process(COMMAND cp -RL "${_ef_pfx}/${FW_NAME}.framework" "${FRAMEWORKS_DIR}/")
      set(_ef_dest "${FRAMEWORKS_DIR}/${FW_NAME}.framework")
      execute_process(COMMAND chmod -R u+w "${_ef_dest}")
      file(REMOVE_RECURSE
           "${_ef_dest}/Headers"
           "${_ef_dest}/Versions/A/Headers"
           "${_ef_dest}/Versions/A/_CodeSignature"
           "${_ef_dest}/_CodeSignature")
      set(_ef_inner "${_ef_dest}/Versions/A/${FW_NAME}")
      execute_process(COMMAND install_name_tool -id
                      "@rpath/${FW_NAME}.framework/Versions/A/${FW_NAME}" "${_ef_inner}")
      list(APPEND ${NEW_BINS_VAR} "${_ef_inner}")
    else()
      message(WARNING "Could not locate ${FW_NAME}.framework in any known Qt prefix")
    endif()
  endif()
endmacro()

# Inspect a Mach-O binary, copy each referenced Qt framework into
# Contents/Frameworks, rewrite the reference to @rpath/, and add an rpath so
# the binary can find it. New framework inner-binaries get appended to
# NEW_BINS_VAR for the caller's worklist to recurse into.
macro(process_binary BIN NEW_BINS_VAR)
  execute_process(COMMAND chmod u+w "${BIN}")
  execute_process(COMMAND otool -L "${BIN}"
                  OUTPUT_VARIABLE _pb_otool
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\n" ";" _pb_lines "${_pb_otool}")

  foreach(_pb_line IN LISTS _pb_lines)
    # Case 1: absolute Homebrew path — capture prefix to rewrite the ref.
    if(_pb_line MATCHES "(/opt/homebrew/opt/[A-Za-z0-9_@]+/lib)/(Qt[A-Za-z0-9_]+)\\.framework/Versions/A/Qt[A-Za-z0-9_]+")
      set(_pb_src_pfx "${CMAKE_MATCH_1}")
      set(_pb_fw "${CMAKE_MATCH_2}")
      ensure_framework("${_pb_fw}" ${NEW_BINS_VAR})
      execute_process(COMMAND install_name_tool
                      -change "${_pb_src_pfx}/${_pb_fw}.framework/Versions/A/${_pb_fw}"
                              "@rpath/${_pb_fw}.framework/Versions/A/${_pb_fw}"
                              "${BIN}"
                      OUTPUT_QUIET ERROR_QUIET)
    # Case 2: already @rpath/Qt*.framework — common inside Homebrew Qt frameworks.
    elseif(_pb_line MATCHES "@rpath/(Qt[A-Za-z0-9_]+)\\.framework/Versions/A/Qt[A-Za-z0-9_]+")
      set(_pb_fw "${CMAKE_MATCH_1}")
      ensure_framework("${_pb_fw}" ${NEW_BINS_VAR})
    endif()
  endforeach()

  # Idempotent: install_name_tool errors if the rpath already exists, ignore.
  execute_process(COMMAND install_name_tool -add_rpath
                  "@executable_path/../../Frameworks" "${BIN}"
                  OUTPUT_QUIET ERROR_QUIET)
endmacro()

set(WORKLIST "${MACOS_BIN}")
set(VISITED_FWS "")
set(QT_LIB_PREFIXES "")

# Phase 1: main binary and its transitive framework deps.
while(WORKLIST)
  list(POP_FRONT WORKLIST CURRENT)
  set(NEW_BINS "")
  process_binary("${CURRENT}" NEW_BINS)
  list(APPEND WORKLIST ${NEW_BINS})
endwhile()

# Phase 2: copy Qt plugins from each prefix the binary actually pulled from.
# Homebrew Qt 6 stores plugins under share/qt/plugins/<category>/.
set(WANTED_CATS platforms imageformats iconengines styles)
foreach(PFX IN LISTS QT_LIB_PREFIXES)
  string(REGEX REPLACE "/lib$" "/share/qt/plugins" PLUGIN_ROOT "${PFX}")
  if(NOT EXISTS "${PLUGIN_ROOT}")
    continue()
  endif()
  foreach(CAT IN LISTS WANTED_CATS)
    if(EXISTS "${PLUGIN_ROOT}/${CAT}")
      file(GLOB PLUGINS "${PLUGIN_ROOT}/${CAT}/*.dylib")
      if(PLUGINS)
        file(MAKE_DIRECTORY "${PLUGINS_DIR}/${CAT}")
        foreach(P IN LISTS PLUGINS)
          get_filename_component(NAME "${P}" NAME)
          set(DEST "${PLUGINS_DIR}/${CAT}/${NAME}")
          if(NOT EXISTS "${DEST}")
            message(STATUS "Copying plugin ${CAT}/${NAME}")
            execute_process(COMMAND cp -L "${P}" "${DEST}")
            execute_process(COMMAND chmod u+w "${DEST}")
            list(APPEND WORKLIST "${DEST}")
          endif()
        endforeach()
      endif()
    endif()
  endforeach()
endforeach()

# Phase 3: patch plugins (and pull any extra transitive frameworks they need).
while(WORKLIST)
  list(POP_FRONT WORKLIST CURRENT)
  set(NEW_BINS "")
  process_binary("${CURRENT}" NEW_BINS)
  list(APPEND WORKLIST ${NEW_BINS})
endwhile()

# qt.conf next to the executable redirects plugin lookup into our bundle.
file(WRITE "${QT_CONF}" "[Paths]\nPlugins = plugins\n")

# Re-sign everything we modified. install_name_tool invalidates code signatures
# and on Apple Silicon dyld refuses to load unsigned arm64 binaries with
# SIGKILL (Code Signature Invalid). Ad-hoc sign (identity "-") is sufficient
# for local distribution; replace with a real identity for the App Store.
# Sign innermost dependencies first, then dependents, then the bundle root.
file(GLOB FW_BINS_TO_SIGN "${FRAMEWORKS_DIR}/*.framework/Versions/A/Qt*")
file(GLOB_RECURSE PLUGIN_BINS_TO_SIGN "${PLUGINS_DIR}/*.dylib")
foreach(TARGET IN LISTS FW_BINS_TO_SIGN PLUGIN_BINS_TO_SIGN)
  execute_process(COMMAND codesign --force --sign - --timestamp=none "${TARGET}"
                  RESULT_VARIABLE _cs ERROR_VARIABLE _cs_err)
  if(NOT _cs EQUAL 0)
    message(WARNING "codesign failed for ${TARGET}: ${_cs_err}")
  endif()
endforeach()
execute_process(COMMAND codesign --force --sign - --timestamp=none "${MACOS_BIN}"
                RESULT_VARIABLE _cs ERROR_VARIABLE _cs_err)
if(NOT _cs EQUAL 0)
  message(WARNING "codesign failed for ${MACOS_BIN}: ${_cs_err}")
endif()
# Seal the whole bundle.
execute_process(COMMAND codesign --force --deep --sign - --timestamp=none "${APP_DIR}"
                RESULT_VARIABLE _cs ERROR_VARIABLE _cs_err)
if(NOT _cs EQUAL 0)
  message(WARNING "codesign failed for ${APP_DIR}: ${_cs_err}")
endif()

message(STATUS "NetAnim macOS bundling complete: ${APP_DIR}")
message(STATUS "  frameworks: ${VISITED_FWS}")
