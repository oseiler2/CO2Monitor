#pragma once
namespace html {

  const char style[] PROGMEM = R"(
 div {
        padding: 2px;
        font-size: 1em;
      }
      .innerDiv {
        text-align: left; display: inline-block; min-width: 260px; width: 100%
      }
      body,
      textarea,
      input,
      select {
        background: 0;
        border-radius: 0;
        font: 16px sans-serif;
        margin: 0;
      }
      textarea,
      input,
      select {
        outline: 0;
        font-size: 14px;
        border: 1px solid #ccc;
        padding: 8px;
        width: 90%;
      }
      .btn a {
        text-decoration: none;
      }
      .container {
        margin: auto;
        width: 90%;
      }
      @media (min-width: 1200px) {
        .container {
          margin: auto;
          width: 30%;
        }
      }
      @media (min-width: 768px) and (max-width: 1200px) {
        .container {
          margin: auto;
          width: 50%;
        }
      }
      .btn,
      h2 {
        font-size: 2em;
      }
      h1 {
        font-size: 3em;
      }
      .btn {
        background: #0ae;
        border-radius: 4px;
        border: 0;
        color: #fff;
        cursor: pointer;
        display: inline-block;
        margin: 2px 0;
        padding: 10px 14px 11px;
        width: 100%;
      }
      .btn:hover {
        background: #09d;
      }
      .btn:active,
      .btn:focus {
        background: #08b;
      }
      label > * {
        display: inline;
      }
      form > * {
        display: block;
        margin-bottom: 10px;
      }
      textarea:focus,
      input:focus,
      select:focus {
        border-color: #5ab;
      }
      .msg {
/*      background: #def;
        border-left: 5px solid #59d; */
        padding: 1.5em;
      }
      .q {
        float: right;
        width: 64px;
        text-align: right;
      }
      .l {
        background: url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==")
          no-repeat left center;
        background-size: 1em;
      }
      input[type="checkbox"] {
        float: left;
        width: 20px;
      }
      .table td {
        padding: 0.5em;
        text-align: left;
      }
      .table tbody > :nth-child(2n-1) {
        background: #ddd;
      }
      fieldset {
        border-radius: 0.5rem;
        margin: 0px;
      }
)";

  const char options_header[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <title>Options</title>
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
    <style>
    </style>    
  </head>
  <body>
    <div class="container">
      <div class="innerDiv">
        <h2>{id} {wifi}</h2>
        <fieldset>
          <form action="/wifi" method="get"><button class="btn">Wifi configuration</button></form><br>
          <form action="/config" method="get"><button class="btn">Configuration</button></form><br>
          <form action="/logs" method="get"><button class="btn">Logs</button></form><br>
          <form action="/reboot" method="get"><button class="btn">Reboot</button></form>
        </fieldset>
        <br>
        <fieldset>
          <div class="msg">        
)";

  const char options_footer[] PROGMEM = R"(
          </div>
        </fieldset>
      </div>
    </div>
  </body>
  </html>
)";

  const char logs[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
<style>
td, th {padding: 0.1rem 0.2rem}
.v{color:#888}
.d{color:#0dd}
.i{color:#32cd32}
.w{color:#ff0}
.e{color:red;font-weight:700}
.parent{height: 95vh}
.flow-x{overflow-x: auto}
.scroll-y{overflow-y: scroll}
table{color:white;background-color: rgb(28, 28, 28);font-family: monospace;border-spacing: 2px;}
.tableFixHead          { overflow: auto; height: 95pv; }
.tableFixHead thead th { position: sticky; top: 0; z-index: 1; }
th {background: rgb(28, 28, 28);text-align: left;}
td {text-align: left;}
</style>    
  </head>
  <body style="font-size:1em">

  <div class="">
  <div class="">
  <div class="tableFixHead">
  
    <table>
      <thead>
        <th>Time</th>
        <th>Uptime</th>
        <th>Level</th>
        <th>Tag</th>
        <th>Method</th>
        <th>Message</th>
      </thead>
      <tbody id="logTableBody"></tbody>
    </table></div></div></div>
    <script>
    if (!!window.EventSource) {
  var source = new EventSource("/events");

  source.addEventListener(
    "open",
    function (e) {
      console.log("Events Connected");
    },
    false
  );

  source.addEventListener(
    "error",
    function (e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    },
    false
  );

  source.addEventListener(
    "message",
    function (e) {
      console.log("message", e.data);
    },
    false
  );

  source.addEventListener(
    "log",
    function (e) {
      const d = e.data;
      const matches = d.match(
        /^(?:\033\[\d;\d*m)?\[[ ]*(?<uptime>[\d]+)]\[(?<level>[ DEWVI])\]\[(?<file>.+):(?<line>\d+)\] (?<method>[^:]+): \[(?<tag>[^\]]*)\] (?<msg>[^\033]*)(?:\033\[0m)?$/
      );
      const [all, uptime = "", level = "", file = "", line = "", method = "", tag = "", msg = ""] = matches;
      const record = {
        uptime,
        level,
        file,
        line,
        method,
        tag,
        msg,
        when: new Date().toTimeString().split(" ")[0],
      };
      console.log(`${record.when} [${uptime}][${level}][${file}:${method}:${line}][${tag}] ${msg}`);
      const tableBody = document.getElementById("logTableBody");
      if (tableBody.rows.length > 150) tableBody.deleteRow(0);
      const row = tableBody.insertRow();
      row.classList.add(level.toLowerCase());
      row.innerHTML = `<td>${record.when}</td><td>[${record.uptime}]</td><td>[${record.level}]</td><td>[${record.tag}]</td><td>[${record.file}:${record.method}:${record.line}]</td><td>${record.msg}</td>`;
      row.scrollIntoView();
      /*const div = tableBody.parentNode.parentNode;
      div.scrollBy(0, 200);*/
    },
    false
  );
}

    </script>
  </body>
</html>
)";

  const char config_header[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
<style>
</style>    
  </head>
  <body>
    <div class="container">
      <div class="innerDiv">
        <h2>Configuration</h2>
        <form method='get' action='configsave'>
          <fieldset>
)";

  const char config_parameter[] PROGMEM = R"(
<div><label for='{i}'>{p}</label><input id='{i}' name='{n}' maxlength={l} value='{v}'></div>
)";

  const char config_parameter_number[] PROGMEM = R"(
<div><label for='{i}'>{p}</label><input id='{i}' name='{n}' type='number' maxlength={l} value='{v}' min='{mi}' max='{ma}'></div>
)";

  const char config_parameter_checkbox[] PROGMEM = R"(
<div><label for='{i}'>{p}</label><input id='{i}' name='{n}' type='checkbox' {v}></div>
)";

  const char config_parameter_select_start[] PROGMEM = R"(
<div><label for='{i}'>{p}</label><select name='{n}' id='{i}'>
)";

  const char config_parameter_select_option[] PROGMEM = R"(
<option value='{v}' {s}>{lbl}</option>
)";

  const char config_parameter_select_end[] PROGMEM = R"(
