# glm has an exported cmake config file that exports glm::glm
# vcpkg has a config file that exports glm

find_package(glm CONFIG QUIET)

if (NOT glm_FOUND AND Wrap_glm_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find glm")
elseif (NOT TARGET glm::glm)
    if (TARGET glm)
        message(DEBUG "Found glm through config file with glm target")
        add_library(glm::glm ALIAS glm)
    else(Wrap_glm_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find glm")
    endif()
else()
    message(DEBUG "Found glm through config file with glm::glm target")
endif()
