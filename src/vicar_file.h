#ifndef VICAR_FILE_H
#define VICAR_FILE_H
#include <gdal_priv.h>
#include "gdal_pam.h"
#include "vicar_gdal_exception.h"
// #include "map_info.h"
// #include "geocal_rpc.h"
#include <boost/utility.hpp>
#include <map>
#include <set>
#include <vector>
#include <string>

namespace VicarGdal {
  class VicarDataset;
//********************************************************************
// Note that when you port to a different machine, these types may
// need to be changed.
//********************************************************************
  typedef unsigned char VicarByte;
  typedef short int VicarHalf;
  typedef int VicarFull;
  typedef float VicarFloat;
  typedef double VicarDouble;
  namespace VicarType {
    template<class T> std::string vicar_type();
    template<> inline std::string vicar_type<VicarByte>() { return "BYTE";}
    template<> inline std::string vicar_type<VicarHalf>() { return "HALF";}
    template<> inline std::string vicar_type<VicarFull>() { return "FULL";}
    template<> inline std::string vicar_type<VicarFloat>() { return "REAL";}
    template<> inline std::string vicar_type<VicarDouble>() { return "DOUB";}
  }

/****************************************************************//**
  This handles opening and closing a Vicar file. This class doesn't
  actually read and write data, but is used by other classes which do.

  This uses the VICAR RTL, which is documented at 
  http://www-mipl.jpl.nasa.gov/RTL

  A note for developers. The Vicar RTL library uses various
  variable argument functions. You need to end the list of arguments
  with "NULL". Note that a cast to a point type is *mandatory*, you
  can't just say "0". If say "0" then you will get fairly difficult to
  track down errors. Without the cast, this gets passed as an int,
  which the RTL library code will then try to interpret as a char *.
  va_args has undefined behavior when called with the wrong type,
  ranging from seeming to work to core dumping.

  Because of the complication involved, we have separated out the
  functionality of reading and writing MapInfo metadata. This is done
  by the class VicarOgr. This is handled behind the scenes, so for a
  user of this class this separation makes no difference. But if you
  are looking for the code that does this, you'll need to look in
  VicarOgr.

  The current implementation of the MapInfo metadata requires the GDAL
  library to be available. If this isn't you can still build the
  GeoCal system but any attempt to read or write MapInfo metadata will
  trigger an exception.
*******************************************************************/

class VicarFile : public boost::noncopyable {
public:
//-----------------------------------------------------------------------
/// Type of each label found in the file
//-----------------------------------------------------------------------
  enum label_type {VICAR_INT, VICAR_REAL, VICAR_STRING};

//-----------------------------------------------------------------------
/// Type of data in file.
//-----------------------------------------------------------------------

  enum data_type {VICAR_BYTE, VICAR_HALF, VICAR_FULL, VICAR_FLOAT,
		  VICAR_DOUBLE};

//-----------------------------------------------------------------------
/// Type of access.
//-----------------------------------------------------------------------

  enum access_type {READ, WRITE, UPDATE};
  enum compression {NONE, BASIC, BASIC2};
  VicarFile(const std::string& Fname, access_type Access = READ);
  VicarFile(const std::string& Fname, int Number_line, int Number_sample,
	    int Number_band,
	    const std::string& Type = "BYTE",
	    const std::string& Org = "BSQ",
	    compression C = NONE);
  VicarFile(int Instance, access_type Access = READ, 
	    const std::string& Name = "INP");
  VicarFile(int Instance, int Number_line, int Number_sample,
	    int Number_band = 1,
	    const std::string& Type = "BYTE",
	    const std::string& Name = "OUT",
	    const std::string& Org = "BSQ",
	    compression C = NONE);
  virtual ~VicarFile();

//-----------------------------------------------------------------------
/// Access type of file.
//-----------------------------------------------------------------------

