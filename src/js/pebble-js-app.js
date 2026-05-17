/*
 * TypeWriter Watch -- PebbleKit JS
 *   Key 0: KEY_PAPER  0=Paper  1=Concrete  2=Cream  3=Aged
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
        'Concrete \u2014 Grey wash',
        'Cream \u2014 Clean ivory',
        'Aged \u2014 Vintage stain'
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
