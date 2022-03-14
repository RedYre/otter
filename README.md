## The tool
The tool "OTTER"(OpenstreetmapsToRDF) reads an OSM data set, infers new data and spatial relations between OSM elements and writes all of those as RDF(ttl) output. This implementation was written in the scope of my master's thesis.

## Prerequisite
Docker is used for the setup. Since we process large datasets which do not fit into memory a library called "stxxl" is used. Set the amount of on-disk storage that shall be used inside /bin/stxxl. Then rename /bin/stxxl to /bin/.stxxl. 

We specifiy the volume from which the docker application can get the OSM data set from. An OSM data set can be found at https://download.geofabrik.de/. It is recommended to use .pbf files, since they are a lot more compact.

## Run and Build
One can build and run the application via:

docker build -t otter .\
docker run -it -v path/to/your/dataset/directory:/container/mountme/ otter

Inside the container one can run otter via:

./otter -osmpath ../mountme/yourOSMdataSet.pbf -ttlpath ../mountme/yourOSMdataSet.ttl.bz2
