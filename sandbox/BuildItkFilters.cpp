/**
 * This program needs the JSON files from SimpleITK which can be found at:
 *
 * git clone ssh://git@github.com/SimpleITK/SimpleITK
 *
 * The program will run through a specific set of the filters (Basic Filters) and
 * generate both a complex filter and a matching unit test.
 */

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

#include <nlohmann/json.hpp>


#include "ItkFilterList.h"

namespace fs = std::filesystem;

static const fs::path k_SimpleItkJsonDir = "/Users/mjackson/Workspace1/SimpleITK/Code/BasicFilters/json/";

#if 0
static const fs::path k_ItkPluginOutputDir = "/tmp/ITKImageProcessing";
static const fs::path k_GeneratedFiltersOutputDir = "/tmp/ITKImageProcessing/src/ITKImageProcessing/Filters";
#else
static const fs::path k_ItkPluginOutputDir =        "/Users/mjackson/Workspace1/complex_plugins/ITKImageProcessing/";
static const fs::path k_GeneratedFiltersOutputDir = "/Users/mjackson/Workspace1/complex_plugins/ITKImageProcessing/src/ITKImageProcessing/Filters";

#endif
static const fs::path k_ItkFilterHeaderTemplatePath = "/Users/mjackson/Workspace1/complex_sandbox/sandbox/ItkFilterTemplate.hpp.in";
static const fs::path k_ItkFilterSourceTemplatePath = "/Users/mjackson/Workspace1/complex_sandbox/sandbox/ItkFilterTemplate.cpp.in";

static const std::string k_ITKIMAGEPROCESSING = "ITKIMAGEPROCESSING";
static const std::string k_ITKImageProcessing = "ITKImageProcessing";

const std::string k_PLUGIN_NAME("@PLUGIN_NAME@");
const std::string k_PLUGIN_DESCRIPTION("@PLUGIN_DESCRIPTION@");
const std::string k_PLUGIN_VENDOR("@PLUGIN_VENDOR@");
const std::string k_FILTER_NAME("@FILTER_NAME@");
const std::string k_PLUGIN_NAME_UPPER("@PLUGIN_NAME_UPPER@");
const std::string k_UUID("@UUID@");
const std::string k_PARAMETER_KEYS("@PARAMETER_KEYS@");
const std::string k_PARAMETER_DEFS("@PARAMETER_DEFS@");
const std::string k_FILTER_HUMAN_NAME("@FILTER_HUMAN_NAME@");
const std::string k_PARAMETER_INCLUDES("@PARAMETER_INCLUDES@");
const std::string k_PREFLIGHT_DEFS("@PREFLIGHT_DEFS@");
const std::string k_DEFAULT_TAGS("@DEFAULT_TAGS@");
const std::string k_PREFLIGHT_UPDATED_VALUES("@PREFLIGHT_UPDATED_VALUES@");
const std::string k_PREFLIGHT_UPDATED_DEFS("@PREFLIGHT_UPDATED_DEFS@");
const std::string k_PROPOSED_ACTIONS("@PROPOSED_ACTIONS@");
const std::string k_ITK_FILTER_STRUCT("@ITK_FILTER_STRUCT@");
const std::string k_ITK_FILTER_INPUTS("@ITK_FILTER_INPUTS@");
const std::string k_ITK_FUNCTOR_DECL("@ITK_FUNCTOR_DECL@");
const std::string k_ITK_FUNCTOR_PROPS("@ITK_FUNCTOR_PROPS@");
const std::string k_ITK_ARRAY_HELPERS_DEFINES("@ITK_ARRAY_HELPERS_DEFINES@");
const std::string k_BRIEF_DESCRIPTION("@BRIEF_DESCRIPTION@");
const std::string k_DETAILED_DESCRIPTION("@DETAILED_DESCRIPTION@");
const std::string k_ITK_MODULE("@ITK_MODULE@");
const std::string k_ITK_GROUP("@ITK_GROUP@");
const std::string k_DATA_CHECK_DECL("@DATA_CHECK_DECL@");
const std::string k_EXECUTE_DECL("@EXECUTE_DECL@");
const std::string k_LINK_OUTPUT_ARRAY("@LINK_OUTPUT_ARRAY@");


static std::set<std::string> s_AllPixelTypes;

/**
 *
 * @param filePathStr
 * @return
 */
nlohmann::json ReadJsonFile( const fs::path& filePathStr)
{
  try
  {
    std::ifstream inputFile(filePathStr.c_str(), std::ios::in);
    if(!inputFile.is_open())
    {
      std::cout << "Error opening json file " << filePathStr.c_str() << std::endl;
      return nullptr;
    }
    nlohmann::json pipelineJson = nlohmann::json::parse(inputFile);
    return pipelineJson;
  } catch(std::exception& exception)
  {
    std::cout << exception.what() << std::endl;
  }
  return nullptr;
}

void CreateOutputDirectories()
{
  fs::create_directories(k_ItkPluginOutputDir / "src"/ "ITKImageProcessing" / "Filters");
  fs::create_directories(k_ItkPluginOutputDir / "src"/ "ITKImageProcessing" / "Filters-Disabled");
}

int32_t WriteFile(const fs::path& filePath, const std::string& contents)
{
  std::ofstream outFile(filePath.c_str(), std::ios::out | std::ios::binary);
  outFile << contents;

  return 0;
}

std::string ReadFile(const fs::path& filePath)
{
  std::ifstream t(filePath.c_str(), std::ios::in);
  std::string contents;

  t.seekg(0, std::ios::end);
  contents.reserve(t.tellg());
  t.seekg(0, std::ios::beg);

  contents.assign((std::istreambuf_iterator<char>(t)),
             std::istreambuf_iterator<char>());
  return std::move(contents);
}

