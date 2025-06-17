//
//  GDAL2Mercator.cpp
//  SwiftGDAL
//
//  Created by MuMuY on 2024/1/23.
//

#include "GDAL2Mercator.hpp"

#include "ogr_api.h"
#include "ogr_srs_api.h"

#include <stdlib.h>
#include <stdint.h>
#include <png.h>

#define PNG_BYTES_TO_CHECK 8

int tP(double dfComplete, const char *pszMessage, void *pProgressArg) {
//    NSLog(@"Progress: %.2f%% - %s\n", dfComplete * 100, pszMessage);
    printf("Progress: %f\n", dfComplete * 100);
    return 1;
}

GDAL2Mercator::GDAL2Mercator(const char *gdal_data_path, const char *proj_lib_path) {
    _tile_size = 256;
    _querysize = _tile_size * 4;
    _tminz = -1;
    _tmaxz = -1;
    _isFileOpened = FALSE;
    
    _bandRange[0][0] = 0;
    _bandRange[0][1] = 255;
    _bandRange[1][0] = 0;
    _bandRange[1][1] = 255;
    _bandRange[2][0] = 0;
    _bandRange[2][1] = 255;
    _bandRange[3][0] = 0;
    _bandRange[3][1] = 255;
    
    _mBrightness = 0.0;
    _mContrast = 0.0;
    _mGamma = 1.0;
    
    _mercator = new GlobalMercator(_tile_size);
    
    GDALAllRegister();
    CPLSetConfigOption("PROJ_LIB", proj_lib_path);
    CPLSetConfigOption("GDAL_DATA", gdal_data_path);
}

GDAL2Mercator::~GDAL2Mercator(void) {
    printf("GDAL2Mercator release\n");
}

void GDAL2Mercator::readFileInfo(GDALDatasetH hSrcDS) {
    _rasterXSize = GDALGetRasterXSize(hSrcDS);
    _rasterYSize = GDALGetRasterYSize(hSrcDS);
    
    char **papszOptions = NULL;
    papszOptions = CSLAddString(papszOptions, "-json");
    GDALInfoOptions *psOptions = GDALInfoOptionsNew(papszOptions, NULL);
    fileInfo = GDALInfo(hSrcDS, psOptions);
    
    bandCount = GDALGetRasterCount(hSrcDS);
    
    for (int t = 1;t <= bandCount;t++) {
        int             bGotMin, bGotMax;
        double          adfMinMax[2];
        adfMinMax[0] = GDALGetRasterMinimum( GDALGetRasterBand( hSrcDS, t), &bGotMin );
        adfMinMax[1] = GDALGetRasterMaximum( GDALGetRasterBand( hSrcDS, t), &bGotMax );
        if( ! (bGotMin && bGotMax) )
            GDALComputeRasterMinMax( GDALGetRasterBand( hSrcDS, t), TRUE, adfMinMax );
        bandsMinMax[t - 1][0] = adfMinMax[0];
        bandsMinMax[t - 1][1] = adfMinMax[1];
    }
}

int GDAL2Mercator::getYTile(int ty, int tz) {
    ///Convert from TMS to XYZ numbering system
    return (pow(2, double(tz)) - 1) - ty;
}

int GDAL2Mercator::nb_data_bands(GDALDatasetH hSrcDS) {
    int tileBands = GDALGetRasterCount(hSrcDS);
    GDALRasterBandH alphaBand = GDALGetMaskBand(GDALGetRasterBand(hSrcDS, 1));
    if ((GDALGetMaskFlags(alphaBand) & GMF_ALPHA) || tileBands == 4 || tileBands == 2) {
        return tileBands - 1;
    } else {
        return tileBands;
    }
}

