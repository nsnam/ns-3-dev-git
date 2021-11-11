# Determine if the git repository is an ns-3 repository
#
# A repository is considered an ns-3 repository if it has at least one tag that matches the regex ns-3*

function(check_git_repo_has_ns3_tags HAS_TAGS GIT_VERSION_TAG)
  execute_process(COMMAND ${GIT} describe --tags --abbrev=0 --match ns-3.[0-9]* OUTPUT_VARIABLE GIT_TAG_OUTPUT)

  string(REPLACE "\r" "" GIT_TAG_OUTPUT ${GIT_TAG_OUTPUT}) # remove CR (carriage return)
  string(REPLACE "\n" "" GIT_TAG_OUTPUT ${GIT_TAG_OUTPUT}) # remove LF (line feed)

  # Check if tag exists and return values to the caller
  string(LENGTH GIT_TAG_OUTPUT GIT_TAG_OUTPUT_LEN)
  set(${HAS_TAGS} FALSE PARENT_SCOPE)
  set(${GIT_VERSION_TAG} "" PARENT_SCOPE)
  if(${GIT_TAG_OUTPUT_LEN} GREATER "0")
    set(${HAS_TAGS} TRUE PARENT_SCOPE)
    set(${GIT_VERSION_TAG} ${GIT_TAG_OUTPUT} PARENT_SCOPE)
  endif()
endfunction()

# Function to generate version fields from an ns-3 git repository
function(check_ns3_closest_tags CLOSEST_TAG VERSION_TAG_DISTANCE VERSION_COMMIT_HASH VERSION_DIRTY_FLAG)
  execute_process(COMMAND ${GIT} describe --tags --dirty --long OUTPUT_VARIABLE GIT_TAG_OUTPUT)

  string(REPLACE "\r" "" GIT_TAG_OUTPUT ${GIT_TAG_OUTPUT}) # remove CR (carriage return)
  string(REPLACE "\n" "" GIT_TAG_OUTPUT ${GIT_TAG_OUTPUT}) # remove LF (line feed)

  # Split ns-3.<version>.<patch>(-RC<digit>)-distance-commit(-dirty) into a list
  # ns;3.<version>.<patch>;(RC<digit);distance;commit(;dirty)
  string(REPLACE "-" ";" TAG_LIST "${GIT_TAG_OUTPUT}")

  list(GET TAG_LIST 0 NS)
  list(GET TAG_LIST 1 VERSION)

  if(${GIT_TAG_OUTPUT} MATCHES "RC")
    list(GET TAG_LIST 2 RC)
    set(${CLOSEST_TAG} "${NS}-${VERSION}-${RC}" PARENT_SCOPE)
    list(GET TAG_LIST 3 DISTANCE)
    list(GET TAG_LIST 4 HASH)
  else()
    set(${CLOSEST_TAG} "${NS}-${VERSION}" PARENT_SCOPE)
    list(GET TAG_LIST 2 DISTANCE)
    list(GET TAG_LIST 3 HASH)
  endif()

  set(${VERSION_TAG_DISTANCE} ${DISTANCE} PARENT_SCOPE)
  set(${VERSION_COMMIT_HASH} "${HASH}" PARENT_SCOPE)

  set(${VERSION_DIRTY_FLAG} 0 PARENT_SCOPE)
  if(${GIT_TAG_OUTPUT} MATCHES "dirty")
    set(${VERSION_DIRTY_FLAG} 1 PARENT_SCOPE)
  endif()
endfunction()
