#ifndef GEOTIFF_FILE_H
#define GEOTIFF_FILE_H
#include <boost/utility.hpp>
#include <string>
#include <unistd.h>
#include <stdint.h>
#include <vector>

struct TIFF;
struct GTIF;

namespace VicarGdal {
/****************************************************************//**
  Provide direct access to a GeoTiff file. Note that this is a
  pretty low level class, you almost certainly don't want to be
  using this but instead want something like a GdalRasterImage. But
  this class if useful if you need to do low level things with a
  geotiff file like directly read and write geotiff tags.

  A note on the hardcode values/protypes:

  Normally we wouldn't have prototypes and hard coded values defined
  directly in a class. However, we have a special case. We know that
  GDAL will supply the GeoTIFF library, because if it isn't found on
  the system GDAL will use its own private copy. However, we don't
  know that the GeoTIFF headers will be available. GDAL doesn't
  install them, and unless there is a another copy of GeoTIFF
  available they won't be found. Since it is entirely possible to have
  GDAL installed but GeoTIFF not, we don't want to add a dependency on
  this library that we don't really need. So, we define the prototypes
  here. There is a risk that these prototypes will become out of date,
  in which case we'll need to update these. But the GeoTIFF library
  hasn't changed in some time, so the trade against adding a
  dependency is probably a good one.
*******************************************************************/

class GeotiffFile: public boost::noncopyable  {
public:
  typedef unsigned short geocode_t;
  // Note that all of these hard coded values are actually registered in
  // the GeoTIFF standard. This should mean that they are fairly stable
  // and unlikely to change.
  typedef enum {
    GTModelTypeGeoKey = 1024,
    GTRasterTypeGeoKey = 1025,
    GTCitationGeoKey = 1026,
    GeographicTypeGeoKey = 2048,
    GeogCitationGeoKey = 2049,
    GeogGeodeticDatumGeoKey = 2050,
    GeogPrimeMeridianGeoKey = 2051,
    GeogLinearUnitsGeoKey = 2052,
    GeogLinearUnitSizeGeoKey = 2053,
    GeogAngularUnitsGeoKey = 2054,
    GeogAngularUnitSizeGeoKey = 2055,
    GeogEllipsoidGeoKey = 2056,
    GeogSemiMajorAxisGeoKey = 2057,
    GeogSemiMinorAxisGeoKey = 2058,
    GeogInvFlatteningGeoKey = 2059,
    GeogAzimuthUnitsGeoKey = 2060,
    GeogPrimeMeridianLongGeoKey = 2061,
    ProjectedCSTypeGeoKey = 3072,
    PCSCitationGeoKey = 3073,
    ProjectionGeoKey = 3074,
    ProjCoordTransGeoKey = 3075,
    ProjLinearUnitsGeoKey = 3076,
    ProjLinearUnitSizeGeoKey = 3077,
    ProjStdParallel1GeoKey = 3078,
    ProjStdParallelGeoKey = ProjStdParallel1GeoKey,
    ProjStdParallel2GeoKey = 3079,
    ProjNatOriginLongGeoKey = 3080,
    ProjOriginLongGeoKey = ProjNatOriginLongGeoKey,
    ProjNatOriginLatGeoKey = 3081,
    ProjOriginLatGeoKey = ProjNatOriginLatGeoKey,
    ProjFalseEastingGeoKey = 3082,
    ProjFalseNorthingGeoKey = 3083,
    ProjFalseOriginLongGeoKey = 3084,
    ProjFalseOriginLatGeoKey = 3085,
    ProjFalseOriginEastingGeoKey = 3086,
    ProjFalseOriginNorthingGeoKey = 3087,
    ProjCenterLongGeoKey = 3088,
    ProjCenterLatGeoKey = 3089,
    ProjCenterEastingGeoKey = 3090,
    ProjCenterNorthingGeoKey = 3091,
    ProjScaleAtNatOriginGeoKey = 3092,
    ProjScaleAtOriginGeoKey = ProjScaleAtNatOriginGeoKey,
    ProjScaleAtCenterGeoKey = 3093,
    ProjAzimuthAngleGeoKey = 3094,
    ProjStraightVertPoleLongGeoKey = 3095,
    ProjRectifiedGridAngleGeoKey = 3096,
    VerticalCSTypeGeoKey = 4096,
    VerticalCitationGeoKey = 4097,
    VerticalDatumGeoKey = 4098,
    VerticalUnitsGeoKey = 4099
  } geokey_t;
  typedef enum {
    TYPE_BYTE=1,
    TYPE_SHORT=2,
    TYPE_LONG=3,
    TYPE_RATIONAL=4,
    TYPE_ASCII=5,
    TYPE_FLOAT=6,
    TYPE_DOUBLE=7,
    TYPE_SBYTE=8,
    TYPE_SSHORT=9,
    TYPE_SLONG=10,
    TYPE_UNKNOWN=11
  } tagtype_t;
  typedef enum {
    TIFFTAG_GEOPIXELSCALE=33550,
    TIFFTAG_GEOTIEPOINTS=33922,
    TIFFTAG_GEOTRANSMATRIX=34264,
    TIFFTAG_IMAGEWIDTH=256,
    TIFFTAG_IMAGELENGTH=257,
    TIFFTAG_COMPRESSION=259,
    TIFFTAG_PLANARCONFIG=284,
    TIFFTAG_PHOTOMETRIC=262,
    TIFFTAG_BITSPERSAMPLE=258,
    TIFFTAG_SAMPLESPERPIXEL=277
  } tiftag_t;
  typedef enum {
    PHOTOMETRIC_MINISBLACK = 1,
    COMPRESSION_NONE = 1,
    PLANARCONFIG_CONTIG = 1
  } tiftagval_t;
  typedef uint32_t ttag_t;

//-----------------------------------------------------------------------
/// Open given file name with the given mode. Mode should be "r",
/// "w", or "a".
//-----------------------------------------------------------------------
  
