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

	HybridMotionModel::HybridMotionModel(std::size_t dy) : _dy(dy)
	{
	}
	
	HybridMotionModel::~HybridMotionModel()
	{
	}

	RealT HybridMotionModel::History::calculate_estimate(const ImageUpdate & image_update, Radians<> current_relative_rotation) const
	{
		auto dr = current_relative_rotation - relative_rotation;
		return image_update.pixels_of(dr);
	}
	
	Average<RealT> HybridMotionModel::History::calculate_offset(Ptr<FeaturePoints> current_feature_points, const RealT & estimate) const
	{
		auto current_table = current_feature_points->table();
		auto previous_table = feature_points->table();

		return previous_table->calculate_offset(*current_table, -estimate);
	}

	RealT HybridMotionModel::History::calculate_bearing(const ImageUpdate & image_update, const Average<RealT> & offset) const
	{
		// The offset is measured in pixels, so we convert it to degrees and use it to rectify errors in the gyro/compass:
		RealT image_bearing_offset = R2D * image_update.angle_of(offset.value());

		return corrected_bearing + image_bearing_offset;
	}

	void HybridMotionModel::update(const ImageUpdate & image_update)
	{
		if (!BasicSensorMotionModel::localization_valid()) return;

		Ref<FeaturePoints> current_feature_points = new FeaturePoints;
		current_feature_points->scan(image_update.image_buffer, tilt(), _dy);

		StringStreamT note;

		if (_history.feature_points)
		{
			auto estimate = _history.calculate_estimate(image_update, _relative_rotation);
			auto offset = _history.calculate_offset(current_feature_points, estimate);

			// At least 3 vertical edges contributed to this sample:

			if (offset.number_of_samples() >= 3) {
				RealT image_bearing = _history.calculate_bearing(image_update, offset);

				note << "Hybrid update (confidence = " << offset.number_of_samples() << "). Hybrid: " << (image_bearing - _history.corrected_bearing) << " Sensors: " << (_bearing - _previous_bearing) << std::endl;

				auto updated_bearing = interpolateAnglesDegrees(_bearing, image_bearing, 0.995);

				//log_debug("Bearing update", _bearing, "previous bearing", _previous_bearing, "updated bearing", updated_bearing, "bearing offset", bearing_offset, "pixel offset", offset.value());
				_corrected_bearing = updated_bearing;
			} else {
				_corrected_bearing = _bearing;

				note << "Sensor update (confidence = " << offset.number_of_samples() << "). Gyroscope: " << (_bearing - _previous_bearing) << std::endl;
			}

			note << "Pixel estimate " << estimate << std::endl;

			// Only update history if there is a signifcant change, otherwise keep tracking local frame of reference.
			if (estimate < -1.0 || estimate > 1.0) {
				note << "Updating tracking reference..." << std::endl;
				_history = History{current_feature_points, _relative_rotation, _corrected_bearing};
			}

			image_update.add_note(note.str());
		} else {
			_corrected_bearing = _bearing;
			
			_history = History{current_feature_points, _relative_rotation, _corrected_bearing};
		}

		// Used to compute a hybrid update between purely sensor based update, or sensor+image based update:
		_previous_bearing = _bearing;
	}
	
	Radians<> HybridMotionModel::bearing() const
	{
		if (_history.feature_points)
			return degrees(_corrected_bearing);
		else
			return BasicSensorMotionModel::bearing();
	}
}
