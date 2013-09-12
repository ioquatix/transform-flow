
#include <UnitTest/UnitTest.h>
#include <TransformFlow/FeaturePoint.h>

namespace TransformFlow {
	UnitTest::Suite BlockTestSuite {
		"Test Feature Point",

		{"Check Similarity",
			[](UnitTest::Examiner & examiner) {
				FeaturePoint p1;
				
				p1.a = Vec3(0.5, 0.5, 0.5);
				p1.b = Vec3(0.5, 0.5, 0.5);
				
				examiner.check_equal(p1.difference(p1), 0.0);
			}
		}
	};
}
