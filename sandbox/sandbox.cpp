

#include "complex/Common/Types.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/Core/Filters/CreateDataArray.hpp"
#include "complex/Core/Filters/CreateDataGroup.hpp"
#include "complex/Core/Filters/TestFilter2.hpp"
#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/NumericTypeParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Pipeline/AbstractPipelineNode.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"

#include <hdf5.h>

#include <any>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

using namespace complex;

template <typename T>
DataArray<T>* ReadFromFile(const std::string& filename, const std::string& name, DataStructure* dataGraph, size_t numTuples, size_t numComponents, DataObject::IdType parentId)
{
  std::cout << "  Reading file " << filename << std::endl;
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;
  constexpr size_t defaultBlocksize = 1048576;

  if(!fs::exists(filename))
  {
    std::cout << "File Does Not Exist:'" << filename << "'" << std::endl;
    return nullptr;
  }

  DataStoreType* dataStore = new DataStoreType(numComponents, numTuples);
  ArrayType* dataArray = ArrayType::Create(*dataGraph, name, dataStore, parentId);

  const size_t fileSize = fs::file_size(filename);
  const size_t numBytesToRead = dataArray->getSize() * sizeof(T);
  if(numBytesToRead != fileSize)
  {
    std::cout << "FileSize and Allocated Size do not match" << std::endl;
    return nullptr;
  }

  FILE* f = std::fopen(filename.c_str(), "rb");
  if(f == nullptr)
  {
    return nullptr;
  }

  std::byte* chunkptr = reinterpret_cast<std::byte*>(dataStore->data());

  // Now start reading the data in chunks if needed.
  size_t chunkSize = std::min(numBytesToRead, defaultBlocksize);

  size_t masterCounter = 0;
  while(masterCounter < numBytesToRead)
  {
    size_t bytesRead = std::fread(chunkptr, sizeof(std::byte), chunkSize, f);
    chunkptr += bytesRead;
    masterCounter += bytesRead;

    size_t bytesLeft = numBytesToRead - masterCounter;

    if(bytesLeft < chunkSize)
    {
      chunkSize = bytesLeft;
    }
  }

  fclose(f);

  return dataArray;
}

