//
//  GDALKitManager.h
//  GDALKit
//
//  Created by alimysoyang on 12/17/24.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol GDALKitManagerDelegate <NSObject>

- (void)onConvertCOGProgress:(double)progress;

- (void)onConvertCOGCompletion;

@end

@interface GDALKitManager : NSObject

@property (weak, nonatomic, nullable) id<GDALKitManagerDelegate> delegate;

@property (strong, nonatomic) NSString *cogFile;

- (void)geoTiles:(CLLocationCoordinate2D)southwest northeast:(CLLocationCoordinate2D)northeast zoomLevel:(int)zoomLevel;

@end

NS_ASSUME_NONNULL_END