  access_type access() const {return access_;}

//-----------------------------------------------------------------------
/// File name
//-----------------------------------------------------------------------
  
  const std::string& file_name() const { return fname_; }

  static bool is_vicar_file(const std::string& Fname);

//-----------------------------------------------------------------------
/// Map between Label names and the type of the label.
/// As a convention, we store a label that is part of a property as
/// the string "property label". We can break this out in the future
/// if needed, but for now this seems sufficient.
//-----------------------------------------------------------------------
  
  const std::map<std::string, int>& label_type() const 
  {return label_type_;}

//-----------------------------------------------------------------------
/// Test if a label is found in a file, and if so return
/// true. Otherwise return false.
//-----------------------------------------------------------------------
  bool has_label(const std::string& Lbl) const
  { return label_type_.count(Lbl) != 0; }

//-----------------------------------------------------------------------
/// Map between Label names and the number of elements.
/// As a convention, we store a label that is part of a property as
/// the string "property label". We can break this out in the future
/// if needed, but for now this seems sufficient.
//-----------------------------------------------------------------------
  
  const std::map<std::string, int>& label_nelement() const 
  {return label_nelement_;}

//-----------------------------------------------------------------------
/// List of properties.
//-----------------------------------------------------------------------

  const std::set<std::string>& property() const {return prop_set_;}

//-----------------------------------------------------------------------
/// Return value for the given label. Optionally also supply a
/// property for labels that are part of one (e.g., GEOTIFF)
//-----------------------------------------------------------------------
  
  template<class T> T label(const std::string& F, 
			    const std::string& Property = "") const;


//-----------------------------------------------------------------------
/// Non template form of label, useful in some contexts.
//-----------------------------------------------------------------------

  std::string label_string(const std::string& F, 
			   const std::string& Property = "") const;

  void label_delete(const std::string& F, const std::string& Property = "");
  void label_set(const std::string& F, 
		 int Val,
		 const std::string& Property = "");
  void label_set(const std::string& F, 
		 float Val,
		 const std::string& Property = "");
  void label_set(const std::string& F, 
		 const std::string& Val,
		 const std::string& Property = "");

//-----------------------------------------------------------------------
/// Number of lines in file.
//-----------------------------------------------------------------------

  int number_line() const {return number_line_;}


//-----------------------------------------------------------------------
/// Number of bands in file.
//-----------------------------------------------------------------------

  int number_band() const {return number_band_;}

//-----------------------------------------------------------------------
/// Number of samples in file.
//-----------------------------------------------------------------------

  int number_sample() const {return number_sample_;}

//-----------------------------------------------------------------------
/// Close file
//-----------------------------------------------------------------------

  void close();

//-----------------------------------------------------------------------
/// Flush data to disk.
//-----------------------------------------------------------------------

  void flush() const { reopen_file(); }
  void reopen_file() const;

//-----------------------------------------------------------------------
/// Type of data in file
//-----------------------------------------------------------------------

  data_type type() const {return type_;}

//-----------------------------------------------------------------------
/// Unit number for VicarFile
//-----------------------------------------------------------------------

  int unit() const {return unit_;}

  bool has_map_info() const;
  bool has_rpc() const;
  void set_metadata_rpc(GDALMajorObject& G) const;
  // MapInfo map_info() const;
  // void map_info(const MapInfo& M);
  // Rpc rpc() const;
  // void rpc(const Rpc& R);
  static int file_name_to_unit(const std::string& Fname);
//-----------------------------------------------------------------------
/// Print to stream.
//-----------------------------------------------------------------------

  virtual void print(std::ostream& Os) const 
  { Os << "Vicar File \n" 
       << "  File:          " << file_name() << "\n"
       << "  Number line:   " << number_line() << "\n"
       << "  Number sample: " << number_sample() << "\n";
  }

