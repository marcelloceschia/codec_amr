project(codec_amr)



aux_source_directory(src SRC)
add_library(codec_amr SHARED ${SRC})

if(UNIX AND NOT APPLE)
  SET_TARGET_PROPERTIES (${LIBRARYNAME} PROPERTIES COMPILE_FLAGS "-fPIC -std=c89")
endif()

set_target_properties(codec_amr PROPERTIES PREFIX "")
install(TARGETS codec_amr LIBRARY DESTINATION "/usr/lib64/asterisk/modules/")
