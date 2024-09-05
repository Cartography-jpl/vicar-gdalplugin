#include "vicar_dataset.h"
#include "vicar_raster_band.h"
#include "vicar_ogr.h"
#include <memory>
#include <ogr_spatialref.h>	
#define BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <iostream>

using namespace VicarGdal;

VicarOgr ogr;

inline std::string to_s2(double x)
{
  const int bufsize = 1000;
  char buf[bufsize];
  snprintf(buf, bufsize, "%.16e", x);
  return std::string(buf);
}

VicarDataset::VicarDataset(char* filename)
  : have_transform(false), skip_label_write(false), 
    need_geographic_write(false),
    is_point(false),
    need_write_cetag(true),
    need_write_rpc1(true),
    gcp_count(0),
    gcps(0)
{
  transform[0] = 0;
  transform[1] = 1;
  transform[2] = 0;
  transform[3] = 0;
  transform[4] = 0;
  transform[5] = 1;
  // Have special handling for things like "1:8" in file name, which
  // means multiple bands with the given range.
  std::string fname(filename);
  boost::smatch m;
  if(boost::regex_match(fname, m, 
			boost::regex("(.*)(\\d+):(\\d+)(.*)"))) {
    std::string base = m[1];
    int start = boost::lexical_cast<int>(m[2]);
    int end = boost::lexical_cast<int>(m[3]);
    std::string baseend = m[4];
    for(int i = start; i <= end; ++i)
      file.push_back(boost::shared_ptr<VicarFile>
    (new VicarFile(base + boost::lexical_cast<std::string>(i) + baseend)));
  } else
    file.push_back(boost::shared_ptr<VicarFile>(new VicarFile(filename)));
}

VicarDataset::VicarDataset(const std::string& Fname, int Number_line, 
			   int Number_sample, int Number_band,
			   const std::string& Type) 
: have_transform(false), skip_label_write(false), 
  need_geographic_write(false),
  is_point(false),
  need_write_cetag(true),
  need_write_rpc1(true),
  gcp_count(0),
  gcps(0)
{ 
  transform[0] = 0;
  transform[1] = 1;
  transform[2] = 0;
  transform[3] = 0;
  transform[4] = 0;
  transform[5] = 1;
  if(Number_band > 1) {
    boost::smatch m;
    if(boost::regex_match(Fname, m, 
			  boost::regex("([^.]*)(_b1)(\\..*)"))) {
      int nband = 1;
      for(int i = 0; i < Number_band; ++i)
	file.push_back
	  (boost::shared_ptr<VicarFile>
	   (new VicarFile(m[1] + "_b" + 
			  boost::lexical_cast<std::string>(i + 1) + m[3],
			  Number_line, Number_sample, nband, Type)));
    } else {
      file.push_back
	(boost::shared_ptr<VicarFile>
	 (new VicarFile(Fname, Number_line, Number_sample, Number_band, Type)));
    }
  } else
    file.push_back(boost::shared_ptr<VicarFile>
		   (new VicarFile(Fname, Number_line, Number_sample, 1, Type)));
}

//-----------------------------------------------------------------------
///  Open a file.
//-----------------------------------------------------------------------

