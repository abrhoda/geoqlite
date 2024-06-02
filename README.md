# geoqlite
an embeddable geospatial and geofencing database

## Commands

### Implemented

 - SET _key_ _id_ POINT lat lon
 - SET _key_ _id_ BOUNDS lat1 lon1 lat2 lon2 lat3 lon3 lat4 lon4 ... (arbitrary amount of points that must form a ring - last point must be = first point or error) 
 - GET _key_ _id_ = returns point(s) of _id_
 - DEL _key_ _id_
 - DROP _key_

### TODO
 - SET _key_ _id_ FIELD name value FIELD name2 value2
 - NEARBY _key_ [LIMIT 1] POINT lat long z distance
 - WITHIN _key_ BOUNDS lat1 lon1 lat2 lon2 lat3 lon3 ...
 - WITHFIELDS clause for GET
 - FSET to set fields on an existing id. Tile38 errors if the name of the field that is being set doesn't already exist on the id. 
 - WHERE to fitler commands based on fields an id has.
 - SETCHAN _channal name_ NEARBY _key_ FENCE POINT lat lon -> produces detect events to a queue for the caller to consume
 - DELCHAN _channel name_

### NOTE: 
 1. While integer and float keys and id are allowed, they are treated as strings
 2. POINT optionally takes a z value with arbitrary meaning)
 3. lat long can be swapped for y and x if you are using cartesian coordinate system.
