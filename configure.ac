AC_INIT(vicar-gdalplugin, 1.12)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config/m4])
AFIDS_COMMON
# We have the top directory for vicar_rtl as a variable. This allows the
# full AFIDS system to include this in a subdirectory. For this standalone
# build, this is just "." relative to srcdir.
vicar_gdalplugin_topdir=.
# This set up all the source directories.
VICAR_GDALPLUGIN_SOURCE_DIRECTORY

AC_BOOST(required, cannot_build, default_search, 1.46.0)
AC_VICAR_RTL(required, cannot_build, default_search)
AC_GDAL(required, cannot_build, default_search)

# Always build VICAR GDAL plugin
AM_CONDITIONAL([BUILD_VICAR_GDALPLUGIN], [true])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

#======================================================================
# Give a nice log message describing what has been configured, and which
# optional libraries are going to be used.
#======================================================================

LOC_MSG()
LOC_MSG([VICAR GDAL plugin is now configured])
LOC_MSG()
LOC_MSG([  Installation directory:        ${prefix}])
LOC_MSG([  Build debug version:           ${enable_debug}])
LOC_MSG([  Fortran compiler:              ${F77} ${FFLAGS}])
LOC_MSG([  C compiler:                    ${CC} ${CFLAGS}])
LOC_MSG([  CXX compiler:                  ${CXX} ${CXXFLAGS}])
