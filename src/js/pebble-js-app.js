(function(window) {
'use strict';

var eventTypes = {
        feed: 0,
        0: 'feed',
        diaper_change: 1,
        1: 'diaper_change',
        sleep_start: 2,
        2: 'sleep_start',
        sleep_stop: 3,
        3: 'sleep_stop'
    },
    allEventTypes = [eventTypes.feed, eventTypes.diaper_change, eventTypes.sleep_start, eventTypes.sleep_stop];

var MESSAGE_KEY_EVENT_BLOB = 1,
    MESSAGE_KEY_MESSAGE_TYPE = 0;

var MESSAGE_TYPE_EVENT_TRANSMISSION = 1,
    MESSAGE_TYPE_RESET_ACK = 2,
    MESSAGE_TYPE_RESET_REQUEST = 3;

var RETRY_TIMEOUT = 5000;

var events = [],
    eventsByTypeAndTimestamp = {
        0: {},
        1: {},
        2: {},
        3: {}
    };

var resetPending = false;

var storage;

///////////////////////////////////////////////////////////////////////////////

function Event(type, timestamp) {
    this.type = type;
    this.timestamp = timestamp;
}

///////////////////////////////////////////////////////////////////////////////

function LocalStorageAdapter() {
    var me = this;

    me.unserialized = {};

    allEventTypes.forEach(function(eventType) {
        var serializedTimestamps = window.localStorage.getItem(eventType),
            timestamps;

        if (serializedTimestamps === null) serializedTimestamps = "[]";

        try {
            timestamps = JSON.parse(serializedTimestamps);
        } catch (e) {
            console.log("Unable to parse local storage - corrupt data? Clearing...");
            window.localStorage.removeItem(eventType);
            
            timestamps = [];
        }

        me.unserialized[eventType] = timestamps;
    });
}

LocalStorageAdapter.prototype.getAll = function() {
    var me = this,
        events = [];

    allEventTypes.forEach(function(eventType) {
        me.unserialized[eventType].forEach(function(timestamp) {
            events.push(new Event(eventType, timestamp));
        });
    });

    return events;
};

LocalStorageAdapter.prototype.add = function(e) {
    this.unserialized[e.type].push(e.timestamp);
    window.localStorage.setItem(e.type, JSON.stringify(this.unserialized[e.type]));
};

LocalStorageAdapter.prototype.clear = function() {
    var me = this;

    allEventTypes.forEach(function(eventType) {
        window.localStorage.setItem(eventType, '[]');
        me.unserialized[eventType] = [];
    });

};

///////////////////////////////////////////////////////////////////////////////

function unserializeEvent(buffer, index) {
    var type = buffer[index] in eventTypes ? buffer[index] : null,
        timestamp;

   if (type !== null) {
        timestamp = ((buffer[index + 1] & 0xFF)      ) |
                    ((buffer[index + 2] & 0xFF) << 8 ) |
                    ((buffer[index + 3] & 0xFF) << 16) |
                    ((buffer[index + 4] & 0xFF) << 24);

        return new Event(type, timestamp);
    } else {
        return null;
    }
}

function registerNewEvent(e) {
    if (e && !eventsByTypeAndTimestamp[e.type][e.timestamp]) {
        events.push(e);
        eventsByTypeAndTimestamp[e.type][e.timestamp] = e;

        return true;
    }

    return false;
}

function handleEventTransmission(e) {
    if (resetPending) {
        console.log('reset pending, ignoring event transmission');
        return;
    }

    if (MESSAGE_KEY_EVENT_BLOB in e.payload) {
        var buffer = e.payload[MESSAGE_KEY_EVENT_BLOB],
            eventCount = buffer[0],
            addedEvents = 0,
            evt;

        console.log('received ' + eventCount + ' events');

        for (var i = 0; i < eventCount; i++) {
            evt = unserializeEvent(buffer, 1 + 5*i);

            if (registerNewEvent(evt)) {
                storage.add(evt);
                addedEvents++;
            }
        }

        console.log('added ' + addedEvents + ' events, now at ' +
            events.length + ' events total');
    } else {
        console.log('invalid event transmission');
    }
}

function startReset() {

    function send() {
        var message = {};
        message[MESSAGE_KEY_MESSAGE_TYPE] = MESSAGE_TYPE_RESET_REQUEST;

        Pebble.sendAppMessage(message,
            function() {},
            function() {
                console.log('send failed, rescheduling...');
                setTimeout(send, RETRY_TIMEOUT);
            }
        );
    }

    if (resetPending) {
        return;
    }

    resetPending = true;

    send();
}

function handleResetAck() {
    storage.clear();
    events = [];

    allEventTypes.forEach(function(type) {
        eventsByTypeAndTimestamp[type] = {};
    });

    resetPending = false;

    console.log('reset complete');
}

Pebble.addEventListener("ready",
	function(e) {
		console.log("Pebby JavaScript ready!");

        storage = new LocalStorageAdapter();

        storage.getAll().forEach(registerNewEvent);

        console.log("loaded " + events.length + " events from local storage");
    }
);

Pebble.addEventListener("appmessage",
	function(e) {
        console.log('incoming message');

        if (!(MESSAGE_KEY_MESSAGE_TYPE in e.payload)) {
            console.log('invalid message');
            return;
        }

        var messageType = e.payload[MESSAGE_KEY_MESSAGE_TYPE];

        switch (messageType) {
            case MESSAGE_TYPE_EVENT_TRANSMISSION:
                console.log('handling event transmission');
                handleEventTransmission(e);
                break;

            case MESSAGE_TYPE_RESET_ACK:
                console.log('handling reset ack');
                handleResetAck();
                break;

            default:
                console.log('invalid message type ' + messageType);
        }
    }
);

Pebble.addEventListener("showConfiguration",
	function(e) {
        var rawData = {
            0: [],
            1: [],
            2: [],
            3: []
        };

        console.log('show config');

        events.forEach(function(event) {
            rawData[event.type].push(event.timestamp);
        });

        var serializedData = JSON.stringify({
            1: JSON.stringify(rawData[eventTypes.feed]),
            2: JSON.stringify(rawData[eventTypes.diaper_change]),
            3: JSON.stringify(rawData[eventTypes.sleep_start]),
            4: JSON.stringify(rawData[eventTypes.sleep_stop])
        });

        var url = "http://www.epicvortex.com/pebble/pebby.html?#" + serializedData;

        console.log(url);

        Pebble.openURL(url);
	}
);

Pebble.addEventListener("webviewclosed",
	function(e) {
		console.log("Configuration window returned: " + e.response);
		if (e.response == "reset") {
			startReset();
		}
	}
);

})(window);