void GDAL2Mercator::geo_query(int *rb, int *wb, double ulx, double uly, double lrx, double lry, int querysize) {
    int rx = int((ulx - _geoTransform[0]) / _geoTransform[1] + 0.001);
    int ry = int((uly - _geoTransform[3]) / _geoTransform[5] + 0.001);
    int rxsize = max(1, int((lrx - ulx) / _geoTransform[1] + 0.5));
    int rysize = max(1, int((lry - uly) / _geoTransform[5] + 0.5));
    int wxsize;
    int wysize;
    if (querysize == 0) {
        wxsize = rxsize;
        wysize = rysize;
    } else {
        wxsize = querysize;
        wysize = querysize;
    }
    
    int wx = 0;
    if (rx < 0) {
        int rxshift = abs(rx);
        wx = int(float(wxsize) * (float(rxshift) / float(rxsize)));
        wxsize = wxsize - wx;
        rxsize = rxsize - int(float(rxsize) * (float(rxshift) / float(rxsize)));
        rx = 0;
    }
    if ((rx + rxsize) > _rasterXSize) {
        wxsize = int(float(wxsize) * (float(_rasterXSize - rx) / float(rxsize)));
        rxsize = _rasterXSize - rx;
    }
    
    int wy = 0;
    if (ry < 0) {
        int ryshift = abs(ry);
        wy = int(float(wysize) * (float(ryshift) / float(rysize)));
        wysize = wysize - wy;
        rysize = rysize - int(float(rysize) * (float(ryshift) / float(rysize)));
        ry = 0;
    }
    if ((ry + rysize) > _rasterYSize) {
        wysize = int(float(wysize) * (float(_rasterYSize - ry) / float(rysize)));
        rysize = _rasterYSize - ry;
    }
    
    rb[0] = rx;
    rb[1] = ry;
    rb[2] = rxsize;
    rb[3] = rysize;
    
    wb[0] = wx;
    wb[1] = wy;
    wb[2] = wxsize;
    wb[3] = wysize;
}

void GDAL2Mercator::openCOGFileWithTile(const char *cogFile) {
    _cogFile = cogFile;
    GDALDatasetH _hSrcDS = GDALOpen(cogFile, GA_ReadOnly);
    if (_hSrcDS == NULL) {
        printf("Open COG dataset error.\n");
        _isFileOpened = FALSE;
        return;
    }
    
    readFileInfo(_hSrcDS);
    
    if (GDALGetGeoTransform(_hSrcDS, _geoTransform) == CE_None) {
        double ominx = _geoTransform[0];
        double omaxx = _geoTransform[0] + _rasterXSize * _geoTransform[1];
        double omaxy = _geoTransform[3];
        double ominy = _geoTransform[3] - _rasterYSize * _geoTransform[1];
        
        for (int tz = 0;tz < MAXZOOMLEVEL;tz++) {
            double tminxy[2];
            double tmaxxy[2];
            _mercator->MetersToTile(ominx, ominy, tz, tminxy);
            _mercator->MetersToTile(omaxx, omaxy, tz, tmaxxy);
            double tminx = tminxy[0];
            double tminy = tminxy[1];
            double tmaxx = tmaxxy[0];
            double tmaxy = tmaxxy[1];
            tminx = max(0.0, tminx);
            tminy = max(0.0, tminy);
            tmaxx = min(pow(2, double(tz)) - 1, tmaxx);
            tmaxy = min(pow(2, double(tz)) - 1, tmaxy);
            _tminmax[tz][0] = tminx;
            _tminmax[tz][1] = tminy;
            _tminmax[tz][2] = tmaxx;
            _tminmax[tz][3] = tmaxy;
        }
        
        if (_tminz == -1) {
            _tminz = _mercator->ZoomForPixelSize(_geoTransform[1] * max(_rasterXSize, _rasterYSize) / float(_tile_size));
        }
        if (_tmaxz == -1) {
            _tmaxz = _mercator->ZoomForPixelSize(_geoTransform[1]);
            _tmaxz = max(_tminz, _tmaxz);
        }
        
        _tminz = min(_tminz, _tmaxz);
        _isFileOpened = TRUE;
    }
    
    GDALClose(_hSrcDS);
}

