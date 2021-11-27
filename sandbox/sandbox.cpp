

#include "complex/Common/Types.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Plugin/AbstractPlugin.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"


#include "sandbox_test_dirs.h"

#include <fmt/format.h>

#include <hdf5.h>

#include <any>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>

#define CREATE_FILTER_HANDLE_CONSTANT(var_name, filter_uuid_string, plugin_uuid_string) \
const FilterHandle k_##var_name(Uuid::FromString(filter_uuid_string).value(), Uuid::FromString(plugin_uuid_string).value());


namespace fs = std::filesystem;


namespace complex
{

namespace ComplexCore
{
// Plugin Uuid
constexpr StringLiteral k_ComplexCore_Uuid = "05cc618b-781f-4ac0-b9ac-43f26ce1854f";
// Filter Uuids
constexpr StringLiteral k_ExampleFilter2_Uuid = "1307bbbc-112d-4aaa-941f-58253787b17e";
constexpr StringLiteral k_CreateDataArray_Uuid = "67041f9b-bdc6-4122-acc6-c9fe9280e90d";
constexpr StringLiteral k_ImportTextFilter_Uuid = "25f7df3e-ca3e-4634-adda-732c0e56efd4";

// Filter Handles
CREATE_FILTER_HANDLE_CONSTANT(ExampleFilter2Handle, k_ExampleFilter2_Uuid, k_ComplexCore_Uuid)
CREATE_FILTER_HANDLE_CONSTANT(CreateDataArrayHandle, k_CreateDataArray_Uuid, k_ComplexCore_Uuid)
CREATE_FILTER_HANDLE_CONSTANT(ImportTextFilterHandle, k_ImportTextFilter_Uuid, k_ComplexCore_Uuid)
}
}
using namespace complex;


template <typename T>
DataArray<T>* ReadFromFile(const std::string& filename, const std::string& name, DataStructure* dataGraph,
                           size_t numTuples, const std::vector<size_t>& numComponents, DataObject::IdType parentId = {})
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

  std::shared_ptr<DataStoreType> dataStore = std::shared_ptr<DataStoreType>( new DataStoreType({numTuples}, numComponents));
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

void ReadFileSystemIntoDataGraph()
{
  std::shared_ptr<DataStructure> dataGraph = std::shared_ptr<DataStructure>(new DataStructure);

  fs::path startingDir = fs::path(complex::unit_test::k_ComplexBinaryDir);
  fs::current_path(startingDir);

  LinkedPath currentDir;//  = dataGraph->makePath(DataPath::FromString(startingDir.filename().u8string()).value()).value();
  for(auto& p: fs::recursive_directory_iterator("."))
  {
    std::cout  << (int)(p.is_directory()) << p.path() << '\n';
    if(p.is_directory())
    {
      currentDir = dataGraph->makePath(DataPath::FromString(p.path().u8string()).value()).value();
    }
    else if (p.is_regular_file())
    {
      if(currentDir.isValid())
      {
        ReadFromFile<uint8_t>(p.path().u8string(), p.path().filename(), dataGraph.get(), p.file_size(), {1ULL}, currentDir.getId());
      }
      else
      {
        ReadFromFile<uint8_t>(p.path().u8string(), p.path().filename(), dataGraph.get(), p.file_size(), {1ULL});
      }
    }
  }

  fs::path filePath = fmt::format("{}/file_system_test.h5", complex::unit_test::k_ComplexBinaryDir);
  {
    std::cout << "Writing DataStructure File ...  " << std::endl;
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = dataGraph->writeHdf5(fileWriter);
  }
}

