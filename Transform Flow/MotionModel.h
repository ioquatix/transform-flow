//
//  MotionModel.h
//  Transform Flow
//
//  Created by Samuel Williams on 16/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#ifndef TRANSFORM_FLOW_MOTION_MODEL_H
#define TRANSFORM_FLOW_MOTION_MODEL_H

#include <Dream/Resources/Loader.h>
#include <Dream/Imaging/Image.h>

#include <Euclid/Numerics/Vector.h>
#include <Euclid/Numerics/Quaternion.h>

namespace TransformFlow
{
	using namespace Dream;
	using namespace Dream::Core;
	using namespace Euclid::Numerics;
	using namespace Dream::Imaging;

	class MotionModel;

	struct SensorUpdate {
		virtual ~SensorUpdate();
		virtual void apply(MotionModel * model) = 0;

		TimeT time_offset;
	};

	struct LocationUpdate : public SensorUpdate {
		virtual ~LocationUpdate();
		virtual void apply(MotionModel * model);

		double latitude, longitude, altitude;

		double horizontal_accuracy, vertical_accuracy;
	};

	struct HeadingUpdate : public SensorUpdate {
		virtual ~HeadingUpdate();
		virtual void apply(MotionModel * model);

		double magnetic_bearing, true_bearing;
	};

	struct MotionUpdate : public SensorUpdate {
		virtual ~MotionUpdate();
		virtual void apply(MotionModel * model);

		Vec3 rotation_rate;
		Vec3 acceleration;
		Vec3 gravity;
	};

	struct ImageUpdate : public SensorUpdate {
		virtual ~ImageUpdate();
		virtual void apply(MotionModel * model);
		
		Ref<Image> image_buffer;
	};

	class SensorData : public Object
	{
		protected:
			Ref<Resources::Loader> _loader;
			
			std::vector<Ref<Image>> _frames;
			Ref<Image> frame_for_index(std::size_t index);

			std::vector<Shared<SensorUpdate>> _sensor_updates;

			void parse_log();

		public:
			SensorData(const Path & path);
			virtual ~SensorData() noexcept;

			const std::vector<Shared<SensorUpdate>> & sensor_updates() const { return _sensor_updates; }
	};

	/// A basic motion model interface. The output is gravity, bearing (rotation about gravity from north axis) and position.
	class MotionModel : public Object
	{
		protected:
			Vec3 _gravity, _position;

			/// Measured in degrees from north.
			RealT _bearing;

		public:
			MotionModel();
			virtual ~MotionModel();

			void update(SensorUpdate * sensor_update);

			virtual void update(const LocationUpdate & location_update) = 0;
			virtual void update(const HeadingUpdate & heading_update) = 0;
			virtual void update(const MotionUpdate & motion_update) = 0;
			virtual void update(const ImageUpdate & image_update) = 0;

			const Vec3 & gravity() const { return _gravity; }
			const Vec3 & position() const { return _position; }
			RealT bearing() const { return _bearing; }
	};
}

#endif /* defined(__Transform_Flow__MorionModel__) */
