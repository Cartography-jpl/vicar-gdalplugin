#ifndef VICAR_OGR_H
#define VICAR_OGR_H
#include "vicar_file.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <set>

namespace VicarGdal {
/****************************************************************//**
  This class is really part of VicarFile, but because of the
  complication in this software we separate this out into its own
  class. This class handles the reading and writing of the GeoTIFF map
  projection and coordinate transformation information, going to and
  from a MapInfo.

  AFIDS stores map projection information as text labels in a VICAR
  file. The text is GeoTIFF tags stored as text. We can't directly
  work with these to supply map projection information. Instead, we
  want to use the GDAL library to handle this (through the
  OgrCoordinate class). However, there is no easy mapping between GDAL
  which use the Well Known Text (WKT) to express its coordinate
  information and GeoTIFF which uses about 40 different tags for this
  information.

  The two systems contain similar information, so one possible
  approach would be to create a mapping between the two systems -
  e.g., Tag X corresponds to WKT node Y. While possible, this would
  result in a large amount of code.

  As an alternative, we take advantage of the ability of GDAL to
  create and read GeoTIFF files. The GDAL library contains all of the
  code connecting the two, which we don't want to duplicate.

  This class creates a temporary GeoTIFF file, and either writes map
  projection information using GDAL and a MapInfo, or from the VICAR
  GeoTIFF information. We then read the file and go the other way,
  creating a MapInfo or the metadata for a VICAR file. The temporary
  file is then removed.

  This is a bit awkward, but this is the best approach I could come up
  with to map VICAR and GDAL together. 
*******************************************************************/

class VicarOgr {
public:
  VicarOgr();
  void vicar_to_gtiff(const VicarFile& F, const std::string& Fname) const;
#if(GDAL_VERSION_MAJOR >= 3)
  void from_vicar(const VicarFile& F,
		  boost::shared_ptr<OGRSpatialReference>& Sref, double* Transform)
    const;
  void to_vicar(const boost::shared_ptr<OGRSpatialReference>& Sref,
		const double* Transform,
		bool is_point, VicarFile& F) const;
#else
  void from_vicar(const VicarFile& F, std::string& Proj_ref, double* Transform)
    const;
  void to_vicar(const std::string& Proj_ref, const double* Transform,
		bool is_point, VicarFile& F) const;
#endif  
  bool is_geotiff_vicar_name(const std::string& F) const
  { return vicar_name.count(F) != 0; }
private:
#if(GDAL_VERSION_MAJOR >= 3)
  template<class T> void from_vicar_template
  (const T& F, boost::shared_ptr<OGRSpatialReference>& Sref,
   double* Transform) const;
#else  
  template<class T> void from_vicar_template
  (const T& F, std::string& Proj_ref, double* Transform) const;
#endif  
  template<class T> void vicar_to_gtiff_template(const T& F, const char* Fname) const;
  mutable std::map<int, std::string> tag_to_vicar_name;
  std::set<std::string> vicar_name;
};
}
#endif
