//
//  GDALKitManager.m
//  GDALKit
//
//  Created by alimysoyang on 12/17/24.
//

#import "GDALKitManager.h"
#import "GDAL2Mercator.hpp"

int convertCOGProgress(double dfComplete, const char *pszMessage, void *pProgressArg) {
    NSDictionary *callbackObject = @{@"OnProgressCallback":@{@"progress":@(dfComplete * 100)}};
    [NSNotificationCenter.defaultCenter postNotificationName:@"ae.abuioabu4.gdal.convertprogress" object:callbackObject];
    if (dfComplete >= 1) {
        [NSNotificationCenter.defaultCenter postNotificationName:@"ae.abuioabu4.gdal.convertcompletion" object:nil];
        return 1;
    }
    return -1;
}

@implementation GDALKitManager {
    GDAL2Mercator *mercator;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        NSString *path = [NSBundle.mainBundle pathForResource:@"Frameworks/GDALKit" ofType:@"framework"];
        NSBundle *bundle = [NSBundle bundleWithPath:[[NSBundle bundleWithPath:path] pathForResource:@"SRGDAL" ofType:@"bundle"]];
        NSString *gdalData = [bundle pathForResource:@"gdal" ofType:nil];
        NSString *projLIB = [bundle pathForResource:@"proj" ofType:nil];
        
        self->mercator = new GDAL2Mercator([gdalData UTF8String], [projLIB UTF8String]);
        self->mercator->progressFunc = convertCOGProgress;
    }
    return self;
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Notification Observer
- (void)onConvertCOGProgressNotification:(NSNotification *)sender {
    NSDictionary *info = (NSDictionary *)sender.object;
    double progress = [info[@"OnProgressCallback"][@"progress"] doubleValue];
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self.delegate && [self.delegate respondsToSelector:@selector(onConvertCOGProgress:)]) {
            [self.delegate onConvertCOGProgress:progress];
        }
    });
}

-(void)onConvertCOGCompletionNotification:(NSNotification *)sender {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self.delegate && [self.delegate respondsToSelector:@selector(onConvertCOGCompletion)]) {
            [self.delegate onConvertCOGCompletion];
        }
    });
}

#pragma mark - public methods
- (void)geoTiles:(CLLocationCoordinate2D)southwest northeast:(CLLocationCoordinate2D)northeast zoomLevel:(int)zoomLevel {
    NSString *outputPath = [[NSString stringWithUTF8String:self->mercator->_cogFile] stringByDeletingPathExtension];
    NSFileManager *fm = [[NSFileManager alloc] init];
    NSError *error;
    [fm createDirectoryAtPath:outputPath withIntermediateDirectories:YES attributes:nil error:&error];
    if (error) {
        return;
    }
    
    if (self->mercator->readGoogleTiles(southwest.latitude, southwest.longitude, northeast.latitude, northeast.longitude, zoomLevel) == 0) {
        int minx = self->mercator->rangTileXY[0];
        int maxx = self->mercator->rangTileXY[1];
        int miny = self->mercator->rangTileXY[2];
        int maxy = self->mercator->rangTileXY[3];
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            dispatch_group_t groupx = dispatch_group_create();
            
            for (int tx = minx;tx <= maxx;tx++) {
                dispatch_group_enter(groupx);
                
                dispatch_group_t groupy = dispatch_group_create();
                
                for (int ty = miny;ty <= maxy;ty++) {
                    dispatch_group_enter(groupy);
                    
                    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                        NSString *filePath = [NSString stringWithFormat:@"%@/%d/%d/%d.png", outputPath, zoomLevel, tx, ty];
                        if (![fm fileExistsAtPath:filePath]) {
                            int ret = self->mercator->readTile(tx, ty, zoomLevel, [outputPath UTF8String]);
                            dispatch_group_leave(groupy);
                        } else {
                            dispatch_group_leave(groupy);
                        }
                    });
                }
                dispatch_group_notify(groupy, dispatch_get_main_queue(), ^{
                    dispatch_group_leave(groupx);
                });
            }
            
            dispatch_group_notify(groupx, dispatch_get_main_queue(), ^{
                
            });
        });
    } else {
        
    }
}


#pragma mark - getter & setter
- (void)setCogFile:(NSString *)cogFile {
    _cogFile = cogFile;
    self->mercator->openCOGFileWithTile([cogFile UTF8String]);
}
@end
