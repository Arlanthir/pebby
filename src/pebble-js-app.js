
/***** Persist Keys *****/
var PERSIST_BOTTLE = 1;
var PERSIST_DIAPER = 2;
var PERSIST_MOON_START = 3;
var PERSIST_MOON_END = 4;

var memoryArray = {};

Pebble.addEventListener("ready",
	function(e) {
		console.log("Pebby JavaScript ready!");
		console.log("LocalStorage: " + JSON.stringify(localStorage));
		
		if (typeof localStorage[PERSIST_BOTTLE] === "undefined") {
			// Init values, first time only
			memoryArray[PERSIST_BOTTLE] = [];
			memoryArray[PERSIST_DIAPER] = [];
			memoryArray[PERSIST_MOON_START] = [];
			memoryArray[PERSIST_MOON_END] = [];
			localStorage[PERSIST_BOTTLE] = JSON.stringify(memoryArray[PERSIST_BOTTLE]);
			localStorage[PERSIST_DIAPER] = JSON.stringify(memoryArray[PERSIST_DIAPER]);
			localStorage[PERSIST_MOON_START] = JSON.stringify(memoryArray[PERSIST_MOON_START]);
			localStorage[PERSIST_MOON_END] = JSON.stringify(memoryArray[PERSIST_MOON_END]);
		} else {
			memoryArray[PERSIST_BOTTLE] = JSON.parse(localStorage[PERSIST_BOTTLE]);
			memoryArray[PERSIST_DIAPER] = JSON.parse(localStorage[PERSIST_DIAPER]);
			memoryArray[PERSIST_MOON_START] = JSON.parse(localStorage[PERSIST_MOON_START]);
			memoryArray[PERSIST_MOON_END] = JSON.parse(localStorage[PERSIST_MOON_END]);
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
			localStorage[updated[i]] = JSON.stringify(memoryArray[updated[i]]);
		}
	}
);

