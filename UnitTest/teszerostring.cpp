#include <string>
#include "catch.hpp"



TEST_CASE("check-char-array", "[zero element]") {

	char szScale[8] = { 0 };
	std::string strScale;
	float fltScale = 1.0;
	szScale[0] = '\0';

	REQUIRE(szScale[0] == NULL);
	REQUIRE(szScale[0] == '\0');
	REQUIRE(szScale == nullptr);
}

