#include "vicar_dataset.h"
#include <iostream>

extern "C" {
  void GDALRegister_vicar(void);
}

//-----------------------------------------------------------------------
/// Register the dataset.
//-----------------------------------------------------------------------

void GDALRegister_vicar(void)
{ 
 if (! GDAL_CHECK_VERSION("VICAR"))
        return;
  if(GDALGetDriverByName( "VICAR" ) == NULL) {
    GDALDriver  *poDriver = new GDALDriver();
    poDriver->SetDescription("VICAR");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "VICAR dataset (.img), JPL Version");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, 
			      "frmt_vicar.html");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "img");
    poDriver->pfnOpen = VicarGdal::VicarDataset::Open;
    poDriver->pfnCreate = VicarGdal::VicarDataset::Create;
    GetGDALDriverManager()->RegisterDriver(poDriver);
  }
}
