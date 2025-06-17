//
//  GDAL2Mercator.hpp
//  SwiftGDAL
//
//  Created by MuMuY on 2024/1/23.
//

#ifndef GDAL2Mercator_hpp
#define GDAL2Mercator_hpp

#include <stdio.h>
#include <math.h>
#include <iostream>

#include "gdal.h"
#include "gdal_utils.h"
#include "gdalwarper.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include "GlobalMercator.hpp"

#define MAXZOOMLEVEL 32

using namespace std;

class GDAL2Mercator {
private:
    GlobalMercator *_mercator;
    
//    const char *_gdal_data_path;
//    const char *_proj_lib_path;
    const char *_inputFile;
    
    bool _isFileOpened;
    
    int _tile_size;
    int _querysize;
    int _tminz;
    int _tmaxz;
    int _rasterXSize;
    int _rasterYSize;
    
    double _tminmax[MAXZOOMLEVEL][4];
    
    double _geoTransform[6];
    
    /// Color Deal
    int _bandRange[4][2];
    
    int _mBrightness;
    
    int _mContrast;
    
    double _mGamma;
    
    void readFileInfo(GDALDatasetH hSrcDS);
    
    int nb_data_bands(GDALDatasetH hSrcDS);
    
    int getYTile(int ty, int tz);
    
    void geo_query(int *rb, int *wb, double ulx, double uly, double lrx, double lry, int querysize=0);
    
    void generate_base_tiles(void);
    
    int createTileDetails(int tx, int ty, int tz, int *tiledetails);
    
    int createTileFile(int *tiledetails, int ty, const char *outputPath);
    
    // MARK: -
    int colorFilter(int value, int min, int max);
    
    int linearStretchedValue(int value, int min, int max);
    
    int adjustColorComponent(int colorComponent, int alpha, int brightness, int contrast, double gamma);
public:
    GDAL2Mercator(const char *gdal_data_path, const char *proj_lib_path);
    ~GDAL2Mercator(void);
    
    const char *_cogFile;
    
    /// 通过GDALInfo读取的文件信息
    char *fileInfo;
    
    /// 
    int bandCount;
    
    int bandsMinMax[4][2];
    
    /// Mapbox bounds struct [leftTopX, rightBottomY, leftTopY, rightBottomX]
    double file_swne[4];
    
    /// minx, maxx, miny, maxy
    int rangTileXY[4];
    
    GDALProgressFunc progressFunc;
    
//    void readSWNE(const char *inputFile);
    
    void toCOGFile(const char *inputFile, const char *outputFile);
    /// 读取COG文件中的信息，计算出生成Tile需要的计算参数
    /// - Parameter cogFile: cog文件路径
    void openCOGFileWithTile(const char *cogFile);
    
    int readGoogleTiles(double lat0, double lon0, double lat1, double lon1, int tz);
    /// 读取指定位置的Tile(Google)，保存成PNG文件
    /// 0 - 成功, 1 - 入参错误, 2 - 生成Tile错误, 3 - 缺少GDAL驱动, 4 - 原始文件打开错误
    /// - Parameters:
    ///   - tx: x
    ///   - ty: y
    ///   - tz: zoomlevel
    ///   - outputPath: 保存文件的文件夹
    int readTile(int tx, int ty, int tz, const char *outputPath);
};
#endif /* SGDAL2Mercator_hpp */
