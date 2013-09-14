# Transform Flow Visualisation

Transform Flow Visualiation is a tool for analysing mobile phone sensor data and video streams. It uses motion models defined by Library/TransformFlow for analysing data sets.

### Stream Format

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

	1,Location,[timestamp],[latitude],[longitude],[accuracy.horizontal],[accuracy.vertical]

### Heading Events

The heading event typically represents an update from the compass. It includes both the magnetic north and true north.

	1,Heading,[timestamp],[heading.magnetic],[heading.true]

### Frame Events

The frame event represents a camera frame captured and includes data as an external PNG file in the same directory as the log file.

	1,Frame,[timestamp],[index]

The file in this case would be called `[index].png`.

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