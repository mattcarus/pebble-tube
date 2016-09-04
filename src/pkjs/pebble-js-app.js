var appId = '1701c678';
var appKey = 'd87d9b5b974f2f4e784bbd680e6cbba5';

function fetchNearestTubeStation(latitude, longitude) {
  var searchRadius = 200;      // Search radius (metres)
  // For testing only
  latitude = 51.51544625;
  longitude = -0.14101982;
  
  var req = new XMLHttpRequest();
  var url = 'https://api.tfl.gov.uk/StopPoint?lat=' + latitude + '&lon=' + longitude + '&stopTypes=NaptanMetroStation,NaptanRailStation&radius=' + searchRadius + '&useStopPointHierarchy=True&returnLines=True&app_id=' + appId + '&app_key=' + appKey;
  console.log(url);
  req.open('GET', url, true);

  req.onload = function () {
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log(req.responseText);
        var response = JSON.parse(req.responseText);

        var chosen;
        var dist = searchRadius;
        response.stopPoints.forEach(function(item, index){
          if ( item.distance < dist ) {
            dist = item.distance;
            chosen = index;
          }
        });
        
        var name = response.stopPoints[chosen].commonName.replace(" Rail Station", "").replace(" Underground Station", "");
        var linesList = [];
        response.stopPoints[chosen].lines.forEach(function(item, index) {
          linesList.push(item.id);
        });
        var zone = '3';
        var distance = Math.floor(response.stopPoints[chosen].distance);
        console.log("Direction from: " + latitude + ", " + longitude + ", " + response.stopPoints[chosen].lat + ", " + response.stopPoints[chosen].lon);
        var direction = getDirection(latitude, longitude, response.stopPoints[chosen].lat, response.stopPoints[chosen].lon);
        //var direction = 123;
        console.log(name);
        console.log(linesList);
        console.log(zone);
        console.log(distance);
        console.log(direction);
        Pebble.sendAppMessage({
          'NAME_KEY': name,
          'LINES_KEY': getLines(linesList),
          'ZONE_KEY': zone,
          'DISTANCE_KEY': distance + 'm',
          'DIRECTION_KEY': direction
        });
      } else {
        console.log('Error');
      }
    }
  };
  req.send(null);
}

function getDirection(fromLat, fromLng, toLat, toLng) {
  var dLon = (fromLng-toLng);
  var y = Math.sin(dLon) * Math.cos(toLat);
  var x = Math.cos(fromLat)*Math.sin(toLat) - Math.sin(fromLat)*Math.cos(toLat)*Math.cos(dLon);
  var brng = _toDeg(Math.atan2(y, x));
  return Math.round(360 - ((brng + 360) % 360));
}

function _toDeg(rad) {
  return rad * 180 / Math.PI;
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchNearestTubeStation(coordinates.latitude, coordinates.longitude);
}

function getLines(arrLines) {
  var lines = 0;
  console.log("circle check: " + arrLines.indexOf('circle'));
  if( arrLines.indexOf('bakerloo') > 0 ) { lines += 1; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('central') > 0 ) { lines += 2; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('circle') > 0 ) { lines += 4; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('district') > 0 ) { lines += 8; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('hammersmith-city') > 0 ) { lines += 16; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('jubilee') > 0 ) { lines += 32; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('metropolitan') > 0 ) { lines += 64; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('northern') > 0 ) { lines += 128; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('piccadilly') > 0 ) { lines += 256; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('victoria') > 0 ) { lines += 512; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('waterloo-city') > 0 ) { lines += 1024; }
  console.log("lines now " + lines);
  if( arrLines.indexOf('dlr') > 0 ) { lines += 2048; }
  console.log("lines now " + lines);
  return lines;
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    'NAME_KEY': 'Loc Unavailable',
    'LINES_KEY': 0,
    'ZONE_KEY': 'N/A',
    'DISTANCE_KEY': 'N/A',
    'DIRECTION_KEY': 0
  });
}

var locationOptions = {
  'timeout': 15000,
  'maximumAge': 60000
};

Pebble.addEventListener('ready', function (e) {
  console.log('connect!' + e.ready);
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
    locationOptions);
  console.log(e.type);
});

Pebble.addEventListener('appmessage', function (e) {
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
    locationOptions);
  console.log(e.type);
  console.log(e.payload.temperature);
  console.log('message!');
});

Pebble.addEventListener('webviewclosed', function (e) {
  console.log('webview closed');
  console.log(e.type);
  console.log(e.response);
});
