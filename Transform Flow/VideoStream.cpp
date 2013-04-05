//
//  VideoStream.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 8/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "VideoStream.h"
#include <Dream/Core/Data.h>
#include <Dream/Numerics/Interpolate.h>

namespace TransformFlow {
	using namespace Dream::Events::Logging;
	
	const char * GYROSCOPE = "Gyroscope";
	const char * ACCELEROMETER = "Accelerometer";
	const char * GRAVITY = "Gravity";
	const char * FRAME = "Frame Captured";
	
	VideoStream::VideoStream(const Path & path) : _loader(new Resources::Loader(path)) {
		_loader->add_loader(new Image::Loader);
		
		parse_log();
	}
	
	VideoStream::~VideoStream() {
		
	}
	
	Ref<IPixelBuffer> VideoStream::frame_for_index(IndexT index) {
		if (index < _images.size()) {
			if (_frames[index]) {
				return _frames[index];
			}
		} else {
			_frames.resize(index+1);
		}
		
		logger()->log(LOG_DEBUG, LogBuffer() << "Loading frame " << index);
		return (_frames[index] = _loader->load<IPixelBuffer>(to_string(index)));
	}
	
	void VideoStream::parse_log() {
		Ref<IData> data = _loader->data_for_resource("log.txt");
		Shared<std::istream> stream = data->input_stream();
		
		std::string line;
		while (stream->good()) {
			std::getline(*stream, line, '\n');
			
			std::vector<std::string> parts;
			split(line, ',', std::back_inserter(parts));
			
			for (auto & part : parts) {
				part = trimmed(part, " ");
			}
			
			if (parts.size() < 2)
				continue;
			
			if (parts.at(1) == GYROSCOPE) {
				GyroscopeUpdate update;
				update.time_offset = to<RealT>(parts.at(2));
				update.rotation = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
				_gyroscope.push_back(update);
			} else if (parts.at(1) == ACCELEROMETER) {
				AccelerometerUpdate update;
				update.time_offset = to<RealT>(parts.at(2));
				update.acceleration = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
				_accelerometer.push_back(update);
			} else if (parts.at(1) == GRAVITY) {
				GravityUpdate update;
				update.time_offset = to<RealT>(parts.at(2));
				update.gravity = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
				_gravity.push_back(update);
			} else if (parts.at(1) == FRAME) {
				ImageUpdate update;
				update.time_offset = to<RealT>(parts.at(3));
				update.image_buffer = frame_for_index(to<IndexT>(parts.at(2)));
				_images.push_back(update);				
			} else {
				logger()->log(LOG_WARN, LogBuffer() << "Unexpected line: " << line);
			}
		}
		
		ImageUpdate * previous = nullptr;
		
		for (auto & image : _images) {
			image.gravity = gravity_at_time(image.time_offset);
			
			if (previous) {
				image.rotation = rotation_between(previous->time_offset, image.time_offset);
			} else {
				image.rotation = Quat(IDENTITY);
			}
			
			image.feature_points = new FeaturePoints;

			Dream::Core::Stopwatch watch;
			watch.start();
			image.feature_points->scan(image.image_buffer);
			watch.pause();

			logger()->log(LOG_INFO, LogBuffer() << "Feature Point Scan: " << watch.time());
			
			previous = &image;
		}
	}
	
	void VideoStream::debug() {
		LogBuffer buffer;
		buffer << "Video Stream Debug:" << std::endl;
		
		buffer << "Gyroscope Updates: " << _gyroscope.size() << std::endl;
		buffer << "Accelerometer Updates: " << _accelerometer.size() << std::endl;
		buffer << "Gravity Updates: " << _gravity.size() << std::endl;
		buffer << "Image Updates: " << _images.size() << std::endl;
		
		logger()->log(LOG_DEBUG, buffer);
	}
	
	Vec3 VideoStream::gravity_at_time(RealT time) {
		for (std::size_t i = 1; i < _gravity.size(); i += 1) {
			if (_gravity[i].time_offset > time) {
				RealT start_time = _gravity[i-1].time_offset;
				RealT end_time = _gravity[i].time_offset;
				
				RealT duration = end_time - start_time;
				RealT offset = end_time - time;
				
				Vec3 result = linear_interpolate(offset / duration, _gravity[i-1].gravity, _gravity[i].gravity);

				return result.normalized_vector();
			}
		}
		
		return ZERO;
	}
	
	Quat VideoStream::rotation_between(RealT start_time, RealT end_time) {
		// We need to find the first gyroscope update to work from:
		std::size_t first = 1;
		for (; first < _gyroscope.size(); first += 1) {
			if (_gyroscope[first].time_offset > start_time) {
				break;
			}
		}
		
		// Now we need to incrementally calculate the total rotation over the matching frames:
		// - could use linear interpolation for the start and end frame, but forget about that for now..
		Quat total(IDENTITY);
		
		for (std::size_t i = first; i < _gyroscope.size(); i += 1) {
			RealT dt = _gyroscope[i].time_offset - _gyroscope[i-1].time_offset;
			total *= Quat::from_euler(_gyroscope[i].rotation * dt);
			
			if (_gyroscope[i].time_offset > end_time)
				break;
		}
		
		return total;
	}
	
}
