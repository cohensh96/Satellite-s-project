# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [Released]
### version 1.0.7
#### Changed 25/04/2024
- Renamed functions
- Added Events Logger
### version 1.0.6
#### Changed 24/04/2024
- Fixed missing crc in Local Sim
### version 1.0.5
#### Changed 24/04/2024
- Added Parser to the CommManager
- Added Check for the CRC in incoming message and calculating the CRC for outgoing messages
- Added TCP and UDP(Win only) to the app configuration and factory
- Added IP and Ports to the configuration ini file and to the app configuration class
- Added comments
- Updated the CMakeLists
- Common:
- CommonStructures, TestRecipe,AlgorithmsEnums, TcaCalculations: Update basic types to cstdint types, to get consistent types on different systems
- CommonStructures, TestRecipe,AlgorithmsEnums, TcaCalculations: Added pragma pack around structs that used in messages, to get consistent struct sized on different systems
- Added SafeQueue class
- CommonStructures: Added CRC to the messages header
- Added CRC32 calculation static class to the Utilities
- Added ifdef around strncpy_s for different systems where the function is not supported
- SGP4: Changed basic types to cstdint types, added pragma pack

### version 1.0.4
#### Changed 11/04/2024
- New Common structure files
### version 1.0.3
#### Changed 09/04/2024
- Added Full catalog options - runnig with a constant number of points over a changing time interval
- Added Full catalog options - runnig over catch degrees - 7/15/31
- Added Tmin factor to the test recipe and results + data generations - Tmin factor is the number we divide the minimum rotation time by to get Gamma

### version 1.0.2
#### Changed 08/04/2024
-Added Unit Tests for SBO ANCAS
-Added Additional data to the test recipe and results: segment and interval size, number of points per segment and initial number of points
-Added Parametes INI file with default values and values for the full catalog test
-Added AppConfigurationManager and INI files
-Added Segment size to the SimpleDataGenerator
-Added the new values to the results logger
-Updated CMakeLists
-Added CommChannel creation in factory based on parameters

### version 1.0.1
#### Changed 21/03/2024
-Added variations to the full catalog test 
-Added another version for the Log function
-Removed the input from ANCAS Iteration function
-Added a variation for SboAncas that uses evenly space points -> required more points but should get more consistance results
#### Changed 20/03/2024
-Added check for dividing by zero to ANCAS - for the coefficient calculations
-Added check if a new minimum point was found for SboAncas, if no new minimum was found there is no point to continue looking for one
-Renamed LocalFileCommChannel to TestedOBCLocalSimulation
#### Changed 17/03/2024
- Added SBO ANCAS to the algorithms types enum
- Added everything SBO-ANCAS required to run to the TestRecipe structure
- Added SBO-ANCAS to the algorithm names in the ResultsLogger
- Added SGP4SinglePointGenerator implementing the interface
- Implemented SGP4SinglePointGenerator using the SGP4 implementation
- Added SGP4SinglePointGenerator and SboAncas to the factory
- Added SBO-ANCAS initialization and testing to the test manager

- Added constructor for the ResultsLogger with no input name, craete name based on the current date and time
- Fixed Bug in the CommChannel and CommManager
- Removed the Logger file name craetion from Main
- Added global variable for the infinite loop in the MainProccess, In Linux the global variable becomes false on signal to make sure we have clean exit
- Added shell scripts for windows + linux for the CMake compilation

#### Changed 16/03/2024
- Added results logger
- Added number of run(repeatedly calling the algorithm on the same input)
- Added Test Name and Test ID to the tets recipe and results
- Added MinRunTime and AvgRunTime to the test results
- Added Eigen Library again to the External libraries folder

### version 1.0.0
#### Changed 07/03/2024
- Reorganized the folders and files
- Renamed some files and structures

#### Added 01/03/2024
- Changed the PointData struct to include the time point and changed the algorithms implementation and interface accordingly, updated the FileReader class accordingly
- Added CommManager class to receive incoming data using a ICommChannel and parse it + sending the results back
- Implemented the MainProcces logic
- Changed types for the WinTime from double to long int
- Added ICommChannel interface for a communication channel
- Implemented a varation of ICommChannel for local testing - LocalFileCommChannelFacade: reading the data from a file and simulating a communication channel

#### Added 27/02/2024
- Changed the arrays in CATCH to static with max size - for better performance
- Added init function to catch and RootsFindAlg
- Added the number of points to the TCA struct(results struct) for SBO-ANCAS future implementation
- Changed the CompanionMatrixRootFinder to abstact class missing the findEigenvalues functions
- Added implementation for CompanionMatrixRootFinder using eigen library
- Added factory for creating object - CATCH or ANCAS 
- Added implementation of the CompanionMatrixRootsFinder using the armadillo library

#### Added 23/02/2024
- Added changelog
- Created the testedObcApp project
- Added main for windows
- Added the main process class
- Added timer interface and implementation for windows
- Added Versions file for the corrent version
- Added Test Manager class
- Added Defines for the required test parameters

