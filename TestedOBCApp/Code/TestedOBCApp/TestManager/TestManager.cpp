#include "TestManager.h"

#include <EventLogger.h>

#include "Factory.h"
#include <string.h>







TestManager::TestManager(ITimer* timer) : m_timer(timer)
{

}

TestManager::~TestManager()
{

}

TestResults::TestResult TestManager::runTest(TestRecipe params, TcaCalculation::sPointData * pointsData)
{
	bool dataGenerated = true;
	std::string logString = "";
	TestResults::TestResult results = { 0 };
	ITcaAlgorithm* Algoritm;

	results.degree = params.catchPolynomialDegree;
	results.testedAlgorithm = params.testedAlgorithm;
	results.catchRootsAlg = params.catchRootsAlg;
	results.testID = params.testID;
	results.numberOfRuns = params.numberOfIterations;

	results.segmentSizeSec = params.segmentSizeSec;
	results.timeIntervalSizeSec = params.timeIntervalSizeSec;
	results.numberOfPointsPerSegment = params.numberOfPointsPerSegment;
	results.initialNumberOfPoints = params.numberOfPoints;
	results.TminFactor = params.TminFactor;

	memcpy(results.testName, params.testName, MAX_TEST_NAME_SIZE);

	switch (params.testedAlgorithm)
	{
	default:
	case AlgorithmsEnums::Algorithm::ANCAS:
		Algoritm = Factory::GetReference()->GetANCAS();
		break;
	case AlgorithmsEnums::Algorithm::CATCH:
		Algoritm = Factory::GetReference()->GetCATCH(params.catchRootsAlg, params.catchPolynomialDegree);
		break;
	case AlgorithmsEnums::Algorithm::SBO_ANCAS:
		Algoritm = Factory::GetReference()->GetSboAncas(params.elsetrec1, params.elsetrec2, params.startTime1Min, params.startTime2Min, params.TOLd, params.TOLt);
		break;
	}

	m_timer->startTimer();
////////////////////////////////////////////To Add Data Generation

	if (nullptr == pointsData) {
		logString = "Failed To Load Test Data";
		EventLogger::getInstance().log(logString, "Lab");
		return results;
	}
	dataGenerated &= m_dataGenerator.CalculateRelativeVectorsForTwoElements(params.numberOfPoints, params.elsetrec1, params.elsetrec2, pointsData, params.startTime1Min, params.startTime2Min);
	if (!dataGenerated)
	{
		logString = "Failed To Generate Test Data";
		EventLogger::getInstance().log(logString, "Lab");
		return results;
	}
	logString = "Test Data Generated";
	EventLogger::getInstance().log(logString, "Lab");

	// if (recipe.elsetrec1.error == 0 && recipe.elsetrec2.error == 0)
	// {
	// 	recipe.segmentSizeSec = DataGenerator::GetGamma(recipe.elsetrec1, recipe.elsetrec2, recipe.TminFactor);
	// 	//For the first segment we use the full number of points, for each of the following we use one less
	// 	recipe.numberOfPoints = 1 + (static_cast<int>(recipe.timeIntervalSizeSec / recipe.segmentSizeSec)) * (recipe.numberOfPointsPerSegment - 1);
	// 	*elementsVectors = new  TcaCalculation::sPointData[recipe.numberOfPoints];
	// 	if (nullptr != elementsVectors)
	// 	{
	// 		TestDataGenerationManager::GeneratePointsByAlgorithm(recipe.numberOfPointsPerSegment, recipe.timeIntervalSizeSec, recipe.segmentSizeSec, *elementsVectors, recipe.testedAlgorithm);
	// 		dataGenerated &= m_dataGenerator.CalculateRelativeVectorsForTwoElements(recipe.numberOfPoints, recipe.elsetrec1, recipe.elsetrec2, *elementsVectors, testInfo.recipe.startTime1Min, testInfo.recipe.startTime2Min);
	// 	}
	// 	else
	// 	{
	// 		dataGenerated = false;
	// 	}
	// }
	// else
	// {
	// 	dataGenerated = false;
	// }
//////////////////
	results.tca = Algoritm->RunAlgorithm(pointsData, params.numberOfPoints - 1);

	m_timer->stopTimer();

	results.runTimeMicro = m_timer->getTimeInMicroSec();
	long double runTimeMicro;
	long double avgTimeMicro = results.runTimeMicro;
	long double minTimeMicro = results.runTimeMicro;
	//Do additional iterations
	TcaCalculation::TCA tca;

	for (int i = 1; i < params.numberOfIterations;i++)
	{
		switch (params.testedAlgorithm)
		{
		default:
		case AlgorithmsEnums::Algorithm::ANCAS:
			Algoritm = Factory::GetReference()->GetANCAS();
			break;
		case AlgorithmsEnums::Algorithm::CATCH:
			Algoritm = Factory::GetReference()->GetCATCH(params.catchRootsAlg, params.catchPolynomialDegree);
			break;
		case AlgorithmsEnums::Algorithm::SBO_ANCAS:
			Algoritm = Factory::GetReference()->GetSboAncas(params.elsetrec1, params.elsetrec2, params.startTime1Min, params.startTime2Min, params.TOLd, params.TOLt);
			break;
		}
		m_timer->startTimer();
		////////////////////////////////////////////To Add Data Generation
		dataGenerated &= m_dataGenerator.CalculateRelativeVectorsForTwoElements(params.numberOfPoints, params.elsetrec1, params.elsetrec2, pointsData, params.startTime1Min, params.startTime2Min);
		if (!dataGenerated)
		{
			logString = "Failed To Generate Test Data";
			EventLogger::getInstance().log(logString, "Lab");
			return results;
		}
		logString = "Test Data Generated";
		EventLogger::getInstance().log(logString, "Lab");
		/////////////////////////////////////////////////////////////////

		tca = Algoritm->RunAlgorithm(pointsData, params.numberOfPoints);

		m_timer->stopTimer();

		runTimeMicro = m_timer->getTimeInMicroSec();

		if (runTimeMicro < minTimeMicro)
		{
			minTimeMicro = runTimeMicro;
		}
		avgTimeMicro += runTimeMicro;
	}
	results.avgTimeMicro = avgTimeMicro / params.numberOfIterations;
	results.minTimeMicro = minTimeMicro;
	return results;
}

