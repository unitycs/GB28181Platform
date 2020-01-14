#include <string>
#include "catch.hpp"

SCENARIO("char array can be set to NULL", "char[]") {

	char szScale[8] = { 0 };
	float fltScale = 1.0;


	CHECK(szScale != nullptr);

	GIVEN("first element of the array seted to NULL") {
		szScale[0] = '\0';

		CHECK(szScale[0] == '\0');

		WHEN("szScale[0]  was set non-zero") {
			szScale[0] = 'a';
			THEN("szScale[0] is not empty! ") {

				std::string strScale(szScale);
				REQUIRE(strScale.length() > 0);
				REQUIRE(strScale.capacity() != 0);
			}
		}
	}
}


