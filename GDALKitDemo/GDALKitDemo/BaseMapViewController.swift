//
//  BaseMapViewController.swift
//  GDALKitDemo
//
//  Created by alimysoyang on 2024/1/24.
//

import UIKit
import Combine
import OSLog
import MapboxMaps
import GDALKit.GDALKitManager
import SwiftUI

class BaseMapViewController: UIViewController {
    private let mapView: MapView = {
        let cameraOptions = CameraOptions(center: CLLocationCoordinate2D(latitude: 24.466667, longitude: 54.366669), zoom: 8, pitch: 45)
        let mapInitOptions = MapInitOptions(cameraOptions: cameraOptions, styleURI: .standard)
        let mapView = MapView(frame: .zero, mapInitOptions: mapInitOptions)
        mapView.autoresizingMask = [.flexibleHeight, .flexibleWidth]
        mapView.ornaments.options.scaleBar.visibility = .hidden
//        try! mapView.mapboxMap.setProjection(StyleProjection(name: StyleProjectionName.globe))
        return mapView
    }()
    
    private lazy var gdalManager: GDALKitManager = { [unowned self] in
        let manager: GDALKitManager = GDALKitManager()
        manager.delegate = self
        return manager
    }()
    
//    private let lbProgress: UILabel = {
//        let label: UILabel = UILabel(frame: CGRect(x: 100, y: 100, width: 160, height: 30))
//        label.textColor = UIColor.black
//        label.font = UIFont.systemFont(ofSize: 15, weight: .semibold)
//        return label
//    }()
    
    private var cancelables = Set<AnyCancellable>()
    
    private let log: Logger = Logger(subsystem: "", category: "")
    
    private let sourceId: String = "radar_source"
    
    private let radarLayerId: String = "raster-layer"
    
    private let bigSourceId: String = "big_radar_source"
    
    private let bigRadarLayerId: String = "big_raster_layer"
    
    private var currentIndex: Int = 0
    
    init() {
        super.init(nibName: nil, bundle: nil)
        let filePath = "\(NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0])/cog_3857.tif"
        if FileManager.default.fileExists(atPath: filePath) {
            "\(NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0])/cog_3857".removeFiles()
            gdalManager.cogFile = "\(NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0])/cog_3857.tif"
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        
        self.mapView.frame = self.view.bounds
        self.view.addSubview(self.mapView)
    
        self.mapView.mapboxMap.onMapLoaded.observeNext { [weak self] _ in
            self?.addRasterSource()
        }.store(in: &cancelables)
        
        self.mapView.mapboxMap.onCameraChanged.observe { [weak self] _ in
            guard let self = self else { return }
            if self.gdalManager.cogFile.isEmpty { return }
            let bounds = self.mapView.mapboxMap.coordinateBounds(for: self.view.bounds)
            self.gdalManager.geoTiles(bounds.southwest, northeast: bounds.northeast, zoomLevel: Int32(round(self.mapView.mapboxMap.cameraState.zoom)))
        }.store(in: &cancelables)
    }
    
    func removeRaster() {
        do {
            try self.mapView.mapboxMap.removeLayer(withId: radarLayerId)
            try self.mapView.mapboxMap.removeSource(withId: sourceId)
        } catch {
            print("Failed to remove layer & source \(error)")
        }
    }
    
    func addRasterSource() {
        let sourceUrl = "file://\(NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0])/cog_3857/{z}/{x}/{y}.png"
        
        // Create a `RasterSource` and set the source's `tiles` to the Stamen watercolor raster tiles.
        var rasterSource = RasterSource(id: sourceId)
        rasterSource.tiles = [sourceUrl]
        // Specify the tile size for the `RasterSource`.
        rasterSource.tileSize = 256
        rasterSource.minimumTileUpdateInterval = 0.001
        rasterSource.tileRequestsDelay = 0.1
        rasterSource.tileNetworkRequestsDelay = 0.1
        rasterSource.volatile = false
        // Specify that the layer should use the source with the ID `raster-source`. This ID will be
        // assigned to the `RasterSource` when it is added to the style.
        let rasterLayer = RasterLayer(id: radarLayerId, source: sourceId)
    
        do {
            try mapView.mapboxMap.addSource(rasterSource)
            try mapView.mapboxMap.addLayer(rasterLayer)
        } catch {
            print("Failed to update the style. Error: \(error)")
        }
    }
}

extension BaseMapViewController: GDALKitManagerDelegate {
    func onConvertCOGProgress(_ progress: Double) {
//        self.lbProgress.text = String(format: "%.2f", progress)
    }
    
    func onConvertCOGCompletion() {
//        self.lbProgress.text = "Done";
    }
}

struct MapboxView: UIViewControllerRepresentable {
    func makeUIViewController(context: Context) -> UIViewController {
        return BaseMapViewController()
    }
    
    func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
        
    }
}

extension String {
    public func removeFiles() {
        var isDir: ObjCBool = ObjCBool(true)
        let fm: FileManager = FileManager()
        if fm.fileExists(atPath: self, isDirectory: &isDir) {
            if isDir.boolValue {
                do {
                    let files: [String] = try fm.contentsOfDirectory(atPath: self)
                    for item in files {
                        String(format: "%@/%@", self, item).removeFiles()
                    }
                    do { try fm.removeItem(atPath: self) } catch {}
                } catch {}
            } else {
                do { try fm.removeItem(atPath: self) } catch {}
            }
        }
    }
}
