#include "geotiff_file.h"
#include "vicar_gdal_exception.h"
#include <boost/algorithm/string.hpp>

using namespace VicarGdal;

// See note in class comment about the direct use of prototypes here.
extern "C" {
typedef int tagtype_t;
typedef uint32_t ttag_t;
TIFF * XTIFFOpen(const char* name, const char* mode);
void XTIFFClose(TIFF *tif);
GTIF *GTIFNew(void *tif);
void GTIFFree(GTIF *gtif);
int  GTIFWriteKeys(GTIF *gtif);
int GTIFKeyInfo(GTIF *gtif, int key, int *size, tagtype_t* type);
int GTIFKeyGet(GTIF *gtif, int key, void *val, int index, int count);
int GTIFKeySet(GTIF *gtif, int keyID, tagtype_t type, int count,...);
char *GTIFKeyName(int key);
char *GTIFValueName(int key,int value);
int TIFFSetField(TIFF*, ttag_t, ...);
int TIFFGetField(TIFF*, ttag_t, ...);
int32_t TIFFWriteEncodedStrip(TIFF*, uint32_t, void*, int32_t);
}

void GeotiffFile::init()
{
  tif = 0;
  gtif = 0;
  tif = XTIFFOpen(fname_.c_str(), mode_.c_str());
  if(!tif) {
    Exception e;
    e << "Trouble opening TIFF file " << fname_;
    throw ;
  }
  gtif = GTIFNew(tif);
  if(!gtif) {
    Exception e;
    e << "Trouble calling GTIFNew for file " << fname_;
    throw ;
  }
}

GeotiffFile::~GeotiffFile()
{
  if(gtif)
    GTIFFree(gtif);
  if(tif)
    XTIFFClose(tif);
}

//-----------------------------------------------------------------------
/// Return a string giving the key name.
//-----------------------------------------------------------------------

std::string GeotiffFile::key_name(GeotiffFile::geokey_t K)
{
  return GTIFKeyName(K);
}

//-----------------------------------------------------------------------
/// Return a string giving the key name, in all uppercase. This is
/// useful because this the tag used in VICAR files to carry the
/// geotiff information
//-----------------------------------------------------------------------

std::string GeotiffFile::key_name_uppercase(GeotiffFile::geokey_t K)
{
  std::string kn = GeotiffFile::key_name(K);
  boost::to_upper(kn);
  return kn;
}

//-----------------------------------------------------------------------
/// Return a string giving the value name for the given key
//-----------------------------------------------------------------------

std::string GeotiffFile::value_name(GeotiffFile::geokey_t K, geocode_t V)
{
  return GTIFValueName(K, V);
}

//-----------------------------------------------------------------------
/// Return the type for the given key.
//-----------------------------------------------------------------------

GeotiffFile::tagtype_t GeotiffFile::key_type(GeotiffFile::geokey_t K)
{
  switch(K) {
  case GTCitationGeoKey:
  case GeogCitationGeoKey:
  case PCSCitationGeoKey:
  case VerticalCitationGeoKey:
    return TYPE_ASCII;
    break;
  case GeogInvFlatteningGeoKey:
  case GeogSemiMajorAxisGeoKey:
  case GeogSemiMinorAxisGeoKey:
  case ProjAzimuthAngleGeoKey:
  case ProjCenterLatGeoKey:
  case ProjCenterLongGeoKey:
  case ProjFalseEastingGeoKey:
  case ProjFalseNorthingGeoKey:
  case ProjFalseOriginEastingGeoKey:
  case ProjFalseOriginLatGeoKey:
  case ProjFalseOriginLongGeoKey:
  case ProjFalseOriginNorthingGeoKey:
  case ProjLinearUnitSizeGeoKey:
  case ProjNatOriginLatGeoKey:
  case ProjNatOriginLongGeoKey:
  case ProjRectifiedGridAngleGeoKey:
  case ProjScaleAtNatOriginGeoKey:
  case ProjStdParallel1GeoKey:
  case ProjStdParallel2GeoKey:
  case ProjStraightVertPoleLongGeoKey:
  case GeogLinearUnitSizeGeoKey:
  case GeogAngularUnitSizeGeoKey:
  case GeogPrimeMeridianLongGeoKey:
  case ProjCenterEastingGeoKey:
  case ProjCenterNorthingGeoKey:
  case ProjScaleAtCenterGeoKey:
    return TYPE_DOUBLE;
    break;
  case GTModelTypeGeoKey:
  case GTRasterTypeGeoKey:
  case GeogAngularUnitsGeoKey:
  case GeogEllipsoidGeoKey:
  case GeogGeodeticDatumGeoKey:
  case GeographicTypeGeoKey:
  case ProjCoordTransGeoKey:
  case ProjLinearUnitsGeoKey:
  case ProjectedCSTypeGeoKey:
  case ProjectionGeoKey:
  case GeogPrimeMeridianGeoKey:
  case GeogLinearUnitsGeoKey:
  case GeogAzimuthUnitsGeoKey:
  case VerticalCSTypeGeoKey:
  case VerticalDatumGeoKey:
  case VerticalUnitsGeoKey:
    return TYPE_SHORT;
  default:
    Exception e;
    e << "Unrecognized key " << K;
    throw e;
  }
}