std::shared_ptr<DataStructure> CreateDataStructure()
{
  std::shared_ptr<DataStructure> dataGraph = std::shared_ptr<DataStructure>(new DataStructure);

  //dataGraph->makePath(DataPath::FromString("1/2/3/4/5").value());

  DataGroup* group = complex::DataGroup::Create(*dataGraph, "Small IN100");
  DataGroup* scanData = complex::DataGroup::Create(*dataGraph, "EBSD Scan Data", group->getId());

  //dataGraph->makePath(DataPath::FromString("Small IN100/EBSD Scan Data/3/4/5").value());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(*dataGraph, "Small IN100 Grid", scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  std::cout << "Creating Data Structure" << std::endl;
  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  std::vector<size_t> compDims = {1};
  size_t tupleCount = imageGeom->getNumberOfElements();

  std::string filePath =  complex::unit_test::k_ComplexSourceDir.str() + "/sandbox/test_data/";

  std::string fileName = "ConfidenceIndex.raw";
  ReadFromFile<float>(filePath + fileName, "Confidence Index", dataGraph.get(), tupleCount, compDims, scanData->getId());

  fileName = "FeatureIds.raw";
  ReadFromFile<int32_t>(filePath + fileName, "FeatureIds", dataGraph.get(), tupleCount, compDims, scanData->getId());

  fileName = "ImageQuality.raw";
  ReadFromFile<float>(filePath + fileName, "Image Quality", dataGraph.get(), tupleCount, compDims, scanData->getId());

  fileName = "Phases.raw";
  ReadFromFile<int32_t>(filePath + fileName, "Phases", dataGraph.get(), tupleCount, compDims, scanData->getId());

  fileName = "IPFColors.raw";
  compDims = {3};
  ReadFromFile<uint8_t>(filePath + fileName, "IPF Colors", dataGraph.get(), tupleCount, compDims, scanData->getId());



  // Add in another group that is just information about the grid data.
  DataGroup* phaseGroup = complex::DataGroup::Create(*dataGraph, "Phase Data", group->getId());
  compDims = {1};
  tupleCount = 2;
  Int32Array::CreateWithStore<Int32DataStore>(*dataGraph, "Laue Class",{2}, {1}, phaseGroup->getId());

  return dataGraph;
}

using namespace complex;
namespace fs = std::filesystem;

#if 1
void PrintFiltersPerPlugin()
{
  // Get an instance of a complex application
  Application* app = Application::Instance();

  std::unordered_set<AbstractPlugin*> plugins = app->getPluginList();
  for(const auto& plugin : plugins)
  {
//    if(plugin->getName() != "ComplexCore" || plugin->getName() != "TestOne" || plugin->getName() != "TestTwo")
//    {
//      continue;
//    }
    std::cout << "#---------------------------------------------------------------------------" << std::endl;

    std::cout << "Plugin: " << plugin->getName() << "  \"" << plugin->getId().str() << "\"  Filter Count: " << plugin->getFilterCount() << std::endl;
    std::cout << "property var p_" << plugin->getName()  << "_Uuid: \"" << plugin->getId().str() << "\"" << std::endl;

    FilterList::FilterContainerType filterHandles =  plugin->getFilterHandles();
    for(const auto& filterHandle : filterHandles)
    {
      // property var aString: "Hello world!"
      std::cout << "const FilterHandle k_" << filterHandle.getClassName()  << "Handle(Uuid::FromString(\"" << filterHandle.getFilterId().str() <<
          "\").value(), Uuid::FromString(\"" << plugin->getId().str() << "\").value());" << std::endl;
//      PipelineFilter* node = PipelineFilter::Create(filterHandle);
//      Parameters parameters = node->getParameters();
//      for(const auto& parameter : parameters)
//      {
//        std::cout << "        " << parameter.first <<  std::endl;
//      }
    }
  }
}
#endif

void PrintAllFilters()
{
  std::cout << "#---------------------------------------------------------------------------" << std::endl;
  Application* app = Application::Instance();
  FilterList* filterList = app->getFilterList();
  std::cout << "Filter Count: "  << filterList->getFilterHandles().size() << std::endl;

  std::unordered_set<FilterHandle> filterHandles = filterList->getFilterHandles();

  std::cout << "----- Available Filters --------" << std::endl;
  for(const auto& filterHandle : filterHandles)
  {
    FilterHandle::PluginIdType pluginId = filterHandle.getPluginId();
    AbstractPlugin* pluginPtr = filterList->getPluginById(pluginId);
    if(nullptr != pluginPtr)
    {
      std::cout << pluginPtr->getName() << "\t" << filterHandle.getFilterName() << std::endl;
    }
    else
    {
      std::cout << "Core\t" << filterHandle.getFilterName() << std::endl;
    }
  }
}

