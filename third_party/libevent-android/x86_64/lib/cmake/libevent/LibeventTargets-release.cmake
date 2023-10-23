#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "event" for configuration "Release"
set_property(TARGET event APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(event PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "m"
  IMPORTED_LOCATION_RELEASE "/home/dengpan/allCompileLibeventLibssl20171009/tmp/libevent-2.1.8/x86_64/lib/libevent.so"
  IMPORTED_SONAME_RELEASE "libevent.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS event )
list(APPEND _IMPORT_CHECK_FILES_FOR_event "/home/dengpan/allCompileLibeventLibssl20171009/tmp/libevent-2.1.8/x86_64/lib/libevent.so" )

# Import target "event_core" for configuration "Release"
set_property(TARGET event_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(event_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "m"
  IMPORTED_LOCATION_RELEASE "/home/dengpan/allCompileLibeventLibssl20171009/tmp/libevent-2.1.8/x86_64/lib/libevent_core.so"
  IMPORTED_SONAME_RELEASE "libevent_core.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS event_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_event_core "/home/dengpan/allCompileLibeventLibssl20171009/tmp/libevent-2.1.8/x86_64/lib/libevent_core.so" )

# Import target "event_extra" for configuration "Release"
set_property(TARGET event_extra APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(event_extra PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "m"
  IMPORTED_LOCATION_RELEASE "/home/dengpan/allCompileLibeventLibssl20171009/tmp/libevent-2.1.8/x86_64/lib/libevent_extra.so"
  IMPORTED_SONAME_RELEASE "libevent_extra.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS event_extra )
list(APPEND _IMPORT_CHECK_FILES_FOR_event_extra "/home/dengpan/allCompileLibeventLibssl20171009/tmp/libevent-2.1.8/x86_64/lib/libevent_extra.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
