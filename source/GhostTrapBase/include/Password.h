/**
 *   GPStar Ghost Trap - Ghostbusters Props, Mods, and Kits.
 *   Copyright (C) 2025 Michael Rajotte <michael.rajotte@gpstartechnologies.com>
 *                    & Nomake Wan <-redacted->
 *                    & Dustin Grau <dustin.grau@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

const char PASSWORD_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta http-equiv="Cache-control" content="public">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>WiFi Password</title>
  <link rel="icon" type="image/svg+xml" href="/favicon.svg"/>
  <link rel="shortcut icon" href="/favicon.ico"/>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <h1>Secure Device WiFi</h1>
  <div class="block left">
    <p>
      Use this form to change the password for the built-in WiFi network for your Proton Pack, securing the device against unwanted connections.
      Note that this value is stored in controller's permanent memory and restoring/upgrading firmware will have no effect on this value.
      <br/>
      After updating, any devices which previously stored the password for the WiFi network will require you to update to the new password.
    </p>
    <br/>
  </div>

  <div class="block left">
    <b>New AP Password:</b> <input type="text" id="password" width="60" maxlength="63" placeholder="Custom Password"/>
    <br/>
    <b>Confirm Password:</b> <input type="text" id="password2" width="60" maxlength="63" placeholder="Confirm Password"/>
  </div>

  <div class="block">
    <hr/>
    <a href="/">&laquo; Back</a>
    &nbsp;&nbsp;&nbsp;
    <button type="button" class="green" onclick="updatePassword()">Update</button>
    <br/>
    <br/>
  </div>

  <script type="application/javascript" src="/common.js"></script>
  <script type="application/javascript">
    function updatePassword() {
      var newPass = getText("password");
      var confPW = getText("password2");
      if (newPass.length < 8) {
        alert("Your new password must be a minimum of 8 characters to meet WPA2 requirements.");
        return;
      }
      if (newPass != confPW) {
        alert("Password and confirmation do not match. Please try again.");
        return;
      }

      var body = JSON.stringify({password: newPass});

      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
          if (this.status == 200) {
            handleStatus(this.responseText);
          }

          if (this.status == 201) {
            handleStatus(this.responseText);

            if (confirm("Restart device now?")) {
              doRestart();
            }
          }
        }
      };
      xhttp.open("PUT", "/password/update", true);
      xhttp.setRequestHeader("Content-Type", "application/json");
      xhttp.send(body);
    }

    function doRestart() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 204) {
          // Reload the page after 2 seconds.
          setTimeout(function() {
            window.location.reload();
          }, 2000);
        }
      };
      xhttp.open("DELETE", "/restart", true);
      xhttp.send();
    }
  </script>
</body>
</html>
)=====";
