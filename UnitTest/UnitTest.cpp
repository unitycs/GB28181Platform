// UnitTest.cpp : Defines the entry point for the console application.
//
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <iostream>
#include <regex>


unsigned int Factorial(unsigned int number) {
	return number <= 1 ? number : Factorial(number - 1)*number;
}

int main(int argc, char* argv[])
{

	int numFailed = Catch::Session().run(argc, argv);

	return (numFailed < 0xff ? numFailed : 0xff);
}

TEST_CASE
(
	"my own"
)
{
	SECTION("check std::toint") {

		//	videoParamAttribute_t vv;
		auto str_c = "0";
		auto ulongret = std::stoul(str_c);
		str_c = "234567";
		ulongret = std::stoul(str_c);
		REQUIRE(ulongret > 0);

	}

	SECTION("check std::regex") {

		std::string contentstr = " sabcdefg\r\nssdasdf\rsadasdas\ndsdfsdfk\r\r\rsdsafg\r\n\n\n\n142432534\n\r\r\rasdasda\r\r\n\r\n\r\n\rsda"
			"f34534\n\r\r\nasd\r\n\r\n\r\r\r\r\n\n\n\n\r\n\r\n\r\n123";
		std::string mode_str = "(\r|\n)[\r\n]*";
		std::regex reg_mode(mode_str, std::regex_constants::icase);
		auto resout = std::regex_replace(contentstr, reg_mode, "\r\n");
		//std::cout << resout << std::endl;
	}


}



TEST_CASE
(
	"Some simple comparisons between doubles",
	"[Approx]"
)
{
	double d = 1.23;

	REQUIRE(d == Approx(1.23));
	REQUIRE(d != Approx(1.22));
	REQUIRE(d != Approx(1.24));

	REQUIRE(Approx(d) == 1.23);
	REQUIRE(Approx(d) != 1.22);
	REQUIRE(Approx(d) != 1.24);
}