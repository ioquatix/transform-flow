//
//  VideoStream.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 8/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "VideoStream.h"
#include <Dream/Core/Data.h>

#include <Euclid/Numerics/Interpolate.h>
#include <Euclid/Numerics/Transforms.h>

namespace TransformFlow {
	using namespace Dream::Events::Logging;
	
	const char * GYROSCOPE = "Gyroscope";
	const char * ACCELEROMETER = "Accelerometer";
	const char * GRAVITY = "Gravity";
	const char * MOTION = "Motion";
	const char * FRAME = "Frame Captured";

	VideoStream::VideoStream(const Path & path) : _loader(new Resources::Loader(path)) {
		_loader->add_loader(new Image::Loader);
		
		parse_log();
	}
	
	VideoStream::~VideoStream() {
		
	}
	
	Ref<Image> VideoStream::frame_for_index(std::size_t index) {
		if (index < _images.size()) {
			if (_frames[index]) {
				return _frames[index];
			}
		} else {
			_frames.resize(index+1);
		}
		
		logger()->log(LOG_DEBUG, LogBuffer() << "Loading frame " << index);
		return (_frames[index] = _loader->load<Image>(to_string(index)));
	}
	
	void VideoStream::parse_log() {
		Ref<IData> data = _loader->data_for_resource("log.txt");
		Shared<std::istream> stream = data->input_stream();
		
		std::string line;

		MotionUpdate motion_update;

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
				motion_update.rotation_rate = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
			} else if (parts.at(1) == ACCELEROMETER) {
				motion_update.acceleration = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
			} else if (parts.at(1) == GRAVITY) {
				motion_update.gravity = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
			} else if (parts.at(1) == MOTION) {
				motion_update.time_offset = to<TimeT>(parts.at(2));
				_motion.push_back(motion_update);
			} else if (parts.at(1) == FRAME) {
				ImageUpdate update;
				update.time_offset = to<RealT>(parts.at(3));
				update.image_buffer = frame_for_index(to<std::size_t>(parts.at(2)));
				_images.push_back(update);				
			} else {
				//logger()->log(LOG_WARN, LogBuffer() << "Unexpected line: " << line);
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
			image.feature_points->scan(image.image_buffer, image.tilt());
			watch.pause();

			log_debug("Feature Point Scan:", watch.time());
			
			previous = &image;
		}
	}
	
	void VideoStream::debug() {
		log_debug("Video Stream", _images.size(), "frames,", _motion.size(), "motion updates.");
	}
	
	Vec3 VideoStream::gravity_at_time(TimeT time) {
		for (std::size_t i = 1; i < _motion.size(); i += 1) {
			if (_motion[i].time_offset > time) {
				return _motion[i-1].gravity;
			}
		}
		
		return ZERO;
	}

	static Quat from_euler(Vec3 angles) {
		return rotate<X>(radians(angles[X])) << rotate<Y>(radians(angles[Y])) << rotate<Z>(radians(angles[Z]));
		//return Quat(radians(angles[X]), Vec3(1,0,0)) * Quat(radians(angles[Y]), Vec3(0,1,0)) * Quat(radians(angles[Z]), Vec3(0,0,1));
	}

	Quat VideoStream::rotation_between(TimeT start_time, TimeT end_time) {
		// We need to find the first gyroscope update to work from:
		std::size_t first = 1;
		for (; first < _motion.size(); first += 1) {
			if (_motion[first].time_offset >= start_time) {
				break;
			}
		}
		
		// Now we need to incrementally calculate the total rotation over the matching frames:
		Quat total(IDENTITY);

		for (std::size_t i = first; i < _motion.size(); i += 1) {
			TimeT sample_start_time = _motion[i-1].time_offset;
			TimeT sample_end_time = _motion[i].time_offset;

			if (start_time > sample_start_time && start_time < sample_end_time) {
				sample_start_time = start_time;
			}

			if (end_time > sample_start_time && end_time < sample_end_time) {
				sample_end_time = end_time;
			}
			
			TimeT dt = sample_end_time - sample_start_time;

			total *= from_euler(_motion.at(i).rotation_rate * dt);
			total = total.normalize();
			
			if (sample_end_time >= end_time)
				break;
		}
		
		return total;
	}
	
}
