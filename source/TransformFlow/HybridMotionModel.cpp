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
				Mat44 r = rotate(tilt(), Vec3{0, 0, 1});
				Vec3 u {r.at(0, 0), r.at(0, 1), r.at(0, 2)};

				// This is hard code...
				auto normalised_offset = offset.value() / 25.0;

				//alignment_offset = translate(u * normalised_offset);
				_bearing += normalised_offset;

				note << "Updating bearing by " << normalised_offset << " from offset = " << offset.value() << std::endl;
			} else {
				note << "Not confident enough to update feature transform" << std::endl;
			}

			image_update.add_note(note.str());
		}
		
		_previous_feature_points = current_feature_points;
	}
}
