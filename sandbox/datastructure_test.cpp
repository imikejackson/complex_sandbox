#include <cstdint>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/UnitTestCommon.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "sandbox_test_dirs.h"


using DataObjectShPtr = std::shared_ptr<DataObject>;

int main(int32_t argc, char**argv)
{
  // Create our Visulization DataStructure that will hold everything that we are visualizing
  DataStructure vtkDataStructure;
  DataPath visDataPath({"Visualization Nodes"});
  vtkDataStructure.makePath(visDataPath);
  DataObject::ParentCollectionType parents;
  DataObjectShPtr imageGeom = nullptr;
  {
    // Now fake up some data on an image geometry
    DataStructure complexDataStructure = complex::UnitTest::CreateDataStructure();
    DataPath imageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
    imageGeom = complexDataStructure.getSharedData(imageGeomPath);

    parents = imageGeom->getParentIds();
    std::cout << "Parent Count: " << parents.size() << std::endl;

    vtkDataStructure.insert(imageGeom, visDataPath);
    parents = imageGeom->getParentIds();
    std::cout << "Parent Count: " << parents.size() << std::endl;
    std::cout << "complex Data structure going out of scope" << std::endl;
    std::cout << "ImageGeom.use_count(): " << imageGeom.use_count() << std::endl;
  }
  parents = imageGeom->getParentIds();
  std::cout << "Parent Count: " << parents.size() << std::endl;
  std::cout << "ImageGeom.use_count(): " << imageGeom.use_count() << std::endl;

  // Write out the DataStructure for later viewing/debugging
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/datastructure_test.dream3d", complex::unit_test::k_ComplexBinaryDir));
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = vtkDataStructure.writeHdf5(fileWriter);

  return 0;
}
