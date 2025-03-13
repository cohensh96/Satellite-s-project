#include "lab.h"
#include "CommChannelFactory.h"
#include "EventLogger.h"
#include "AppVersion.h"
#include <vector>
#include <fstream>
#include <string>
#include <filesystem> 

Lab::Lab() : m_databaseManager("Tests.db"),
m_commManager(),
m_resultManager(),
m_dataGenerator(),
m_testManager(),
m_globalFlag(false)
{
    EventLogger::getInstance().log("Lab Instance Created", "Lab");

    std::string versionInfo = "App Version: " +
        std::to_string(ProjectVersions::VERSION_MAJOR) + "." +
        std::to_string(ProjectVersions::VERSION_MINOR) + "." +
        std::to_string(ProjectVersions::VERSION_PATCH);
    EventLogger::getInstance().log(versionInfo, "Lab");


    m_commManager.Init(&(CommChannelFactory::GetInstance().getCommChannel()));
    EventLogger::getInstance().log("Comm Manager Initialized", "Lab");

    m_testManager.init(m_resultManager, m_commManager);
    if (!m_databaseManager.createTables()) {
        std::cerr << "Failed to create tables." << std::endl;
        EventLogger::getInstance().log("DatabaseManager:Failed to create tables", "Lab");
    }
}

Lab::~Lab() {

}

Lab& Lab::GetInstance() {
    static Lab instance;
    return instance;
}

TestInfo Lab::GetTestInfo(int testId) {
    TestInfo testInfo = m_databaseManager.getTestInfo(testId);
    return testInfo;
}

void Lab::DeleteTest(int testId) {
    m_databaseManager.deleteTest(testId);
}

// Functions for accessing the global boolean variable
bool Lab::getGlobalFlag()
{
    return m_globalFlag;
}

void Lab::setGlobalFlag(bool val)
{
    m_globalFlag = val;
}

#include <windows.h>
#include <iostream>

std::string GetShortPath(const std::string& longPath) {
    char shortPath[MAX_PATH];
    if (GetShortPathNameA(longPath.c_str(), shortPath, MAX_PATH)) {
        return std::string(shortPath);
    }
    return longPath; // If it fails, the original path is returned.
}

#ifdef _WIN32
#include <windows.h>
#endif

// A function that returns the path of the EXE at runtime
std::string GetExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path().string(); // Returns only the EXE folder.
#else
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(dirname(buffer));
    }
    return "";
#endif
}