int GDAL2Mercator::createTileDetails(int tx, int ty, int tz, int *tiledetails) {
    /// zoomlevel is not range in (0, _tmaxz)
    if (tz > _tmaxz || tz < 0) {
        return 1;
    }
    
    double tminx = _tminmax[tz][0];
    double tminy = _tminmax[tz][1];
    double tmaxx = _tminmax[tz][2];
    double tmaxy = _tminmax[tz][3];
    
    /// x is not range in (0, _tmaxx)
    if (tx > tmaxx || tx < 0) {
        return 1;
    }
    
    int _tmsy = ty;
    for (int tmpy = tmaxy;tmpy > tminy - 1; tmpy--) {
        if (getYTile(tmpy, tz) == ty) {
            _tmsy = tmpy;
            break;
        }
    }
    
    double bound[4];
    _mercator->TileBounds(tx, _tmsy, tz, bound);
    //    printf("%f %f %f %f\n", bound[0], bound[1], bound[2], bound[3]);
    
    int rb[4];
    int wb[4];
    geo_query(rb, wb, bound[0], bound[3], bound[2], bound[1]);
    //    printf("%d %d %d %d %d %d %d %d\n", rb[0], rb[1], rb[2], rb[3], wb[0], wb[1], wb[2], wb[3]);
    
    geo_query(rb, wb, bound[0], bound[3], bound[2], bound[1], _tile_size);
    //    printf("%d %d %d %d %d %d %d %d\n", rb[0], rb[1], rb[2], rb[3], wb[0], wb[1], wb[2], wb[3]);
    
    tiledetails[0] = tx;
    tiledetails[1] = ty;
    tiledetails[2] = tz;
    tiledetails[3] = rb[0];
    tiledetails[4] = rb[1];
    tiledetails[5] = rb[2];
    tiledetails[6] = rb[3];
    tiledetails[7] = wb[0];
    tiledetails[8] = wb[1];
    tiledetails[9] = wb[2];
    tiledetails[10] = wb[3];
    
    //    printf("tx: %d, ytile: %d-%d, tz: %d, rx: %d, ry: %d, rxsize: %d, rysize: %d, wx: %d, wy: %d, wxsize: %d, wysize: %d\n", tiledetails[0], tiledetails[1], ty, tiledetails[2], tiledetails[3], tiledetails[4], tiledetails[5], tiledetails[6], tiledetails[7], tiledetails[8], tiledetails[9], tiledetails[10]);
    
    return 0;
}

int GDAL2Mercator::createTileFile(int *tiledetails, int ty, const char *outputPath) {
    int _tx =       tiledetails[0];
    //    int _ty =       tiledetails[1];
    int _tz =       tiledetails[2];
    int _rx =       tiledetails[3];
    int _ry =       tiledetails[4];
    int _rxsize =   tiledetails[5];
    int _rysize =   tiledetails[6];
    int _wx =       tiledetails[7];
    int _wy =       tiledetails[8];
    int _wxsize =   tiledetails[9];
    int _wysize =   tiledetails[10];
    
    const char *zoomDir = CPLSPrintf("%s/%d", outputPath, _tz);
    mkdir(zoomDir, 0777);
    const char *txDir = CPLSPrintf("%s/%d", zoomDir, _tx);
    mkdir(txDir, 0777);
    
    GDALDatasetH _cogDS = GDALOpen(_cogFile, GA_ReadOnly);
    GDALDriverH memDriver = GDALGetDriverByName("MEM");
    if (memDriver == NULL) {
        printf("MEM Driver is NULL.\n");
        GDALClose(_cogDS);
        return 3;
    }
    
    GDALDriverH pngDriver = GDALGetDriverByName("PNG");
    if (pngDriver == NULL) {
        printf("PNG Driver is NULL.\n");
        GDALClose(_cogDS);
        return 3;
    }
    
    int tileBands = nb_data_bands(_cogDS) + 1;
    
    GDALRasterBandH alphaBand = alphaBand = GDALGetMaskBand(GDALGetRasterBand(_cogDS, 1));
    GDALDataType eDT = GDALGetRasterDataType(GDALGetRasterBand(_cogDS,1));
    GDALDatasetH dsTile = GDALCreate(memDriver, "", _tile_size, _tile_size, tileBands, GDT_Byte, NULL);
    
    if (_rxsize != 0 && _rysize != 0 && _wxsize != 0 && _wysize != 0) {
        for (int b = 1; b <= tileBands; ++b) {
            GDALRasterBandH hSrcBand = GDALGetRasterBand(_cogDS, b);
            GDALRasterBandH hDstBand = GDALGetRasterBand(dsTile, b);
            
            void* pData = CPLMalloc(_wxsize * _wysize * GDALGetDataTypeSize(eDT) / 8);
            CPLErr seErr = GDALRasterIO(hSrcBand, GF_Read, _rx, _ry, _rxsize, _rysize, pData, _wxsize, _wysize, eDT, 0, 0);
            CPLErr deErr = GDALRasterIO(hDstBand, GF_Write, _wx, _wy, _wxsize, _wysize, pData, _wxsize, _wysize, eDT, 0, 0);
            CPLFree(pData);
        }
        
        GDALRasterBandH alphaDstBand = GDALGetRasterBand(dsTile, tileBands);
        void *alphaData = CPLMalloc(_wxsize * _wysize * GDALGetDataTypeSize(eDT) / 8);
        CPLErr seErr = GDALRasterIO(alphaBand, GF_Read, _rx, _ry, _rxsize, _rysize, alphaData, _wxsize, _wysize, eDT, 0, 0);
        CPLErr deErr = GDALRasterIO(alphaDstBand, GF_Write, _wx, _wy, _wxsize, _wysize, alphaData, _wxsize, _wysize, eDT, 0, 0);
        CPLFree(alphaData);
    }
    
    const char* pngFile = CPLSPrintf("%s/%d.png", txDir, ty);
    GDALDatasetH d = GDALCreateCopy(pngDriver, pngFile, dsTile, FALSE, NULL, GDALTermProgress, NULL);
    if (d == NULL) {
        printf("Create Tile PNG error\n");
        GDALClose(dsTile);
        GDALClose(_cogDS);
        return 2;
    }
    
    GDALClose(_cogDS);
    GDALClose(dsTile);
    GDALClose(d);
    return 0;
}