//-----------------------------------------------------------------------
/// Return the list of tags that take ASCII data.
//-----------------------------------------------------------------------

const std::vector<GeotiffFile::geokey_t>& GeotiffFile::geotiff_tag_ascii()
{
  static bool filled_in = false;
  static std::vector<geokey_t> data;
  if(!filled_in) {
    data.push_back(GTCitationGeoKey);
    data.push_back(GeogCitationGeoKey);
    data.push_back(PCSCitationGeoKey);
    data.push_back(VerticalCitationGeoKey);
    filled_in = true;
  }
  return data;
}

//-----------------------------------------------------------------------
/// Return the list of tags that take double data.
//-----------------------------------------------------------------------

const std::vector<GeotiffFile::geokey_t>& GeotiffFile::geotiff_tag_double()
{
  static bool filled_in = false;
  static std::vector<geokey_t> data;
  if(!filled_in) {
    data.push_back(GeogInvFlatteningGeoKey);
    data.push_back(GeogSemiMajorAxisGeoKey);
    data.push_back(GeogSemiMinorAxisGeoKey);
    data.push_back(ProjAzimuthAngleGeoKey);
    data.push_back(ProjCenterLatGeoKey);
    data.push_back(ProjCenterLongGeoKey);
    data.push_back(ProjFalseEastingGeoKey);
    data.push_back(ProjFalseNorthingGeoKey);
    data.push_back(ProjFalseOriginEastingGeoKey);
    data.push_back(ProjFalseOriginLatGeoKey);
    data.push_back(ProjFalseOriginLongGeoKey);
    data.push_back(ProjFalseOriginNorthingGeoKey);
    data.push_back(ProjLinearUnitSizeGeoKey);
    data.push_back(ProjNatOriginLatGeoKey);
    data.push_back(ProjNatOriginLongGeoKey);
    data.push_back(ProjRectifiedGridAngleGeoKey);
    data.push_back(ProjScaleAtNatOriginGeoKey);
    data.push_back(ProjStdParallel1GeoKey);
    data.push_back(ProjStdParallel2GeoKey);
    data.push_back(ProjStraightVertPoleLongGeoKey);
    data.push_back(GeogLinearUnitSizeGeoKey);
    data.push_back(GeogAngularUnitSizeGeoKey);
    data.push_back(GeogPrimeMeridianLongGeoKey);
    data.push_back(ProjCenterEastingGeoKey);
    data.push_back(ProjCenterNorthingGeoKey);
    data.push_back(ProjScaleAtCenterGeoKey);
    filled_in = true;
  }
  return data;
}

//-----------------------------------------------------------------------
/// Return the list of tags that take short data.
//-----------------------------------------------------------------------

const std::vector<GeotiffFile::geokey_t>& GeotiffFile::geotiff_tag_short()
{
  static bool filled_in = false;
  static std::vector<geokey_t> data;
  if(!filled_in) {
    data.push_back(GTModelTypeGeoKey);
    data.push_back(GTRasterTypeGeoKey);
    data.push_back(GeogAngularUnitsGeoKey);
    data.push_back(GeogEllipsoidGeoKey);
    data.push_back(GeogGeodeticDatumGeoKey);
    data.push_back(GeographicTypeGeoKey);
    data.push_back(ProjCoordTransGeoKey);
    data.push_back(ProjLinearUnitsGeoKey);
    data.push_back(ProjectedCSTypeGeoKey);
    data.push_back(ProjectionGeoKey);
    data.push_back(GeogPrimeMeridianGeoKey);
    data.push_back(GeogLinearUnitsGeoKey);
    data.push_back(GeogAzimuthUnitsGeoKey);
    data.push_back(VerticalCSTypeGeoKey);
    data.push_back(VerticalDatumGeoKey);
    data.push_back(VerticalUnitsGeoKey);
    filled_in = true;
  }
  return data;
}

//-----------------------------------------------------------------------
/// Set the value of the given key. Not actually written to the file
/// until write_key is called.
//-----------------------------------------------------------------------

void GeotiffFile::set_key(geokey_t K, GeotiffFile::geocode_t V)
{
  int status = GTIFKeySet(gtif, K, GeotiffFile::TYPE_SHORT, 1, V);
  if(status != 1)
    throw Exception("Call to GTIFKeySet failed");
}

//-----------------------------------------------------------------------
/// Set the value of the given key. Not actually written to the file
/// until write_key is called.
//-----------------------------------------------------------------------

void GeotiffFile::set_key(geokey_t K, double V)
{
  int status = GTIFKeySet(gtif, K, GeotiffFile::TYPE_DOUBLE, 1, V);
  if(status != 1)
    throw Exception("Call to GTIFKeySet failed");
}