std::string ReplaceKeywords(std::string& contents, const std::string& searchWord, const std::string& replaceWord)
{
  std::string::size_type startPos = 0;
  while ( (startPos = contents.find(searchWord, startPos)) != std::string::npos)
  {
    contents.replace(contents.find(searchWord, startPos), searchWord.size(), replaceWord);
    startPos = startPos + replaceWord.size();
  }
  return contents;
}

/**
 *
 * @param rootJson
 */
void CreateFilterHeader(const nlohmann::json& rootJson)
{
  // Read the template file
  std::string templateContents = ReadFile(k_ItkFilterHeaderTemplatePath);

  std::string filterName = "ITK" + rootJson["name"].get<std::string>();
  filterName = ReplaceKeywords(filterName, "Filter", "");
  std::string outputFileName =  filterName + ".hpp";

  // Do the easy replacements..
  templateContents = ReplaceKeywords(templateContents, k_PLUGIN_NAME_UPPER, k_ITKIMAGEPROCESSING);
  templateContents = ReplaceKeywords(templateContents, k_PLUGIN_NAME, k_ITKImageProcessing);
  templateContents = ReplaceKeywords(templateContents, k_FILTER_NAME, filterName);
  templateContents = ReplaceKeywords(templateContents, k_UUID, s_UuidMap[filterName]);

  // Pull out the descriptions
  std::string briefDesc = rootJson["briefdescription"].get<std::string>();
  std::string detailDesc = rootJson["detaileddescription"].get<std::string>();
  std::string itkModule = rootJson["itk_module"].get<std::string>();
  std::string itkGroup = rootJson["itk_group"].get<std::string>();

  detailDesc = ReplaceKeywords(detailDesc, "\\see", "@see");
  detailDesc = ReplaceKeywords(detailDesc, "\\author", "@author");
  detailDesc = ReplaceKeywords(detailDesc, "\\li", "@li");
  detailDesc = ReplaceKeywords(detailDesc, "\n", "\n * ");


  templateContents = ReplaceKeywords(templateContents, k_BRIEF_DESCRIPTION, briefDesc);
  templateContents = ReplaceKeywords(templateContents, k_DETAILED_DESCRIPTION, detailDesc);
  templateContents = ReplaceKeywords(templateContents, k_ITK_MODULE, itkModule);
  templateContents = ReplaceKeywords(templateContents, k_ITK_GROUP, itkGroup);


  // Now we need to loop through the parameters
  nlohmann::json membersJson = rootJson["members"];
  std::stringstream propertiesKeys;

  for (auto& jsonIdx : membersJson.items()) {
    //std::cout << jsonIdx.key() << " : " << jsonIdx.value() << "\n";
    nlohmann::json memberJson = jsonIdx.value();
    std::string propName = memberJson["name"].get<std::string>();
    propertiesKeys << "  static inline constexpr StringLiteral k_" << propName << "_Key = \"" << propName << "\";\n";
  }
  // See if there are multiple inputs to this filter
  if(rootJson.find("inputs") != rootJson.end())
  {
    nlohmann::json inputs = rootJson["inputs"];
    if(inputs.is_array())
    {
      for(auto& jsonIdx : inputs.items())
      {
        nlohmann::json input = jsonIdx.value();
        std::string inputImageName = input["name"].get<std::string>();
        std::string inputImageType = input["type"].get<std::string>();
        if(inputImageName != "Image")
        {
          propertiesKeys << "  static inline constexpr StringLiteral k_" << inputImageName << "DataPath_Key = \"" << inputImageName << "DataPath\";\n";
        }
      }
    }
  }
  templateContents = ReplaceKeywords(templateContents, k_PARAMETER_KEYS, propertiesKeys.str());

  fs::path outputFilePath = k_GeneratedFiltersOutputDir / outputFileName;
  WriteFile(outputFilePath, templateContents);
}
/**
 * @brief
 * @param name
 * @param pType
 * @param defaultValue
 * @param parameterDefs
 */