int Lab::CreateTestsFromCatalog(TestInfo testInfo, CatalogTestFormat catalogTestFormat) {
    std::cout << "Hello " << std::endl;
    int returnValue = -8;
    EventLogger::getInstance().log("Comm Manager Initialized", "Lab");
    std::string catalogFile = testInfo.firstElemData;
    EventLogger::getInstance().log("firstElemData:" + catalogFile, "Lab");
    // Building the full path to the catalog file
    std::wstring rootDirectory = std::filesystem::current_path().wstring();
    std::wstring subfolder = L"\\FullCatalogs\\";
    std::wstring catalogFileWideString(catalogFile.length(), L' ');
    std::copy(catalogFile.begin(), catalogFile.end(), catalogFileWideString.begin());
    std::wstring catalogFullPath = rootDirectory.append(subfolder).append(catalogFileWideString);
    std::string catalogFullPathStr(catalogFullPath.begin(), catalogFullPath.end());

    //std::string catalogFullPathStr = "C:\\ProjectMainCopy2\\Code\\TestingStationApp\\BlazorApp1\\BlazorApp1\\FullCatalogs\\activeCatalog.txt";

    std::cout << "Full Catalog File Path: " << catalogFullPathStr << std::endl;
    EventLogger::getInstance().log("Full Catalog File Path:" + catalogFullPathStr, "Lab");

    // Checking the existence of the file
    if (!std::filesystem::exists(catalogFullPathStr)) {
        std::cout << "Error: Catalog file not found at: " << catalogFullPathStr << std::endl;
        EventLogger::getInstance().log("Error: Catalog file not exist!", "Lab");
        return -5;
    }

    // Opening the file and reading it entirely into a string
    std::ifstream catalogFileStream(catalogFullPathStr, std::ios::binary);
    //if (!catalogFileStream.is_open()) {
    //    std::cout << "Error: Failed to open catalog file: " << catalogFullPathStr << std::endl;
    //    EventLogger::getInstance().log("Error: Failed to open catalog file", "Lab");
    //    return -3;
    //}
    std::string catalogContent((std::istreambuf_iterator<char>(catalogFileStream)),
        std::istreambuf_iterator<char>());
    catalogFileStream.close();

    // Using istringstream to process the content line by line
    std::istringstream catalogStream(catalogContent);

    // Reading the catalog records: Each record consists of three lines
    // First line: Satellite name, Second line: TLE line 1, Third line: TLE line 2
    std::vector<std::pair<std::string, std::string>> satellites;
    satellites.reserve(10000);
    std::string name, tle1, tle2;
    int satelliteCount = 0;
    while (std::getline(catalogStream, name) && std::getline(catalogStream, tle1) && std::getline(catalogStream, tle2)) {
        satellites.emplace_back(name, tle1 + "\n" + tle2);
        satelliteCount++;
    }
    std::cout << "Total Satellite count: " << satelliteCount << std::endl;

    if (satelliteCount < 2) {
        std::cout << "The Catalog Size is too small" << std::endl;
        EventLogger::getInstance().log("The Catalog Size is too small", "Lab");
        return -4;
    }

    int numberOfCases;
    switch (catalogTestFormat) {
    case CatalogTestFormat::AllWithAll:
        numberOfCases = (satelliteCount - 1) * satelliteCount / 2.0 - 1;
        break;
    case CatalogTestFormat::OneWithAll:
        numberOfCases = satelliteCount - 1;
        break;
    default:
        numberOfCases = satelliteCount - 1;
        break;
    }

    // Define a threshold distance for filtering.
    const double D = 10.0;  // 1 [km]

    // Two temporary variables to hold the a and the e
    double aS, eS, aD, eD;

    // Creating test cases according to the catalog format
    if (catalogTestFormat == CatalogTestFormat::OneWithAll) {
        int passedSatellites = 0;
        for (int i = 1; i < satelliteCount; i++) {

            // First we decode a,e for satellite 0
            {
                // Splitters for TLE line1, line2
                std::istringstream iss(satellites[0].second);
                std::string l1, l2;
                if (!std::getline(iss, l1) || !std::getline(iss, l2)) {
                    // parse error
                    continue;
                }
                if (!parseOrbitParamsFromTLE(l1, l2, aS, eS)) {
                    // parse error
                    continue;
                }
            }

            // Then we decode a,e for satellite i
            {
                std::istringstream iss(satellites[i].second);
                std::string l1, l2;
                if (!std::getline(iss, l1) || !std::getline(iss, l2)) {
                    // parse error
                    continue;
                }
                if (!parseOrbitParamsFromTLE(l1, l2, aD, eD)) {
                    continue;
                }
            }

            // Perigee/apogee calculation for both
            double perigeeS = aS * (1.0 - eS);
            double apogeeS = aS * (1.0 + eS);
            double perigeeD = aD * (1.0 - eD);
            double apogeeD = aD * (1.0 + eD);

            double q = (std::max)(perigeeS, perigeeD);
            double Q = (std::min)(apogeeS, apogeeD);

            std::cout << "the q-Q value is: " << (q - Q) << std::endl;
            // Filter test (3c in the article ): If q - Q > D => reject
            if ((q - Q) > D) {
                // No need to create a test, continue to the next one.
                std::cout << "Continue->Does Not Create Test " << std::endl;
                continue;
            }
            std::cout << "Pass and Create New Test!! " << std::endl;
            passedSatellites++;

            Lab::GetInstance().setGlobalFlag(false);
            memset(testInfo.firstElemData, 0, sizeof(testInfo.firstElemData));
            memset(testInfo.secondElemData, 0, sizeof(testInfo.secondElemData));

            strncpy_s(testInfo.firstElemData, sizeof(testInfo.firstElemData), satellites[0].second.c_str(), _TRUNCATE);
            strncpy_s(testInfo.secondElemData, sizeof(testInfo.secondElemData), satellites[i].second.c_str(), _TRUNCATE);
            testInfo.format = SatelliteDataFormat::Text;

            memset(testInfo.recipe.testName, 0, 80);
            std::string testName = satellites[0].first + "_" + satellites[i].first;
            strncpy_s(testInfo.recipe.testName, testName.c_str(), 80);

            testInfo.firstElemData[sizeof(testInfo.firstElemData) - 1] = '\0';
            testInfo.secondElemData[sizeof(testInfo.secondElemData) - 1] = '\0';

            //std::this_thread::sleep_for(std::chrono::seconds(5));
            returnValue = CreateTest(testInfo);
            if (returnValue < 0) {
                return returnValue;
            }
            while (!m_globalFlag) {}
        }
        std::cout << "Number of satellites (OneWithAll) that pass the filter: "
            << passedSatellites << std::endl;
    }
    else if (catalogTestFormat == CatalogTestFormat::AllWithAll) {
        // Count how many pairs (i,j) pass the filter
        int validPairs = 0;
        for (int i = 0; i < satelliteCount - 1; i++) {

            // First, we extracted satellite i
            std::istringstream issI(satellites[i].second);
            std::string iLine1, iLine2;
            bool parsedI = false;
            if (std::getline(issI, iLine1) && std::getline(issI, iLine2)) {
                parsedI = parseOrbitParamsFromTLE(iLine1, iLine2, aS, eS);
            }
            if (!parsedI) continue; // parse error on satellite i


            for (int j = i + 1; j < satelliteCount; j++) {

                // Satellite extractor j
                std::istringstream issJ(satellites[j].second);
                std::string jLine1, jLine2;
                bool parsedJ = false;
                if (std::getline(issJ, jLine1) && std::getline(issJ, jLine2)) {
                    parsedJ = parseOrbitParamsFromTLE(jLine1, jLine2, aD, eD);
                }
                if (!parsedJ) continue; // parse error on satellite j

                // Filter calculation
                double perigeeS = aS * (1.0 - eS);
                double apogeeS = aS * (1.0 + eS);
                double perigeeD = aD * (1.0 - eD);
                double apogeeD = aD * (1.0 + eD);

                double q = (std::max)(perigeeS, perigeeD);
                double Q = (std::min)(apogeeS, apogeeD);

                if ((q - Q) > D) {
                    // They won't come close => there's no point in creating a test
                    continue;
                }

                // If we got here => the pair (i,j) passed the filter
                validPairs++;

                Lab::GetInstance().setGlobalFlag(false);
                memset(testInfo.firstElemData, 0, sizeof(testInfo.firstElemData));
                memset(testInfo.secondElemData, 0, sizeof(testInfo.secondElemData));

                strncpy_s(testInfo.firstElemData, sizeof(testInfo.firstElemData), satellites[i].second.c_str(), _TRUNCATE);
                strncpy_s(testInfo.secondElemData, sizeof(testInfo.secondElemData), satellites[j].second.c_str(), _TRUNCATE);
                testInfo.format = SatelliteDataFormat::Text;

                memset(testInfo.recipe.testName, 0, 80);
                std::string testName = satellites[i].first + "_" + satellites[j].first;
                strncpy_s(testInfo.recipe.testName, testName.c_str(), 80);

                testInfo.firstElemData[sizeof(testInfo.firstElemData) - 1] = '\0';
                testInfo.secondElemData[sizeof(testInfo.secondElemData) - 1] = '\0';

                //std::this_thread::sleep_for(std::chrono::seconds(5));
                returnValue = CreateTest(testInfo);
                if (returnValue < 0) {
                    return returnValue;
                }
                while (!m_globalFlag) {}
            }
        }
        std::cout << "Number of valid satellite pairs (AllWithAll) that pass the filter: "
            << validPairs << std::endl;
    }
    returnValue = 1;
    return returnValue;
}

