/*
 * TypeWriter Watch -- PebbleKit JS
 *   Key 0: KEY_PAPER   0=Paper  1=Concrete  2=Cream  3=Aged
 *   Key 1: KEY_CITY    string — city name from reverse geocode
 */

function loadCfg() {
  return { paper: +(localStorage.getItem('paper') || '0') };
}
function saveCfg(c) {
  localStorage.setItem('paper', c.paper);
}
function sendMsg(c) {
  Pebble.sendAppMessage(
    { 0: c.paper },
    function() { console.log('ok'); },
    function(e) { console.log('fail', e); }
  );
}
function sendCity(city) {
  Pebble.sendAppMessage(
    { 1: city },
    function() { console.log('city sent: ' + city); },
    function(e) { console.log('city fail', e); }
  );
}

/* ── reverse geocode via Nominatim ── */
function fetchCity(lat, lon) {
  var url = 'https://nominatim.openstreetmap.org/reverse'
    + '?lat=' + lat + '&lon=' + lon
    + '&format=json&zoom=7';
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    try {
      var data = JSON.parse(xhr.responseText);
      var addr = data.address || {};
      /* try city → town → village → county in order */
      var city = addr.city || addr.county || addr.state || 'Unknown';
      /* trim to 20 chars max so it fits the watch screen */
      if (city.length > 20) city = city.substring(0, 20);
      localStorage.setItem('city', city);
      sendCity(city);
      console.log('City: ' + city);
    } catch(e) {
      console.log('Geocode parse error: ' + e);
    }
  };
  xhr.onerror = function() { console.log('Geocode network error'); };
  xhr.open('GET', url);
  xhr.setRequestHeader('Accept', 'application/json');
  xhr.send();
}

/* ── get location ── */
function getLocation() {
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      fetchCity(pos.coords.latitude, pos.coords.longitude);
    },
    function(err) {
      console.log('Geolocation error: ' + err.message);
      /* fall back to cached city if available */
      var cached = localStorage.getItem('city');
      if (cached) sendCity(cached);
    },
    { timeout: 10000, maximumAge: 300000 }  /* cache 5 min */
  );
}

function buildConfig(c) {
  function radio(name, opts, sel) {
    return opts.map(function(l, i) {
      return '<label class="opt"><input type="radio" name="' + name
        + '" value="' + i + '"' + (i === sel ? ' checked' : '')
        + '><span>' + l + '</span></label>';
    }).join('');
  }
  var h = '<!DOCTYPE html><html><head>'
    + '<meta charset="utf-8">'
    + '<meta name="viewport" content="width=device-width,initial-scale=1">'
    + '<style>'
    + 'body{margin:0;font:15px/1.6 -apple-system,sans-serif;background:#0d0d0d;color:#ccc;padding:20px}'
    + 'h3{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#555;margin:22px 0 8px}'
    + 'h3:first-child{margin-top:0}'
    + '.opt{display:flex;align-items:center;gap:12px;background:#1a1a1a;border-radius:8px;padding:13px;margin:5px 0;cursor:pointer}'
    + '.opt input{accent-color:#aaa;width:18px;height:18px;flex-shrink:0;margin:0}'
    + '.opt span{font-size:14px}'
    + '#s{display:block;width:100%;padding:14px;background:#222;color:#fff;border:1px solid #3a3a3a;border-radius:8px;font-size:15px;margin-top:24px;cursor:pointer;box-sizing:border-box}'
    + '</style></head><body>'
    + '<h3>Paper</h3>'
    + radio('paper', [
        'Original \u2014 Warm paper',
        'Rice Paper \u2014 Cream texture',
        'White \u2014 Clean minimal',
        'Teal Weave \u2014 Dark pattern',
        'Lined \u2014 Notebook paper'
      ], c.paper)
    + '<button id="s">Save</button>'
    + '<script>'
    + 'document.getElementById("s").onclick=function(){'
    + 'function g(n){var e=document.querySelector("input[name="+n+"]:checked");return e?+e.value:0;}'
    + 'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify({'
    + 'paper:g("paper")}));'
    + '};<\/script></body></html>';
  return 'data:text/html,' + encodeURIComponent(h);
}

Pebble.addEventListener('ready', function() {
  console.log('TypeWriter ready');
  sendMsg(loadCfg());
  /* push cached city immediately, then refresh from GPS */
  var cached = localStorage.getItem('city');
  if (cached) sendCity(cached);
  getLocation();
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(buildConfig(loadCfg()));
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response || e.response === '' || e.response === 'CANCELLED') return;
  var raw = e.response;
  if (raw.indexOf('#') !== -1) raw = raw.substring(raw.lastIndexOf('#') + 1);
  var c;
  try { c = JSON.parse(decodeURIComponent(raw)); } catch(err) { return; }
  saveCfg(c);
  sendMsg(c);
});
