#include <string>
#include <iostream>
#include "catch.hpp"

TEST_CASE
(
	"test string allocaction for space",
	"[tmpstr]"
)
{

	std::string tmpstr('0', 40);
	std::cout << tmpstr << std::endl;

	REQUIRE(tmpstr.length() == 40);
}



SCENARIO("test smartzoom for ptz dragZoom", "[SmartZoom][Approx]") {

	int g_videoWidth = 1920, g_videoHeight = 1080;
	//CHECK(szScale != nullptr);

	GIVEN("基本的屏幕参数配置1920*1080 ") {

		int screenWidth = 1920, screenHeight = 1080;

		WHEN("[Group1]拿到到实际区域的坐标参数值组") {

			int point_x = 842, point_y = 182;
			int areaWidth = 289, areaHeigh = 183;

			THEN("按顺序计算出目标区域的参数值") {

				int x = (point_x - (areaWidth*1.0 / 2)) * g_videoWidth / screenWidth;
				int y = (point_y - (areaHeigh*1.0 / 2)) * g_videoHeight / screenHeight;
				int width = areaWidth*1.0 * g_videoWidth / screenWidth;
				int height = areaHeigh*1.0  * g_videoHeight / screenHeight;

				REQUIRE(x == Approx(698).epsilon(1.0));
				REQUIRE(y == Approx(91).epsilon(1.0));
				REQUIRE(width == Approx(289).epsilon(1.0));
				REQUIRE(height == Approx(183).epsilon(1.0));

			}
		}

		WHEN("[Group2] get the virtual screen size from hus-gb-up") {

			int point_x = 842, point_y = 182;
			int areaWidth = 289, areaHeigh = 183;
			
			THEN("用括号表达准确的计算语义，再次计算出目标参数值并比较") {

				int x = (point_x - (areaWidth*1.0 / 2)) * (g_videoWidth / screenWidth);
				int y = (point_y - (areaHeigh*1.0 / 2)) * (g_videoHeight / screenHeight);
				int width = areaWidth*1.0 * (g_videoWidth / screenWidth);
				int height = areaHeigh*1.0  * (g_videoHeight / screenHeight);

				REQUIRE(x == Approx(698).epsilon(1.0));
				REQUIRE(y == Approx(91).epsilon(1.0));
				REQUIRE(width == Approx(289).epsilon(1.0));
				REQUIRE(height == Approx(183).epsilon(1.0));
			}
		}


	}
}




