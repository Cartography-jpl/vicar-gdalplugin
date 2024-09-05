#include "vicar_file.h"
#include "vicar_dataset.h"
#include "vicar_gdal_exception.h"
#include "vicar_ogr.h"
#include "ogr_spatialref.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <string>
#include <cstdio>
#define BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#include <boost/lexical_cast.hpp>
#undef EQUAL
#define HAVE_VICAR_RTL		// We'll always have vicar available
				// when we compile this
#ifdef HAVE_VICAR_RTL
#include <zvproto.h>
#include <errdefs.h>
#endif
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace VicarGdal;

int VicarFile::instance = 1;

void VicarGdal::VicarFile::set_metadata(VicarDataset& G) const
{
  using namespace std;
  typedef std::map<std::string, 
		   std::map<std::string, std::string> >::value_type cint;
  typedef std::map<std::string, std::string>::value_type cint2;
  VicarOgr vogr;
  BOOST_FOREACH(cint i,  keyword_equal) {
    if(i.first != "GEOTIFF") {	// We handle GEOTIFF separately.
      BOOST_FOREACH(cint2 j, i.second)
	G.SetMetadataItem(j.first.c_str(), j.second.c_str(), i.first.c_str());
    }
    if(i.first == "GEOTIFF") {	// We handle GEOTIFF separately.
      BOOST_FOREACH(cint2 j, i.second) {
	if(j.first.length() > 4 &&
	   j.first.substr(0,5) == "RPC_")
	  // Skip RPC, we handle this separately
	  ;
	else
	  // Skip geotiff labels, we handle this separately
	  if(!vogr.is_geotiff_vicar_name(j.first))
	    G.SetMetadataItem(j.first.c_str(), j.second.c_str());
      }
    }
  }
  if(has_rpc())
    set_metadata_rpc(G);
  if(label_type().count("GEOTIFF GTRASTERTYPEGEOKEY") != 0) {
    std::istringstream ise(label<std::string>("GTRASTERTYPEGEOKEY", "GEOTIFF"));
    int rt_id;
    ise >> rt_id;
    if(rt_id ==2)
      G.SetMetadataItem("AREA_OR_POINT", "Point");
    else
      G.SetMetadataItem("AREA_OR_POINT", "Area");
  }
  string g = "GEOTIFF";
  if(label_type().count("GEOTIFF NITF_CORNERLAT1") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLAT2") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLAT3") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLAT4") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLON1") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLON2") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLON3") != 0 &&
     label_type().count("GEOTIFF NITF_CORNERLON4") != 0) {
    int ngcp = 4;
    GDAL_GCP* gcps = (GDAL_GCP*) CPLMalloc(sizeof(GDAL_GCP) * ngcp);
    GDALInitGCPs(ngcp, gcps);
    gcps[0].dfGCPPixel	= 0.5;
    gcps[0].dfGCPLine = 0.5;
    gcps[1].dfGCPPixel = number_sample()-0.5;
    gcps[1].dfGCPLine = 0.5;
    gcps[2].dfGCPPixel = number_sample()-0.5;
    gcps[2].dfGCPLine = number_line()-0.5;
    gcps[3].dfGCPPixel = 0.5;
    gcps[3].dfGCPLine = number_line()-0.5;
    gcps[0].dfGCPX = atof(label<string>("NITF_CORNERLON1",g).c_str());
    gcps[0].dfGCPY = atof(label<string>("NITF_CORNERLAT1",g).c_str());
    gcps[1].dfGCPX = atof(label<string>("NITF_CORNERLON2",g).c_str());
    gcps[1].dfGCPY = atof(label<string>("NITF_CORNERLAT2",g).c_str());
    gcps[2].dfGCPX = atof(label<string>("NITF_CORNERLON3",g).c_str());
    gcps[2].dfGCPY = atof(label<string>("NITF_CORNERLAT3",g).c_str());
    gcps[3].dfGCPX = atof(label<string>("NITF_CORNERLON4",g).c_str());
    gcps[3].dfGCPY = atof(label<string>("NITF_CORNERLAT4",g).c_str());
    CPLFree(gcps[0].pszId );
    gcps[0].pszId = CPLStrdup( "UpperLeft" );
    CPLFree(gcps[1].pszId );
    gcps[1].pszId = CPLStrdup( "UpperRight" );
    CPLFree(gcps[2].pszId );
    gcps[2].pszId = CPLStrdup( "LowerRight" );
    CPLFree(gcps[3].pszId );
    gcps[3].pszId = CPLStrdup( "LowerLeft" );
    char* projection = 0;
    OGRSpatialReference  t;
    t.SetWellKnownGeogCS("WGS84");
    t.exportToWkt(&projection);
    G.LocalSetGCPs(ngcp, gcps, projection);
    CPLFree(gcps);
    CPLFree(projection);
  }
}

