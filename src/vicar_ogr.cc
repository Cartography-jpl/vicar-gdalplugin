#include "vicar_ogr.h"
#include "geotiff_file.h"
#include <boost/utility.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

using namespace VicarGdal;

//-----------------------------------------------------------------------
// Temporary file
//-----------------------------------------------------------------------
namespace VicarGdal {
namespace VicarOgrNsp {
class TempFile {
public:
  TempFile()
  {
    temp_f = boost::filesystem::temp_directory_path();
    temp_f /= boost::filesystem::unique_path();
  }
  virtual ~TempFile()
  {
    boost::filesystem::remove(temp_f);
  }
  const std::string& temp_fname() const {return temp_f.string();}
private:
  boost::filesystem::path temp_f;
};

}
}
//-----------------------------------------------------------------------
/// Constructor. Normally we only have one of these objects around,
/// although nothing is hurt if you create multiple copies.
//-----------------------------------------------------------------------

VicarOgr::VicarOgr()
{
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_ascii()) {
    tag_to_vicar_name[(int) t] = GeotiffFile::key_name_uppercase(t);
    vicar_name.insert(GeotiffFile::key_name_uppercase(t));
  }
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_double()) {
    tag_to_vicar_name[(int) t] = GeotiffFile::key_name_uppercase(t);
    vicar_name.insert(GeotiffFile::key_name_uppercase(t));
  }
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_short()) {
    tag_to_vicar_name[(int) t] = GeotiffFile::key_name_uppercase(t);
    vicar_name.insert(GeotiffFile::key_name_uppercase(t));
  }    
  vicar_name.insert("MODELPIXELSCALETAG");
  vicar_name.insert("MODELTIEPOINTTAG");
  vicar_name.insert("MODELTRANSFORMATIONTAG");
  // Make sure we have registered all the drivers, including geotiff.
  //GdalRegister::gdal_register();
}

//-----------------------------------------------------------------------
/// We can use the same code to create a gtiff file with the same
/// labels as either a VicarFile or a VicarLiteFile. We use a template
/// so we don't need to replace the code.
//-----------------------------------------------------------------------

template<class T> void VicarOgr::vicar_to_gtiff_template(const T& F, const char*Fname) const
{
  using namespace VicarOgrNsp;
  // Write out all of the keys we find in the VICAR file into the
  // temporary file.
  GeotiffFile g(Fname, "w");
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_short()) {
    if(F.has_label("GEOTIFF " + tag_to_vicar_name[t])) {
      std::string tv = F.label_string(tag_to_vicar_name[t],
				      "GEOTIFF");
      GeotiffFile::geocode_t v = (GeotiffFile::geocode_t) atoi(tv.c_str());
      g.set_key(t, v);
    }
  }
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_double()) {
    if(F.has_label("GEOTIFF " + tag_to_vicar_name[t])) {
      std::string tv = F.label_string(tag_to_vicar_name[t],
				      "GEOTIFF");
      double v = atof(tv.c_str());
      g.set_key(t, v);
    }
  }
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_ascii()) {
    if(F.has_label("GEOTIFF " + tag_to_vicar_name[t])) {
      std::string v = F.label_string(tag_to_vicar_name[t], "GEOTIFF");
      g.set_key(t, v);
    }
  }
  if(F.has_label("GEOTIFF MODELPIXELSCALETAG")) {
    std::string vs = F.label_string("MODELPIXELSCALETAG", "GEOTIFF");
    BOOST_FOREACH(char& c, vs)
      if(c == '(' || c == ')' || c == ',')
	c = ' ';
    size_t istart = 0;
    size_t iend = 0;
    std::vector<double> v(3);
    for(int i = 0; i < 3; ++i) {
      istart = vs.find_first_not_of(" ", iend);
      iend = vs.find_first_of(" ", istart);
      v[i] = atof(vs.substr(istart, iend - istart).c_str());
    }
    g.set_tiftag(GeotiffFile::TIFFTAG_GEOPIXELSCALE, v);
  }
  if(F.has_label("GEOTIFF MODELTIEPOINTTAG")) {
    std::string vs = F.label_string("MODELTIEPOINTTAG", "GEOTIFF");
    BOOST_FOREACH(char& c, vs)
      if(c == '(' || c == ')' || c == ',')
	c = ' ';
    size_t istart = 0;
    size_t iend = 0;
    std::vector<double> v(6);
    for(int i = 0; i < 6; ++i) {
      istart = vs.find_first_not_of(" ", iend);
      iend = vs.find_first_of(" ", istart);
      v[i] = atof(vs.substr(istart, iend - istart).c_str());
    }
    g.set_tiftag(GeotiffFile::TIFFTAG_GEOTIEPOINTS, v);
  }
  if(F.has_label("GEOTIFF MODELTRANSFORMATIONTAG")) {
    std::string vs = F.label_string("MODELTRANSFORMATIONTAG", "GEOTIFF");
    BOOST_FOREACH(char& c, vs)
      if(c == '(' || c == ')' || c == ',')
	c = ' ';
    size_t istart = 0;
    size_t iend = 0;
    std::vector<double> v(16);
    for(int i = 0; i < 16; ++i) {
      istart = vs.find_first_not_of(" ", iend);
      iend = vs.find_first_of(" ", istart);
      v[i] = atof(vs.substr(istart, iend - istart).c_str());
    }
    g.set_tiftag(GeotiffFile::TIFFTAG_GEOTRANSMATRIX, v);
  }

  //----------------------------------------------------------------
  // Special handling for the Geographic case. VICAR files sometimes
  // leave off the GeographicTypeGeoKey in the Geographic case. This 
  // is required by GDAL to know what to do with the file. So check
  // if we have a geographic model w/o the GeographicTypeGeoKey
  // specified, and if so add a WGS84 key. The Key is "4326", and
  // this corresponds to "GCS_WGS_84".
  //----------------------------------------------------------------

  if(!F.has_label("GEOTIFF GEOGRAPHICTYPEGEOKEY") &&
     F.has_label("GEOTIFF GTMODELTYPEGEOKEY") &&
     atoi(F.label_string("GTMODELTYPEGEOKEY", "GEOTIFF").c_str()) 
     == 2)
    g.set_key(GeotiffFile::GeographicTypeGeoKey, (GeotiffFile::geocode_t) 4326);
  
  //----------------------------------------------------------------
  // Set other tags needed to make a valid file, and write data.
  // This is just a 1x1 file.
  //----------------------------------------------------------------

  g.write_1x1_file();
  g.write_key();
}