GDALDataset *VicarDataset::Open(GDALOpenInfo* Info)
{
//-----------------------------------------------------------------------
// Determine if this is VICAR file. This check just sees if the file
// starts with the string "LBLSIZE="
//-----------------------------------------------------------------------

  // Handling for the band range format.
  boost::smatch m;
  std::string fname(Info->pszFilename);
  if(boost::regex_match(fname, m, 
			boost::regex("(.*)(\\d+):(\\d+)(.*)"))) {
    std::string base = m[1];
    std::string start = m[2];
    std::string baseend = m[4];
    if(!VicarFile::is_vicar_file(base + start + baseend))
      return 0;
  } else {
    if(!Info->bStatOK)
      return 0;
    if(Info->nHeaderBytes < 8)
      return 0;
    if(std::string((char*) Info->pabyHeader, 8) != "LBLSIZE=")
      return 0;
  }
  

//-----------------------------------------------------------------------
// Create GDALDataset
//-----------------------------------------------------------------------
  try {
    std::unique_ptr<VicarDataset> ds(new VicarDataset(Info->pszFilename));
    // This is read only, so don't try to write labels if GDAL wants to.
    ds->skip_label_write = true;
    ds->nRasterXSize = ds->file[0]->number_sample();
    ds->nRasterYSize = ds->file[0]->number_line();
    int nband = ds->file[0]->number_band();

//-----------------------------------------------------------------------
// Read band. 
//-----------------------------------------------------------------------

    if(nband == 1)
      for(int i = 0; i < (int) ds->file.size(); ++i)
	ds->SetBand(i + 1, new VicarRasterBand(*ds, i));
    else
      for(int i = 0; i < nband; ++i)
	ds->SetBand(i + 1, new VicarRasterBand(*ds, 0, i + 1));

//-----------------------------------------------------------------------
// Set metadata.
//-----------------------------------------------------------------------

    ds->file[0]->set_metadata(*ds);
    if(ds->file[0]->has_map_info()) {
#if(GDAL_VERSION_MAJOR >= 3)
      ogr.from_vicar(*ds->file[0], ds->sref, ds->transform);
#else      
      ogr.from_vicar(*ds->file[0], ds->proj_ref, ds->transform);
#endif      
      ds->have_transform =  true;
    } else {
#if(GDAL_VERSION_MAJOR < 3)
      ds->proj_ref = "";
#endif      
    }

//-----------------------------------------------------------------------
// Initialize any PAM information. 
//-----------------------------------------------------------------------

    ds->SetDescription(Info->pszFilename );
    ds->TryLoadXML();

//-----------------------------------------------------------------------
// Check for overviews. 
//-----------------------------------------------------------------------

    ds->oOvManager.Initialize(ds.get(), Info->pszFilename );
    return(ds.release());
  } catch(std::exception& e) {
    return 0;
  }
}

//-----------------------------------------------------------------------
///  Create a file
//-----------------------------------------------------------------------

GDALDataset *VicarDataset::Create(const char * pszFilename,
			     int nXSize, int nYSize, int nBands,
			     GDALDataType eType,
			     char ** ParmList)
{
//-----------------------------------------------------------------------
// Create GDALDataset
//-----------------------------------------------------------------------
  std::string type;
  try {
    switch(eType) {
    case GDT_Byte:
      type = "BYTE";
      break;
    case GDT_UInt16:
    case GDT_Int16:
      type = "HALF";
      break;
    case GDT_UInt32:
    case GDT_Int32:
      type = "FULL";
      break;
    case GDT_Float32:
      type = "REAL";
      break;
    case GDT_Float64:
      type = "DOUB";
      break;
    default:
      throw Exception("Unknown element type");
    }
    std::unique_ptr<VicarDataset> ds(new VicarDataset(pszFilename, nYSize, 
						    nXSize, nBands, type));
    ds->skip_label_write = false; // Go ahead and write labels if
				  // requested. 

    ds->nRasterXSize = ds->file[0]->number_sample();
    ds->nRasterYSize = ds->file[0]->number_line();
    ds->eAccess = (ds->file[0]->access() == VicarFile::READ ? 
		   GA_ReadOnly : GA_Update);
    int nband = ds->file[0]->number_band();

//-----------------------------------------------------------------------
// Afids expects to be able to read NITF_NROWS and NITF_NCOLS
//-----------------------------------------------------------------------

    ds->SetMetadataItem("NITF_NROWS", 
	boost::lexical_cast<std::string>(ds->file[0]->number_line()).c_str());
    ds->SetMetadataItem("NITF_NCOLS", 
	boost::lexical_cast<std::string>(ds->file[0]->number_sample()).c_str());

//-----------------------------------------------------------------------
// Read band. 
//-----------------------------------------------------------------------

    if(nband == 1)
      for(int i = 0; i < (int) ds->file.size(); ++i)
	ds->SetBand(i + 1, new VicarRasterBand(*ds, i));
    else
      for(int i = 0; i < nband; ++i)
	ds->SetBand(i + 1, new VicarRasterBand(*ds, 0, i + 1));

//-----------------------------------------------------------------------
// Initialize any PAM information. 
//-----------------------------------------------------------------------

    ds->SetDescription(pszFilename );
    return(ds.release());
  } catch(std::exception&) {
    return 0;
  }
}

//-----------------------------------------------------------------------
/// Get projection transform
//-----------------------------------------------------------------------

CPLErr VicarDataset::GetGeoTransform(double * padfTransform)
{
  for(int i = 0; i < 6; ++i)
    padfTransform[i] = transform[i];
  if(have_transform)
    return CE_None;
  else
    return CE_Failure;
}

//-----------------------------------------------------------------------
/// Get projection reference
//-----------------------------------------------------------------------

