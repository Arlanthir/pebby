
/***** Persist Keys *****/
var PERSIST_BOTTLE = 1;
var PERSIST_DIAPER = 2;
var PERSIST_MOON_START = 3;
var PERSIST_MOON_END = 4;

var memoryArray = {};

Pebble.addEventListener("ready",
	function(e) {
		console.log("Pebby JavaScript ready!");

		if (typeof window.localStorage[PERSIST_BOTTLE] === 'undefined' || window.localStorage[PERSIST_BOTTLE] === null) {
			console.log('LocalStorage is empty, initting');
			// Init values, first time only
			memoryArray[PERSIST_BOTTLE] = [];
			memoryArray[PERSIST_DIAPER] = [];
			memoryArray[PERSIST_MOON_START] = [];
			memoryArray[PERSIST_MOON_END] = [];
			window.localStorage[PERSIST_BOTTLE] = JSON.stringify(memoryArray[PERSIST_BOTTLE]);
			window.localStorage[PERSIST_DIAPER] = JSON.stringify(memoryArray[PERSIST_DIAPER]);
			window.localStorage[PERSIST_MOON_START] = JSON.stringify(memoryArray[PERSIST_MOON_START]);
			window.localStorage[PERSIST_MOON_END] = JSON.stringify(memoryArray[PERSIST_MOON_END]);
		} else {
			console.log('LocalStorage not empty, loading');
			memoryArray[PERSIST_BOTTLE] = JSON.parse(window.localStorage[PERSIST_BOTTLE]);
			memoryArray[PERSIST_DIAPER] = JSON.parse(window.localStorage[PERSIST_DIAPER]);
			memoryArray[PERSIST_MOON_START] = JSON.parse(window.localStorage[PERSIST_MOON_START]);
			memoryArray[PERSIST_MOON_END] = JSON.parse(window.localStorage[PERSIST_MOON_END]);
		}
	}
);

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("Received message: " + JSON.stringify(e.payload));
		var updated = [];
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
			window.localStorage[updated[i]] = JSON.stringify(memoryArray[updated[i]]);
		}
	}
);

Pebble.addEventListener("showConfiguration",
	function(e) {
		console.log('showConfiguration');
		var qData = {};
		qData[PERSIST_BOTTLE] = window.localStorage[PERSIST_BOTTLE];
		qData[PERSIST_DIAPER] = window.localStorage[PERSIST_DIAPER];
		qData[PERSIST_MOON_START] = window.localStorage[PERSIST_MOON_START];
		qData[PERSIST_MOON_END] = window.localStorage[PERSIST_MOON_END];
		console.log('qData: ' + JSON.stringify(qData));
		console.log('URL: ' + 'http://www.epicvortex.com/pebble/bwatch.html#' + encodeURIComponent(JSON.stringify(qData)));
		Pebble.openURL('http://www.epicvortex.com/pebble/bwatch.html#' + encodeURIComponent(JSON.stringify(qData)));
	}
);

Pebble.addEventListener("webviewclosed",
	function(e) {
		console.log("Configuration window returned: " + e.response);
		if (e.response == "reset") {
			console.log("Local Storage cleared");
			for (var v in window.localStorage) {
				window.localStorage.removeItem(v);
			}
			window.localStorage.clear();
			Pebble.sendAppMessage({"0": "reset" });
			memoryArray[PERSIST_BOTTLE] = [];
			memoryArray[PERSIST_DIAPER] = [];
			memoryArray[PERSIST_MOON_START] = [];
			memoryArray[PERSIST_MOON_END] = [];
			window.localStorage[PERSIST_BOTTLE] = JSON.stringify(memoryArray[PERSIST_BOTTLE]);
			window.localStorage[PERSIST_DIAPER] = JSON.stringify(memoryArray[PERSIST_DIAPER]);
			window.localStorage[PERSIST_MOON_START] = JSON.stringify(memoryArray[PERSIST_MOON_START]);
			window.localStorage[PERSIST_MOON_END] = JSON.stringify(memoryArray[PERSIST_MOON_END]);
		}
	}
);



