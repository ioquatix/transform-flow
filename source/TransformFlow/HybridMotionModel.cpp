//
//  HybridMotionModel.cpp
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 12/8/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#include "HybridMotionModel.h"

#include <Dream/Events/Logger.h>

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
		Ref<FeaturePoints> current_feature_points = new FeaturePoints;
		current_feature_points->scan(image_update.image_buffer, tilt());

		StringStreamT note;

		if (_previous_feature_points)
		{
			auto current_table = current_feature_points->table();
			auto previous_table = _previous_feature_points->table();

			auto offset = previous_table->calculate_offset(*current_table);

			// At least 3 vertical edges contributed to this sample:
			if (offset.number_of_samples() > 3) {
				// This offset is measured in pixels, so we convert it to degrees and use it to rectify errors in the gyro/compass:
				RealT bearing_offset = R2D * image_update.angle_of(offset.value());

				//log_debug("bearing offset", bearing_offset, "actual offset", (_bearing - _previous_bearing));

				auto updated_bearing = interpolateAnglesDegrees(_bearing, _previous_bearing + bearing_offset, 1.0);
				//log_debug("Bearing update", _bearing, "previous bearing", _previous_bearing, "updated bearing", updated_bearing, "bearing offset", bearing_offset, "pixel offset", offset.value());
				_bearing = updated_bearing;

				note << "Updating bearing by " << bearing_offset << " from offset = " << offset.value() << std::endl;
			} else {
				_bearing = _previous_bearing;

				note << "Not confident enough to update feature transform" << std::endl;
			}

			image_update.add_note(note.str());
		}

		_previous_bearing = _bearing;
		_previous_feature_points = current_feature_points;
	}
}
