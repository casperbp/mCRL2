# Authors: Frank Stappers 
# Copyright: see the accompanying file COPYING or copy at
# https://github.com/mCRL2org/mCRL2/blob/master/COPYING
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

include(FindPackageHandleStandardArgs)
find_library(MLIB_LIBRARY NAMES m mlib)
set( mlib_FOUND ${MLIB_LIBRARY} )
find_package_handle_standard_args(mlib DEFAULT_MSG MLIB_LIBRARY)
mark_as_advanced(MLIB_LIBRARY)