void DetermineParameterClass(const std::string& name, const std::string& pType,
                             const nlohmann::json& defaultValue,
                             std::stringstream& parameterDefs,
                             std::stringstream& itkFunctorOut,
                             std::stringstream& itkFunctorBodyOut,
                             std::stringstream& itkFunctorDeclOut,
                             std::stringstream& preflightDefs,
                             std::stringstream& includeOut
                             )
{

  if(name == "KernelRadius")
  {
    includeOut << "#include \"complex/Parameters/VectorParameter.hpp\"\n";
    parameterDefs << "  params.insert(std::make_unique<VectorUInt32Parameter>(k_KernelRadius_Key, \"KernelRadius\", \"\", std::vector<uint32_t>(3), std::vector<std::string>(3)));\n";
    itkFunctorOut << "  std::vector<uint32_t> p" << name << " = {1, 1, 1};\n";
    itkFunctorBodyOut << "    auto kernel = itk::simple::CreateKernel<Dimension>( static_cast<itk::simple::KernelEnum>(pKernelType), pKernelRadius);\n";
    itkFunctorBodyOut << "    filter->SetKernel(kernel);\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<VectorUInt32Parameter::ValueType>(k_" << name << "_Key);\n";
  }
  else if (name == "KernelType")
  {
    includeOut << "#include \"complex/Parameters/ChoicesParameter.hpp\"\n";
    parameterDefs << "  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, \"Kernel Type\", \"\", " << defaultValue.get<std::string>() << ", ChoicesParameter::Choices{\"Annulus\", \"Ball\", \"Box\", \"Cross\"}));\n";
    itkFunctorOut << "  itk::simple::KernelEnum p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = static_cast<itk::simple::"<<pType<<">(filterArgs.value<uint64>(k_" << name << "_Key));\n";
  }
  else if(pType == "double")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<float64>(k_" << name << "_Key);\n";

    if(defaultValue.is_number_float())
    {
      parameterDefs << "  params.insert(std::make_unique<Float64Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<float>()<<  "));\n";
      itkFunctorOut << "  float64 p" << name << " = " << defaultValue.get<float>() << ";\n";
    }
    else if (defaultValue.is_string())
    {
      std::string value = defaultValue.get<std::string>();
      if(value.find("std::vector<") == std::string::npos)
      {
        parameterDefs << "  params.insert(std::make_unique<Float64Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
        std::string defValue = defaultValue.get<std::string>();
        //defValue = ReplaceKeywords(defValue, "u", ".0");
        itkFunctorOut << "  float64 p" << name << " = " << defValue << ";\n";
      }
      else
      {
        includeOut << "#include \"complex/Parameters/VectorParameter.hpp\"\n";
        parameterDefs << "  params.insert(std::make_unique<VectorFloat64Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << ", std::vector<std::string>(3)));\n";
        itkFunctorOut << "  " << pType << " p" << name << " = 0.0;\n";
      }
    }
    else if (defaultValue.is_number_integer())
    {
      parameterDefs << "  params.insert(std::make_unique<Float64Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<int>()<<  "));\n";
      itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<int>() << ";\n";
    }
    else
    {
      parameterDefs << "#error Pararameter '" << name << "' default value could not be determined from the JSON";
      itkFunctorOut << "#error Pararameter '" << name << "' default value could not be determined from the JSON";
    }

  }
  else if(pType == "bool")
  {
    includeOut << "#include \"complex/Parameters/BoolParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  params.insert(std::make_unique<BoolParameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
    itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
  }
  else if(pType == "unsigned int")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    if (defaultValue.is_string())
    {
      std::string defValue = defaultValue.get<std::string>();
      if(defValue.find("std::vector<") != std::string::npos)
      {
        parameterDefs << "  params.insert(std::make_unique<VectorUInt32Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << ", std::vector<std::string>(3)));\n";
        includeOut << "#include \"complex/Parameters/VectorParameter.hpp\"\n";
        itkFunctorOut << "  std::vector<" << pType << "> p" << name << " = " << defaultValue.get<std::string>() << ";\n";
        preflightDefs << "  auto p" << name << " = filterArgs.value<VectorUInt32Parameter::ValueType>(k_" << name << "_Key);\n";
        itkFunctorBodyOut << "    filter->Set" << name << "(itk::simple::CastToRadiusType<FilterType, std::vector<uint32_t>>(p" << name << "));\n";
      }
      else
      {
        parameterDefs << "  params.insert(std::make_unique<UInt32Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
        preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
        itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
        itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
      }
    }
  }
  else if(pType == "uint8_t")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  params.insert(std::make_unique<UInt8Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
    itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
  }
  else if(pType == "float")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  params.insert(std::make_unique<Float32Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
    itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
  }
  else if(pType == "uint32_t")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  params.insert(std::make_unique<UInt32Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
    itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
  }
  else if(pType == "int")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
    parameterDefs << "  params.insert(std::make_unique<Int32Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", ";
    if(defaultValue.is_number_integer())
    {
      parameterDefs << defaultValue.get<int32_t>();
      itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<int32_t>() << ";\n";
    }
    else if (defaultValue.is_string())
    {
      parameterDefs << defaultValue.get<std::string>();
      itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    }
    else
    {
      parameterDefs << "#error THE DEFAULT VALUE FROM THE JSON WAS NOT DETERMINED";
    }
    parameterDefs <<  "));\n";
  }
  else if(pType == "uint64_t")
  {
    includeOut << "#include \"complex/Parameters/NumberParameter.hpp\"\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  params.insert(std::make_unique<UInt64Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
    itkFunctorOut << "  " << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
  }
  else if(pType == "PixelIDValueEnum")
  {
    //itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  params.insert(std::make_unique<UInt32Parameter>(k_" << name << "_Key, \"" << name << "\", \"\", " << defaultValue.get<std::string>() << "));\n";
    itkFunctorOut << "  itk::simple::" << pType << " p" << name << " = " << defaultValue.get<std::string>() << ";\n";
    preflightDefs << "  auto p" << name << " = filterArgs.value<itk::simple::" << pType << ">(k_" << name << "_Key);\n";
  }
  else
  {
    includeOut << "  #error " << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";
    itkFunctorBodyOut << "    filter->Set" << name << "(p" << name << ");\n";
    parameterDefs << "  // " << name << ": " << pType << " = " << defaultValue << std::endl;
    itkFunctorOut << "  #error " << pType << " p" << name << ";\n";
    preflightDefs << "  #error " << name << " = filterArgs.value<" << pType << ">(k_" << name << "_Key);\n";

    std::cout << name << ": " << pType << " = " << defaultValue << std::endl;
  }
}

