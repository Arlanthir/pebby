Pebble.addEventListener("ready",
	function(e) {
		console.log("Pebby JavaScript ready!");
    }
);

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("Received message: " + JSON.stringify(e.payload));

        var buffer = e.payload[0];
        if (buffer) {
            var timestamp = buffer[2] | (buffer[3] << 8) | (buffer[4] << 16) | (buffer[5] << 24);
            console.log('decoded timestamp: ', timestamp);
        }
    }
);

Pebble.addEventListener("showConfiguration",
	function(e) {
		//Pebble.openURL("http://www.epicvortex.com/pebble/pebby.html?#" + serializeStorage(window.localStorage));
	}
);

Pebble.addEventListener("webviewclosed",
	function(e) {
		console.log("Configuration window returned: " + e.response);
		if (e.response == "reset") {
			//Pebble.sendAppMessage({"0": "reset" });
		}
	}
);