//-----------------------------------------------------------------------
/// Read metadata for Rpc.
//-----------------------------------------------------------------------

void VicarFile::set_metadata_rpc(GDALMajorObject& G) const
{
  using std::string;
  string g = "GEOTIFF";
  G.SetMetadataItem("LINE_OFF", label<string>("RPC_FIELD4",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("SAMP_OFF", label<string>("RPC_FIELD5",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("LAT_OFF", label<string>("RPC_FIELD6",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("LONG_OFF", label<string>("RPC_FIELD7",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("HEIGHT_OFF", label<string>("RPC_FIELD8",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("LINE_SCALE", label<string>("RPC_FIELD9",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("SAMP_SCALE", label<string>("RPC_FIELD10",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("LAT_SCALE", label<string>("RPC_FIELD11",  g).c_str(),
		    "RPC");
  G.SetMetadataItem("LONG_SCALE", label<string>("RPC_FIELD12",  g).c_str(), 
		    "RPC");
  G.SetMetadataItem("HEIGHT_SCALE", label<string>("RPC_FIELD13",  g).c_str(), 
		    "RPC");
  std::string line_num = "";
  std::string line_den = "";
  std::string samp_num = "";
  std::string samp_den = "";
  for(int i = 1; i <= 20; ++i) {
    std::string is = boost::lexical_cast<std::string>(i);
    line_num += label<string>("RPC_FIELD14" + is, g) + " ";
    line_den += label<string>("RPC_FIELD15" + is, g) + " ";
    samp_num += label<string>("RPC_FIELD16" + is, g) + " ";
    samp_den += label<string>("RPC_FIELD17" + is, g) + " ";
  }
  G.SetMetadataItem("LINE_NUM_COEFF", line_num.c_str(), "RPC");
  G.SetMetadataItem("LINE_DEN_COEFF", line_den.c_str(), "RPC");
  G.SetMetadataItem("SAMP_NUM_COEFF", samp_num.c_str(), "RPC");
  G.SetMetadataItem("SAMP_DEN_COEFF", samp_den.c_str(), "RPC");
}


//-----------------------------------------------------------------------
/// Return a VICAR unit number that is tied to the given file name.
//-----------------------------------------------------------------------

int VicarFile::file_name_to_unit(const std::string& Fname)
{
#ifdef HAVE_VICAR_RTL
  int res;
  int status = zvunit(&res, const_cast<char*>("NONEGDAL"), instance, 
		      const_cast<char*>("U_NAME"), Fname.c_str(), 
		      NULL);
  instance += 1;
  if(status != 1)
    throw VicarException(status, "Call to zvunit failed for file " + Fname);
  // Suppress copying of labels from primary input to the output file.
  zvselpi(0);
  return res;
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Determine if a given file is a VICAR file or not.
///
/// This looks for the first few characters of the file being "LBLSIZE="
//-----------------------------------------------------------------------

bool VicarFile::is_vicar_file(const std::string& Fname)
{
  std::ifstream in(Fname.c_str());
  char buf[8  + 1];		// 8 is length of "LBLSIZE="
  buf[8] = '\0';
  in.read(buf, 8);
  return (std::string(buf) == "LBLSIZE=");
}

//-----------------------------------------------------------------------
/// Open an existing VICAR file for reading or update.
//-----------------------------------------------------------------------

VicarFile::VicarFile(const std::string& Fname, access_type Access)
: fname_(Fname), unit_(-1), access_(Access)
{
  unit_ = file_name_to_unit(Fname);
  open_unit();
}

//-----------------------------------------------------------------------
/// Create a new VICAR file with the given size.
//-----------------------------------------------------------------------

VicarFile::VicarFile(const std::string& Fname, int Number_line, 
		     int Number_sample, int Number_band,
		     const std::string& Type,
		     const std::string& Org,
		     compression Compress)
  : fname_(Fname), 
    unit_(-1), number_line_(Number_line), 
    number_sample_(Number_sample), number_band_(Number_band), access_(WRITE)
{
#ifdef HAVE_VICAR_RTL
  if(Org == "BIP")
    throw Exception("Note that BIP doesn't seem to currently work. This hasn't been worth fixing, so we'll need to fix it only if you actually need BIP");
  unit_ = file_name_to_unit(Fname);
  int status;
  switch(Compress) {
  case NONE:
    status = zvopen(unit(), "OP", "WRITE", "U_NL", Number_line, "U_NS",
		    Number_sample, "U_NB", Number_band, "U_ORG", Org.c_str(),
		    "O_FORMAT", Type.c_str(), 
		    "U_FORMAT", Type.c_str(), NULL);
    break;
  case BASIC:
    status = zvopen(unit(), "OP", "WRITE", "U_NL", Number_line, "U_NS",
		    Number_sample, "U_NB", Number_band, "U_ORG", Org.c_str(),
		    "O_FORMAT", Type.c_str(), 
		    "U_FORMAT", Type.c_str(), "COMPRESS", "BASIC", NULL);
    break;
  case BASIC2:
    status = zvopen(unit(), "OP", "WRITE", "U_NL", Number_line, "U_NS",
		    Number_sample, "U_NB", Number_band, "U_ORG", Org.c_str(),
		    "O_FORMAT", Type.c_str(), 
		    "U_FORMAT", Type.c_str(), "COMPRESS", "BASIC2", NULL);
    break;
  default:
    throw Exception("Unrecognized compression type");
  }    
  if(status != 1)
    throw VicarException(status, "Call to zvopen failed for file " + fname_);
  set_type(Type);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Initialize type_ from the string.
//-----------------------------------------------------------------------

void VicarFile::set_type(const std::string& Type) 
{
  if(Type =="BYTE")
    type_ = VICAR_BYTE;
  else if(Type =="HALF")
    type_ = VICAR_HALF;
  else if(Type =="FULL")
    type_ = VICAR_FULL;
  else if(Type =="REAL")
    type_ = VICAR_FLOAT;
  else if(Type =="DOUB")
    type_ = VICAR_DOUBLE;
  else
    throw Exception("Unrecognized type");
}

//-----------------------------------------------------------------------
/// Create a new VICAR file with the given size. Use the VICAR Name
/// and Instance input (so for example, "INP" and 2 is the second INP
/// file passed to a VICAR program. 
//-----------------------------------------------------------------------

VicarFile::VicarFile(int Instance, int Number_line, int Number_sample,
		     int Number_band,
		     const std::string& Type,
		     const std::string& Name, 
		     const std::string& Org,
		     compression Compress)
  : unit_(-1), number_line_(Number_line), number_sample_(Number_sample),
    number_band_(Number_band),
    access_(WRITE)
{
#ifdef HAVE_VICAR_RTL
  if(Org == "BIP")
    throw Exception("Note that BIP doesn't seem to currently work. This hasn't been worth fixing, so we'll need to fix it only if you actually need BIP");
  int status = zvunit(&unit_, const_cast<char*>(Name.c_str()), Instance, 
		      NULL);
  fname_ = Name + " Instance: " + boost::lexical_cast<std::string>(Instance);
  if(status != 1)
    throw VicarException(status, "Call to zvunit failed for file " + fname_);
  switch(Compress) {
  case NONE:
    status = zvopen(unit(), "OP", "WRITE", "U_NL", Number_line, "U_NS",
		    Number_sample, "O_FORMAT", Type.c_str(), 
		    "U_NB", Number_band, "U_ORG", Org.c_str(),
		    "U_FORMAT", Type.c_str(), 
		    NULL);
    break;
  case BASIC:
    status = zvopen(unit(), "OP", "WRITE", "U_NL", Number_line, "U_NS",
		    Number_sample, "O_FORMAT", Type.c_str(), 
		    "U_NB", Number_band, "U_ORG", Org.c_str(),
		    "U_FORMAT", Type.c_str(), "COMPRESS", "BASIC", 
		    NULL);
    break;
  case BASIC2:
    status = zvopen(unit(), "OP", "WRITE", "U_NL", Number_line, "U_NS",
		    Number_sample, "O_FORMAT", Type.c_str(), 
		    "U_NB", Number_band, "U_ORG", Org.c_str(),
		    "U_FORMAT", Type.c_str(), "COMPRESS", "BASIC2", 
		    NULL);
    break;
  default:
    throw Exception("Unrecognized compression type");
  }    

  if(status != 1)
    throw VicarException(status, "Call to zvopen failed for file " + fname_);
  set_type(Type);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Open a file, using the VICAR Name and Instance input (so for
/// example, "INP" and 2 is the second INP file passed to a VICAR program.
//-----------------------------------------------------------------------

VicarFile::VicarFile(int Instance, access_type Access, 
		     const std::string& Name)
  : unit_(-1), access_(Access)
{
#ifdef HAVE_VICAR_RTL
  int status = zvunit(&unit_, const_cast<char*>(Name.c_str()), Instance, 
		      NULL);
  fname_ = Name + " Instance: " + boost::lexical_cast<std::string>(Instance);
  if(status != 1)
    throw VicarException(status, "Call to zvunit failed for file " + fname_);
  open_unit();
#else
  throw VicarNotAvailableException();
#endif
}

//  There is some weird conflict between GDALMajorObject and defines
//  header file. Not worth tracking down, instead we just do inclusion
//  after all the GDALMajorObject stuff has been done.

#ifdef HAVE_VICAR_RTL
#include <defines.h>
#endif

//-----------------------------------------------------------------------
/// Once a unit has been generated, open the file and process the
/// labels. 
//-----------------------------------------------------------------------

void VicarFile::open_unit()
{
#ifdef HAVE_VICAR_RTL
  std::string a = (access_ == READ ? "READ" :
		   (access_ == UPDATE ? "UPDATE" : "WRITE"));
  int status = zvopen(unit(), "OP", a.c_str(), NULL);
  if(status != 1)
    throw VicarException(status, "Call to zvopen failed for file " + fname_);

// Property is a nesting of labels. Because it is convenient, we store
// regular labels and nested labels together in label_type_ and
// label_nelement_. This is because other than the nesting and a
// slightly different call sequence, these get treated the same.

// First, get a list of all properties.
  int nprop = 1;
  int ulen = MAX_LABEL_KEY_SIZE + 1;
  std::vector<char> buf(ulen * nprop);
  int npropret;
  status = zlpinfo(unit(), &buf[0], &nprop, "NRET", &npropret, "ULEN", 
		   ulen, NULL);
  if(status != 1)
    throw VicarException(status, "Call to zlpinfo failed");
  nprop = (npropret > 0 ? npropret : 1);
  buf.resize(ulen * nprop);
  status = zlpinfo(unit(), &buf[0], &nprop, "ULEN", ulen, NULL);
  if(status != 1)
    throw VicarException(status, "Call to zlpinfo failed");
  std::vector<std::string> prop_list;
  for(int i = 0; i < nprop; ++i)
    prop_list.push_back(std::string(&buf[i * ulen]));
  prop_set_.insert(prop_list.begin(), prop_list.end());
  std::string prop = "";
  std::vector<std::string>::const_iterator next_prop = prop_list.begin();
  std::map<std::string, std::vector<std::string> > prop_to_keywords;
  while(status ==1) {
    char key[MAX_LABEL_KEY_SIZE + 1];	
    char format[9];
    int maxlength, nelement;
    status = zlninfo(unit(), key, format, &maxlength, &nelement, NULL);
    if(status == 1) {		// Error handling after this loop
      std::string f(format);
      std::string k(key);
      if(k == "PROPERTY") 	// Starting a new property.
	prop = *next_prop++;
      else if(k =="TASK")	// End of property section
	prop = "";
      else {
	prop_to_keywords[prop].push_back(k);
	if(prop != "")
	  k = prop + " " + k;
	if(f == "INT")
	  label_type_[k] = VicarFile::VICAR_INT;
	else if(f =="REAL")
	  label_type_[k] = VicarFile::VICAR_REAL;
	else if(f =="STRING") {
	  label_type_[k] = VicarFile::VICAR_STRING;
	  label_maxlength_[k] = maxlength;
	} else
	  throw Exception("Unrecognized label type " + f + " found in file " + 
			  fname_);
	label_nelement_[k] = nelement;
      }
    }
  }
  if(status != END_OF_LABEL)
    throw VicarException(status, "zlninfo failed for file " + fname_);
  number_line_ = label<int>("NL");
  number_sample_ = label<int>("NS");
  number_band_ = label<int>("NB");
  set_type(label<std::string>("FORMAT"));
  typedef std::map<std::string, std::vector<std::string> >::value_type vtype;
  BOOST_FOREACH(vtype i, prop_to_keywords) {
    BOOST_FOREACH(const std::string& j, i.second) 
      try {
	keyword_equal[i.first][j] = label_string(j, i.first);
      } catch(const VicarException&) {
	// Don't worry about labels we can't read. The parser gets
	// confused sometimes and tries to read some labels. We
	// can just ignore these kinds of problems.
      }
  }
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Destructor, closes file.
//-----------------------------------------------------------------------

VicarFile::~VicarFile()
{
  close();
}

//-----------------------------------------------------------------------
/// Close file.
//-----------------------------------------------------------------------

void VicarFile::close()
{
#ifdef HAVE_VICAR_RTL
  if(unit() != -1) {
    int status = zvclose(unit(), "CLOS_ACT", "FREE", NULL);
    if(status != 1 && status != FILE_NOT_OPEN) {
      VicarException e(status);
      e << "Call to zvclose failed for unit " << unit() << " file name " 
	<< file_name();
      throw e;
    }
    unit_ = -1;
  }
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Return true if the file has a GEOTIFF label in it, indicating it
/// has map information.
//-----------------------------------------------------------------------

bool VicarFile::has_map_info() const
{
  if(!(has_label("GEOTIFF MODELTRANSFORMATIONTAG") ||
       has_label("GEOTIFF MODELTIEPOINTTAG")))
    return false;
  return true;
}

//-----------------------------------------------------------------------
/// Return true if the file has a NITF_CETAG label in it, indicating it
/// has RPC information.
//-----------------------------------------------------------------------

bool VicarFile::has_rpc() const
{
  return (has_label("GEOTIFF RPC_FIELD4"));
}

// //-----------------------------------------------------------------------
// /// Read metadata for MapInfo.
// //-----------------------------------------------------------------------

// MapInfo VicarFile::map_info() const
// {
//   if(map_info_.get())
//     return *map_info_;
//   if(!has_map_info())
//     throw Exception("Attempt to call map_info() on a file that doesn't have it.");
// #ifdef HAVE_GDAL
//   map_info_.reset(new MapInfo(vogr.from_vicar(*this)));
//   return *map_info_;
// #else
//   throw Exception("The current implementation of VicarFile::map_info uses the GDAL library, which was not found during the build of this library");
// #endif
// }

// //-----------------------------------------------------------------------
// /// Set metadata for MapInfo.
// //-----------------------------------------------------------------------

// void VicarFile::map_info(const MapInfo& M)
// {
//   map_info_.reset(new MapInfo(M));
// #ifdef HAVE_GDAL
//   return vogr.to_vicar(M, *this);
// #else
//   throw Exception("The current implementation of VicarFile::map_info uses the GDAL library, which was not found during the build of this library");
// #endif
// }

//-----------------------------------------------------------------------
/// Delete a label from a file.
//-----------------------------------------------------------------------

void VicarFile::label_delete(const std::string& F, const std::string& Property)
{
#ifdef HAVE_VICAR_RTL
  int status;
  if(Property == "") {
    status = zldel(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY)
      throw VicarException(status, "Call to zldel failed");
  } else {
    status = zldel(unit(), const_cast<char*>("PROPERTY"),
		   const_cast<char*>(F.c_str()), 
		   const_cast<char*>("PROPERTY"), Property.c_str(), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY && status != NO_SUCH_PROPERTY)
      throw VicarException(status, "Call to zldel failed");
  }
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Set the value of a label. If the label is already in the file, is 
/// is deleted and replaced with this new value. Otherwise, it is
/// simply added. Optionally the label can be part of a Property.
//-----------------------------------------------------------------------

void VicarFile::label_set(const std::string& F, 
			  int Val,
			  const std::string& Property)
{
#ifdef HAVE_VICAR_RTL
  int status;
  if(Property == "") {
    status = zldel(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY)
      throw VicarException(status, "Call to zldel failed");
    status = zladd(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), 
		   &Val,
		   const_cast<char*>("FORMAT"), const_cast<char*>("INT"), NULL);
    if(status != 1)
      throw VicarException(status, "Call to zladd failed");
  } else {
    status = zldel(unit(), const_cast<char*>("PROPERTY"),
		   const_cast<char*>(F.c_str()), 
		   const_cast<char*>("PROPERTY"), Property.c_str(), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY && status != NO_SUCH_PROPERTY)
      throw VicarException(status, "Call to zldel failed");
    status = zladd(unit(), const_cast<char*>("PROPERTY"), 
		   const_cast<char*>(F.c_str()), 
		   &Val,
		   const_cast<char*>("PROPERTY"), Property.c_str(),
		   const_cast<char*>("FORMAT"), const_cast<char*>("INT"), NULL);
    if(status != 1)
      throw VicarException(status, "Call to zladd failed");
  }
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Set the value of a label. If the label is already in the file, is 
/// is deleted and replaced with this new value. Otherwise, it is
/// simply added. Optionally the label can be part of a Property.
//-----------------------------------------------------------------------

void VicarFile::label_set(const std::string& F, 
			  float Val,
			  const std::string& Property)
{
#ifdef HAVE_VICAR_RTL
  int status;
  if(Property == "") {
    status = zldel(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY)
      throw VicarException(status,"Call to zldel failed");
    status = zladd(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), 
		   &Val,
		   const_cast<char*>("FORMAT"), const_cast<char*>("REAL"), 
		   NULL);
    if(status != 1)
      throw VicarException(status,"Call to zladd failed");
  } else {
    status = zldel(unit(), const_cast<char*>("PROPERTY"), 
		   const_cast<char*>(F.c_str()), 
		   const_cast<char*>("PROPERTY"), Property.c_str(), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY && status != NO_SUCH_PROPERTY)
      throw VicarException(status,"Call to zldel failed");
    status = zladd(unit(), const_cast<char*>("PROPERTY"), 
		   const_cast<char*>(F.c_str()), 
		   &Val,
		   const_cast<char*>("PROPERTY"), Property.c_str(),
		   const_cast<char*>("FORMAT"), const_cast<char*>("REAL"), 
		   NULL);
    if(status != 1)
      throw VicarException(status,"Call to zladd failed");
  }
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// Set the value of a label. If the label is already in the file, is 
/// is deleted and replaced with this new value. Otherwise, it is
/// simply added. Optionally the label can be part of a Property.
//-----------------------------------------------------------------------

void VicarFile::label_set(const std::string& F, 
			  const std::string& Val,
			  const std::string& Property)
{
#ifdef HAVE_VICAR_RTL
  int status;
  if(Property == "") {
    status = zldel(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY)
      throw VicarException(status,"Call to zldel failed");
    status = zladd(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(F.c_str()), 
		   const_cast<char*>(Val.c_str()),
		   const_cast<char*>("FORMAT"), const_cast<char*>("STRING"), 
		   NULL);
    if(status != 1)
      throw VicarException(status,"Call to zladd failed");
  } else {
    status = zldel(unit(), const_cast<char*>("PROPERTY"), 
		   const_cast<char*>(F.c_str()), 
		   const_cast<char*>("PROPERTY"), Property.c_str(), NULL);
    if(status != 1 && status != CANNOT_FIND_KEY && status != NO_SUCH_PROPERTY)
      throw VicarException(status,"Call to zldel failed");
    status = zladd(unit(), const_cast<char*>("PROPERTY"), 
		   const_cast<char*>(F.c_str()), 
		   const_cast<char*>(Val.c_str()),
		   const_cast<char*>("PROPERTY"), Property.c_str(),
		   const_cast<char*>("FORMAT"), 
		   const_cast<char*>("STRING"), NULL);
    if(status != 1)
      throw VicarException(status,"Call to zladd failed");
  }
#else
  throw VicarNotAvailableException();
#endif
}

// Couple of helper routines for formating rpc labels

inline std::string to_s1(double x)
{
  const int bufsize = 1000;
  char buf[bufsize];
  snprintf(buf, bufsize, "%.4f", x);
  return std::string(buf);
}

inline std::string to_s2(double x)
{
  const int bufsize = 1000;
  char buf[bufsize];
  snprintf(buf, bufsize, "%.16e", x);
  return std::string(buf);
}

//-----------------------------------------------------------------------
/// Set metadata for Rpc.
//-----------------------------------------------------------------------

// void VicarFile::rpc(const Rpc& V)
// {
//   if(V.rpc_type ==Rpc::RPC_B)
//     label_set("NITF_CETAG", "RPC00B", "GEOTIFF");
//   else if(V.rpc_type ==Rpc::RPC_A)
//     label_set("NITF_CETAG", "RPC00A", "GEOTIFF");
//   else 
//     throw Exception("Unrecognized rpc type");
//   label_set("RPC_FIELD1", "1", "GEOTIFF");
//   label_set("RPC_FIELD2", "0", "GEOTIFF");
//   label_set("RPC_FIELD3", "0", "GEOTIFF");
//   label_set("RPC_FIELD4", to_s1(V.line_offset), "GEOTIFF");
//   label_set("RPC_FIELD5", to_s1(V.sample_offset), "GEOTIFF");
//   label_set("RPC_FIELD6", to_s1(V.latitude_offset), "GEOTIFF");
//   label_set("RPC_FIELD7", to_s1(V.longitude_offset), "GEOTIFF");
//   label_set("RPC_FIELD8", to_s1(V.height_offset), "GEOTIFF");
//   label_set("RPC_FIELD9", to_s1(V.line_scale), "GEOTIFF");
//   label_set("RPC_FIELD10", to_s1(V.sample_scale), "GEOTIFF");
//   label_set("RPC_FIELD11", to_s1(V.latitude_scale), "GEOTIFF");
//   label_set("RPC_FIELD12", to_s1(V.longitude_scale), "GEOTIFF");
//   label_set("RPC_FIELD13", to_s1(V.height_scale), "GEOTIFF");
//   for(int i = 1; i <= 20; ++i) {
//     std::string is = boost::lexical_cast<std::string>(i);
//     label_set("RPC_FIELD14" + is, 
// 	      to_s2(V.line_numerator[i - 1]), "GEOTIFF");
//     label_set("RPC_FIELD15" + is, 
// 	      to_s2(V.line_denominator[i - 1]), "GEOTIFF");
//     label_set("RPC_FIELD16" + is, 
// 	      to_s2(V.sample_numerator[i - 1]), "GEOTIFF");
//     label_set("RPC_FIELD17" + is, 
// 	      to_s2(V.sample_denominator[i - 1]), "GEOTIFF");
//   }

//   // VICAR expects NITF corners when it finds an RPC. We estimiate this
//   // by finding the corners at the height offset of the RPC.
//   SimpleDem d(V.height_offset);
//   Geodetic g1 = V.ground_coordinate(ImageCoordinate(0, 0), d);
//   Geodetic g2 = V.ground_coordinate(ImageCoordinate(0, number_sample() - 1), d);
//   Geodetic g3 = V.ground_coordinate(ImageCoordinate(number_line() - 1, number_sample() - 1), d);
//   Geodetic g4 = V.ground_coordinate(ImageCoordinate(number_line() - 1, 0), d);
//   label_set("NITF_CORNERLAT1", to_s2(g1.latitude()), "GEOTIFF");
//   label_set("NITF_CORNERLON1", to_s2(g1.longitude()), "GEOTIFF");
//   label_set("NITF_CORNERLAT2", to_s2(g2.latitude()), "GEOTIFF");
//   label_set("NITF_CORNERLON2", to_s2(g2.longitude()), "GEOTIFF");
//   label_set("NITF_CORNERLAT3", to_s2(g3.latitude()), "GEOTIFF");
//   label_set("NITF_CORNERLON3", to_s2(g3.longitude()), "GEOTIFF");
//   label_set("NITF_CORNERLAT4", to_s2(g4.latitude()), "GEOTIFF");
//   label_set("NITF_CORNERLON4", to_s2(g4.longitude()), "GEOTIFF");
// }

//-----------------------------------------------------------------------
/// Close and reopen the file. Vicar is odd about reading to the end
/// of the file, and we sometimes need to reopen it to clear end of
/// file status.
//-----------------------------------------------------------------------

void VicarFile::reopen_file() const 
{
#ifdef HAVE_VICAR_RTL
  int status = zvclose(unit(), NULL);
  if(status != 1 && status != FILE_NOT_OPEN) {
    VicarException e(status);
    e << "Call to zvclose failed for unit " << unit() << " file name " 
      << file_name();
    throw e;
  }
  // Use UPDATE access for both WRITE and UPDATE, since you can't
  // reopen a file for WRITE.
  std::string a = (access_ == READ ? "READ" : "UPDATE");
  status = zvopen(unit(), "OP", a.c_str(), NULL);
  if(status != 1)
    throw VicarException(status, "Call to zvopen failed for file " + fname_);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zvread. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zvreadw(void* buffer, int Line, int Band) const
{
#ifdef HAVE_VICAR_RTL
  int status = zvread(unit(), buffer, "LINE", Line, "BAND", Band, 
		      "NBANDS", 1, NULL);

// Vicar is odd about reading to the end of file. If we get this
// error, then try closing and reopening file and then doing the read.

  if(status ==END_OF_FILE) {
    reopen_file();
    status = zvread(unit(), buffer, "LINE", Line, "BAND", Band, 
		    "NBANDS", 1, NULL);
  }
  return status;
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zvread. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zvreadw(void* buffer, int Band) const
{
#ifdef HAVE_VICAR_RTL
  int status = zvread(unit(), buffer, "BAND", Band, "NBANDS", 1, NULL);

// Vicar is odd about reading to the end of file. If we get this
// error, then try closing and reopening file and then doing the read.

  if(status ==END_OF_FILE) {
    reopen_file();
    status = zvread(unit(), buffer, "BAND", Band, "NBANDS", 1, NULL);
  }
  return status;
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zvwrit. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zvwritw(void* buffer, int Band)
{
#ifdef HAVE_VICAR_RTL
  return zvwrit(unit(), buffer, "BAND", Band, "NBANDS", 1, NULL);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zvwrit. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zvwritw(void* buffer, int Line, int Band)
{
#ifdef HAVE_VICAR_RTL
  return zvwrit(unit(), buffer, "LINE", Line, "BAND", Band, NULL);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zlget. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zlgetw(int unit, const char* type, const char* key, char* value)
{
#ifdef HAVE_VICAR_RTL
  return zlget(unit, const_cast<char*>(type), 
	       const_cast<char*>(key), value, NULL);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zlget. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zlgetsh(int unit, const char* key, char* value)
{
#ifdef HAVE_VICAR_RTL
  int status = VicarFile::zlgetw(unit, "SYSTEM", const_cast<char*>(key), value);
  if(status ==CANNOT_FIND_KEY)
    status = VicarFile::zlgetw(unit, "HISTORY", const_cast<char*>(key), value);
  return status;
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zlget. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zlgetsh(int unit, const char* key, char* value, int ulen)
{
#ifdef HAVE_VICAR_RTL
  int status = VicarFile::zlgetw(unit, "SYSTEM", const_cast<char*>(key), 
				 value, ulen);
  if(status ==CANNOT_FIND_KEY)
    status = VicarFile::zlgetw(unit, "HISTORY", const_cast<char*>(key), 
			       value, ulen);
  return status;
#else
  throw VicarNotAvailableException();
#endif
}


//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zlget. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zlgetw(int unit, const char* type, const char* key, 
		      char* value, const char* prop)
{
#ifdef HAVE_VICAR_RTL
  return zlget(unit, const_cast<char*>(type), const_cast<char*>(key), 
	       value, "PROPERTY", prop, NULL);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zlget. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zlgetw(int unit, const char* type, const char* key, 
		      char* value, int ulen)
{
#ifdef HAVE_VICAR_RTL
  return zlget(unit, const_cast<char*>(type), const_cast<char*>(key), 
	       value, "ULEN", ulen, NULL);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zlget. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zlgetw(int unit, const char* type, const char* key, 
		      char* value, int ulen, const char* prop)
{
#ifdef HAVE_VICAR_RTL
  return zlget(unit, const_cast<char*>(type), const_cast<char*>(key), 
	       value, "ULEN", ulen, "PROPERTY", prop, NULL);
#else
  throw VicarNotAvailableException();
#endif
}

//-----------------------------------------------------------------------
/// This is a thin wrapper around the RTL function zvzinit. If the
/// VICAR library is installed then we just forward to that library,
/// otherwise we throw an exception saying that it isn't available.
//-----------------------------------------------------------------------

int VicarFile::zvzinitw(int argc, char *argv[])
{
#ifdef HAVE_VICAR_RTL
  return zvzinit(argc, argv);
#else
  throw VicarNotAvailableException();
#endif
  
}

//-----------------------------------------------------------------------
/// Return true if vicar functionality is available, otherwise return
/// false. We have vicar functionality of the library was configured
/// to use the VICAR library.
//-----------------------------------------------------------------------

bool VicarFile::vicar_available()
{
#ifdef HAVE_VICAR_RTL
  return true;
#else
  return false;
#endif
}

//-----------------------------------------------------------------------
/// Return value for given label. This returns a string regardless of
/// the underlying type.
//-----------------------------------------------------------------------

std::string VicarFile::label_string(const std::string& K, 
				       const std::string& P) const
{
#ifdef HAVE_VICAR_RTL
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  // We might have a int or double value that we want to look at as a
  // string. In that case, just use a "large enough" buffer.
  int bufsize = 1000;
  if((*label_type().find(t)).second == VICAR_STRING)
    bufsize = (*label_maxlength_.find(t)).second + 1;
  std::vector<char> buf(bufsize);
  int status;
  if(P != "") {
    status = zlget(unit(), const_cast<char*>("PROPERTY"), 
		   const_cast<char*>(K.c_str()),
		   reinterpret_cast<char*>(&(*buf.begin())), 
		   "PROPERTY", P.c_str(), "FORMAT", "STRING", NULL);
  } else {
    status = zlget(unit(), const_cast<char*>("SYSTEM"), 
		   const_cast<char*>(K.c_str()),
		   reinterpret_cast<char*>(&(*buf.begin())), 
		   "FORMAT", "STRING", NULL);
    if(status ==CANNOT_FIND_KEY)
      status = zlget(unit(), const_cast<char*>("HISTORY"), 
		     const_cast<char*>(K.c_str()),
		     reinterpret_cast<char*>(&(*buf.begin())), 
		     "FORMAT", "STRING", NULL);
  }
  if(status != 1)
    throw VicarException(status,"zlget failed for label " + K + " of file " + fname_);
  return std::string(&(*buf.begin()));
#else
  throw VicarNotAvailableException();
#endif
}