void GeneratePixelTypeDefines(const std::string& pixelTypes, const std::string& vectorPixelType, const std::string& outputPixelType, std::stringstream& pixelTypeDefines)
{
  pixelTypeDefines << "/**\n * This filter only works with certain kinds of data. We\n"
                   << " * enable the types that the filter will compile against. The \n"
                   << " * Allowed PixelTypes as defined in SimpleITK are: \n *   " << pixelTypes << "\n";

  if(!vectorPixelType.empty())
  {
    pixelTypeDefines << " * In addition the following VectorPixelTypes are allowed: \n *   " << vectorPixelType << "\n";
  }
  if(!outputPixelType.empty())
  {
    pixelTypeDefines << " * The filter defines the following output pixel types: \n *   " << outputPixelType << "\n";
  }
  pixelTypeDefines << " */\n";

#if 0
  if("VectorPixelIDTypeList" == vectorPixelType)
  {
    pixelTypeDefines << "#define ITK_VECTOR_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Vector 0\n";
  }
  else if("RealVectorPixelIDTypeList" == vectorPixelType)
  {
    pixelTypeDefines << "#define ITK_REAL_VECTOR_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Vector 0\n";
  }
  else if("SignedVectorPixelIDTypeList" == vectorPixelType)
  {
    pixelTypeDefines << "#define ITK_SIGNED_VECTOR_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Vector 0\n";
  }
#endif
  if("BasicPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_BASIC_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1\n";
  }
  else if("IntegerPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_INTEGER_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1\n";
  }
  else if("NonLabelPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_NON_LABEL_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1\n";
  }
  else if("RealPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_REAL_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1\n";
  }
  else if("RealVectorPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_REAL_VECTOR_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Vector 0\n";
  }
  else if("ScalarPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_SCALAR_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1\n";
  }
  else if("SignedPixelIDTypeList" == pixelTypes)
  {
    pixelTypeDefines << "#define ITK_SIGNED_PIXEL_ID_TYPE_LIST 1\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1\n";
  }
  else if("typelist::Append<BasicPixelIDTypeList, VectorPixelIDTypeList>::Type" == pixelTypes)
  {
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_int64 0\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_uint64 0\n";
    pixelTypeDefines << "#define COMPLEX_ITK_ARRAY_HELPER_USE_Vector 0\n";
  }
}

/**
 * @brief
 * @param rootJson
 */
