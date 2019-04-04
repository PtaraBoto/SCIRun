/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2016 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <Modules/Python/PythonInterfaceParser.h>
#include <Modules/Python/InterfaceWithPython.h>
#ifdef BUILD_WITH_PYTHON
#include <Core/Logging/Log.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#endif

using namespace SCIRun::Modules::Python;
using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Core::Algorithms::Python;

PythonInterfaceParser::PythonInterfaceParser(const std::string& moduleId,
  const ModuleStateHandle& state,
  const std::vector<std::string>& portIds)
  : moduleId_(moduleId), state_(state), portIds_(portIds)
{
}

std::string PythonInterfaceParser::convertOutputSyntax(const std::string& code) const
{
  auto outputVarsToCheck = InterfaceWithPython::outputNameParameters();

  for (const auto& var : outputVarsToCheck)
  {
    auto varName = state_->getValue(var).toString();

    auto regexString = "(\\h*)" + varName + " = (.+)";
    //std::cout << "REGEX STRING " << regexString << std::endl;
    boost::regex outputRegex(regexString);
    boost::smatch what;
    if (regex_match(code, what, outputRegex))
    {
      int rhsIndex = what.size() > 2 ? 2 : 1;
      auto whitespace = what.size() > 2 ? boost::lexical_cast<std::string>(what[1]) : "";
      auto rhs = boost::lexical_cast<std::string>(what[rhsIndex]);
      auto converted = whitespace + "scirun_set_module_transient_state(\"" +
        moduleId_ + "\",\"" + varName + "\"," + rhs + ")";
      //std::cout << "CONVERTED TO " << converted << std::endl;
      return converted;
    }
  }

  return code;
}

std::string PythonInterfaceParser::convertInputSyntax(const std::string& code) const
{
  for (const auto& portId : portIds_)
  {
    auto inputName = state_->getValue(Name(portId)).toString();
    //std::cout << "FOUND INPUT VARIABLE NAME: " << inputName << " for port " << portId << std::endl;
    //std::cout << "NEED TO REPLACE " << inputName << " with\n\t" << "scirun_get_module_input_value(\"" << moduleId_ << "\", \"" << portId << "\")" << std::endl;
    auto index = code.find(inputName);
    if (index != std::string::npos)
    {
      auto codeCopy = code;
      return codeCopy.replace(index, inputName.length(),
        "scirun_get_module_input_value(\"" + moduleId_ + "\", \"" + portId + "\")");
    }
  }
  return code;
}


void PythonInterfaceParser::extractSpecialBlocks(const std::string& code) const
{
  //logCritical("Code: {}", code);
  static boost::regex matlabBlock("(.*)\\/\\*matlab(.*)matlab\\*\\/(.*)");

  std::string::const_iterator start, end;
  start = code.begin();
  end = code.end();
  boost::match_results<std::string::const_iterator> what;
  boost::match_flag_type flags = boost::match_default;
  while (regex_search(start, end, what, matlabBlock, flags))
  {
    // what[0] contains the whole string
    // what[5] contains the class name.
    // what[6] contains the template specialisation if any.
    // add class name and position to map:
    // m[std::string(what[5].first, what[5].second)
    //       + std::string(what[6].first, what[6].second)]
    //    = what[5].first - file.begin();
    auto firstPart = std::string(what[1]);
    auto matlabPart = std::string(what[2]);
    auto secondPart = std::string(what[3]);
    // update search position:
    start = what[2].second;

    // update flags:
    flags |= boost::match_prev_avail;
    flags |= boost::match_not_bob;

    //logCritical("First: {}", firstPart);
    //logCritical("Matlab: {}", matlabPart);
    //logCritical("Second: {}", secondPart);

    //logCritical("Next search string: {}", std::string(start, end));

  }

}