std::shared_ptr<DataStructure> CreateDataStructure()
{
  std::shared_ptr<DataStructure> dataGraph = std::shared_ptr<DataStructure>(new DataStructure);

  dataGraph->makePath(DataPath::FromString("1/2/3/4/5").value());

  DataGroup* group = complex::DataGroup::Create(*dataGraph, "Small IN100");
  DataGroup* scanData = complex::DataGroup::Create(*dataGraph, "EBSD Scan Data", group->getId());

  dataGraph->makePath(DataPath::FromString("Small IN100/EBSD Scan Data/3/4/5").value());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(*dataGraph, "Small IN100 Grid", scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  std::cout << "Creating Data Structure" << std::endl;
  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  size_t tupleSize = 1;
  size_t tupleCount = imageGeom->getNumberOfElements();

  std::string filePath = "/Users/mjackson/Workspace1/complex/sandbox/test_data/";

  std::string fileName = "ConfidenceIndex.raw";
  Float32Array* ciData = ReadFromFile<float>(filePath + fileName, "Confidence Index", dataGraph.get(), tupleCount, tupleSize, scanData->getId());

  fileName = "FeatureIds.raw";
  Int32Array* featureIdsData = ReadFromFile<int32_t>(filePath + fileName, "FeatureIds", dataGraph.get(), tupleCount, tupleSize, scanData->getId());

  fileName = "ImageQuality.raw";
  Float32Array* iqData = ReadFromFile<float>(filePath + fileName, "Image Quality", dataGraph.get(), tupleCount, tupleSize, scanData->getId());

  fileName = "IPFColors.raw";
  UInt8Array* ipfColorData = ReadFromFile<uint8_t>(filePath + fileName, "IPF Colors", dataGraph.get(), tupleCount * 3, tupleSize, scanData->getId());

  fileName = "Phases.raw";
  Int32Array* phasesData = ReadFromFile<int32_t>(filePath + fileName, "Phases", dataGraph.get(), tupleCount, tupleSize, scanData->getId());

  // Add in another group that is just information about the grid data.
  DataGroup* phaseGroup = complex::DataGroup::Create(*dataGraph, "Phase Data", group->getId());
  tupleSize = 1;
  tupleCount = 2;
  Int32DataStore* laueDataStore = new Int32DataStore(tupleSize, tupleCount);
  Int32Array* laueData = Int32Array::Create(*dataGraph, "Laue Class", laueDataStore, phaseGroup->getId());

  return dataGraph;
}

int32_t main(int32_t argc, char** args)
{
  // Get an instance of a complex application
  Application app;
  FilterList* filterList = app.getFilterList();
  std::unordered_set<FilterHandle> filterHandles = filterList->getFilterHandles();
  std::cout << "----- Available Filters --------" << std::endl;
  for(const auto& filterHandle : filterHandles)
  {
    std::cout << filterHandle.getFilterName() << std::endl;
  }

  // Create a shared pointer to a DataStructure instance
  std::shared_ptr<DataStructure> dataGraph = CreateDataStructure();

  // Create a Pipeline
  Pipeline pipeline;
  DataPath outputDataPath = DataPath({"Small IN100", "EBSD Scan Data", "Fit"});

  // Create a vector to store our PipelineFilters for later use. The pipeline will clean up the pointers.
  std::vector<PipelineFilter*> filters;

  TestFilter2 tf2;   // Create a filter to go into the pipeline
  Arguments tf2Args; // Create the Arguments for the filter
  tf2Args.insert("param1", std::make_any<int32_t>(234234));
  tf2Args.insert("param2", std::make_any<std::string>("Hello World"));
  tf2Args.insert("param3", std::make_any<ChoicesParameter::ValueType>(1ULL));
  // Create a Pipeline Node that will store the filter instance
  PipelineFilter* tf2Node = PipelineFilter::Create({tf2.uuid(), Uuid()});
  tf2Node->setArguments(tf2Args); // Attach the arguments to the FilerNode
  pipeline.push_back(tf2Node);    // Insert the filter into the pipeline
  filters.push_back(tf2Node);

  CreateDataArray cda; // Create a filter to go into the pipeline
  Arguments cdaArgs;   // Create the Arguments for the filter
  cdaArgs.insert(CreateDataArray::k_NumericType_Key, std::any(complex::NumericType::float32));
  cdaArgs.insert(CreateDataArray::k_NumComps_Key, std::any(1ULL));
  cdaArgs.insert(CreateDataArray::k_NumTuples_Key, std::any(38900ULL));
  cdaArgs.insert(CreateDataArray::k_DataPath_Key, std::any(outputDataPath));
  // Create a Pipeline Node that will store the filter instance
  PipelineFilter* cdaNode = PipelineFilter::Create({cda.uuid(), Uuid()});
  cdaNode->setArguments(cdaArgs); // Attach the arguments to the FilerNode
  pipeline.push_back(cdaNode);    // Insert the filter into the pipeline
  filters.push_back(cdaNode);

  CreateDataGroup cdg; // Create a filter to go into the pipeline
  Arguments cgdArgs;   // Create the Arguments for the filter
  DataPath newDataPath = DataPath({"Small IN100", "Processed Data Group"});
  cgdArgs.insert(CreateDataGroup::k_DataObjectPath, std::any(newDataPath));
  // cgdArgs.insert("Bad Key", std::make_any<std::string>("EBSD Scan Data"));
  // Create a Pipeline Node that will store the filter instance
  PipelineFilter* cdgNode = PipelineFilter::Create({cdg.uuid(), Uuid()});
  cdgNode->setArguments(cgdArgs); // Attach the arguments to the FilerNode
  pipeline.push_back(cdgNode);    // Insert the filter into the pipeline
  filters.push_back(cdgNode);

  std::cout << "pipeline.size(): " << pipeline.size() << std::endl;

  // Preflight the pipeline
  bool passed = pipeline.preflight(*dataGraph);
  std::cout << "Preflight Result: " << (passed ? "true" : "false") << std::endl;
  // if(!passed)
  {
    for(const auto filter : filters)
    {
      auto warnings = filter->getWarnings();
      if(!warnings.empty())
      {
        std::cout << filter->getName() << " WARNINGS:" << std::endl;
      }
      for(const auto& warning : warnings)
      {
        std::cout << "  " << warning.message << std::endl;
      }

      auto errors = filter->getErrors();
      if(!errors.empty())
      {
        std::cout << filter->getName() << " ERRORS:" << std::endl;
      }
      for(const auto& error : errors)
      {
        std::cout << "  " << error.message << std::endl;
      }
    }
    return EXIT_FAILURE;
  }

  dataGraph = CreateDataStructure();
  passed = pipeline.execute(*dataGraph);
  std::cout << "Execute Result: " << static_cast<int32_t>(passed) << std::endl;

  DataObject* outputDataObject = dataGraph->getData(outputDataPath);
  std::cout << "Inserted DataObject: " << outputDataObject->getName() << " as a " << outputDataObject->getTypeName() << std::endl;

  {
    std::cout << "Writing DataStructure File ...  " << std::endl;
    auto fileId = H5Fcreate("Sandbox_test_file.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    dataGraph->writeHdf5(fileId);
    auto h5Error = H5Fclose(fileId);
  }

  {
    H5::ErrorType h5Err = 0;
    auto fileId = H5Fopen("Sandbox_test_file.h5", H5F_ACC_RDONLY, H5P_DEFAULT);

    DataStructure ds = DataStructure::ReadFromHdf5(fileId, h5Err);
    H5Fclose(fileId);
    std::cout << "Read Data Structure from HDF5 file: " << ds.size() << std::endl;
    for(const auto& thing : ds)
    {
      std::cout << thing.second->getName() << std::endl;
    }
  }

  return 0;
}