int32_t main(int32_t argc, char** args)
{
//  ReadFileSystemIntoDataGraph();
//if(true) return 1;
  // Get an instance of a complex application
  Application app;
  // Load the plugins
  fs::path pluginPath = fmt::format("{}/{}", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  std::cout << "pluginPath: " << pluginPath << std::endl;
  app.loadPlugins(pluginPath, true);

  // Get a list of all the filters
  FilterList* allFilters = app.getFilterList();

  PrintFiltersPerPlugin();
  //PrintAllFilters();

  // Create a shared pointer to a DataStructure instance
  std::shared_ptr<DataStructure> dataGraph = CreateDataStructure();

  // Create a Pipeline
  Pipeline pipeline;
  DataPath outputDataPath = DataPath({"Small IN100", "EBSD Scan Data", "Fit"});

  // Create a vector to store our PipelineFilters for later use. The pipeline will clean up the pointers.
  std::vector<PipelineFilter*> filters;


  // Create a Pipeline Node that will store the filter instance
  std::unique_ptr<PipelineFilter> tf2Node = PipelineFilter::Create(complex::ComplexCore::k_ExampleFilter2Handle);
  Arguments tf2Args; // Create the Arguments for the filter
  tf2Args.insert("param1", std::make_any<int32_t>(234234));
  tf2Args.insert("param2", std::make_any<std::string>("Hello World"));
  tf2Args.insert("param3", std::make_any<ChoicesParameter::ValueType>(1ULL));
  tf2Node->setArguments(tf2Args); // Attach the arguments to the FilerNode
  pipeline.push_back(std::move(tf2Node));    // Insert the filter into the pipeline
  filters.push_back(tf2Node.get());


  // Create a Pipeline Node that will store the filter instance
  std::unique_ptr<PipelineFilter> cdaNode = PipelineFilter::Create(complex::ComplexCore::k_CreateDataArrayHandle);
  Arguments cdaArgs;   // Create the Arguments for the filter
  cdaArgs.insert("numeric_type", std::any(complex::NumericType::float32));
  cdaArgs.insert("component_count", std::any(1ULL));
  cdaArgs.insert("tuple_count", std::any(38900ULL));
  cdaArgs.insert("output_data_array", std::any(outputDataPath));
  cdaNode->setArguments(cdaArgs); // Attach the arguments to the FilerNode
  pipeline.push_back(std::move(cdaNode));    // Insert the filter into the pipeline
  filters.push_back(cdaNode.get());

#if 0
  CreateDataGroup cdg; // Create a filter to go into the pipeline
  Arguments cgdArgs;   // Create the Arguments for the filter
  DataPath newDataPath = DataPath({"Small IN100", "Processed Data Group"});
  cgdArgs.insert(CreateDataGroup::k_DataObjectPath.str(), std::any(newDataPath));
  // cgdArgs.insert("Bad Key", std::make_any<std::string>("EBSD Scan Data"));
  // Create a Pipeline Node that will store the filter instance
  PipelineFilter* cdgNode = PipelineFilter::Create({cdg.uuid(), Uuid()});
  cdgNode->setArguments(cgdArgs); // Attach the arguments to the FilerNode
  pipeline.push_back(cdgNode);    // Insert the filter into the pipeline
  filters.push_back(cdgNode);
#endif

  std::cout << "pipeline.size(): " << pipeline.size() << std::endl;

  // Preflight the pipeline
  bool passed = pipeline.preflight(*dataGraph);
  std::cout << "Preflight Result: " << (passed ? "true" : "false") << std::endl;
  if(!passed)
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
   // return EXIT_FAILURE;
  }

  dataGraph = CreateDataStructure();
  passed = pipeline.execute(*dataGraph);
  std::cout << "Execute Result: " << static_cast<int32_t>(passed) << std::endl;

  DataObject* outputDataObject = dataGraph->getData(outputDataPath);
  std::cout << "Inserted DataObject: " << outputDataObject->getName() << " as a " << outputDataObject->getTypeName() << std::endl;

  fs::path filePath = fmt::format("{}/image_geometry_io.h5", complex::unit_test::k_ComplexBinaryDir);
  {
    std::cout << "Writing DataStructure File ...  " << std::endl;
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = dataGraph->writeHdf5(fileWriter);
  }

  {
    auto fileReader = H5::FileReader(filePath);
    herr_t err;
    auto ds = DataStructure::readFromHdf5(fileReader, err);
    std::cout << "Read Data Structure from HDF5 file: " << ds.getSize() << std::endl;
    for(const auto& thing : ds)
    {
      std::cout << thing.second->getName() << std::endl;
    }
  }

  return 0;
}
