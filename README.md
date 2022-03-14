# The tool
The tool otter(OpenstreetmapsToRDF) reads an OSM data set, infers new data and spatial relations between OSM elements and writes all of those as RDF(ttl) output. This implementation was written in the scope of my master's thesis.

# Get started
Docker is used for the setup. We specifiy the volume from which the docker application can get the OSM data set from. An OSM data set can be found at https://download.geofabrik.de/. It is recommended to use .pbf files, since they are a lot more compact. One can build and run the application via:

docker build -t otter .
docker run -it -v path/to/your/dataset/directory:/container/mountme/ otter
