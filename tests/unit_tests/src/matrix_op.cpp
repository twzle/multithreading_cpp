#include "matrix_op/matrix.hpp"
#include "matrix_op/matrix_exception.hpp"

#include "catch2/catch_test_macros.hpp"

using namespace matrix_op;

TEST_CASE("Check matrix operations", "[matrix_op]")
{
    float content[] = { 1, 2, 3, 4 };

    Matrix m1(2, 2, content, content + sizeof(content) / sizeof(float));
    REQUIRE(m1.Rows() == 2);
    REQUIRE(m1.Columns() == 2);
    CHECK(m1[0][0] == 1);
    CHECK(m1[0][1] == 2);
    CHECK(m1[1][0] == 3);
    CHECK(m1[1][1] == 4);

    {
        Matrix m2(4, 1, content, content + sizeof(content) / sizeof(float));
        REQUIRE(m2.Rows() == 4);
        REQUIRE(m2.Columns() == 1);
        CHECK(m2[0][0] == 1);
        CHECK(m2[1][0] == 2);
        CHECK(m2[2][0] == 3);
        CHECK(m2[3][0] == 4);
    }

    // Неправильные данные
    CHECK_THROWS_AS(Matrix(5, 1, content, content + sizeof(content) / sizeof(float)), std::runtime_error);


    // Умножение
    {
        Matrix mul1 = m1 * m1;
        REQUIRE(mul1.Rows() == 2);
        REQUIRE(mul1.Columns() == 2);
        CHECK(mul1[0][0] == 7);
        CHECK(mul1[0][1] == 10);
        CHECK(mul1[1][0] == 15);
        CHECK(mul1[1][1] == 22);
    }
    {
        Matrix mul2 = m1 * Matrix(2, 1, content, content + 2);
        REQUIRE(mul2.Rows() == 2);
        REQUIRE(mul2.Columns() == 1);
        CHECK(mul2[0][0] == 5);
        CHECK(mul2[1][0] == 11);
    }

    // Умножение неправильных размерностей
    CHECK_THROWS_AS(m1 * Matrix(1, 1, content, content + 1), MatrixCalcError);
}
