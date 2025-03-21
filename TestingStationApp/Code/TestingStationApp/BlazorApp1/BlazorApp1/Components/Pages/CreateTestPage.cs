﻿using Microsoft.AspNetCore.Components;
using Microsoft.AspNetCore.Components.Forms;
using System.IO;

namespace BlazorApp1.Components.Pages
{

    public partial class CreateTestPage : ComponentBase
    {

        public IBrowserFile elemenetFile1;
        public IBrowserFile elemenetFile2;
        private bool submissionSuccess = false;
        private bool isInProcess = false;
        private string submissionMessage = "";

        private TestDataWrapper userTestDataWrapper = new TestDataWrapper()
        {
            testName = "",
            orbitingElementData1 = "",
            orbitingElementData2 = "",
            TOLdKM = "",
            TOLtSec = "",
        };

        private IntPtr labPtr;

        protected override void OnInitialized()
        {
            labPtr = LabInterop.Lab_Create();
            userTestDataWrapper.numberOfIterations = 1;
            userTestDataWrapper.numberOfPointsPerSegment = 16;
            userTestDataWrapper.TminFactor = 2;
            userTestDataWrapper.timeIntervalSizeSec = 1209600;
            userTestDataWrapper.TOLdKM = "0.00000001";
            userTestDataWrapper.TOLtSec = "0.0004";
            userTestDataWrapper.catalogTestFormat = CatalogTestFormat.OneWithAll;
        }

        private void UpdateTestName()
        {
            if (userTestDataWrapper.format == SatelliteDataFormat.Catalog)
            {
                userTestDataWrapper.testName = "Catalog Test";
            }
        }

        private void ClearSubmissionMessage()
        {
            submissionMessage = "";
            StateHasChanged(); // Updates the UI instantly
        }


        private async Task HandleValidSubmit()
        {
            UserTestData testData = CreateStructFromWrapper(userTestDataWrapper);

            if ((testData.format == SatelliteDataFormat.XML || testData.format == SatelliteDataFormat.Catalog)
                && (elemenetFile1 == null || elemenetFile2 == null))
            {
                submissionSuccess = false;
                submissionMessage = "Error: Missing input file.";
                return;
            }

            if (testData.TOLdKM.Equals(0) || testData.TOLtSec.Equals(0))
            {
                submissionSuccess = false;
                submissionMessage = "Error: invalid tolerance value";
                return;
            }

            // Test creation path from a catalog
            if (testData.format == SatelliteDataFormat.Catalog)
            {
                isInProcess = true;
                StateHasChanged(); // Instant UI refresh (spinner + button lock)

                int result;
                try
                {
                    // Running the long operation in Task.Run so as not to block the UI Thread
                    // Task.Run and run it directly, and the process will still be async)
                    result = await Task.Run(() => LabInterop.Lab_CreateTestsFromCatalog(labPtr, testData));
                }
                catch (Exception ex)
                {
                    // In case of an internal exception
                    isInProcess = false;
                    submissionSuccess = false;
                    submissionMessage = $"Unexpected error: {ex.Message}";
                    StateHasChanged();
                    return;
                }

                switch (result)
                {
                    case >= 0:
                        submissionSuccess = true;
                        submissionMessage = "Tests List created successfully!";
                        break;
                    case -3:
                        submissionSuccess = false;
                        submissionMessage = "Failed to open catalog file";
                        break;
                    case -4:
                        submissionSuccess = false;
                        submissionMessage = "The Catalog Size is too small";
                        break;
                    case -5:
                        submissionSuccess = false;
                        submissionMessage = "The Catalog Path Doesn't Exist";
                        break;
                    case -1:
                        submissionSuccess = false;
                        submissionMessage = "Error: Failed to save one of the tests to the database.";
                        break;
                    case -2:
                        submissionSuccess = false;
                        submissionMessage = "Error: Data generation failed for one of the tests.";
                        break;
                    default:
                        submissionSuccess = false;
                        submissionMessage = "Error: Failed to create the tests from catalog";
                        break;
                }

                isInProcess = false;
                StateHasChanged(); // Final refresh at the end of the process
            }
            else
            {
                // Single test creation path (XML/Text)
                int result = LabInterop.Lab_CreateTest(labPtr, testData);
                switch (result)
                {
                    case >= 0:
                        submissionSuccess = true;
                        submissionMessage = $"Test created successfully! Test ID: {result}";
                        break;
                    case -1:
                        submissionSuccess = false;
                        submissionMessage = "Error: Failed to save the test to the database.";
                        break;
                    case -2:
                        submissionSuccess = false;
                        submissionMessage = "Error: Data generation failed, try different input.";
                        break;
                    default:
                        submissionSuccess = false;
                        submissionMessage = "Error: Failed to create the test";
                        break;
                }
            }
        }


