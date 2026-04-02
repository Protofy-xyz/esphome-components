#pragma once

// Static HTML fragments for chunked response — no large allocations.
// Dynamic values are sent between fragments via httpd_resp_send_chunk.

namespace esphome {
namespace setup_portal {

// CSS: %s = accent, %s = accent dark
static const char HTML_CSS[] =
    "<!DOCTYPE html><html><head>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>%s</title>"
    "<style>"
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{font-family:-apple-system,system-ui,sans-serif;background:#f0f2f5;color:#1a1a1a;padding:16px}"
    ".c{max-width:420px;margin:0 auto}"
    "h1{text-align:center;font-size:1.3em;padding:16px 0}"
    ".b{background:#fff;border-radius:12px;padding:16px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,.08)}"
    ".b h2{font-size:.85em;color:#888;margin-bottom:12px;text-transform:uppercase;letter-spacing:.5px}"
    "label{display:block;font-size:.9em;color:#555;margin:10px 0 4px}"
    "label:first-child{margin-top:0}"
    "input,select{width:100%%;padding:10px 12px;border:1px solid #ddd;border-radius:8px;font-size:1em;background:#fafafa}"
    "input:focus,select:focus{outline:none;border-color:%s;background:#fff}"
    ".btn{display:block;width:100%%;padding:14px;background:%s;color:#fff;border:none;border-radius:10px;font-size:1em;font-weight:600;cursor:pointer;margin-top:8px}"
    ".btn:active{background:%s}"
    ".st{text-align:center;font-size:.8em;color:#999;padding:12px}"
    ".row{display:flex;gap:8px;align-items:end}.row>*:first-child{flex:1}"
    ".btn-s{padding:10px 14px;font-size:.9em;margin:0;width:auto;border-radius:8px}"
    ".pw{position:relative}.pw input{padding-right:44px}"
    ".eye{position:absolute;right:8px;top:50%%;transform:translateY(-50%%);background:none;border:none;padding:6px;cursor:pointer;color:#888;line-height:1}"
    ".logo{text-align:center;padding:16px 0 0}.logo img{max-width:140px;max-height:80px}"
    ".lnk{color:#aaa;font-size:.8em;cursor:pointer;display:inline-block;padding:4px 0}"
    ".pin-c{max-height:0;overflow:hidden;transition:max-height .3s ease}.pin-c.open{max-height:200px}"
    "</style></head><body><div class=\"c\">";

// WiFi card: %s = wifi placeholder
static const char HTML_CONFIG_WIFI[] =
    "<div class=\"b\"><h2>WiFi</h2>"
    "<label>Network</label>"
    "<div class=\"row\"><select name=\"ssid\" id=\"ss\"><option>Scanning...</option></select>"
    "<button type=\"button\" class=\"btn btn-s\" onclick=\"scan()\">&#x21bb;</button></div>"
    "<label>Password</label>"
    "<div class=\"pw\"><input type=\"password\" name=\"password\" id=\"wp\" placeholder=\"%s\">"
    "<button type=\"button\" class=\"eye\" onclick=\"tog('wp',this)\">"
    "<svg width=\"22\" height=\"22\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">"
    "<path d=\"M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z\"/><circle cx=\"12\" cy=\"12\" r=\"3\"/></svg>"
    "</button></div></div>";

// Save + status: %s = status
static const char HTML_CONFIG_END[] =
    "<button type=\"submit\" class=\"btn\">Save</button></form>"
    "<div class=\"st\">%s</div></div></div>";

// Script: %d = has_pin, %s = current_ssid, %s = accent
static const char HTML_SCRIPT[] =
    "<script>"
    "var hp=%d,cs='%s',ac='%s';"
    "function unlock(p){document.getElementById('ps').style.display='none';"
    "document.getElementById('cs').style.display='block';"
    "document.getElementById('fp').value=p;sessionStorage.setItem('sp_pin',p);scan();}"
    "if(!hp)scan();"
    "else{var sp=sessionStorage.getItem('sp_pin');"
    "if(sp){fetch('/auth?pin='+encodeURIComponent(sp)).then(function(r){return r.json()}).then(function(d){if(d.ok)unlock(sp);});}}"
    "function auth(){"
    "var p=document.getElementById('pin').value;"
    "fetch('/auth?pin='+encodeURIComponent(p)).then(function(r){return r.json()}).then(function(d){"
    "if(d.ok)unlock(p);else alert('Invalid PIN');});}"
    "function tog(id,b){var i=document.getElementById(id);"
    "if(i.type==='password'){i.type='text';b.style.color=ac;}else{i.type='password';b.style.color='#888';}}"
    "function scan(){var s=document.getElementById('ss');s.innerHTML='<option>Scanning...</option>';"
    "fetch('/scan').then(function(r){return r.json()}).then(function(n){s.innerHTML='';"
    "if(cs){var o=document.createElement('option');o.value=cs;o.textContent=cs+' (current)';o.selected=true;s.appendChild(o);}"
    "n.forEach(function(x){if(x.s===cs)return;var o=document.createElement('option');"
    "o.value=x.s;o.textContent=x.s+' ('+x.r+'dBm)';s.appendChild(o);});"
    "if(!s.children.length)s.innerHTML='<option>No networks found</option>';"
    "}).catch(function(){s.innerHTML='<option>Scan error</option>';});}"
    "</script></body></html>";

// Reboot page — auto-redirects after AP comes back
static const char HTML_SAVE_REBOOT[] =
    "<!DOCTYPE html><html><head>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<style>body{font-family:-apple-system,system-ui,sans-serif;text-align:center;padding:60px 20px;background:#f0f2f5;color:#1a1a1a}"
    "h2{margin-bottom:12px}.ic{color:#34c759;font-size:48px}</style></head><body>"
    "<div class=\"ic\">&#10003;</div><h2>Configuration saved</h2>"
    "<p>Restarting device...</p></body></html>";

// Saved page — back and close buttons: %s = accent color
static const char HTML_SAVE_OK[] =
    "<!DOCTYPE html><html><head>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<style>body{font-family:-apple-system,system-ui,sans-serif;text-align:center;padding:60px 20px;background:#f0f2f5;color:#1a1a1a}"
    "h2{margin-bottom:12px}.ic{color:#34c759;font-size:48px}"
    ".btns{display:flex;gap:12px;justify-content:center;margin-top:24px;max-width:300px;margin-left:auto;margin-right:auto}"
    ".bt{flex:1;padding:12px;border-radius:10px;font-size:.95em;font-weight:600;text-decoration:none;text-align:center;cursor:pointer}"
    ".bt-p{background:%s;color:#fff;border:none}"
    ".bt-s{background:none;color:#888;border:1px solid #ddd}"
    "</style></head><body>"
    "<div class=\"ic\">&#10003;</div><h2>Settings saved</h2>"
    "<div class=\"btns\"><a class=\"bt bt-p\" href=\"/\">&#8592; Back</a>"
    "<button class=\"bt bt-s\" onclick=\"window.close()\">Close</button></div>"
    "<p style=\"color:#aaa;font-size:.75em;margin-top:16px\">Close will disconnect from the device</p>"
    "</body></html>";

}  // namespace setup_portal
}  // namespace esphome
