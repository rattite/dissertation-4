"""Prepares a good sqlite file from a raw geojson file
(probably downloaded from OSM)

usage:
    prepare.py <name>
    where <name> is the name of the dataset, eg. "example" if our input file is "example.geojson"
    """

from osgeo import ogr, gdal
import subprocess
import os
import sys
def convert(geojson_path, spatialite_path, layer_name):
    geojson_driver = ogr.GetDriverByName("GeoJSON")
    source_ds = geojson_driver.Open(geojson_path, 0) # 0 means read-only
    if source_ds is None:
        raise FileNotFoundError(f"Could not open {geojson_path}")
    source_layer = source_ds.GetLayer()
    sqlite_driver = ogr.GetDriverByName("SQLite")
    options = [
        'SPATIALITE=YES',
        'INIT_WITH_EPSG=YES'
    ]
    dest_ds = sqlite_driver.CreateDataSource(spatialite_path, options=options)
    if dest_ds is None:
        raise RuntimeError(f"Could not create {spatialite_path}")
    dest_ds.CopyLayer(source_layer, layer_name)
    source_ds = None
    dest_ds = None
    print(f"Success! {geojson_path} converted to {spatialite_path}")

def add_col(name):
    subprocess.run(["bin/process", "data/"+name+".sqlite",name,"GEOMETRY","cent"])

def gp(name):
    subprocess.run(["bin/getpoints","data/"+name+".sqlite",name,"cent", "data/"+name+".dat"])

def clean(name):
    subprocess.run(["data/clean_db","data/"+name+".sqlite",name,"cent"])

#takes a raw geojson file, converts it and prepares it

if len(sys.argv) < 2:
    print("usage: draw3.py <name>")
    exit()
name = sys.argv[1]
convert("data/"+name+".geojson", "data/"+name+".sqlite", name)
add_col(name)
gp(name)
clean(name)
print("COMPLETED!")

