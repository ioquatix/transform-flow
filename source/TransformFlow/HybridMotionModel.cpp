//
//  HybridMotionModel.cpp
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 12/8/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#include "HybridMotionModel.h"

#include <Dream/Events/Logger.h>
#include <cmath>

namespace TransformFlow
{
	using namespace Dream::Events::Logging;

	HybridMotionModel::HybridMotionModel()
	{
	}
	
	HybridMotionModel::~HybridMotionModel()
	{
	}

	void HybridMotionModel::update(const ImageUpdate & image_update)
	{
		if (!BasicSensorMotionModel::localization_valid()) return;

		Ref<FeaturePoints> current_feature_points = new FeaturePoints;
		current_feature_points->scan(image_update.image_buffer, tilt());

		StringStreamT note;

		if (_previous_feature_points)
		{
			auto current_table = current_feature_points->table();
			auto previous_table = _previous_feature_points->table();

			auto dr = _relative_rotation - _previous_relative_rotation;
			int estimate = image_update.pixels_of(dr);

			note << "Estimated change = " << estimate << std::endl;

			auto offset = previous_table->calculate_offset(*current_table, -estimate);

			// At least 3 vertical edges contributed to this sample:
			if (offset.number_of_samples() >= 5) {
				// This offset is measured in pixels, so we convert it to degrees and use it to rectify errors in the gyro/compass:
				RealT image_bearing_offset = R2D * image_update.angle_of(offset.value());
				
				// This is the change computed by the sensors:
				RealT sensor_bearing_offset = _bearing - _previous_bearing;

				note << "Hybrid update (confidence = " << offset.number_of_samples() << "). Image: " << image_bearing_offset << " Gyroscope: " << sensor_bearing_offset << std::endl;

				//log_debug("bearing offset", bearing_offset, "actual offset", (_bearing - _previous_bearing));

				auto updated_bearing = interpolateAnglesDegrees(_corrected_bearing + sensor_bearing_offset, _corrected_bearing + image_bearing_offset, 0.98);

				//log_debug("Bearing update", _bearing, "previous bearing", _previous_bearing, "updated bearing", updated_bearing, "bearing offset", bearing_offset, "pixel offset", offset.value());
				_corrected_bearing = updated_bearing;
			} else {
				_corrected_bearing = _bearing;

				note << "Sensor update (confidence = " << offset.number_of_samples() << "). Gyroscope: " << (_bearing - _previous_bearing) << std::endl;
			}

			image_update.add_note(note.str());
		} else {
			_corrected_bearing = _bearing;
		}

		// Used to compute a hybrid update between purely sensor based update, or sensor+image based update:
		_previous_bearing = _bearing;

		// Used to calculate estimate rotation for image processing:
		_previous_relative_rotation = _relative_rotation;

		//log_debug("previous rotation", _previous_relative_rotation, "current rotation", _relative_rotation);

		// Previous frame's feature points, kept around to match next frame based on estimate:
		_previous_feature_points = current_feature_points;
	}
	
	Radians<> HybridMotionModel::bearing() const
	{
		if (_previous_feature_points)
			return degrees(_corrected_bearing);
		else
			return BasicSensorMotionModel::bearing();
	}
}
