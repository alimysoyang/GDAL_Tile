//
//  GlobalMercator.hpp
//  SwiftGDAL
//
//  Created by MuMuY on 2024/1/18.
//

#ifndef GlobalMercator_hpp
#define GlobalMercator_hpp

#include <stdio.h>
#include <list>
#include <math.h>
#include <iostream>

using namespace std;

class GlobalMercator {
private:
    int _tile_size;
    double _initialResolution;
    double _originShift;
    
    double Resolution(int zoom);
public:
    GlobalMercator(int tile_size = 256);
    virtual ~GlobalMercator(void);
    
    void LatLonToMeters(double lat, double lon, double *mxy);
    
    void MetersToLatLon(double mx, double my, double *LatLon);
    
    void PixelsToMeters(double px, double py, int zoom, double *mxy);
    
    void MetersToPixels(double mx, double my, int zoom, double *pxy);
    
    void PixelsToTile(double px, double py, double *txy);
    
    void PixelsToRaster(double px, double py, int zoom, double *rxy);
    
    void MetersToTile(double mx, double my, int zoom, double *txy);
    
    void TileBounds(int tx, int ty, int zoom, double *bound);
    
    void TileLatLonBounds(int tx, int ty, int zoom, double *bound);
    
    int ZoomForPixelSize(double pixelSize);
    
    void GoogleTile(int tx, int ty, int zoom, int *txy);
    
    void LatLonToGoogleTile(double lat, double lon, int zoom, int *xy);
};
#endif /* GlobalMercator_hpp */
