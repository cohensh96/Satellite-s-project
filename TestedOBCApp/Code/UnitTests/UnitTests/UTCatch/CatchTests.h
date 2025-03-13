#ifndef SHIELD_CatchTests_H    // Check if the symbol SHIELD_CatchTests_H is not defined
#define SHIELD_CatchTests_H    // Define the symbol SHIELD_CatchTests_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "CATCH.h"

//Mock classes
class MockCpp : public CPP
{
public:
    MockCpp()
    {

    }
    void getInterpulationMatrix(double interpolationMatrix[CATCH_MAX_DEGREE + 1][CATCH_MAX_DEGREE + 1])
    {
        for (int i = 0; i <= CATCH_MAX_DEGREE; i++)
        {
            for (int j = 0; j <= CATCH_MAX_DEGREE; j++)
            {
                interpolationMatrix[i][j] = m_interpolationMatrix[i][j];
            }
        }
    }
};





/// <summary>
/// Tests for the Chebyshev Polynomial class
/// </summary>
class CPPTests : public ::testing::Test {
protected:
    double maxError = 0.0001;
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};


class CATCHTests : public ::testing::Test {
protected:
    double maxError = 0.0001;
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};

class CATCHTestCase : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Teardown code
    }
};


#endif //SHIELD_CatchTests_H