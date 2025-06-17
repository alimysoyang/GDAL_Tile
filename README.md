# GDALKitDemo Application: Local Geospatial Tile Generation for iOS

This project, provides a **robust solution for processing and visualizing large geospatial datasets (specifically Cloud Optimized GeoTIFFs - COG) directly on iOS devices**. It eliminates the need for external cloud services for map tile generation, offering a truly **offline-first and performant mapping experience**.

### Demo Video

### Notebooklm Audio

### Mind Map
![Mind Map](https://github.com/alimysoyang/GDAL_Tile/blob/main/Explanation/Mind%20Map.png)

## 1. Purpose

The primary purpose of this code is to demonstrate and enable **on-the-fly map tile generation from large COG files within an iOS application**. It showcases how the powerful GDAL (Geospatial Data Abstraction Library) can be leveraged on mobile platforms to handle complex geospatial operations, specifically the conversion of raw raster data into displayable map tiles (PNGs).

## 2. Applicable Environment

This codebase is specifically designed for the **iOS operating system**. It integrates with **UIKit** for the user interface, utilizes **Swift** for the application logic, and bridges to **Objective-C++ (`GDALKitManager`)** to interact with the underlying **C++ GDAL library (`GDAL2Mercator`, `GlobalMercator`)**. The demo application uses **Mapbox Maps SDK** for rendering the map and displaying the generated tiles.

## 3. Main Features

*   **GDAL Integration:** Seamlessly integrates core GDAL functionalities into an iOS application, allowing for advanced geospatial data processing.
*   **COG File Handling:** Capable of reading and processing Cloud Optimized GeoTIFF (.tif) files as the primary data source.
*   **Coordinate System Transformations:** Utilizes the `GlobalMercator` class to perform essential conversions between various coordinate systems, including WGS84 (latitude/longitude), Spherical Mercator (EPSG:3857), and pixel/tile coordinates at different zoom levels.
*   **Dynamic Tile Generation:** Generates individual PNG map tiles (`.png`) on demand based on the map's visible area and zoom level. This process is triggered by map camera changes in `BaseMapViewController`.
*   **Local File System Management:** Manages the storage of generated tiles within the application's local Documents directory. It includes a `String` extension utility to clean up previously generated tile caches at application startup, ensuring a fresh demo experience.
*   **Multi-threading for Tile Generation:** `GDALKitManager` dispatches tile generation tasks to global concurrent queues, leveraging device capabilities for efficient processing.

## 4. Problems Solved

This project addresses several challenges in mobile geospatial applications:

*   **Offline Access to Large Geospatial Data:** It enables **access to and rendering of large raster datasets (like satellite imagery or elevation models) even without an active internet connection** by performing all tile generation locally.
*   **Dynamic Tile Creation:** Instead of relying on pre-rendered tile sets that may be static or require frequent updates, this solution **generates tiles dynamically as the user navigates the map**, providing up-to-date visualization directly from the source COG file.
*   **Reducing Server Dependency:** It removes the need for a dedicated tile server or cloud-based rendering services, simplifying deployment and reducing infrastructure costs.
*   **Customizable Map Experience:** Developers have full control over the tile generation process, allowing for custom styling, data adjustments, and integration with unique geospatial workflows.

## 5. Key Advantage: Fully Local Tile Generation

A cornerstone of this solution is that **the entire tile generation algorithm is completed locally on the iOS device, without any reliance on cloud-based implementations or external servers**.

The process unfolds as follows:
1.  The `BaseMapViewController` observes map camera changes.
2.  Upon a camera change, it determines the current visible geographic bounds (latitude/longitude) and zoom level.
3.  These coordinates and the zoom level are passed to the `GDALKitManager`'s `geoTiles` method.
4.  `GDALKitManager` then uses the `GDAL2Mercator` class, which holds a `GlobalMercator` instance, to calculate the required Google Tile XY coordinates for the visible area.
5.  Crucially, the `GDAL2Mercator` class, built upon the GDAL C++ library, directly opens and reads the COG file. It performs all necessary geo-transformations and raster I/O operations to extract the relevant data for each tile.
6.  Finally, `GDAL2Mercator` creates the PNG image files for the tiles and saves them to the application's local file system.

### Benefits and Advantages of Local Tile Generation:

*   **True Offline Capability:** Since all calculations and data processing happen on the device, the application can **function fully without an internet connection** once the initial COG file is available. This is ideal for field operations or areas with poor network coverage.
*   **Enhanced Performance and Responsiveness:** Eliminating network latency for tile requests means **faster map loading and smoother navigation**. Tiles are generated and rendered immediately on the device, leading to a highly responsive user experience [Not explicitly stated, but a direct consequence of local processing].
*   **Cost Efficiency:** There are **no recurring costs associated with cloud compute for tile rendering or data transfer fees** from a tile server. Once the app is deployed, the operational costs for map display are minimal.
*   **Data Privacy and Security:** Sensitive geospatial data remains entirely on the user's device, **never leaving the local environment** for processing. This is critical for applications dealing with confidential or proprietary information [Not explicitly stated, but a direct consequence of local processing].
*   **Complete Control and Customization:** Developers have granular control over the tile generation parameters, color adjustments, and rendering logic, allowing for highly specialized and customized map displays tailored to specific application needs.
*   **Scalability Dependent on Device:** The performance scales with the device's processing power, not external server capacity. This can be advantageous for large-scale deployments without needing to scale cloud infrastructure.

This architecture provides a powerful and flexible foundation for building advanced geospatial applications on iOS, leveraging the full potential of GDAL directly on the mobile platform.


# Important Notes
* Due to their large size, the TIF resource files are not included in this project repository. For testing, please download them from 
  * **[part_000](https://mumuy.serv00.net/images/cog_3857.tif.000)**,
  * **[part_001](https://mumuy.serv00.net/images/cog_3857.tif.001)**,
  * **[part_002](https://mumuy.serv00.net/images/cog_3857.tif.002)**,
  * **[part_003](https://mumuy.serv00.net/images/cog_3857.tif.003)**,
  * **[part_004](https://mumuy.serv00.net/images/cog_3857.tif.004)**.
  
  * To recombine the split files, you can use the cat command as follows:
  ```
  cat cog_3857.tif.000 cog_3857.tif.001 cog_3857.tif.002 cog_3857.tif.003 cog_3857.tif.004 > cog_3857.tif
  ```

* The GDAL for iOS static library is not provided in the project. Please navigate to [Shell](https://gis.stackexchange.com/questions/434514/build-gdal-3-x-for-ios) and use the provided script to compile it yourself.

* Please enter your MBXAccessToken in the Info settings for the GDALKitDemo project.