  int zvreadw(void* buffer, int Line, int Band) const;
  int zvreadw(void* buffer, int Band) const;
  int zvwritw(void* buffer, int Band);
  int zvwritw(void* buffer, int Line, int Band);
  static int zlgetw(int unit, const char* type, const char* key, char* value);
  static int zlgetsh(int unit, const char* key, char* value);
  static int zlgetsh(int unit, const char* key, char* value, int ulen);
  static int zlgetw(int unit, const char* type, const char* key, char* value, 
		    const char* prop);
  static int zlgetw(int unit, const char* type, const char* key, char* value,
		    int ulen);
  static int zlgetw(int unit, const char* type, const char* key, char* value, 
		    int ulen, const char* prop);
  static int zvzinitw(int argc, char *argv[]);

  static bool vicar_available();
  void set_metadata(VicarDataset& G) const;
private:
  //  mutable boost::shared_ptr<MapInfo> map_info_; 
				///< Cache of MapInfo information.
  // Labels as a list of keyword=value pairs. This is used when
  // matching this up with gdal.
  std::map<std::string, std::map<std::string, std::string> > keyword_equal;
  std::set<std::string> prop_set_; ///<List of properties.
  std::string fname_;
  int unit_;
  mutable int number_line_;
  mutable int number_sample_;
  mutable int number_band_;
  access_type access_;
  data_type type_;
  std::map<std::string, int> label_type_;
  std::map<std::string, int> label_nelement_;
  std::map<std::string, int> label_maxlength_;
				///< Maximum length of a string. We
				/// only have entries here for labels
				/// that are type string.
  
