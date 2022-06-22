# Moon watch
![alt text](moonwatch.gif "Watchy with moon phases")

A set of watch faces for the open-source E-Ink smartwatch [Watchy](https://watchy.sqfmi.com/).

## Features

- 12-hour time 
- Date
- Accurate moon phase, using NASA moon images. (2 per day)
- Additional 24-hour watch face with moon phase
- Third watch face with month calendar

Based on sqfmi's 7-segment example watch face, some free fonts, and NASA moon images.

Clock hands are drawn using polar coordinates, a reasonably simple way of rotating them.

Text (day & month names) are in Norwegian, which needs characters outside of ascii. So I made a library for printing UTF-8 strings to make this possible. Any unicode character in the basic multiligual plane is now printable on arduino, if backed by a suitable font and a bitmap display supported by Adafruit GFX. See [GFX-utf8](https://github.com/Hafting/gfx-utf8/)

The up and down menu keys are used to switch between watch faces (7segment digital, 12-hour analog, 24-hour analog, calendar). This could not be done with a simple override, so the repository contains an altered version of the 7-segment example.

Development was done with platformio on linux.

Future plans, if time permits:
- moon phases in the calendar display
- settable alarms
- fetching events (calendar sync), creating alarms & more calendar displays

The other watch faces:
![alt text](moonwatch24h.png "24-hour analog clock")
![alt text](moonwatchcal.png "Month calendar")
