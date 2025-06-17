//
//  GlobalMercator.cpp
//  SwiftGDAL
//
//  Created by MuMuY on 2024/1/18.
//

#include "GlobalMercator.hpp"

#define PI 3.14159265358979323846
#define EARTH_RADIUS 6378137.0
#define MAXZOOMLEVEL 32

///Initialize the TMS Global Mercator pyramid
GlobalMercator::GlobalMercator(int tile_size) {
    _tile_size = tile_size;
    _initialResolution = 2 * PI * EARTH_RADIUS / _tile_size;
    _originShift = 2 * PI * EARTH_RADIUS / 2.0;
}

GlobalMercator::~GlobalMercator(void) {
    
}

double GlobalMercator::Resolution(int zoom) {
    return _initialResolution / pow(2, double(zoom));
}

///Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:3857
void GlobalMercator::LatLonToMeters(double lat, double lon, double *mxy) {
    mxy[0] = lon * _originShift / 180.0;
    mxy[1] = log(tan((90.0 + lat) * PI / 360.0)) / (PI / 180.0);
    mxy[1] = mxy[1] * _originShift / 180.0;
}

///Converts XY point from Spherical Mercator EPSG:3857 to lat/lon in WGS84 Datum
void GlobalMercator::MetersToLatLon(double mx, double my, double *LatLon) {
    LatLon[0] = (mx * _originShift) * 180.0; // lon
    LatLon[1] = (my * _originShift) * 180.0; // lat
    
    LatLon[1] = (180.0 / PI * (2 * atan(exp(LatLon[1] * PI / 180.0)) - PI / 2.0));
}

///Converts pixel coordinates in given zoom level of pyramid to EPSG:3857
void GlobalMercator::PixelsToMeters(double px, double py, int zoom, double *mxy) {
    double res = Resolution(zoom);
    mxy[0] = px * res - _originShift;
    mxy[1] = py * res - _originShift;
}

///Converts EPSG:3857 to pyramid pixel coordinates in given zoom level
void GlobalMercator::MetersToPixels(double mx, double my, int zoom, double *pxy) {
    double res = Resolution(zoom);
    pxy[0] = (mx + _originShift) / res;
    pxy[1] = (my + _originShift) / res;
}

///Returns a tile covering region in given pixel coordinates
void GlobalMercator::PixelsToTile(double px, double py, double *txy) {
    txy[0] = int(ceil(px / float(_tile_size)) - 1);
    txy[1] = int(ceil(py / float(_tile_size)) - 1);
}

///Move the origin of pixel coordinates to top-left corner
void GlobalMercator::PixelsToRaster(double px, double py, int zoom, double *rxy) {
    double mapSize = double(_tile_size << zoom);
    rxy[0] = px;
    rxy[1] = mapSize - py;
}

///Returns tile for given mercator coordinates
void GlobalMercator::MetersToTile(double mx, double my, int zoom, double *txy) {
    double px;
    double py;
    double res = Resolution(zoom);
    px = (mx + _originShift) / res;
    py = (my + _originShift) / res;
    
    txy[0] = int(ceil(px / float(_tile_size)) - 1);
    txy[1] = int(ceil(py / float(_tile_size)) - 1);
}

///Returns bounds of the given tile in EPSG:3857 coordinates
void GlobalMercator::TileBounds(int tx, int ty, int zoom, double *bound) {
    double res = Resolution(zoom);
    /// minx
    bound[0] = (tx * _tile_size) * res - _originShift;
    /// miny
    bound[1] = (ty * _tile_size) * res - _originShift;
    /// maxx
    bound[2] = ((tx + 1) * _tile_size) * res - _originShift;
    /// maxy
    bound[3] = ((ty + 1) * _tile_size) * res - _originShift;
}

///Returns bounds of the given tile in latitude/longitude using WGS84 datum
void GlobalMercator::TileLatLonBounds(int tx, int ty, int zoom, double *bound) {
    double res = Resolution(zoom);
    double minx = tx * _tile_size * res - _originShift;
    double miny = ty * _tile_size * res - _originShift;
    double maxx = (tx + 1) * _tile_size * res - _originShift;
    double maxy = (ty + 1) * _tile_size * res - _originShift;
    
    double minlon = (minx / _originShift) * 180.0;
    double minlat = (miny / _originShift) * 180.0;
    minlat = (180.0 / PI * (2 * atan(exp(minlat * PI / 180.0)) - PI / 2.0));
    
    double maxlon = (maxx / _originShift) * 180.0;
    double maxlat = (maxy / _originShift) * 180.0;
    maxlat = (180.0 / PI * (2 * atan(exp(maxlat * PI / 180.0)) - PI / 2.0));
    
    bound[0] = minlat;
    bound[1] = minlon;
    bound[2] = maxlat;
    bound[3] = maxlon;
}

///Maximal scaledown zoom of the pyramid closest to the pixelSize.
int GlobalMercator::ZoomForPixelSize(double pixelSize) {
    for (int i = 0;i < MAXZOOMLEVEL;i++) {
        if (pixelSize > Resolution(i)) {
            return max(0, i - 1);
        }
    }
    return MAXZOOMLEVEL - 1;
}

///Converts TMS tile coordinates to Google Tile coordinates
void GlobalMercator::GoogleTile(int tx, int ty, int zoom, int *txy) {
    txy[0] = tx;
    txy[1] = (pow(2, double(zoom)) - 1) - ty;
}

///Converts coordinates from WGS84 to EPSG:3857 and return Google Tile XY
void GlobalMercator::LatLonToGoogleTile(double lat, double lon, int zoom, int *xy) {
    double res = _initialResolution / pow(2, double(zoom));
    /// WGS84 to EPSG:3857
    double m_lon = lon * _originShift / 180.0;
    double m_lat = log(tan((90.0 + lat) * PI / 360.0)) / (PI / 180.0);
    m_lat = (m_lat * _originShift / 180.0);
    
    xy[0] = (m_lon + _originShift) / res / (double)_tile_size;
    /// TMS Y
    xy[1] = (m_lat + _originShift) / res / (double)_tile_size;
    /// TMS Y to Google Y
    xy[1] = (pow(2, double(zoom)) - 1) - xy[1];
}