void GDAL2Mercator::toCOGFile(const char *inputFile, const char *outputFile) {
    _inputFile = inputFile;
    GDALDatasetH hSrcDS = GDALOpen(inputFile, GA_ReadOnly);
    if (hSrcDS == NULL) {
        printf("Open input file error.\n");
        return;
    }
    
    GDALDriverH hDriver = GDALGetDriverByName("COG");
    if (hDriver == NULL) {
        printf("Get COG Driver error.\n");
        GDALClose(hSrcDS);
        return;
    }
    
    char **papszOptions = NULL;
    papszOptions = CSLSetNameValue(papszOptions, "BLOCKSIZE", "256");
    papszOptions = CSLSetNameValue(papszOptions, "NUM_THREADS", "ALL_CPUS");
    papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", "IF_SAFER");
    papszOptions = CSLSetNameValue(papszOptions, "TARGET_SRS", "EPSG:3857");
    GDALDatasetH hDstDS = GDALCreateCopy(hDriver, outputFile, hSrcDS, FALSE, papszOptions, progressFunc, NULL);
    if (hDstDS == NULL) {
        printf("Copy Dataset error.\n");
        GDALClose(hSrcDS);
        return;
    }
    
    GDALClose(hSrcDS);
    GDALClose(hDstDS);
    CSLDestroy(papszOptions);
    _cogFile = outputFile;
}

int GDAL2Mercator::readTile(int tx, int ty, int tz, const char *outputPath) {
    int tiledetails[11];
    int result = createTileDetails(tx, ty, tz, tiledetails);
    if (result == 0) {
        result = createTileFile(tiledetails, ty, outputPath);
    }
    return result;
}

int GDAL2Mercator::readGoogleTiles(double lat0, double lon0, double lat1, double lon1, int tz) {
    if (_isFileOpened) {
        int xy0[2];
        int xy1[2];
        _mercator->LatLonToGoogleTile(lat0, lon0, tz, xy0);
        _mercator->LatLonToGoogleTile(lat1, lon1, tz, xy1);
        //        printf("0 - get google tile (lat0: %f - lon0: %f) (x: %d, y: %d)\n", lat0, lon0, xy0[0], xy0[1]);
        //        printf("1 - get google tile (lat0: %f - lon0: %f) (x: %d, y: %d)\n", lat1, lon1, xy1[0], xy1[1]);
        
        rangTileXY[0] = min(xy0[0], xy1[0]);
        rangTileXY[1] = max(xy0[0], xy1[0]);
        rangTileXY[2] = min(xy0[1], xy1[1]);
        rangTileXY[3] = max(xy0[1], xy1[1]);
        
        //        printf("Google Tile (minx: %d, maxx: %d miny: %d maxy: %d)\n", rangTileXY[0], rangTileXY[1], rangTileXY[2], rangTileXY[3]);
        
        return 0;
    }
    
    return 4;
}

