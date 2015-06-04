
/***** Persist Keys *****/
var PERSIST_BOTTLE = 1;
var PERSIST_DIAPER = 2;
var PERSIST_MOON_START = 3;
var PERSIST_MOON_END = 4;

var memoryArray = {};

Pebble.addEventListener("ready",
	function(e) {
		console.log("Pebby JavaScript ready!");
//        console.log("local storage: " + JSON.stringify(window.localStorage));

		if (window.localStorage.getItem(PERSIST_BOTTLE) === null) {
            console.log("First start, initializing local storage");

			// Init values, first time only
			memoryArray[PERSIST_BOTTLE] = [];
			memoryArray[PERSIST_DIAPER] = [];
			memoryArray[PERSIST_MOON_START] = [];
			memoryArray[PERSIST_MOON_END] = [];

			window.localStorage.setItem(PERSIST_BOTTLE, JSON.stringify(memoryArray[PERSIST_BOTTLE]));
			window.localStorage.setItem(PERSIST_DIAPER, JSON.stringify(memoryArray[PERSIST_DIAPER]));
			window.localStorage.setItem(PERSIST_MOON_START, JSON.stringify(memoryArray[PERSIST_MOON_START]));
			window.localStorage.setItem(PERSIST_MOON_END, JSON.stringify(memoryArray[PERSIST_MOON_END]));
		} else {
			memoryArray[PERSIST_BOTTLE] = JSON.parse(window.localStorage.getItem(PERSIST_BOTTLE));
			memoryArray[PERSIST_DIAPER] = JSON.parse(window.localStorage.getItem(PERSIST_DIAPER));
			memoryArray[PERSIST_MOON_START] = JSON.parse(window.localStorage.getItem(PERSIST_MOON_START));
			memoryArray[PERSIST_MOON_END] = JSON.parse(window.localStorage.getItem(PERSIST_MOON_END));
		}
	}
);

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("Received message: " + JSON.stringify(e.payload));
		var updated = [];

        console.log(JSON.stringify(e.payload));

		for (var v in e.payload) {
			if (memoryArray[v].indexOf(e.payload[v]) == -1) {
				if (updated.indexOf(v) == -1) {
					updated.push(v);
				}
				memoryArray[v].push(e.payload[v]);
			}
		}
		
		for (var i = 0; i < updated.length; ++i) {
			console.log("Updating localStorage[" + updated[i] + "], new value: " + JSON.stringify(memoryArray[updated[i]]));
			window.localStorage.setItem(updated[i], JSON.stringify(memoryArray[updated[i]]));
		}
	}
);

Pebble.addEventListener("showConfiguration",
	function(e) {
		Pebble.openURL("http://www.epicvortex.com/pebble/pebby.html?#" + JSON.stringify(window.localStorage));
	}
);

Pebble.addEventListener("webviewclosed",
	function(e) {
		console.log("Configuration window returned: " + e.response);
		if (e.response == "reset") {
			console.log("Local Storage cleared");

			window.localStorage.clear();
			Pebble.sendAppMessage({"0": "reset" });
		}
	}
);