</select></div>
)";

  const char config_footer[] PROGMEM = R"(
          </fieldset>
          <br>
          <button class='btn' type='submit'>Save</button>
        </form>
        <form action='/' method='get'>
          <button class='btn'>Cancel</button>
        </form>
      </div>
    </div>
  </body>
</html>
)";

  const char config_saved[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta http-equiv="refresh" content="5;url=/" />
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
    <title>Configuration saved</title>
  </head>
  <body>
    <div class="container">
      <div class="innerDiv">
        <h2>Configuration saved</h2>
        <fieldset>
          <form action="/" method="get"><button class="btn">Back</button></form>
        </fieldset>
      </div>
    </div>
  </body>
  </html>
  )";

  const char wifi_header[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <title>Configure WiFi</title>
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
    <script>
      function c(l) {
        document.getElementById("s").value = l.innerText || l.textContent;
        document.getElementById("p").focus();
      }
    </script>
  </head>
  <body>
    <div class="container">
      <div class="innerDiv">
        <h2>Wifi configuration</h2>
        <fieldset>
          <div id="wifiDiv">
            Fetching results....
)";

  const char wifi_form[] PROGMEM = R"(
          </div>
        </fieldset>
        <br>
        <a onclick='loadScanResults(true)'><button class="btn">Re-scan</button></a>        
        <br><br>
        <form method="get" action="wifisave">
          <fieldset>
            <div>
              <label>SSID</label><input id="s" name="s" length="32" placeholder="SSID" />
            </div>
            <div>
              <label>Password</label><input id="p" name="p" length="64" minlength="8" maxlength="63" placeholder="password" />
            </div>
          </fieldset>
          <br>
          <button class='btn' type='submit'>Save</button><br>
        </form>
        <form action='/' method='get'><button class='btn'>Cancel</button></form>
 )";


  const char wifi_footer[] PROGMEM = R"(
      </div>
    </div>
    <script>
      async function loadScanResults(force = false) {
         console.log(`***** force: ${force}`);
        try {
          const div = document.getElementById("wifiDiv");
          div.innerHTML = "Fetching results....";
          const scanResults = await (await fetch(`/scan${force?"?f=1":""}`)).json();
          let newContent = "";
          scanResults.forEach((network) => {
            newContent += `<div><a href='#p' onclick='c(this)'>${network.ssid}</a>&nbsp;<span class='q ${network.enc!=0?"l":""}'>${network.rssi}% </span></div>`;
          });
          div.innerHTML = newContent;
        } catch (e) {
          console.log(`ERROR: ${e}`);
        }
      }
      document.addEventListener("DOMContentLoaded", async () => {
        await loadScanResults();
      });
    </script>
  </body>
</html>
)";

  const char wifi_saved[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
    <title>Wifi saved</title>
  </head>
  <body>
    <div class="container">
      <div class="innerDiv">
        <fieldset>
          <form action="/" method="get"><button class="btn">Back</button></form>
          <br>
        </fieldset>
      </div>
    </div>
  </body>
  </html>
  )";

  const char reboot[] PROGMEM = R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta http-equiv="refresh" content="5;url=/" />
    <link rel="stylesheet" type="text/css" href="styles.css" /> 
    <title>Restart</title>
  </head>
  <body>
    <div class="container">
      <div class="innerDiv">
        <h2>Restarting shortly...</h2>
      </div>
    </div>
  </body>
  </html>
  )";

  const char header_cache_control[] PROGMEM = "Cache-Control";
  const char cache_control_no_cache[] PROGMEM = "no-cache, no-store, must-revalidate";

  const char header_access_control_allow_origin[] PROGMEM = "Access-Control-Allow-Origin";
  const char cors_asterix[] PROGMEM = "*";

  const char content_type_html[] PROGMEM = "text/html";
  const char content_type_plain[]PROGMEM = "text/plain";
  const char content_type_json[] PROGMEM = "application/json";
}