void CreateFilterSource(const nlohmann::json& rootJson)
{
  // Read the template file
  std::string templateContents = ReadFile(k_ItkFilterSourceTemplatePath);
  std::string humanFilterName = "ITK::" + rootJson["name"].get<std::string>();
  std::string filterName = "ITK" + rootJson["name"].get<std::string>();
  filterName = ReplaceKeywords(filterName, "Filter", "");
  std::string itkClassName = rootJson["name"].get<std::string>();
  std::string pixelTypes = rootJson["pixel_types"].get<std::string>();
  std::string vectorPixelTypes;
  if(rootJson.find("vector_pixel_types_by_component") != rootJson.end())
  {
    vectorPixelTypes = rootJson["vector_pixel_types_by_component"].get<std::string>();
  }
  std::string outputPixelType;
  if(rootJson.find("output_pixel_type") != rootJson.end())
  {
    outputPixelType = rootJson["output_pixel_type"].get<std::string>();
  }
  s_AllPixelTypes.insert(pixelTypes);
  s_AllPixelTypes.insert(vectorPixelTypes);

  std::string outputFileName =  filterName + ".cpp";
  // Do the easy replacements..
  templateContents = ReplaceKeywords(templateContents, k_PLUGIN_NAME_UPPER, k_ITKIMAGEPROCESSING);
  templateContents = ReplaceKeywords(templateContents, k_PLUGIN_NAME, k_ITKImageProcessing);
  templateContents = ReplaceKeywords(templateContents, k_FILTER_NAME, filterName);
  templateContents = ReplaceKeywords(templateContents, k_FILTER_HUMAN_NAME, humanFilterName);
  // Create some default tags
  std::string itkModule = rootJson["itk_module"].get<std::string>();
  std::string itkGroup = rootJson["itk_group"].get<std::string>();

  std::stringstream defaultTags;
  defaultTags << "\"ITKImageProcessing\", \""<< filterName<< "\", \""<< itkModule << "\", \""<< itkGroup << "\"";
  templateContents = ReplaceKeywords(templateContents, k_DEFAULT_TAGS, defaultTags.str());

  // Now we need to loop through the parameters
  nlohmann::json membersJson = rootJson["members"];
  nlohmann::json measurementsJson;

  std::stringstream parameterDefs;
  std::stringstream preflightDefs;
  std::stringstream includeOut;
  std::stringstream itkFunctorOut;
  std::stringstream itkFunctorBodyOut;
  std::stringstream itkFunctorDeclOut;
  std::stringstream pixelTypeDefines;
  // Document the input types
  if(rootJson.find("inputs") != rootJson.end())
  {
    nlohmann::json inputs = rootJson["inputs"];
    if(inputs.is_array())
    {
      pixelTypeDefines << "/**\n * This filter has multiple Input images: \n";
      for(auto& jsonIdx : inputs.items())
      {
        nlohmann::json input = jsonIdx.value();
        std::string inputImageName = input["name"].get<std::string>();
        std::string inputImageType = input["type"].get<std::string>();
        pixelTypeDefines << " *    ";
        if(input.find("optional") != input.end() && input["optional"].get<bool>())
        {
          pixelTypeDefines << "[OPTIONAL] ";
        }
        pixelTypeDefines << inputImageName << " of type: " << inputImageType << "\n";

        if(inputImageName != "Image")
        {
          parameterDefs << "  params.insert(std::make_unique<ArraySelectionParameter>(k_" << inputImageName << "DataPath_Key, \"" << inputImageName << "\", \"\", DataPath{}));\n";
          preflightDefs << "  auto p" << inputImageName << " = filterArgs.value<DataPath>(k_"<<inputImageName<< "DataPath_Key);\n";
        }
      }
      pixelTypeDefines << " */\n";
    }
  }
  // Document any Measurements that are made during execution of the filter
  if(rootJson.find("measurements") != rootJson.end())
  {
    measurementsJson = rootJson["measurements"];
    pixelTypeDefines << "/**\n * This filter can report a number of measurements: \n";
    for(auto& jsonIdx : measurementsJson.items())
    {
      nlohmann::json measurementJson = jsonIdx.value();
      std::string name = measurementJson["name"].get<std::string>();
      std::string outType = measurementJson["type"].get<std::string>();
      std::string defaultValue = measurementJson["default"].get<std::string>();
      std::string description;
      if(measurementJson.find("detaileddescriptionGet") != measurementJson.end())
      {
        description = measurementJson["detaileddescriptionGet"].get<std::string>();
      }
      pixelTypeDefines << " * @name " << name << "\n"
                       << " * @type " << outType << "\n"
                       << " * @description " << description << "\n *\n";
    }
    pixelTypeDefines << " */\n";
  }

  GeneratePixelTypeDefines(pixelTypes, vectorPixelTypes, outputPixelType, pixelTypeDefines);
  std::string itkArrayHelperNamespace = rootJson["name"].get<std::string>();
  itkArrayHelperNamespace = ReplaceKeywords(itkArrayHelperNamespace, "Filter", "");
  pixelTypeDefines << "#define ITK_ARRAY_HELPER_NAMESPACE " << itkArrayHelperNamespace << "\n";

  if(rootJson.find("filter_type") != rootJson.end())
  {
    nlohmann::json itkFilterType = rootJson["filter_type"];
    if(!itkFilterType.empty())
    {
      itkFunctorBodyOut << "    using FilterType = " << itkFilterType.get<std::string>() << ";\n";
    }
  }
  else
  {
    itkFunctorBodyOut << "    using FilterType = itk::" <<itkClassName<<"<InputImageType, OutputImageType>;\n";
  }
  itkFunctorBodyOut << "    typename FilterType::Pointer filter = FilterType::New();\n";

  itkFunctorDeclOut << "  ::" << filterName << "CreationFunctor itkFunctor = {";
  std::string filterOutputTypePlaceHolder;
  std::string filterOutputTypeExeTemplateDef;
  std::stringstream dataCheckDeclOut;
  std::stringstream executeDeclOut;

  itkFunctorOut << "namespace\n{\n";
  if(!outputPixelType.empty())
  {
    filterOutputTypePlaceHolder = "<FilterOutputType>";
    filterOutputTypeExeTemplateDef = "<" + filterName + "CreationFunctor, FilterOutputType>";
    itkFunctorOut << "/**\n * This filter uses a fixed output type.\n */\n";
    if(outputPixelType == "typename itk::NumericTraits<typename InputImageType::PixelType>::RealType")
    {
      itkFunctorOut << "using FilterOutputType = float64;\n\n";
    }
    else
    {
      if(outputPixelType == "float")
      {
        outputPixelType = "float32";
      }
      itkFunctorOut << "using FilterOutputType = " << outputPixelType << ";\n\n";
    }
  }
  dataCheckDeclOut << "  complex::Result<OutputActions> resultOutputActions = " << itkArrayHelperNamespace  << "::ITK::DataCheck"<< filterOutputTypePlaceHolder<<"(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath);\n";
  executeDeclOut  << " return "<< itkArrayHelperNamespace << "::ITK::Execute"<< filterOutputTypeExeTemplateDef<<"(dataStructure, pSelectedInputArray, pImageGeomPath, pOutputArrayPath, itkFunctor);";

  itkFunctorOut  << "struct " << filterName << "CreationFunctor\n{\n";

  includeOut << "#include \"complex/Parameters/ArrayCreationParameter.hpp\"\n"
             << "#include \"complex/Parameters/ArraySelectionParameter.hpp\"\n"
             << "#include \"complex/Parameters/GeometrySelectionParameter.hpp\"\n";

  // Loop over all the Input Parameters to the ITK Filter
  for (auto& jsonIdx : membersJson.items())
  {
    nlohmann::json memberJson = jsonIdx.value();
    std::string propName = memberJson["name"].get<std::string>();
    nlohmann::json pTypeJson = memberJson["type"];
    std::string pType;
    if(!pTypeJson.empty())
    {
      pType = pTypeJson.get<std::string>();
    }
    else
    {
      pTypeJson = memberJson["itk_type"];
      if(!pTypeJson.empty())
      {
        pType = pTypeJson.get<std::string>();
      }
    }
    DetermineParameterClass(propName, pType, memberJson["default"], parameterDefs,
                            itkFunctorOut, itkFunctorBodyOut, itkFunctorDeclOut, preflightDefs, includeOut);
    itkFunctorDeclOut << "p" << propName;
    if(membersJson.back() != memberJson)
    {
      itkFunctorDeclOut << ", ";
    }
  }

  itkFunctorDeclOut << "};\n";

  itkFunctorBodyOut << "    return filter;\n";
  itkFunctorBodyOut << "  }\n";
  // Close up the itkFunctor Struct
  itkFunctorOut << "\n"
                << "  template <class InputImageType, class OutputImageType, uint32 Dimension>\n"
                << "  auto operator()() const\n"
                << "  {\n";
  itkFunctorOut << itkFunctorBodyOut.str();
  itkFunctorOut << "};\n} // namespace";

  includeOut << "\n#include <itk" << itkClassName << ".h>\n";

  std::stringstream linkOutputArrayOut;
  if(itkClassName.find("Projection") == std::string::npos)
  {
    linkOutputArrayOut << "  /****************************************************************************\n";
    linkOutputArrayOut << "   * Associate the output image with the Image Geometry for Visualization\n";
    linkOutputArrayOut << "   ***************************************************************************/\n";
    linkOutputArrayOut << "  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);\n";
    linkOutputArrayOut << "  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);\n";
  }

  templateContents = ReplaceKeywords(templateContents, k_PARAMETER_DEFS, parameterDefs.str());
  templateContents = ReplaceKeywords(templateContents, k_PARAMETER_INCLUDES, includeOut.str());
  templateContents = ReplaceKeywords(templateContents, k_ITK_FILTER_STRUCT, itkFunctorOut.str());
  templateContents = ReplaceKeywords(templateContents, k_ITK_FUNCTOR_DECL, itkFunctorDeclOut.str());
  templateContents = ReplaceKeywords(templateContents, k_PREFLIGHT_DEFS, preflightDefs.str());
  templateContents = ReplaceKeywords(templateContents, k_PREFLIGHT_UPDATED_DEFS, "");
  templateContents = ReplaceKeywords(templateContents, k_PROPOSED_ACTIONS, "");
  templateContents = ReplaceKeywords(templateContents, k_PREFLIGHT_UPDATED_VALUES, "");
  templateContents = ReplaceKeywords(templateContents, k_ITK_ARRAY_HELPERS_DEFINES, pixelTypeDefines.str());
  templateContents = ReplaceKeywords(templateContents, k_DATA_CHECK_DECL, dataCheckDeclOut.str());
  templateContents = ReplaceKeywords(templateContents, k_EXECUTE_DECL, executeDeclOut.str());
  templateContents = ReplaceKeywords(templateContents, k_LINK_OUTPUT_ARRAY, linkOutputArrayOut.str());

  fs::path outputFilePath = k_GeneratedFiltersOutputDir / outputFileName;
  WriteFile(outputFilePath, templateContents);
}