//-----------------------------------------------------------------------
/// Set the value of the given key. Not actually written to the file
/// until write_key is called.
//-----------------------------------------------------------------------

void GeotiffFile::set_key(geokey_t K, const std::string& V)
{
  int status = GTIFKeySet(gtif, K, GeotiffFile::TYPE_ASCII, 0, V.c_str());
  if(status != 1)
    throw Exception("Call to GTIFKeySet failed");
}
  

//-----------------------------------------------------------------------
/// Write the key value to the file.
//-----------------------------------------------------------------------

void GeotiffFile::write_key()
{
  int status = GTIFWriteKeys(gtif);
  if(status != 1)
    throw Exception("Call to GTIFWriteKeys failed");
}

//-----------------------------------------------------------------------
/// Return true if we have the key.
//-----------------------------------------------------------------------

bool GeotiffFile::has_key(geokey_t K) const
{
  int size = GTIFKeyInfo(gtif, K, 0, 0);
  return size > 0;
}

//-----------------------------------------------------------------------
/// Get key value.
//-----------------------------------------------------------------------

template<> GeotiffFile::geocode_t GeotiffFile::get_key(geokey_t K) const
{
  geocode_t v;
  int status = GTIFKeyGet(gtif, K, &v, 0, 1);
  if(status != 1)
    throw Exception("Error getting geotiff key");
  return v;
}

//-----------------------------------------------------------------------
/// Get key value.
//-----------------------------------------------------------------------

template<> double GeotiffFile::get_key(geokey_t K) const
{
  double v;
  int status = GTIFKeyGet(gtif, K, &v, 0, 1);
  if(status != 1)
    throw Exception("Error getting geotiff key");
  return v;
  
}

//-----------------------------------------------------------------------
/// Get key value.
//-----------------------------------------------------------------------

template<> std::string GeotiffFile::get_key(geokey_t K) const
{
  int size = GTIFKeyInfo(gtif, K, 0, 0);
  if(size == 0)
    throw Exception("Error getting geotiff key");
  std::vector<char> v(size);
  if(GTIFKeyGet(gtif, K, &v[0], 0, size) ==0)
    throw Exception("Trouble reading ASCII tag in geotiff file");
  std::string vs(&v[0]);
  return vs;
}

//-----------------------------------------------------------------------
/// Set the tiff tag value.
//-----------------------------------------------------------------------

void GeotiffFile::set_tiftag(tiftag_t K, int V)
{
  int status = TIFFSetField(tif, (ttag_t) K, V);
  if(status != 1)
    throw Exception("Error setting tiff field");
}

//-----------------------------------------------------------------------
/// Set the tiff tag value.
//-----------------------------------------------------------------------

void GeotiffFile::set_tiftag(tiftag_t K, const std::vector<double>& V)
{
  int status = TIFFSetField(tif, (ttag_t) K, V.size(), &V[0]);
  if(status != 1)
    throw Exception("Error setting tiff field");
}

//-----------------------------------------------------------------------
/// As a way to process VICAR geotiff tags, we write out a single 1x1
/// file along with metadata. The 1x1 is just to make a valid geotiff
/// file. We may want to also add writing real images, but right now
/// we have no need for that.
//-----------------------------------------------------------------------

void GeotiffFile::write_1x1_file()
{
  //----------------------------------------------------------------
  // Set other tags needed to make a valid file, and write data.
  // This is just a 1x1 file.
  //----------------------------------------------------------------

  set_tiftag(TIFFTAG_IMAGEWIDTH, 1);
  set_tiftag(TIFFTAG_IMAGELENGTH, 1);
  set_tiftag(TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  set_tiftag(TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  set_tiftag(TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  set_tiftag(TIFFTAG_BITSPERSAMPLE, 8);
  set_tiftag(TIFFTAG_SAMPLESPERPIXEL, 1);
  char c = '\0';
  TIFFWriteEncodedStrip(tif, 0, &c, 1);
}

//-----------------------------------------------------------------------
/// Return true if we have the given tag.
//-----------------------------------------------------------------------

bool GeotiffFile::has_tiftag(tiftag_t K) const
{
  uint32_t count;
  void *data;
  int status = TIFFGetField(tif, K, &count, &data);
  return status == 1;
}


//-----------------------------------------------------------------------
/// Return the value of a tiff tag.
//-----------------------------------------------------------------------

template<> int GeotiffFile::get_tiftag(tiftag_t K) const
{
  int data;
  int status = TIFFGetField(tif, K, &data);
  if(status != 1)
    throw Exception("Trouble retrieving tiftag.");
  return data;
}

//-----------------------------------------------------------------------
/// Return the value of a tiff tag.
//-----------------------------------------------------------------------

template<> std::vector<double> GeotiffFile::get_tiftag(tiftag_t K) const
{
  uint16_t count;
  double *data;
  int status = TIFFGetField(tif, K, &count, &data);
  if(status != 1)
    throw Exception("Trouble retrieving tiftag.");
  std::vector<double> res(data, data + count);
  return res;
}
