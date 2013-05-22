//
//  VideoStream.h
//  Transform Flow
//
//  Created by Samuel Williams on 8/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#ifndef _TRANSFORM_FLOW_VIDEO_STREAM_H
#define _TRANSFORM_FLOW_VIDEO_STREAM_H

#include <Dream/Resources/Loader.h>
#include <Euclid/Numerics/Vector.h>
#include <Euclid/Numerics/Quaternion.h>
#include <Dream/Imaging/Image.h>

#include "FeaturePoints.h"

namespace TransformFlow {
	using namespace Dream;
	using namespace Dream::Core;
	using namespace Euclid::Numerics;
	using namespace Dream::Imaging;

	struct GyroscopeUpdate {
		TimeT time_offset;
		Vec3 rotation;
	};
	
	struct AccelerometerUpdate {
		TimeT time_offset;
		Vec3 acceleration;
	};
	
	struct GravityUpdate {
		TimeT time_offset;
		Vec3 gravity;
	};
	
	class FeaturePoints;
	
	struct ImageUpdate {
		TimeT time_offset;
		Ref<Image> image_buffer;
		
		Ref<FeaturePoints> feature_points;
		
		Vec3 gravity;
		Quat rotation;

		Radians<> tilt() const {
			Vec3 down = {-1, 0, 0};
			Vec3 r(gravity[X], gravity[Y], 0);

			auto angle = r.normalize().angle_between(down);
			auto orthogonal = cross_product(r, down);

			if (orthogonal.dot({0, 0, -1}) < 0)
				return (double)angle;
			else
				return (double)(R360 - angle);
		}
	};
		
	class VideoStream : public Object {
	protected:
		Ref<Resources::Loader> _loader;
		
		std::vector<Ref<Image>> _frames;
		Ref<Image> frame_for_index(std::size_t index);
		
		void parse_log();
		
		std::vector<GyroscopeUpdate> _gyroscope;
		std::vector<AccelerometerUpdate> _accelerometer;
		std::vector<GravityUpdate> _gravity;
		std::vector<ImageUpdate> _images;
		
	public:		
		VideoStream(const Path & path);
		virtual ~VideoStream() noexcept;
		
		void debug();
		
		std::vector<AccelerometerUpdate> & accelerometer() { return _accelerometer; }
		
		std::vector<ImageUpdate> & images() { return _images; }
		
		Vec3 gravity_at_time(TimeT time);
		Quat rotation_between(TimeT start, TimeT end);
	};
	
}

#endif