//-----------------------------------------------------------------------
/// We can use the same code to get MapInfo for either a VicarFile or
/// a VicarLiteFile. We use a template here so we don't need to repeat
/// our code
//-----------------------------------------------------------------------

#if(GDAL_VERSION_MAJOR >= 3)
template<class T> void VicarOgr::from_vicar_template
(const T& F, boost::shared_ptr<OGRSpatialReference>& Sref,
 double* Transform) const
#else
template<class T> void VicarOgr::from_vicar_template
(const T& F, std::string& Proj_ref, double* Transform) const
#endif  
{
  using namespace VicarOgrNsp;
  TempFile f;
  vicar_to_gtiff_template(F, f.temp_fname().c_str());

  boost::shared_ptr<GDALDataset> 
    ds((GDALDataset *) GDALOpen(f.temp_fname().c_str(), GA_ReadOnly));
#if(GDAL_VERSION_MAJOR >= 3)
  const OGRSpatialReference* sref = ds->GetSpatialRef();
  Sref.reset(sref ? sref->Clone() : nullptr);
#else  
  Proj_ref = std::string(ds->GetProjectionRef());
#endif  
  ds->GetGeoTransform(Transform);
}


//-----------------------------------------------------------------------
/// Create a 1 pixel geotiff file that contains all the geotiff labels
/// from the Vicar file.
//-----------------------------------------------------------------------

void VicarOgr::vicar_to_gtiff(const VicarFile& F, const std::string& Fname) const
{
  vicar_to_gtiff_template(F, Fname.c_str());
}

//-----------------------------------------------------------------------
/// Read the metadata from a Vicar File, and use to create a MapInfo
/// based on GDAL.
//-----------------------------------------------------------------------

#if(GDAL_VERSION_MAJOR >= 3)
void VicarOgr::from_vicar
(const VicarFile& F, boost::shared_ptr<OGRSpatialReference>& Sref,
 double* Transform) const
{
  return from_vicar_template(F, Sref, Transform);
}
#else
void VicarOgr::from_vicar(const VicarFile& F, std::string& Proj_ref,
			  double* Transform) const
{
  return from_vicar_template(F, Proj_ref, Transform);
}
#endif
//-----------------------------------------------------------------------
/// Use a MapInfo to write the GeoTIFF metadata for a Vicar File.
//-----------------------------------------------------------------------

#if(GDAL_VERSION_MAJOR >= 3)
void VicarOgr::to_vicar(const boost::shared_ptr<OGRSpatialReference>& Sref,
			const double *Transform,
			bool Is_point, VicarFile& F) const
#else
void VicarOgr::to_vicar(const std::string& Proj_ref, const double *Transform,
			bool Is_point, VicarFile& F) const
