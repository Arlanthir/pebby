var eventTypes = {
    feed: 0,
    0: 'feed',
    diaper_change: 1,
    1: 'diaper_change',
    sleep_start: 2,
    2: 'sleep_start',
    sleep_stop: 3,
    3: 'sleep_stop'
};

var MESSAGE_KEY_EVENT_BLOB = 0;

function unserializeEvent(buffer, index) {
    var type = buffer[index] in eventTypes ? eventTypes[buffer[index]] : null;

    if (type !== null) {
        return {
            type: type,
            timestamp: 
                ((buffer[index + 1] & 0xFF)      ) |
                ((buffer[index + 2] & 0xFF) << 8 ) |
                ((buffer[index + 3] & 0xFF) << 16) |
                ((buffer[index + 4] & 0xFF) << 24)
        };
    } else {
        return null;
    }
}

Pebble.addEventListener("ready",
	function(e) {
		console.log("Pebby JavaScript ready!");
    }
);

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("Received message: " + JSON.stringify(e.payload));

        if (MESSAGE_KEY_EVENT_BLOB in e.payload) {
            var buffer = e.payload[MESSAGE_KEY_EVENT_BLOB],
                eventCount = buffer[0];

            console.log('received ' + eventCount + ' events');
            
            for (var i = 0; i < eventCount; i++) {
                console.log(JSON.stringify(unserializeEvent(buffer, 1 + 5*i), null, '  '));
            }
        } else {
            console.log('invalid message');
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



