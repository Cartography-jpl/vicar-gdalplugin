# SYNOPSIS
#
#   AC_GDAL([required], [can_build], [default_build])
#
# DESCRIPTION
#
# This looks for the GDAL libraries. If we find them, we set the Makefile
# conditional HAVE_GDAL. We as set GDAL_CFLAGS and GDAL_LIBS.
#
# The variable GDAL_EXTRA_ARG is set with anything extra to pass to the
# GDAL configure line when building (e.g., specify location of extra libraries
# to use such as Kakadu)
# 
# To allow users to build there own copy of GDAL, we also define
# BUILD_GDAL
#
# If the variable $V2OLB is defined (so we are in a MIPL environment) we
# initially look for GDAL at $GDALLIB directory

AC_DEFUN([AC_GDAL],
[
# Guard against running twice
if test "x$done_gdal" = "x"; then
AC_HANDLE_WITH_ARG([gdal], [gdal], [GDAL], $2, $3)

if test "x$want_gdal" = "xyes"; then
        AC_MSG_CHECKING([for GDAL library])
        succeeded=no
        if test "$ac_gdal_path" != ""; then
            GDAL_PREFIX="$ac_gdal_path"
            GDAL_LIBS="-L$ac_gdal_path/lib -lgdal"
            GDAL_CFLAGS="-I$ac_gdal_path/include"
            succeeded=yes
        else
	    if test "$V2OLB" != "" && test "$GDALLIB" != ""; then
		GDAL_PREFIX="$GDALLIB"
		GDAL_LIBS="-L$GDAL_PREFIX/lib -lgdal"
		GDAL_CFLAGS="-I$GDAL_PREFIX/include"
		succeeded=yes
	    else
	      AC_SEARCH_LIB([GDAL], [gdal], , [gdal.h], , [libgdal], [-lgdal])
	    fi
        fi
	if test "$succeeded" != "yes" -a "x$build_needed_gdal" = "xyes" ; then
            build_gdal="yes"
            ac_gdal_path="\${prefix}"
            GDAL_PREFIX="$ac_gdal_path"
            GDAL_LIBS="-L$ac_gdal_path/lib -lgdal"
            GDAL_CFLAGS="-I$ac_gdal_path/include"
            succeeded=yes
        fi
        if test "$succeeded" != "yes" ; then
                AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
                AC_SUBST(GDAL_CFLAGS)
                AC_SUBST(GDAL_LIBS)
                AC_DEFINE(HAVE_GDAL,,[Defined if we have GDAL])
                have_gdal="yes"
        fi
fi
if test "$build_gdal" = "yes"; then
  AC_GEOS(required, $2, default_search)
  AC_OGDI(required, $2, default_search)
  # AC_EXPAT(required, $2, default_search)
  # AC_OPENJPEG(required, $2, default_search)
else # Not building GDAL
  AM_CONDITIONAL([HAVE_GEOS], [false])
  AM_CONDITIONAL([HAVE_OGDI], [false])
  AM_CONDITIONAL([HAVE_EXPAT], [false])
  AM_CONDITIONAL([HAVE_OPENJPEG], [false])
  AM_CONDITIONAL([BUILD_GEOS], [false])
  AM_CONDITIONAL([BUILD_OGDI], [false])
  AM_CONDITIONAL([BUILD_EXPAT], [false])
  AM_CONDITIONAL([BUILD_OPENJPEG], [false])
  build_geos="no"
  build_ogdi="no"
  build_expat="no"
  build_openjpeg="yes"
fi # End if/else building GDAL
AM_CONDITIONAL([HAVE_GDAL], [test "$have_gdal" = "yes"])
AM_CONDITIONAL([BUILD_GDAL], [test "$build_gdal" = "yes"])

AC_CHECK_FOUND([gdal], [gdal],[Gdal],$1,$2)

if test "$build_ecw_plugin" = "yes"; then
    ECW_CFLAGS="-I$ecw_dir/include"
    ECW_LIBS="-R$ecw_dir/lib/x64/release -L$ecw_dir/lib/x64/release -lNCSEcw"
    AC_SUBST(ECW_CFLAGS)
    AC_SUBST(ECW_LIBS)
fi

AM_CONDITIONAL([BUILD_ECW_PLUGIN], [test "$build_ecw_plugin" = "yes"])
done_gdal="yes"
fi
])