#endif
{
  using namespace VicarOgrNsp;
  TempFile ft;

//----------------------------------------------------------------
// Shrink MapInfo down to 1 pixel, since we don't need to create a
// full size GeoTIFF file
//----------------------------------------------------------------

  GDALDriver* driver = (GDALDriver*) GDALGetDriverByName("gtiff");
  boost::shared_ptr<GDALDataset> 
    ds(driver->Create(ft.temp_fname().c_str(), 1, 1, 1, GDT_Byte, 0));
#if(GDAL_VERSION_MAJOR >= 3)
  ds->SetSpatialRef(Sref.get());
#else
  ds->SetProjection(Proj_ref.c_str());
#endif  
  ds->SetGeoTransform(const_cast<double*>(Transform));
  if(Is_point)
    ds->SetMetadataItem("AREA_OR_POINT", "Point", "");
  ds.reset();			// Force flush

  // Now open the file back as a GeoTIFF file and copy keys we find to
  // VICAR.

  GeotiffFile g(ft.temp_fname(), "r");
  const int bufsize = 1000;
  char buf[bufsize];
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_short()) {
    if(g.has_key(t)) {
      GeotiffFile::geocode_t v = g.get_key<GeotiffFile::geocode_t>(t);
      snprintf(buf, bufsize, "%d", v);
      std::string s(buf);
      s = s + "(" + GeotiffFile::value_name(t, v) + ")";
      F.label_set(tag_to_vicar_name[t], s, "GEOTIFF");
    }
  }
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_double()) {
    if(g.has_key(t)) {
      double v = g.get_key<double>(t);
      snprintf(buf, bufsize, "%.12g", v);
      std::string s(buf);
      F.label_set(tag_to_vicar_name[t], s, "GEOTIFF");
    }
  }
  BOOST_FOREACH(GeotiffFile::geokey_t t, GeotiffFile::geotiff_tag_ascii()) {
    if(g.has_key(t)) {
      std::string v = g.get_key<std::string>(t);
      F.label_set(tag_to_vicar_name[t], v, "GEOTIFF");
    }
  }
  
  if(g.has_tiftag(GeotiffFile::TIFFTAG_GEOPIXELSCALE)) {
    std::vector<double> scale =
      g.get_tiftag<std::vector<double> >(GeotiffFile::TIFFTAG_GEOPIXELSCALE);
    if(scale.size() == 2)	// Fill in 0.0 for third entry if
				// missing
      snprintf(buf, bufsize, "(%.12g,%.12g,0.0)", scale[0], scale[1]);
    else if(scale.size() == 3)
      snprintf(buf, bufsize, "(%.12g,%.12g,%.12g)", scale[0], scale[1],
	       scale[2]);
    else
      throw Exception("Not sure how to interpret TIFFTAG_GEOPIXELSCALE");
    std::string s(buf);
    F.label_set("MODELPIXELSCALETAG", s, "GEOTIFF");
  }
  if(g.has_tiftag(GeotiffFile::TIFFTAG_GEOTIEPOINTS)) {
    std::vector<double> tiepoint =
      g.get_tiftag<std::vector<double> >(GeotiffFile::TIFFTAG_GEOTIEPOINTS);
    if(tiepoint.size() != 6)
      throw Exception("Not sure how to interpret TIFFTAG_GEOTIEPOINTS");
    snprintf(buf,bufsize, "(%.12g,%.12g,%.12g,%.12g,%.12g,%.12g)", tiepoint[0], 
	     tiepoint[1], tiepoint[2], tiepoint[3], tiepoint[4], tiepoint[5]);
    std::string s(buf);
    F.label_set("MODELTIEPOINTTAG", s, "GEOTIFF");
  }
  if(g.has_tiftag(GeotiffFile::TIFFTAG_GEOTRANSMATRIX)) {
    std::vector<double> transform =
      g.get_tiftag<std::vector<double> >(GeotiffFile::TIFFTAG_GEOTRANSMATRIX);
    if(transform.size() != 16)
      throw Exception("Not sure how to interpret TIFFTAG_GEOTRANSMATRIX");
    snprintf(buf,bufsize, "(%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%12g,%12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%12g,%12g)",
	     transform[0], transform[1], transform[2], transform[3],
	     transform[4], transform[5], transform[6], 
	     transform[7], transform[8], transform[9], transform[10],
	     transform[11], transform[12], 
	     transform[13], transform[14], transform[15]);
    std::string s(buf);
    F.label_set("MODELTRANSFORMATIONTAG", s, "GEOTIFF");
  }
}
