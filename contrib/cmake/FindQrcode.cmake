# Copyright (c) 2019-2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#.rst
# FindQrcode
# -------------
#
# Find the Qrcode library. The following
# components are available::
#   qrencode
#
# This will define the following variables::
#
#   Qrcode_FOUND - system has Qrcode lib
#   Qrcode_INCLUDE_DIRS - the Qrcode include directories
#   Qrcode_LIBRARIES - Libraries needed to use Qrcode
#
# And the following imported target::
#
#   Qrcode::qrencode

find_brew_prefix(_Qrcode_BREW_HINT qrencode)

find_package(PkgConfig)
pkg_check_modules(PC_Qrcode QUIET libqrencode)

find_path(Qrcode_INCLUDE_DIR
	NAMES qrencode.h
	HINTS ${_Qrcode_BREW_HINT}
	PATHS ${PC_Qrcode_INCLUDE_DIRS}
	PATH_SUFFIXES include
)

set(Qrcode_INCLUDE_DIRS "${Qrcode_INCLUDE_DIR}")
mark_as_advanced(Qrcode_INCLUDE_DIR)

# TODO: extract a version number.
# For now qrencode does not provide an easy way to extract a version number.

if(Qrcode_INCLUDE_DIR)
	include(ExternalLibraryHelper)
	find_component(Qrcode qrencode
		NAMES qrencode qrencoded
		HINTS ${_Qrcode_BREW_HINT}
		PATHS ${PC_Qrcode_LIBRARY_DIRS}
		INCLUDE_DIRS ${Qrcode_INCLUDE_DIRS}
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qrcode
	REQUIRED_VARS
		Qrcode_INCLUDE_DIR
	HANDLE_COMPONENTS
)