string Lab::StatusToString(TestStatus status) {
    switch (status) {
    case TestStatus::Queued:   return "Queued";
    case TestStatus::InProgress: return "InProgress";
    case TestStatus::Completed:  return "Completed";
    case TestStatus::Failed:  return "Failed";
    default:    return "Failed";
    }
};

int Lab::CreateTest(TestInfo testInfo) {
    int returnValue = -1; //return testID on success and -1 on failure
    std::string logString = "";
    TestRecipe& recipe = testInfo.recipe;
    recipe.elsetrec1 = { 0 };
    recipe.elsetrec2 = { 0 };
    testInfo.status = TestStatus::Queued;
    //Create unique pointer and don't free the memory - the test manager will delete it when needed
    TcaCalculation::sPointData* pointsData = nullptr;
    logString = "Creating Test";
    EventLogger::getInstance().log(logString, "Lab");
    if (true == m_dataGenerator.GenerateTestData(testInfo, &pointsData))
    {
        logString = "Test Data Generated";
        EventLogger::getInstance().log(logString, "Lab");

        std::cout << "Lab Create test after Data Generation: " << StatusToString(testInfo.status) << std::endl;

        if (true == m_databaseManager.createTest(testInfo))
        {
            m_testManager.PlaceTestInQueue(recipe, pointsData, recipe.numberOfPoints);
            std::cout << "Lab Create test after Place Test In Queue: " << StatusToString(testInfo.status) << std::endl;
            returnValue = testInfo.recipe.testID;
            logString = "Test Created And In Queue, Test ID " + std::to_string(testInfo.recipe.testID);
            EventLogger::getInstance().log(logString, "Lab");
        }
        else
        {
            logString = "Failed To Save Test To DB";
            EventLogger::getInstance().log(logString, "Lab");
            if (nullptr != pointsData)
            {
                delete[] pointsData;
            }
            returnValue = -1;
            std::cout << "Failed To Save Test To DB" << std::endl;
        }
    }
    else
    {
        logString = "Failed To Generate Test Data";
        EventLogger::getInstance().log(logString, "Lab");
        if (nullptr != pointsData)
        {
            delete[] pointsData;
        }
        returnValue = -2;
        std::cout << "Failed To Generate Test Data" << std::endl;
    }
    return returnValue;
}