#if(GDAL_VERSION_MAJOR < 3)  
const char *VicarDataset::GetProjectionRef()
{
  if(proj_ref == "")
    return 0;
  else
    return proj_ref.c_str();
}
#endif

CPLErr VicarDataset::SetMetadata( char ** papszMetadata,
				  const char * pszDomain)
{
  if(skip_label_write)
    return GDALPamDataset::SetMetadata(papszMetadata, pszDomain);
  while(*papszMetadata != 0) {
    std::string t(*papszMetadata);
    size_t tin = t.find('=');
    CPLErr status = SetMetadataItem(t.substr(0, tin).c_str(),
				    t.substr(tin + 1).c_str(), pszDomain);
    if(status != CE_None)
      return status;
    ++papszMetadata;
  }
  return CE_None;
}

CPLErr VicarDataset::SetGCPs(int Num, const GDAL_GCP* Gcp_list, 
			     const char* Gcp_projection)
{
  LocalSetGCPs(Num, Gcp_list, Gcp_projection);

// We don't normally keep GCP in VICAR, but treat the some of these
// points as special. We translate these to NITF_CORNERLAT1 etc, which
// is what VICAR programs key off of.
  OGRSpatialReference wgs84;
  wgs84.SetWellKnownGeogCS("WGS84");
  OGRSpatialReference inproj;
  if(std::string(Gcp_projection) == "")
    inproj.SetWellKnownGeogCS("WGS84");
  else {
    inproj.importFromWkt(&Gcp_projection);
  }
  boost::shared_ptr<OGRCoordinateTransformation> ogr_transform
    (OGRCreateCoordinateTransformation(&inproj, &wgs84));
  std::vector<double> x, y, z;
  for(int i = 0; i < Num; ++i) {
    x.push_back(Gcp_list[i].dfGCPX);
    y.push_back(Gcp_list[i].dfGCPY);
    z.push_back(Gcp_list[i].dfGCPZ);
  }
  ogr_transform->Transform(Num, &x[0], &y[0], &z[0]);
  for(int i = 0; i < Num; ++i) {
    if(std::string(Gcp_list[i].pszId) == "UpperLeft") {
      label_set("NITF_CORNERLAT1", to_s2(y[i]), "GEOTIFF");
      label_set("NITF_CORNERLON1", to_s2(x[i]), "GEOTIFF");
    } else if(std::string(Gcp_list[i].pszId) == "UpperRight") {
      label_set("NITF_CORNERLAT2", to_s2(y[i]), "GEOTIFF");
      label_set("NITF_CORNERLON2", to_s2(x[i]), "GEOTIFF");
    } else if(std::string(Gcp_list[i].pszId) == "LowerRight") {
      label_set("NITF_CORNERLAT3", to_s2(y[i]), "GEOTIFF");
      label_set("NITF_CORNERLON3", to_s2(x[i]), "GEOTIFF");
    } else if(std::string(Gcp_list[i].pszId) == "LowerLeft") {
      label_set("NITF_CORNERLAT4", to_s2(y[i]), "GEOTIFF");
      label_set("NITF_CORNERLON4", to_s2(x[i]), "GEOTIFF");
    } else {
      std::cerr << "Unrecognized GCP ID '" << Gcp_list[i].pszId 
		<< "', ignoring point.\n";
    }
  }
  return CE_None;
}