  static int instance;		///< Instance number, which must be
				/// unique for each file.
  void open_unit();
  void set_type(const std::string& Type);
};

//-----------------------------------------------------------------------
/// Return value for the given label.
//-----------------------------------------------------------------------
  
template<> inline int VicarFile::label<int>(const std::string& K, 
					    const std::string& P) const
{
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  if((*label_type().find(t)).second != VICAR_INT)
    throw Exception("Label " + K + " is not int type in file " + fname_);
  if((*label_nelement().find(t)).second != 1)
    throw Exception("Label " + K + " is not a single value in file " + fname_);
  int res;
  int status;
  if(P != "") {
    status = VicarFile::zlgetw(unit(), "PROPERTY", 
			       K.c_str(), 
			       reinterpret_cast<char*>(&res), P.c_str());
  } else {
    status = VicarFile::zlgetsh(unit(), K.c_str(),
			       reinterpret_cast<char*>(&res));
  }
  if(status != 1)
    throw VicarException(status, "zlget failed for label " + K + " of file " + fname_);
  return res;
}

//-----------------------------------------------------------------------
/// Return value for the given label.
//-----------------------------------------------------------------------

template<> inline std::vector<int> 
VicarFile::label<std::vector<int> >(const std::string& K, 
				    const std::string& P) const
{
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  if((*label_type().find(t)).second != VICAR_INT)
    throw Exception("Label " + K + " is not int type in file " + fname_);
  std::vector<int> res((*label_nelement().find(t)).second);
  int status;
  if(P != "") {
    status = VicarFile::zlgetw(unit(), "PROPERTY", 
			       K.c_str(), 
			       reinterpret_cast<char*>(&(*res.begin())), 
			       P.c_str());
  } else {
    status = VicarFile::zlgetsh(unit(), K.c_str(), 
			       reinterpret_cast<char*>(&(*res.begin())));
  }
  if(status != 1)
    throw VicarException(status,"zlget failed for label " + K + " of file " + fname_);
  return res;
}

//-----------------------------------------------------------------------
/// Return value for the given label.
//-----------------------------------------------------------------------

template<> inline float VicarFile::label<float>(const std::string& K,
						const std::string& P) const
{
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  if((*label_type().find(t)).second != VICAR_REAL)
    throw Exception("Label " + K + " is not real type in file " + fname_);
  if((*label_nelement().find(t)).second != 1)
    throw Exception("Label " + K + " is not a single value in file " + fname_);
  float res;
  int status;
  if(P != "") {
    status = VicarFile::zlgetw(unit(), "PROPERTY", 
			       K.c_str(), 
			       reinterpret_cast<char*>(&res), 
			       P.c_str());
  } else {
    status = VicarFile::zlgetsh(unit(), K.c_str(), 
				reinterpret_cast<char*>(&res));
  }
  if(status != 1)
    throw VicarException(status,"zlget failed for label " + K + " of file " + fname_);
  return res;
}

//-----------------------------------------------------------------------
/// Return value for the given label.
//-----------------------------------------------------------------------

template<> inline std::vector<float> 
VicarFile::label<std::vector<float> >(const std::string& K,
				      const std::string& P) const
{
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  if((*label_type().find(t)).second != VICAR_REAL)
    throw Exception("Label " + K + " is not real type in file " + fname_);
  std::vector<float> res((*label_nelement().find(t)).second);
  int status;
  if(P != "") {
    status = VicarFile::zlgetw(unit(), "PROPERTY", 
			       K.c_str(), 
			       reinterpret_cast<char*>(&(*res.begin())), 
			       P.c_str());
  } else {
    status = VicarFile::zlgetsh(unit(), K.c_str(), 
				reinterpret_cast<char*>(&(*res.begin())));
  }
  if(status != 1)
    throw VicarException(status,"zlget failed for label " + K + " of file " + fname_);
  return res;
}

//-----------------------------------------------------------------------
/// Return value for the given label.
//-----------------------------------------------------------------------
  
template<> inline std::string 
VicarFile::label<std::string>(const std::string& K,
			      const std::string& P) const
{
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  if((*label_type().find(t)).second != VICAR_STRING)
    throw Exception("Label " + K + " is not string type in file " + fname_);
  if((*label_nelement().find(t)).second != 1)
    throw Exception("Label " + K + " is not a single value in file " + fname_);
  std::vector<char> buf((*label_maxlength_.find(t)).second + 1);
  int status;
  if(P != "") {
    status = VicarFile::zlgetw(unit(), "PROPERTY", 
			       K.c_str(), 
			       reinterpret_cast<char*>(&(*buf.begin())), 
			       P.c_str());
  } else {
    status = VicarFile::zlgetsh(unit(), K.c_str(), 
				reinterpret_cast<char*>(&(*buf.begin())));
  }
  if(status != 1)
    throw VicarException(status,"zlget failed for label " + K + " of file " + fname_);
  return std::string(&(*buf.begin()));
}

template<> inline std::vector<std::string>
VicarFile::label<std::vector<std::string> >(const std::string& K,
					    const std::string& P) const
{
  std::string t = K;
  if(P != "")
    t = P + " " + t;
  if(label_type().count(t) != 1)
    throw Exception("Label " + K + " is not found in file " + fname_);
  if((*label_type().find(t)).second != VICAR_STRING)
    throw Exception("Label " + K + " is not string type in file " + fname_);
  int nelem = (*label_nelement().find(t)).second;
  int ulen = (*label_maxlength_.find(t)).second + 1;
  std::vector<char> buf(nelem * ulen);
  int status;
  if(P != "") {
    status = VicarFile::zlgetw(unit(), "PROPERTY", 
			       K.c_str(), 
			      reinterpret_cast<char*>(&(*buf.begin())), 
			      ulen, P.c_str());
  } else {
    status = VicarFile::zlgetsh(unit(), K.c_str(), 
				reinterpret_cast<char*>(&(*buf.begin())), 
				ulen);
  }
  if(status != 1)
    throw VicarException(status,"zlget failed for label " + K + " of file " + fname_);
  std::vector<std::string> res(nelem);
  for(int i = 0; i < nelem; ++i)
    res[i] = std::string(&(buf[i * ulen]));
  return res;
}
}
#endif
