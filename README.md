# Transform Flow Visualisation

Transform Flow Visualiation is a tool for analysing mobile phone sensor data and video streams. It uses motion models defined by Library/TransformFlow for analysing data sets.

There are a number of ways to contribute to this project:

- Support more platforms, primarily Android and Linux, but potentially Windows in the future.
- Develop motion models, including designing a motion model which reconciles global and local tracking approaches.
- Better automatic evaluation of motion models in the visualisation tool.

## Organisation

Transform Flow is a C++ library for the development of outdoor augmented reality tracking algorithms. It includes a data capture and visualisation tool, as well as a browser application for using the algorithms in real time:

- [Transform Flow](https://github.com/HITLabNZ/transform-flow)
- [Transform Flow Capture for iOS](https://github.com/HITLabNZ/transform-flow-capture-ios)
- [Transform Flow Capture for Android](https://github.com/HITLabNZ/transform-flow-capture-android) (incomplete)
- [Transform Flow Data Sets](https://github.com/HITLabNZ/transform-flow-data)
- [Transform Flow Visualisation](https://github.com/HITLabNZ/transform-flow-visualisation)
- [Transform Flow Browser for iOS](https://github.com/HITLabNZ/transform-flow-browser-ios)
- [Transform Flow Browser for Android](https://github.com/HITLabNZ/transform-flow-browser-ios) (incomplete)

Currently the main development platform is Mac OS X and iOS, but we are expanding this to include Linux and Android.

## Video Stream Format

The video stream format consists of a directory of images and a CSV log file.

The CSV format is as follows:

	[sequence-number],[event-name],[event-arguments,]

There are several pre-defined events:

	const char * GYROSCOPE = "Gyroscope";
	const char * ACCELEROMETER = "Accelerometer";
	const char * GRAVITY = "Gravity";
	const char * MOTION = "Motion";
	const char * LOCATION = "Location";
	const char * HEADING = "Heading";
	const char * FRAME = "Frame";

### Motion Events

The motion event is a group of device sensor information at a single timestamp. It currently includes the `Gyroscope`, `Accelerometer` and `Gravity` events with the same timestamp and typically takes the form:

	1,Gyroscope,[timestamp],[rotation.x],[rotation.y],[rotation.z]
	2,Accelerometer,[timestamp],[acceleration.x],[acceleration.y],[acceleration.z]
	3,Gravity,[timestamp],[gravity.x],[gravity.y],[gravity.z]
	4,Motion,[timestamp]

The timestamp SHOULD be the same value for grouped motion events.

### Location Events

The location event typically represents an update from the GPS and includes the position and accuracy of the update:

	1,Location,[timestamp],[latitude],[longitude],[horizontal_accuracy],[vertical_accuracy]

### Heading Events

The heading event typically represents an update from the compass. It includes both the magnetic north and true north.

	1,Heading,[timestamp],[magnetic_bearing],[true_bearing]

### Frame Events

The frame event represents a camera frame captured and includes data as an external PNG file in the same directory as the log file.

	1,Frame,[timestamp],[index]

The file in this case would be called `[index].png`.

### Device Information

It is useful to log device specific information and frame rates. These are currently not used directly by the visualisation tool, but may be in the future.

	1,Device,[device_name],[device_model],[system_version]

This record is normally logged at the start of the capture.

Additional records relating to configuration may be included (e.g. frame rates, sensor rates) but these are for informational purposes at this time.

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## License

Released under the MIT license.

Copyright, 2013, by [Samuel G. D. Williams](http://www.codeotaku.com/samuel-williams).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.