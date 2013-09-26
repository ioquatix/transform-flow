
#include <UnitTest/UnitTest.h>
#include <TransformFlow/FeaturePoints.h>
#include <Dream/Imaging/Image.h>
#include <Euclid/Numerics/Vector.IO.h>

namespace TransformFlow {
	UnitTest::Suite FeaturePointsTestSuite {
		"Test Feature Points Functionality",

		{"Check Extraction",
			[](UnitTest::Examiner & examiner) {
				using namespace Dream::Events::Logging;

				Ref<Resources::Loader> loader = new Resources::Loader("../share/transform-flow");
				loader->add_loader(new Image::Loader);

				Ref<Image> bw16 = loader->load<Image>("samples/bw_16_0deg");
				Ref<Image> bw16g = loader->load<Image>("samples/bw_16_1g");
				
				{
					Ref<FeaturePoints> feature_points = new FeaturePoints();
					auto & offsets = feature_points->offsets();

					feature_points->scan(bw16, R0);
					examiner << "Offset " << offsets[0] << " is in the middle";
					examiner.check_equal(offsets[0][X], 15.5);
				}

				{
					Ref<FeaturePoints> feature_points = new FeaturePoints();
					auto & offsets = feature_points->offsets();
					
					feature_points->scan(bw16g, R0);
					examiner << "Offset " << offsets[0] << " is in the middle";
					examiner.check_equal(offsets[0][X], 15.5);
				}
			}
		}
	};
}
