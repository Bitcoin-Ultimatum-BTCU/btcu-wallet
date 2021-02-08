# Copyright (c) 2017-2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#.rst
# FindEvent
# -------------
#
# Find the Event library. The following components are available::
#
#   event
#   pthreads
#
# This will define the following variables::
#
#   Event_FOUND - system has Event lib
#   LibEvent_INCLUDE_DIRS - the Event include directories
#   LibEvent_LIBRARIES - Libraries needed to use Event
#   LibEvent_VERSION - The library version MAJOR.MINOR.PATCH
#
# And the following imported target::
#
#   LibEvent::event
#   LibEvent::pthreads

find_package(PkgConfig)
pkg_check_modules(PC_Event QUIET libevent)

include(BrewHelper)
find_brew_prefix(_Event_BREW_HINT libevent)

find_path(LibEvent_INCLUDE_DIR
	NAMES event.h
	PATHS ${PC_LibEvent_INCLUDE_DIRS}
	HINTS ${_Event_BREW_HINT}
	PATH_SUFFIXES include
)

set(LibEvent_INCLUDE_DIRS ${LibEvent_INCLUDE_DIR})
mark_as_advanced(LibEvent_INCLUDE_DIR)

if(LibEvent_INCLUDE_DIR)
	include(ExternalLibraryHelper)

	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		find_component(LibEvent event
			NAMES event
			HINTS "${_Event_BREW_HINT}"
			INCLUDE_DIRS ${LibEvent_INCLUDE_DIRS}
			PATHS ${PC_LibEvent_LIBRARY_DIRS}
			INTERFACE_LINK_LIBRARIES "ws2_32;shell32;advapi32;iphlpapi"
		)
	else()
		find_component(LibEvent event
			NAMES event
			HINTS "${_Event_BREW_HINT}"
			INCLUDE_DIRS ${LibEvent_INCLUDE_DIRS}
			PATHS ${PC_LibEvent_LIBRARY_DIRS}
		)
	endif()

	pkg_check_modules(PC_LibEvent_pthreads QUIET event_pthreads libevent_pthreads)
	find_component(LibEvent pthreads
		NAMES event_pthreads
		INCLUDE_DIRS ${LibEvent_INCLUDE_DIRS}
		PATHS ${PC_LibEvent_pthreads_LIBRARY_DIRS}
	)

	if(NOT LibEvent_VERSION)
		# If pkgconfig found a version number, use it.
		if(PC_LibEvent_VERSION AND (LibEvent_INCLUDE_DIR STREQUAL PC_Event_INCLUDEDIR))
			set(_LibEvent_VERSION ${PC_LibEvent_VERSION})
		else()
			# There is no way to determine the version.
			# Let's assume the user read the doc.
			set(_LibEvent_VERSION 99.99.99)
		endif()

		set(LibEvent_VERSION ${_LibEvent_VERSION}
			CACHE INTERNAL "LibEvent library full version"
		)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibEvent
	REQUIRED_VARS LibEvent_INCLUDE_DIR
	VERSION_VAR LibEvent_VERSION
	HANDLE_COMPONENTS
)