CPLErr VicarDataset::SetMetadataItem(const char* Name, const char* Value,
				     const char* Domain)
{
  if(skip_label_write)
    return GDALPamDataset::SetMetadataItem(Name, Value, Domain);
  std::string nm(Name);
  std::string val(Value);
  if(nm == "AREA_OR_POINT") {
    if(val == "Point")
      is_point = true;
    else
      is_point = false;
    return CE_None;
  }
  if(nm.length() > 5 &&
     nm.substr(0, 5) == "NITF_") {
    label_set(nm, val, "GEOTIFF");
    if(nm == "NITF_CETAG")
      need_write_cetag = false;
    return CE_None;
  }
  // We want stuff from a TRE to go into the geotiff property. We
  // can't do this directly, so we use this special name to indicate
  // this is something that should go into geotiff property
  if(nm.length() > 4 &&
     nm.substr(0, 4) == "TRE_") {
    label_set(nm.substr(4), val, "GEOTIFF");
    return CE_None;
  }
  if(Domain && Domain == std::string("RPC")) {
    if(need_write_cetag) {
      // Default to B type. If later metadata gives a different value,
      // then this gets overriden.
      label_set("NITF_CETAG", "RPC00B", "GEOTIFF");
      need_write_cetag = false;
    }
    if(need_write_rpc1) {
      label_set("RPC_FIELD1", "1", "GEOTIFF");
      label_set("RPC_FIELD2", "0", "GEOTIFF");
      label_set("RPC_FIELD3", "0", "GEOTIFF");
      need_write_rpc1 = false;
    }
    if(nm == "LINE_OFF") label_set("RPC_FIELD4", val, "GEOTIFF");
    if(nm == "SAMP_OFF") label_set("RPC_FIELD5", val, "GEOTIFF");
    if(nm == "LAT_OFF") label_set("RPC_FIELD6", val, "GEOTIFF");
    if(nm == "LONG_OFF") label_set("RPC_FIELD7", val, "GEOTIFF");
    if(nm == "HEIGHT_OFF") label_set("RPC_FIELD8", val, "GEOTIFF");
    if(nm == "LINE_SCALE") label_set("RPC_FIELD9", val, "GEOTIFF");
    if(nm == "SAMP_SCALE") label_set("RPC_FIELD10", val, "GEOTIFF");
    if(nm == "LAT_SCALE") label_set("RPC_FIELD11", val, "GEOTIFF");
    if(nm == "LONG_SCALE") label_set("RPC_FIELD12", val, "GEOTIFF");
    if(nm == "HEIGHT_SCALE") label_set("RPC_FIELD13", val, "GEOTIFF");
    if(nm == "LINE_NUM_COEFF" || nm == "LINE_DEN_COEFF" || 
       nm == "SAMP_NUM_COEFF" || nm == "SAMP_DEN_COEFF") {
      std::string base;
      if(nm == "LINE_NUM_COEFF")
	base = "RPC_FIELD14";
      if(nm == "LINE_DEN_COEFF")
	base = "RPC_FIELD15";
      if(nm == "SAMP_NUM_COEFF")
	base = "RPC_FIELD16";
      if(nm == "SAMP_DEN_COEFF")
	base = "RPC_FIELD17";
      int fstart = 0;
      for(int i = 1; i <= 20; ++i) {
	while(val[fstart] == ' ' && fstart < (int) val.length())
	  ++fstart;
	int nelem = 0;
	while(val[fstart + nelem] != ' ' && fstart + nelem < (int) val.length())
	  ++nelem;
	if(nelem > 0) {
	  std::string is = boost::lexical_cast<std::string>(i);
	  label_set(base + is, val.substr(fstart, nelem), "GEOTIFF");
	  fstart += nelem + 1;
	}
      }
    }
    return CE_None;
  }
  // We sometimes get labels that we should not actually write. This
  // can happen when we roundtrip a file (e.g., convert VICAR to
  // another format and then back, where metadata is copied both
  // places). Just silently ignore this, rather that causing an error.
  std::string name_str(Name);
  if(name_str == "BHOST" ||
     name_str == "BINTFMT" ||
     name_str == "BLTYPE" ||
     name_str == "BREALFMT" ||
     name_str == "BUFSIZ" ||
     name_str == "COMPRESS" ||
     name_str == "DIM" ||
     name_str == "EOCI1" ||
     name_str == "EOCI2" ||
     name_str == "EOL" ||
     name_str == "FORMAT" ||
     name_str == "HOST" ||
     name_str == "INTFMT" ||
     name_str == "LBLSIZE" ||
     name_str == "N1" ||
     name_str == "N2" ||
     name_str == "N3" ||
     name_str == "N4" ||
     name_str == "NB" ||
     name_str == "NBB" ||
     name_str == "NL" ||
     name_str == "NLB" ||
     name_str == "NS" ||
     name_str == "ORG" ||
     name_str == "REALFMT" ||
     name_str == "RECSIZE" ||
     name_str == "TYPE")
    ;
  else
    label_set(Name, Value, (Domain ? Domain : ""));
  return CE_None;
}

void VicarDataset::label_set(const std::string& Name, const std::string& Val, 
			     const std::string& Property)
{
  BOOST_FOREACH(boost::shared_ptr<VicarFile>& i, file)
    i->label_set(Name, Val, Property);
}

VicarDataset::~VicarDataset() 
{
  if(need_geographic_write) {
    BOOST_FOREACH(boost::shared_ptr<VicarFile>& i, file)
#if(GDAL_VERSION_MAJOR >= 3)
      ogr.to_vicar(sref, transform, is_point, *i);
#else      
      ogr.to_vicar(proj_ref, transform, is_point, *i);
#endif    
  }
  FlushCache();
  CPLFree(gcps);
}
