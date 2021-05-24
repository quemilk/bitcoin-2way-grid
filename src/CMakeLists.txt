cmake_minimum_required (VERSION 3.8)

aux_source_directory(. DIR_SRC)

if (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup") 
endif (MSVC)

add_executable(ibitcoin2waygrid WIN32 MACOSX_BUNDLE
	${DIR_SRC})
	

install(TARGETS ibitcoin2waygrid RUNTIME DESTINATION ibitcoin2waygrid)