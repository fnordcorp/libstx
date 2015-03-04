// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <xzero-base/Api.h>
#include <xzero-base/RuntimeError.h>
#include <xzero-base/cli/FlagType.h>
#include <functional>
#include <list>
#include <vector>
#include <string>

namespace xzero {

class IPAddress;
class Flags;
class Flag;

/**
 * CLI - Command Line Interface.
 *
 * It reads program options from:
 * <ul>
 *   <li>environment variables, as well as</li>
 *   <li>program parameters</li>
 * </ul>
 */
class XZERO_API CLI {
 public:
  struct FlagDef;
  class ValidationError;
  class TypeMismatchError;
  class UnknownOptionError;
  class MissingOptionError;
  class MissingOptionValueError;

  CLI();

  // required string flag
  CLI& defineString(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      std::function<void(const std::string&)> callback = nullptr);

  // defaulted string flag
  CLI& defineString(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      const std::string& defaultValue,
      std::function<void(const std::string&)> callback = nullptr);

  // required number flag
  CLI& defineNumber(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      std::function<void(long int)> callback = nullptr);

  // defaulted number flag
  CLI& defineNumber(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      long int defaultValue,
      std::function<void(long int)> callback = nullptr);

  // required floating-number flag
  CLI& defineFloat(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      std::function<void(float)> callback = nullptr);

  // defaulted floating-number flag
  CLI& defineFloat(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      float defaultValue,
      std::function<void(float)> callback = nullptr);

  // required IP-address flag
  CLI& defineIPAddress(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      std::function<void(const IPAddress&)> callback = nullptr);

  // defaulted IP-address flag
  CLI& defineIPAddress(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      const IPAddress& defaultValue,
      std::function<void(const IPAddress&)> callback = nullptr);

  // defaulted bool flag (always defaults to false)
  CLI& defineBool(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      std::function<void(bool)> callback = nullptr);

  // whether or not to allow unnamed raw parameter values
  CLI& enableParameters(
      const std::string& valuePlaceholder,
      const std::string& helpText);

  const FlagDef* find(const std::string& longOption) const;
  const FlagDef* find(char shortOption) const;

  Flags evaluate(int argc, const char* argv[]) const;
  Flags evaluate(const std::vector<std::string>& args) const;

  std::string helpText(size_t width = 78, size_t helpTextOffset = 30) const;

 private:
  CLI& define(
      const std::string& longOpt,
      char shortOpt,
      bool required,
      FlagType type,
      const std::string& helpText,
      const std::string& valuePlaceholder,
      const std::string& defaultValue,
      std::function<void(const std::string&)> callback);

 private:
  std::list<FlagDef> flagDefs_;

  // non-option parameters
  bool parametersEnabled_;
  std::string parametersPlaceholder_;
  std::string parametersHelpText_;
};

class XZERO_API CLI::ValidationError : public RuntimeError {
 public:
  ValidationError(
      const std::string& what, const char* sourceFile, int sourceLine)
      : RuntimeError(what, sourceFile, sourceLine) {}
};

class XZERO_API CLI::TypeMismatchError  : public ValidationError {
 public:
  TypeMismatchError(const std::string& name, const char* sourceFile, int sourceLine)
      : ValidationError("Type mismatch in " + name, sourceFile, sourceLine) {}
};

class XZERO_API CLI::UnknownOptionError : public ValidationError {
 public:
  UnknownOptionError(const std::string& name, const char* sourceFile, int sourceLine)
      : ValidationError("Unknown option " + name, sourceFile, sourceLine) {}
};

class XZERO_API CLI::MissingOptionError : public ValidationError {
 public:
  MissingOptionError(const std::string& name, const char* sourceFile, int sourceLine)
      : ValidationError("Missing option " + name, sourceFile, sourceLine) {}
};

class XZERO_API CLI::MissingOptionValueError : public ValidationError {
 public:
  MissingOptionValueError(const std::string& name, const char* sourceFile, int sourceLine)
      : ValidationError("Missing option value for " + name, sourceFile, sourceLine) {}
};

class XZERO_API FlagBuilder {
 public:
  FlagBuilder(CLI& cli) : cli_(cli) {}

  CLI& cli() const { return cli_; }

 private:
  CLI& cli_;
};

struct XZERO_API CLI::FlagDef {
  FlagType type;
  std::string longOption;
  char shortOption;
  bool required;
  std::string helpText;
  std::string valuePlaceholder;
  std::string defaultValue;
  std::function<void(const std::string&)> callback;

  std::string makeHelpText(size_t width, size_t helpTextOffset) const;
};

}  // namespace xzero