        async Task HandleFileSelection(InputFileChangeEventArgs e, int orbitingElementIndex)
        {
            var file = e.File;
            if (file != null)
            {
                string? currentDirectory = Environment.CurrentDirectory;
                string savePath = Path.Combine(currentDirectory, $"OmmXmls/{file.Name}");

                if (orbitingElementIndex == 1)
                {
                    elemenetFile1 = file;
                    await SaveFile(elemenetFile1, savePath);
                    userTestDataWrapper.orbitingElementData1 = file.Name;
                }
                else if (orbitingElementIndex == 2)
                {
                    elemenetFile2 = file;
                    await SaveFile(elemenetFile2, savePath);
                    userTestDataWrapper.orbitingElementData2 = file.Name;
                }
            }
        }

        async Task HandleFullCatalogFileSelection(InputFileChangeEventArgs e)
        {
            var file = e.File;
            if (file != null)
            {
                string currentDirectory = Environment.CurrentDirectory;
                string savePath = Path.Combine(currentDirectory, $"FullCatalogs\\{file.Name}");
                elemenetFile1 = file;
                await SaveFile(elemenetFile1, savePath);
                elemenetFile2 = file;
                await SaveFile(elemenetFile2, savePath);

                userTestDataWrapper.orbitingElementData1 = file.Name;
                userTestDataWrapper.orbitingElementData2 = file.Name;

                Console.WriteLine($"Catalog file saved at: {savePath}");
            }
        }


        private UserTestData CreateStructFromWrapper(TestDataWrapper userTestDataWrapper)
        {
            UserTestData userTestData = new UserTestData();
            userTestData.testName = userTestDataWrapper.testName;
            userTestData.numberOfPointsPerSegment = userTestDataWrapper.numberOfPointsPerSegment;
            userTestData.catchPolynomialDegree = userTestDataWrapper.numberOfPointsPerSegment - 1;
            userTestData.catchRootsAlg = userTestDataWrapper.catchRootsAlg;
            userTestData.testedAlgorithm = userTestDataWrapper.testedAlgorithm;
            userTestData.numberOfIterations = (uint)userTestDataWrapper.numberOfIterations;
            userTestData.TminFactor = userTestDataWrapper.TminFactor;
            userTestData.timeIntervalSizeSec = userTestDataWrapper.timeIntervalSizeSec;
            //userTestData.TOLdKM = double.Parse(userTestDataWrapper.TOLdKM);
            //userTestData.TOLtSec = double.Parse(userTestDataWrapper.TOLtSec);
            if (double.TryParse(userTestDataWrapper.TOLdKM, out double tolDkmResult))
            {
                userTestData.TOLdKM = tolDkmResult;
            }
            else
            {
                userTestDataWrapper.TOLdKM = "0";
                userTestData.TOLdKM = 0;
            }

            if (double.TryParse(userTestDataWrapper.TOLtSec, out double tolTSecResult))
            {
                userTestData.TOLtSec = tolTSecResult;
            }
            else
            {
                userTestDataWrapper.TOLtSec = "0";
                userTestData.TOLtSec = 0;
            }
            userTestData.format = userTestDataWrapper.format;

            userTestData.orbitingElementData1 = userTestDataWrapper.orbitingElementData1;
            userTestData.orbitingElementData2 = userTestDataWrapper.orbitingElementData2;

            return userTestData;

        }

        public async Task<string> SaveFile(IBrowserFile file, string savePath)
        {
            try
            {
                if (file != null && file.Size > 0)
                {

                    if (File.Exists(savePath))
                    {
                        File.Delete(savePath);
                    }

                    Directory.CreateDirectory(Path.GetDirectoryName(savePath));

                    using (var fileStream = new FileStream(savePath, FileMode.Create, FileAccess.Write, FileShare.None))
                    {
                        await file.OpenReadStream().CopyToAsync(fileStream);
                        await fileStream.FlushAsync();
                    }
                }
                else
                {
                    throw new ArgumentException("File is null or empty");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error saving file: {ex.Message}");
                submissionSuccess = false;
                submissionMessage = $"Unexpected error: {ex.Message}";
                StateHasChanged();
  
            }

            return savePath;
        }

    }

}