/**
 * @brief
 * @param filter
 * @param incString
 * @param pString
 */
void CreateUnitTestSetting(const std::string& filterName, const nlohmann::json& setting,
                           const nlohmann::json& members,
                           std::stringstream& tOut,
                           std::stringstream& includeOut)
{

  nlohmann::json parameterJson = setting["parameter"];
  std::string parameter = parameterJson.get<std::string>();
  nlohmann::json valueJson = setting["value"];
  //tOut << "    //" << parameter << " = " << valueJson <<  "\n";
  for (auto& memberIdx : members.items())
  {
    nlohmann::json member = memberIdx.value();
    nlohmann::json defaultValue = member["default"];
    if(member.find("name") != member.end() && member["name"] == parameterJson)
    {
      std::string name = member["name"].get<std::string>();
      std::string pType = member["type"].get<std::string>();

      if(name == "KernelRadius")
      {
        includeOut << "#include \"complex/Parameters/VectorParameter.hpp\"\n";
        tOut << "    auto p" << name << " = ";
        if(valueJson.is_array())
        {
          std::vector<int32_t> array = valueJson.get<std::vector<int32_t>>();
          tOut << "std::vector<uint32_t>{";
          for(size_t i = 0; i < array.size(); i++)
          {
            tOut << array[i];
            if(i < array.size() -1)
            {tOut << ", ";}
          }
          tOut << "};\n";
        }
        else if (valueJson.is_number_integer())
        {
          int value =  valueJson.get<int>();
          tOut << "std::vector<uint32_t>(3, " << value << ");\n";
        }

        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<VectorUInt32Parameter::ValueType>(p" << name << "));\n";
      }
      else if (name == "KernelType")
      {
        includeOut << "#include \"complex/Parameters/ChoicesParameter.hpp\"\n";
        tOut << "    itk::simple::KernelEnum p" << name << " = " << valueJson.get<std::string>() << ";\n";
        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<ChoicesParameter::ValueType>(p" << name << "));\n";
      }
      else if(pType == "double")
      {
        if(valueJson.is_number_float())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<float64>(" << valueJson.get<float>() << "));\n";
        }
        else if (valueJson.is_string())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<float64>(" << valueJson.get<std::string>() << "));\n";
        }
        else if (valueJson.is_number_integer())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<float64>(" << valueJson.get<int>() << "));\n";
        }
        else if (valueJson.is_array())
        {
          std::vector<double> values = valueJson.get<std::vector<double>>();
          tOut << "    auto p" << name << " = " << "std::vector<float64>{";
          for(size_t i = 0; i < values.size(); i++)
          {
            tOut << values[i];
            if(i < values.size() -1)
            {tOut << ", ";}
          }
          tOut << "};\n";
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<std::vector<float64>>(p" << name << "));\n";
        }
        else
        {
          tOut << "#error THE Parameter VALUE FROM THE JSON WAS NOT DETERMINED: " << filterName << "::" << name << "::" << valueJson << "\n";
        }
      }
      else if(pType == "bool")
      {
        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<bool>(";
        if(valueJson.is_string())
        {
          tOut << valueJson.get<std::string>();
        }
        else
        {
          tOut << "#error:" << __FILE__ << "(" << __LINE__ << ")\n";
        }
        tOut << "));\n";
      }
      else if(pType == "unsigned int")
      {
        if(valueJson.is_string())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<unsigned int>(" << valueJson.get<std::string>() << "));\n";;
        }
        else if (valueJson.is_array())
        {
          std::vector<uint32_t> values = valueJson.get<std::vector<uint32_t>>();
          tOut << "    auto p" << name << " = " << "std::vector<uint32_t>{";
          for(size_t i = 0; i < values.size(); i++)
          {
            tOut << values[i];
            if(i < values.size() -1)
            {tOut << ", ";}
          }
          tOut << "};\n";
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<std::vector<uint32_t>>(p" << name << "));\n";
        }
        else
        {
          tOut << "#error:" << __FILE__ << "(" << __LINE__ << ")\n";
        }
      }
      else if(pType == "uint8_t")
      {
        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<uint8_t>(";
        if(valueJson.is_string())
        {
          tOut << valueJson.get<std::string>();
        }
        else if (valueJson.is_number_integer())
        {
          tOut << valueJson.get<uint32_t>();
        }
        else
        {
          tOut << "#error:" << __FILE__ << "(" << __LINE__ << ")\n";
        }
        tOut << "));\n";
      }
      else if(pType == "float")
      {
        if(valueJson.is_string())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<float>(" << valueJson.get<std::string>() << "));\n";
        }
        else if (valueJson.is_array())
        {
          std::vector<double> values = valueJson.get<std::vector<double>>();
          tOut << "    auto p" << name << " = " << "std::vector<float>{";
          for(size_t i = 0; i < values.size(); i++)
          {
            tOut << values[i];
            if(i < values.size() -1)
            {tOut << ", ";}
          }
          tOut << "};\n";
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<std::vector<float>>(p" << name << "));\n";
        }
        if(valueJson.is_number_float())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<float>(" << valueJson.get<float>() << "));\n";
        }
        else
        {
          tOut << "#error:" << __FILE__ << "(" << __LINE__ << ")\n";
        }
      }
      else if(pType == "uint32_t")
      {
        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<uint32_t>(";
        if(valueJson.is_string())
        {
          tOut << valueJson.get<std::string>();
        }
        else
        {
          tOut << "#error:" << __FILE__ << "(" << __LINE__ << ")\n";
        }
        tOut << "));\n";
      }
      else if(pType == "int")
      {
        if(valueJson.is_number_integer())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<int>(" << valueJson << "));\n";
        }
        else if (valueJson.is_string())
        {
          tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<int>(" << valueJson << "));\n";
        }
        else
        {
          tOut << "#error THE Parameter VALUE FROM THE JSON WAS NOT DETERMINED: " << filterName << "::" << name << "::" << valueJson << "\n";
        }
      }
      else if(pType == "uint64_t")
      {
        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<uint64_t>(";
        if(valueJson.is_string())
        {
          tOut << valueJson.get<std::string>();
        }
        else
        {
          tOut << "#error:" << __FILE__ << "(" << __LINE__ << ")\n";
        }
        tOut << "));\n";
      }
      else if(pType == "PixelIDValueEnum")
      {
        tOut << "    args.insertOrAssign("<< filterName<< "::k_" << name << "_Key, std::make_any<itk::simple::PixelIDValueEnum>(" << valueJson.get<std::string>() << "));\n";
      }
      else
      {
        tOut << "#error THE Parameter VALUE FROM THE JSON WAS NOT DETERMINED: " << filterName << "::" << name << "::" << valueJson << "\n";
      }
    }
  }
}


