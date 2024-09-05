#ifndef VICAR_RASTER_BAND_H
#define VICAR_RASTER_BAND_H
#include "gdal_pam.h"
#include <boost/shared_ptr.hpp>

namespace VicarGdal {
  class VicarDataset;
  class VicarFile;
/****************************************************************//**
  This is the VICAR raster band used by GDAL. 
*******************************************************************/

class VicarRasterBand : public GDALPamRasterBand {
public:
  VicarRasterBand(VicarDataset& D, int num_band = 0, int Band_index = 1);
  ~VicarRasterBand() { FlushCache(); }
  virtual CPLErr IReadBlock( int, int, void * );
  virtual CPLErr IWriteBlock( int, int, void * );
  virtual double GetNoDataValue(int * pbSuccess = NULL);
  virtual CPLErr SetNoDataValue(double dfNoData);
private:
  boost::shared_ptr<VicarFile> file;
  int bindex;
};
}
#endif