std::set<int> Lab::getAllTestIds()
{
    return m_databaseManager.getAllTestIds();
}

Lab& Lab::operator=(const Lab&)
{
    return *this;
}

bool Lab::updateTestResults(const TestInfo test)
{
    return m_databaseManager.updateTestResults(test);
}

bool Lab::updateRealTca(const TestInfo test)
{
    return m_databaseManager.updateRealTca(test);
}

bool Lab::updateTestStatus(const TestStatus status, const unsigned int testId)
{
    return m_databaseManager.updateTestStatus(status, testId);
}

bool Lab::CheckConnection()
{
    return m_commManager.CheckConnection();
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cmath>
#include <stdexcept>

// Earth's gravitational constant in [km^3/s^2]
static constexpr double GM_EARTH = 398600.4418;

/**
 * This function extracts a substring from 'str' (starting at 'start', with length 'len')
 *        and converts it to a double using std::stod.
 */
inline double Lab::helpExtract(const std::string& str, size_t start, size_t len)
{
    std::string temp = str.substr(start, len);
    return std::stod(temp);
}

/**
 * This function extracts the semimajor axis (a) and eccentricity (e)
 *        from two lines of a TLE (line1 and line2).
 *
 * The function assumes that:
 *   - line2 (the second line of the TLE) contains eccentricity in columns ~26..33 as an integer without a decimal point.
 *     For example, "0012345" corresponds to 0.0012345.
 *   - The mean motion is typically in columns ~52..63 of line2 (about 11 characters), in units of revolutions per day.
 *   - We convert mean motion from [rev/day] to [rad/s], then use Kepler's 3rd law to compute 'a':
 *       a = cbrt( mu / n^2 ),
 *     where mu (GM_EARTH) is Earth's gravitational constant.
 *  Parameters:
 *   line1  The first line of the TLE (currently not used here, but provided for completeness).
 *   line2  The second line of the TLE, from which e and mean motion are extracted.
 *   outA   (output) semimajor axis 'a' in kilometers.
 *   outE   (output) eccentricity 'e' (dimensionless).
 * the function returns true if parsing was successful, false otherwise.
 */
bool Lab::parseOrbitParamsFromTLE(const std::string& line1,
    const std::string& line2,
    double& outA,  // in kilometers
    double& outE)  // dimensionless
{
    try {
        // Extract the eccentricity substring according to the columns.
        std::string eccPart = line2.substr(26, 7);
        std::string eccFull = "0." + eccPart;
        outE = std::stod(eccFull);

        // Extract mean motion (columns ~52..63), typically 11 chars.
        // Check indices based on actual TLE format in use.
        double meanMotionRevPerDay = helpExtract(line2, 52, 11);

        // Convert meanMotion from [rev/day] to [rad/s]:
        // n(rad/s) = meanMotionRevPerDay * (2 * pi / 86400)
        double n_rad_s = meanMotionRevPerDay * (2.0 * M_PI / 86400.0);

        // Compute semimajor axis via Kepler's 3rd law: a = cbrt(mu / n^2)
        double n2 = n_rad_s * n_rad_s;
        if (n2 < 1e-15) {
            // Prevents division by near-zero or ill-conditioned values
            return false;
        }
        outA = std::cbrt(GM_EARTH / n2);

        return true;
    }
    catch (...) {
        // If any parsing or conversion fails, return false
        return false;
    }
}