/**
 * @brief
 * @param inputJson
 */
void CreateUnitTest(const nlohmann::json& rootObject)
{
  std::string humanFilterName = "ITK::" + rootObject["name"].get<std::string>();
  std::string filterName = "ITK" + rootObject["name"].get<std::string>();
  filterName = ReplaceKeywords(filterName, "Filter", "");

  std::string itkClassName = rootObject["name"].get<std::string>();
  std::string pixelTypes = rootObject["pixel_types"].get<std::string>();
  std::string baseFiltername = rootObject["name"].get<std::string>();

  std::stringstream tOut;
  std::stringstream includeOut;

  includeOut << "#include <catch2/catch.hpp>\n\n";
  includeOut << "#include \"complex/UnitTest/UnitTestCommon.hpp\"\n";
  includeOut << "#include \"complex/Utilities/Parsing/HDF5/H5FileWriter.hpp\"\n";

  // Get the "tests" object
  nlohmann::json testsArray = rootObject["tests"];
  for (auto& jsonIdx : testsArray.items())
  {
    nlohmann::json testObject = jsonIdx.value();
    std::string tag = testObject["tag"].get<std::string>();
    std::string description = testObject["description"].get<std::string>();
    nlohmann::json settings = testObject["settings"];
    nlohmann::json inputsArray = testObject["inputs"];
    tOut << "// " << description << "\n";
    tOut << "TEST_CASE(\"ITK" << itkClassName <<"(" << tag << ")\",\"[" << k_ITKImageProcessing << "][" << filterName << "]["<< tag << "]\")\n";
    tOut << "{\n";
    tOut << "  // Instantiate the filter, a DataStructure object and an Arguments Object\n";
    tOut << "  " << filterName << " filter;\n";
    tOut << "  DataStructure ds;\n";

    for (auto& inputIdx : inputsArray.items())
    {
      nlohmann::json inputArray = inputIdx.value();
      tOut << "  // Read the input image: " << inputArray.get<std::string>() << "\n";
      tOut << "  {\n";
      tOut << "    Arguments args;\n";
      tOut << "    fs::path filePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / \"JSONFilters\" / \"" << inputArray.get<std::string>() << "\";\n";
      tOut << "    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});\n";
      tOut << "    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);\n";
      tOut << "    int32_t result = complex::ITKTestBase::ReadImage(ds, filePath, inputGeometryPath, inputDataPath);\n";
      tOut << "    REQUIRE(result == 0);\n";
      tOut << "  } // End Scope Section\n\n";
    }

    tOut << "  // Test the filter itself\n";
    tOut << "  {\n";
    tOut << "    Arguments args;\n";
    tOut << "    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});\n";
    tOut << "    DataPath inputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_InputDataPath);\n";
    tOut << "    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);\n";
    tOut << "    args.insertOrAssign(" << filterName << "::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));\n";
    tOut << "    args.insertOrAssign(" << filterName << "::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));\n";
    tOut << "    args.insertOrAssign(" << filterName << "::k_OutputIamgeDataPath_Key, std::make_any<DataPath>(outputDataPath));\n";

    for(auto& settingIdx : settings.items())
    {
      nlohmann::json setting = settingIdx.value();
      CreateUnitTestSetting(filterName, setting, rootObject["members"], tOut, includeOut);
    }

    tOut << "    // Preflight the filter and check result\n";
    tOut << "    auto preflightResult = filter.preflight(ds, args);\n";
    tOut << "    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);\n";

    tOut << "    // Execute the filter and check the result\n";
    tOut << "    auto executeResult = filter.execute(ds, args);\n";
    tOut << "    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);\n";
    tOut << "  } // End Scope Section\n";

    tOut << "\n  // Compare to baseline image\n";
    tOut << "  {\n";
    bool hasMd5Hash = (testObject.find("md5hash") != testObject.end());
    bool md5HashIsNull = false;
    if(hasMd5Hash)
    {
      md5HashIsNull = testObject["md5hash"].is_null();
    }
    if( !hasMd5Hash || md5HashIsNull)
    {
      tOut << "    fs::path baselineFilePath = fs::path(unit_test::k_SourceDir.view()) / complex::unit_test::k_DataDir.str() / \"JSONFilters/Baseline/BasicFilters_" << baseFiltername << "_" << tag << ".nrrd\";\n";
      tOut << "    DataPath baselineGeometryPath({complex::ITKTestBase::k_BaselineGeometryPath});\n";
      tOut << "    DataPath baselineDataPath = baselineGeometryPath.createChildPath(complex::ITKTestBase::k_BaselineDataPath);\n";
      tOut << "    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});\n";
      tOut << "    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);\n";
      tOut << "    int32_t error = complex::ITKTestBase::ReadImage(ds, baselineFilePath, baselineGeometryPath, baselineDataPath);\n";
      tOut << "    REQUIRE(error == 0);\n";
      tOut << "    Result<> result = complex::ITKTestBase::CompareImages(ds, baselineGeometryPath, baselineDataPath, inputGeometryPath, outputDataPath, ";
      if(testObject["tolerance"].is_string())
      {
        std::string tolerance = testObject["tolerance"].get<std::string>();
        tOut << tolerance << ");\n";
      }
      else if (testObject["tolerance"].is_number_float())
      {
        double tolerance = testObject["tolerance"].get<double>();
        tOut << tolerance << ");\n";
      }
      else if (testObject["tolerance"].is_number_integer())
      {
        double tolerance = testObject["tolerance"].get<int>();
        tOut << tolerance << ");\n";
      }
      tOut << "    if(result.invalid())\n";
      tOut << "    {\n";
      tOut << "       for(const auto& err : result.errors())\n";
      tOut << "       {\n";
      tOut << "         std::cout << err.code << \": \" << err.message << std::endl;\n";
      tOut << "       }\n";
      tOut << "    }\n";
      tOut << "    REQUIRE(result.valid() == true);\n";
    }
    else
    {
      tOut << "    // Compare md5 hash of final image\n";
      tOut << "    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});\n";
      tOut << "    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);\n";
      tOut << "    std::string md5Hash = complex::ITKTestBase::ComputeMd5Hash(ds, outputDataPath);\n";
      if(!testObject["md5hash"].is_null())
      {
        tOut << "    REQUIRE(md5Hash == \"" << testObject["md5hash"].get<std::string>() << "\");\n";
      }
    }
    tOut << "  }\n";

    tOut << "\n  // Write the output data to a file\n";
    tOut << "  {\n";
    tOut << "    fs::path filePath = fs::path(unit_test::k_BinaryDir.view()) / \"test/BasicFilters_" << baseFiltername << "_" << tag << ".nrrd\";\n";
    tOut << "    DataPath inputGeometryPath({complex::ITKTestBase::k_ImageGeometryPath});\n";
    tOut << "    DataPath outputDataPath = inputGeometryPath.createChildPath(complex::ITKTestBase::k_OutputDataPath);\n";
    tOut << "    int32_t error = complex::ITKTestBase::WriteImage(ds, filePath, inputGeometryPath, outputDataPath);\n";
    tOut << "    // Remove *all* files generated by this test\n";
    tOut << "    fs::path testDir = fs::path(unit_test::k_BinaryDir.view()) / \"test\";\n";
    tOut << "    ITKTestBase::RemoveFiles(testDir, \"BasicFilters_" << baseFiltername << "_" << tag << "\");\n";
 //   tOut << "    REQUIRE(error == 0);\n";
    tOut << "  } // End Scope Section\n";

    tOut << "#if 0\n";
    tOut << "  {\n";
    tOut << "    fs::path filePath =fs::path( unit_test::k_BinaryDir.view()) / \"test\" / \"" << baseFiltername << "_" << tag << ".h5\";\n";
    tOut << "    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);\n";
    tOut << "    REQUIRE(result.valid() == true);\n";
    tOut << "    H5::FileWriter fileWriter = std::move(result.value());\n";
    tOut << "    herr_t err = ds.writeHdf5(fileWriter);\n";
    tOut << "    REQUIRE(err == 0);\n";
    tOut << "  }\n";
    tOut << "#endif\n";

    tOut << "}\n";
  }

  includeOut << "#include \"ITKImageProcessing/Common/sitkCommon.hpp\"\n";
  includeOut << "#include \"" << k_ITKImageProcessing << "/Filters/" << filterName << ".hpp\"\n";
  includeOut << "#include \"" << k_ITKImageProcessing << "/ITKImageProcessing_test_dirs.hpp\"\n";
  includeOut << "#include \"ITKTestBase.hpp\"\n\n";
  includeOut << "#include <filesystem>\n\n";
  includeOut << "namespace fs = std::filesystem;\n\n";
  includeOut << "using namespace complex;\n\n";

  includeOut << tOut.str();

  const fs::path outputFilePath = k_ItkPluginOutputDir / "test" / (filterName + "Test.cpp");
  //std::cout << "Writing UnitTest File: " << outputFilePath.string() << std::endl;
  WriteFile(outputFilePath, includeOut.str());
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int32_t argc, char** argv)
{
  CreateOutputDirectories();
  int count = 0;
  // Loop over all of the SimpleITK basic filters
  for(const auto& jsonFileName : k_ItkFilterList)
  {
    count++;
    fs::path jsonFilePath = k_SimpleItkJsonDir / jsonFileName;
    nlohmann::json simpleItkJson = ReadJsonFile(jsonFilePath);
    std::string filterName = simpleItkJson["name"];
    std::cout <<  "/************** " << filterName << " ********************/" << std::endl;
    CreateFilterHeader(simpleItkJson);
    CreateFilterSource(simpleItkJson);
    CreateUnitTest(simpleItkJson);
  }
  std::cout << "Processed " << count << " JSON Files" << std::endl;
  for(const auto& pixelType : s_AllPixelTypes)
  {
    std::cout << pixelType << std::endl;
  }
  return 0;
}

