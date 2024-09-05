#include "vicar_raster_band.h"
#include "vicar_dataset.h"
using namespace VicarGdal;

//-----------------------------------------------------------------------
/// Open a file
//-----------------------------------------------------------------------

VicarRasterBand::VicarRasterBand(VicarDataset& D, int num_band, 
				 int Band_index) 
  : file(D.file[num_band]), bindex(Band_index)
{
  poDS = &D;
  switch(file->type()) {
  case VicarFile::VICAR_BYTE:
    eDataType = GDT_Byte;
    break;
  case VicarFile::VICAR_HALF:
    eDataType = GDT_Int16;
    break;
  case VicarFile::VICAR_FULL:
    eDataType = GDT_Int32;
    break;
  case VicarFile::VICAR_FLOAT:
    eDataType = GDT_Float32;
    break;
  case VicarFile::VICAR_DOUBLE:
    eDataType = GDT_Float64;
    break;
  default:
    throw Exception("Unrecognized element type");
  }
  nBlockXSize = D.GetRasterXSize();
  nBlockYSize = 1;
}

//-----------------------------------------------------------------------
/// Read a block of data.
//-----------------------------------------------------------------------

CPLErr VicarRasterBand::IReadBlock( int Sample, int Line, void *Data) 
{
  if(Sample != 0) {
    CPLError( CE_Failure, CPLE_AppDefined, 
	      "Sample passed to VicarRasterBand::IReadBlock needs to be 0");
    return CE_Failure;
  }
  // If we have write access, then reopen to get update access instead.
  if(file->access() == VicarFile::WRITE)
      file->reopen_file();

  // +1 is because VICAR is 1 based line number.
  int status = file->zvreadw(Data, Line + 1, bindex);
  if(status != 1) {
    CPLError( CE_Failure, CPLE_AppDefined, 
	      "Call to zvreadw failed.");
    return CE_Failure;
  }

// Vicar is odd about reading to the end of file, it will give errors
// if we go back and try to read earlier parts of the file.
// If we have read the last line, then close and reopen the file.
  if(Line == file->number_line() - 1)
    file->reopen_file();
  
  return CE_None;
}

//-----------------------------------------------------------------------
/// Write a block of data.
//-----------------------------------------------------------------------

CPLErr VicarRasterBand::IWriteBlock( int Sample, int Line, void *Data) 
{
  if(Sample != 0) {
    CPLError( CE_Failure, CPLE_AppDefined, 
	      "Sample passed to VicarRasterBand::IReadBlock needs to be 0");
    return CE_Failure;
  }

  // +1 is because VICAR is 1 based line number.
  int status = file->zvwritw(Data, Line + 1, bindex);
  if(status != 1) {
    CPLError( CE_Failure, CPLE_AppDefined, 
	      "Call to zvwritw failed.");
    return CE_Failure;
  }
  return CE_None;
}

//-----------------------------------------------------------------------
/// Get no data value.
//-----------------------------------------------------------------------

double VicarRasterBand::GetNoDataValue(int * pbSuccess )
{
  if(file->label_type().count("NODATA") == 1) {
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;
    return (double) file->label<float>("NODATA");
  } else {
    if( pbSuccess != NULL )
        *pbSuccess = FALSE;
    return -1e10;
  }
}

//-----------------------------------------------------------------------
/// Set no data value.
//-----------------------------------------------------------------------
CPLErr VicarRasterBand::SetNoDataValue(double dfNoData)
{
  file->label_set("NODATA", (float) dfNoData);
  return CE_None;
}
