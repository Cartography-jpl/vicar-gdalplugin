#ifndef VICAR_DATASET_H
#define VICAR_DATASET_H
#include "gdal_pam.h"
#include "vicar_file.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <iostream>

namespace VicarGdal {
/****************************************************************//**
  This is the VICAR data set used by GD AL. 
*******************************************************************/

class VicarDataset : public GDALPamDataset {
public:
  VicarDataset(char* filename);
  VicarDataset(const std::string& Fname, int Number_line, int Number_sample,
	       int Number_band,
	       const std::string& Type);
  virtual ~VicarDataset();
  static GDALDataset *Open(GDALOpenInfo*);
  static GDALDataset *Create(const char * pszFilename,
			     int nXSize, int nYSize, int nBands,
			     GDALDataType eType,
			     char ** ParmList);
  virtual CPLErr SetMetadata( char ** papszMetadata,
			      const char * pszDomain = "" );
  virtual CPLErr SetMetadataItem(const char* Name, const char* Value,
				 const char* Domain = "");
  CPLErr GetGeoTransform( double * padfTransform );
#if(GDAL_VERSION_MAJOR >= 3)  
  virtual const OGRSpatialReference* GetSpatialRef() const
  {
    return sref.get();
  }
  CPLErr SetSpatialRef(const OGRSpatialReference* poSRS)
  {
    sref.reset((poSRS ? poSRS->Clone() : nullptr));
    need_geographic_write = true;
    return CE_None;
  }
#else  
  const char *GetProjectionRef();
  virtual CPLErr SetProjection( const char * P)
  { proj_ref = std::string(P); need_geographic_write = true; return CE_None;}
#endif  
  virtual CPLErr SetGeoTransform( double * T)
  {
    for(int i = 0; i < 6; ++i) 
      transform[i] = T[i]; 
    need_geographic_write = true;
    have_transform = true;
    return CE_None;
  }
  virtual int GetGCPCount() { return gcp_count; }
  virtual const char* GetGCPProjection() {return gcp_projection.c_str();}
  virtual const GDAL_GCP* GetGCPs() {return gcps; }
  virtual CPLErr LocalSetGCPs(int Num, const GDAL_GCP* Gcp_list, 
			    const char* Gcp_projection)
  { gcp_count = Num;
    gcp_projection = std::string(Gcp_projection);
    CPLFree(gcps);
    gcps = GDALDuplicateGCPs(gcp_count, Gcp_list);
    return CE_None;
  }
  virtual CPLErr SetGCPs(int Num, const GDAL_GCP* Gcp_list, 
			 const char* Gcp_projection);
  std::vector<boost::shared_ptr<VicarFile> > file;
#if(GDAL_VERSION_MAJOR >= 3)
  boost::shared_ptr<OGRSpatialReference> sref;
#else  
  std::string proj_ref;
#endif  
  double transform[6];
  bool have_transform;
  bool skip_label_write;
  bool need_geographic_write;
  bool is_point;
  void label_set(const std::string& Name, const std::string& Val, 
	    const std::string& Property);
private:
  // We want to make sure to set the NITF_CETAG even if it isn't
  // directly set. We set this the first time we write another RPC
  // value out.
  bool need_write_cetag;
  // Need to write out fields 1 -3 of RPC. This doesn't actually get
  // used for anything by VICAR, but it does look for them in the
  // file. 
  bool need_write_rpc1;
  int gcp_count;
  GDAL_GCP* gcps;
  std::string gcp_projection;
};
}
#endif
