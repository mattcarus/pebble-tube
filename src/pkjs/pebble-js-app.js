function fetchNearestTubeStation(latitude, longitude) {
  
  // For testing only
  latitude = 51.483745;
  longitude = 0.007231;
  
  var req = new XMLHttpRequest();
  var url = 'http://marquisdegeek.com/api/tube/?long=' + longitude + '&lat=' + latitude + '&distance=1000&sortby=distance';
  console.log(url);
  req.open('GET', url, true);

  req.onload = function () {
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log(req.responseText);
        var response = JSON.parse(req.responseText);
        var name = response[0].name;
        var zone = response[0].zone;
        var distance = Math.floor(response[0].results.distance);
        console.log("Direction from: " + latitude + ", " + longitude + ", " + response[0].latitude + ", " + response[0].longitude);
        var direction = getDirection(latitude, longitude, response[0].latitude, response[0].longitude);
        //var direction = 123;
        console.log(name);
        console.log(zone);
        console.log(distance);
        console.log(direction);
        Pebble.sendAppMessage({
          'TUBE_STATION_NAME_KEY': name,
          'TUBE_STATION_ZONE_KEY': 'Zone: ' + zone,
          'TUBE_STATION_DISTANCE_KEY': distance + 'm',
          'TUBE_STATION_DIRECTION_KEY': direction + '\xB0'
        });
      } else {
        console.log('Error');
      }
    }
  };
  req.send(null);
}

function getDirection(fromLat, fromLng, toLat, toLng) {
  var dLon = (toLng-fromLng);
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

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    'TUBE_STATION_NAME_KEY': 'Loc Unavailable',
    'TUBE_STATION_ZONE_KEY': 'N/A',
    'TUBE_STATION_DISTANCE_KEY': 'N/A',
    'TUBE_STATION_DIRECTION_KEY': 'N/A'
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
