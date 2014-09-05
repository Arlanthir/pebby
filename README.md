pebby
=====

Pebby - Pebble Baby Helper

This repository contains the full code to the Pebby app.
For an example configuration page, check http://epicvortex.com/pebble/pebby.html

A basic configuration page for this app should have a way to display statistics received by a JSON anchor.
Example:

    yourpage.html#{  
      "1": `[<timestamp>`, `<timestamp>`, `<timestamp>`, ...],  
      "2": `[<timestamp>`, `<timestamp>`, `<timestamp>`, ...],  
      "3": `[<timestamp>`, `<timestamp>`, `<timestamp>`, ...],  
      "4": `[<timestamp>`, `<timestamp>`, `<timestamp>`, ...]  
    }

Where:
* 1 is a collection of timestamps when the user pressed the feeding icon,
* 2 is a collection of when the user pressed the diaper icon,
* 3 is a collection of when the user pressed the sleeping start icon and
* 4 is a collection of when the user pressed the sleeping end icon.

In the current page, Chart.js (http://www.chartjs.org/) is used to display the graphs.