  GeotiffFile(const std::string& Fname, const std::string& Mode)
    : fname_(Fname), mode_(Mode), tif(0), gtif(0)
  {
    init();
  }
  virtual ~GeotiffFile();

//-----------------------------------------------------------------------
/// Return the file name we have open.
//-----------------------------------------------------------------------

  const std::string& file_name() const {return fname_;}

//-----------------------------------------------------------------------
/// Return mode the file was opened with.
//-----------------------------------------------------------------------

  const std::string& mode() const {return mode_;}
  
  void set_tiftag(tiftag_t K, int V);
  void set_tiftag(tiftag_t K, const std::vector<double>& V);
  void set_key(geokey_t K, geocode_t V);
  void set_key(geokey_t K, double V);
  void set_key(geokey_t K, const std::string& V);
  void write_key();
  template<class T> T get_key(geokey_t K) const;
  bool has_key(geokey_t K) const;
  bool has_tiftag(tiftag_t K) const;
  template<class T> T get_tiftag(tiftag_t K) const;
  static std::string key_name(geokey_t K);
  static std::string key_name_uppercase(geokey_t K);
  static std::string value_name(geokey_t K, geocode_t V);
  static tagtype_t key_type(geokey_t K);
  static const std::vector<geokey_t>& geotiff_tag_ascii();
  static const std::vector<geokey_t>& geotiff_tag_double();
  static const std::vector<geokey_t>& geotiff_tag_short();
  void write_1x1_file();
private:
  void init();
  std::string fname_, mode_;
  TIFF *tif;
  GTIF *gtif;
  GeotiffFile() : tif(0), gtif(0) {}
};

template<> GeotiffFile::geocode_t GeotiffFile::get_key(geokey_t K) const;
template<> double GeotiffFile::get_key(geokey_t K) const;
template<> std::string GeotiffFile::get_key(geokey_t K) const;
template<> int GeotiffFile::get_tiftag(tiftag_t K) const;
template<> std::vector<double> GeotiffFile::get_tiftag(tiftag_t K) const;
}

#endif
