// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_misc.h"
#include "JoSIM/Constants.hpp"

#include <cassert>

std::string Misc::file_from_path(const std::string &path) {
  auto posLastSlash = path.find_last_of("/\\");
  if (posLastSlash == std::string::npos) {
    posLastSlash = 0;
    return path.substr(posLastSlash);
  } else
    return path.substr(posLastSlash + 1);
}

bool Misc::has_suffix(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool Misc::starts_with(const std::string &input, char test) {
  for (const auto i : input) {
    if (i != ' ') {
      return i == test;
    }
  }
  return false;
}

std::vector<std::string> Misc::tokenize_space(const std::string &c) {
  std::string::size_type pos, lastPos = 0, length = c.length();
  std::string delimiters = " \t";
  std::vector<std::string> tokens;
  bool trimEmpty = true;

  using value_type = typename std::vector<std::string>::value_type;
  using size_type = typename std::vector<std::string>::size_type;

  while (lastPos < length + 1) {
    pos = c.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos) {
      pos = length;
    }

    if (pos != lastPos || !trimEmpty)
      tokens.push_back(
          value_type(c.data() + lastPos, (size_type)pos - lastPos));

    lastPos = pos + 1;
  }
  return tokens;
}

std::vector<std::string> Misc::tokenize_space_once(const std::string &c) {
  std::vector<std::string> tokens (2, "");
  std::string::size_type pos = c.find_first_of(" \t");
  std::string::size_type length = c.length();
  if (pos == std::string::npos) {
      pos = length;
  }
  tokens.at(0) = c.substr(0, pos);
  if(pos != length) tokens.at(1) = c.substr(pos + 1, length);
  return tokens;
}

std::vector<std::string> Misc::tokenize_delimeter(const std::string &c,
                                                  const std::string &d) {
  std::vector<std::string> tokens;
  std::stringstream stringStream(c);
  std::string line;
  while (std::getline(stringStream, line)) {
    std::size_t prev = 0, pos;
    while ((pos = line.find_first_of(d, prev)) != std::string::npos) {
      if (pos > prev)
        tokens.push_back(line.substr(prev, pos - prev));
      prev = pos + 1;
    }
    if (prev < line.length())
      tokens.push_back(line.substr(prev, std::string::npos));
  }
  return tokens;
}

void Misc::ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

void Misc::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

int Misc::map_value_count(const std::unordered_map<std::string, int> &map,
                          int value) {
  int counter = 0;
  for (const auto &i : map) {
    if (i.second == value)
      counter++;
  }
  return counter;
}

double Misc::modifier(const std::string &value) {
  std::string::size_type sz;
  double number;
  try {
    number = std::stod(value, &sz);
  } catch (const std::invalid_argument &) {
    Errors::misc_errors(static_cast<int>(MiscErrors::STOD_ERROR), value);
    throw;
  } catch (std::exception &e) {
    Errors::misc_errors(static_cast<int>(MiscErrors::STOD_ERROR), value);
    throw;
  }
  switch (value.substr(sz)[0]) {
    /* mega */
  case 'X':
    return number * 1E6;
    /* mega or milli */
  case 'M':
    /* mega */
    if (value.substr(sz)[1] == 'E' && value.substr(sz)[2] == 'G')
      return number * 1E6;
    /* milli */
    else
      return number * 1E-3;
    /* micro */
  case 'U':
    return number * 1E-6;
    /* nano */
  case 'N':
    return number * 1E-9;
    /* pico */
  case 'P':
    return number * 1E-12;
    /* femto */
  case 'F':
    return number * 1E-15;
    /* kilo */
  case 'K':
    return number * 1E3;
    /* giga */
  case 'G':
    return number * 1E9;
    /* tera */
  case 'T':
    return number * 1E12;
    /* auto modifier */
  case 'E':
    return std::stod(value);
  default:
    return number;
  }
}

void Misc::unique_push(std::vector<std::string> &vector,
                       const std::string &string) {
  if (std::find(vector.begin(), vector.end(), string) == vector.end()) {
    vector.push_back(string);
  }
}

int Misc::index_of(const std::vector<std::string> &vector,
                   const std::string &value) {
  int counter = 0;
  for (const auto &i : vector) {
    /* Value found, return counter */
    if (value == vector.at(counter))
      return counter;
    counter++;
  }
  /* Value was not found, set counter equal to -1 and return */
  counter = -1;
  return counter;
}

std::string Misc::substring_after(const std::string &str,
                                  const std::string &whatpart) {
  std::size_t pos = 0;
  std::string substring;
  if (str.find(whatpart) != std::string::npos)
    pos = str.find(whatpart) + whatpart.size();
  substring = str.substr(pos);
  return substring;
}

std::string Misc::substring_before(const std::string &str,
                                   const std::string &whatpart) {
  std::size_t pos = 0;
  std::string substring;
  if (str.find(whatpart) != std::string::npos) {
    pos = str.find(whatpart);
    substring = str.substr(0, pos);
    return substring;
  } else
    return str;
}

std::vector<double> Misc::parse_function(std::string &str, Input &iObj,
                                         const std::string &subckt) {
  std::vector<double> functionOfT(iObj.transSim.simsize(), 0.0);
  std::vector<std::string> tokens;
  std::string posVarName; // subckt = "";
  tokens = tokenize_space(str);
  auto first = str.find('(') + 1;
  auto last = str.find(')');
  std::string params = str.substr(first, last - first);
  tokens = tokenize_delimeter(params, " ,");
  /* PWL(0 0 T1 V1 T2 V2 ... TN VN) */
  if (str.find("PWL") != std::string::npos) {
    if (std::stod(tokens.at(0)) != 0.0 || std::stod(tokens.at(1)) != 0.0)
      Errors::function_errors(static_cast<int>(FunctionErrors::INITIAL_VALUES), tokens.at(0) + " & " + tokens.at(1));
    std::vector<double> timesteps, values;
    for (int i = 0; i < tokens.size(); i = i + 2) {
      timesteps.push_back(modifier(tokens.at(i)));
    }
    for (int i = 1; i < tokens.size(); i = i + 2) {
      if (iObj.parameters.parsedParams.count(
              JoSIM::ParameterName(tokens.at(i), subckt)) != 0)
        values.push_back(iObj.parameters.parsedParams.at(
            JoSIM::ParameterName(tokens.at(i), subckt)));
      else if (iObj.parameters.parsedParams.count(
              JoSIM::ParameterName(tokens.at(i), "")) != 0)
        values.push_back(iObj.parameters.parsedParams.at(
            JoSIM::ParameterName(tokens.at(i), "")));
      else
        // values.push_back(modifier(tokens.at(i)));
        values.push_back(Parser::parse_param(
            tokens.at(i), iObj.parameters.parsedParams, subckt));
    }
    if (timesteps.size() < values.size())
      Errors::function_errors(static_cast<int>(FunctionErrors::TOO_FEW_TIMESTEPS),
                              std::to_string(timesteps.size()) +
                                  " timesteps & " +
                                  std::to_string(timesteps.size()) + " values");
    if (timesteps.size() > values.size())
      Errors::function_errors(
          static_cast<int>(FunctionErrors::TOO_FEW_VALUES), std::to_string(timesteps.size()) + " timesteps & " +
                              std::to_string(timesteps.size()) + " values");
    if ((timesteps.back() > iObj.transSim.tstop) &&
        (values.back() > values.at(values.size() - 2))) {
      values.at(values.size() - 1) = (iObj.transSim.tstop / timesteps.back()) *
                                  (values.back() - values.at(values.size() - 2));
      timesteps.at(timesteps.size() - 1) = iObj.transSim.tstop;
    } else if ((timesteps.back() > iObj.transSim.tstop) &&
               (values.back() == values.at(values.size() - 2))) {
      timesteps.at(timesteps.size() - 1) = iObj.transSim.tstop;
    }
    if (values.at(values.size() - 1) != 0.0) {
      std::fill(functionOfT.begin() +
                    timesteps.at(timesteps.size() - 1) / iObj.transSim.prstep,
                functionOfT.end(), values.at(values.size() - 1));
    }
    double startpoint, endpoint, value = 0.0;
    if((timesteps.back() / iObj.transSim.prstep) > iObj.transSim.simsize()) functionOfT.resize(int(floor(timesteps.back() / iObj.transSim.prstep)), 0.0);
    for (int i = 1; i < timesteps.size(); i++) {
      startpoint = floor(timesteps.at(i - 1) / iObj.transSim.prstep);
      endpoint = floor(timesteps.at(i) / iObj.transSim.prstep);
      functionOfT.at(startpoint) = values.at(i - 1);
      for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
        if (values.at(i - 1) < values.at(i))
          value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                          (endpoint - startpoint) *
                                          (j - (int)startpoint);
        else if (values.at(i - 1) > values.at(i))
          value = values.at(i - 1) -
                  ((values.at(i - 1) - (values.at(i))) /
                   (endpoint - startpoint) * (j - (int)startpoint));
        else if (values.at(i - 1) == values.at(i))
          value = values.at(i);
        functionOfT.at(j) = value;
      }
    }
  }
  /* PULSE */
  else if (str.find("PULSE") != std::string::npos) {
    if (std::stod(tokens.at(0)) != 0.0)
      Errors::function_errors(static_cast<int>(FunctionErrors::INITIAL_PULSE_VALUE), tokens.at(0));
    if (tokens.size() < 7)
      Errors::function_errors(static_cast<int>(FunctionErrors::PULSE_TOO_FEW_ARGUMENTS),
                              std::to_string(tokens.size()));
    double vPeak, timeDelay, timeRise, timeFall, pulseWidth, pulseRepeat;
    vPeak = modifier(tokens.at(1));
    if (vPeak == 0.0)
      if (iObj.argVerb)
        Errors::function_errors(static_cast<int>(FunctionErrors::PULSE_VPEAK_ZERO), tokens.at(1));
    timeDelay = modifier(tokens.at(2));
    timeRise = modifier(tokens.at(3));
    timeFall = modifier(tokens.at(4));
    pulseWidth = modifier(tokens.at(5));
    if (pulseWidth == 0.0)
      if (iObj.argVerb)
        Errors::function_errors(static_cast<int>(FunctionErrors::PULSE_WIDTH_ZERO), tokens.at(5));
    pulseRepeat = modifier(tokens.at(6));
    if (pulseRepeat == 0.0)
      if (iObj.argVerb)
        Errors::function_errors(static_cast<int>(FunctionErrors::PULSE_REPEAT), tokens.at(6));
    int PR = pulseRepeat / iObj.transSim.prstep;
    int TD = timeDelay / iObj.transSim.prstep;
    std::vector<double> timesteps, values;
    double timestep;
    for (int i = 0; i < ((iObj.transSim.simsize() - TD) / PR); i++) {
      timestep = timeDelay + (pulseRepeat * i);
      if (timestep < iObj.transSim.tstop)
        timesteps.push_back(timestep);
      else
        break;
      values.push_back(0.0);
      timestep = timeDelay + (pulseRepeat * i) + timeRise;
      if (timestep < iObj.transSim.tstop)
        timesteps.push_back(timestep);
      else
        break;
      values.push_back(vPeak);
      timestep = timeDelay + (pulseRepeat * i) + timeRise + pulseWidth;
      if (timestep < iObj.transSim.tstop)
        timesteps.push_back(timestep);
      else
        break;
      values.push_back(vPeak);
      timestep =
          timeDelay + (pulseRepeat * i) + timeRise + pulseWidth + timeFall;
      if (timestep < iObj.transSim.tstop)
        timesteps.push_back(timestep);
      else
        break;
      values.push_back(0.0);
    }
    double startpoint, endpoint, value = 0;
    for (int i = 1; i < timesteps.size(); i++) {
      startpoint = floor(timesteps.at(i - 1) / iObj.transSim.prstep);
      endpoint = floor(timesteps.at(i) / iObj.transSim.prstep);
      functionOfT.at(startpoint) = values.at(i - 1);
      for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
        if (values.at(i - 1) < values.at(i))
          if (values.at(i - 1) < 0)
            value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                           (endpoint - startpoint) *
                                           (j - (int)startpoint);
          else
            value = values.at(i) / (endpoint - startpoint) * (j - (int)startpoint);
        else if (values.at(i - 1) > values.at(i))
          value = values.at(i - 1) -
                  ((values.at(i - 1) - (values.at(i))) /
                   (endpoint - startpoint) * (j - (int)startpoint));
        else if (values.at(i - 1) == values.at(i))
          value = values.at(i);
        else
          assert(false);
        functionOfT.at(j) = value;
      }
    }
  }
  /* SIN(VO VA <FREQ <TD <THETA>>>) */
  else if (str.find("SIN") != std::string::npos) {
    if (tokens.size() < 2)
      Errors::function_errors(static_cast<int>(FunctionErrors::SIN_TOO_FEW_ARGUMENTS),
                              std::to_string(tokens.size()));
    if (tokens.size() > 5)
      Errors::function_errors(static_cast<int>(FunctionErrors::SIN_TOO_MANY_ARGUMENTS),
                              std::to_string(tokens.size()));
    double VO = 0.0, VA = 0.0, TD = 0.0, FREQ = 1 / iObj.transSim.tstop,
           THETA = 0.0;
    VO = modifier(tokens.at(0));
    VA = modifier(tokens.at(1));
    if (VA == 0.0)
      if (iObj.argVerb)
        Errors::function_errors(static_cast<int>(FunctionErrors::SIN_VA_ZERO), tokens.at(1));
    if (tokens.size() == 5) {
      FREQ = modifier(tokens.at(2));
      TD = modifier(tokens.at(3));
      THETA = modifier(tokens.at(4));
    } else if (tokens.size() == 4) {
      FREQ = modifier(tokens.at(2));
      TD = modifier(tokens.at(3));
    } else if (tokens.size() == 3) {
      FREQ = modifier(tokens.at(2));
    }
    double currentTimestep, value;
    int beginTime;
    beginTime = TD / iObj.transSim.prstep;

    assert(iObj.transSim.simsize() == functionOfT.size());
    for (int i = beginTime; i < iObj.transSim.simsize(); i++) {
      currentTimestep = i * iObj.transSim.prstep;
      value = VO + VA * sin(2 * JoSIM::Constants::PI * FREQ * (currentTimestep - TD)) *
                       exp(-THETA * (currentTimestep - TD));
      functionOfT.at(i) = value;
    }
  }
  /* CUSTOM: CUS(WaveFile.dat TS SF IM <TD PER>) */
  else if (str.find("CUS") != std::string::npos) {
    if (tokens.size() < 2)
      Errors::function_errors(static_cast<int>(FunctionErrors::CUS_TOO_FEW_ARGUMENTS),
                              std::to_string(tokens.size()));
    if (tokens.size() > 5)
      Errors::function_errors(static_cast<int>(FunctionErrors::CUS_TOO_MANY_ARGUMENTS),
                              std::to_string(tokens.size()));
    std::string WFline = tokens.at(0);
    std::vector<std::string> WF;
    double TS = 0.0, SF = 0.0, TD = 0.0;
    int PER = 0;
    TS = modifier(tokens.at(1));
    SF = modifier(tokens.at(2));
    if (SF == 0.0)
      if (iObj.argVerb)
        Errors::function_errors(static_cast<int>(FunctionErrors::CUS_SF_ZERO), tokens.at(2));
    if (tokens.size() == 6) {
      TD = modifier(tokens.at(4));
      PER = stoi(tokens.at(5));
    } else if (tokens.size() == 5) {
      TD = modifier(tokens.at(4));
    }
    std::ifstream wffile(WFline);
    if (wffile.good())
      getline(wffile, WFline);
    else
      Errors::function_errors(static_cast<int>(FunctionErrors::CUS_WF_NOT_FOUND), WFline);
    wffile.close();
    WF = tokenize_delimeter(WFline, " ,;");
    std::vector<double> timesteps, values;
    for (int i = 0; i < WF.size(); i++) {
      values.push_back(modifier(WF.at(i)) * SF);
      timesteps.push_back(TD + i * TS);
    }
    if (TS < iObj.transSim.prstep)
      TS = iObj.transSim.prstep;
    double functionSize = (iObj.transSim.tstop - TD) / TS;
    if (PER == 1) {
      double repeats = functionSize / values.size();
      for (int j = 0; j < repeats; j++) {
        double lastTimestep = timesteps.back() + TS;
        for (int i = 0; i < WF.size(); i++) {
          values.push_back(modifier(WF.at(i)) * SF);
          timesteps.push_back(lastTimestep + i * TS);
        }
      }
    }
    double startpoint, endpoint, value = 0.0;
    for (int i = 1; i < timesteps.size(); i++) {
      startpoint = floor(timesteps.at(i - 1) / iObj.transSim.prstep);
      endpoint = floor(timesteps.at(i) / iObj.transSim.prstep);
      functionOfT.at(startpoint) = values.at(i - 1);
      for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
        if (values.at(i - 1) < values.at(i))
          if (values.at(i - 1) < 0)
            value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                           (endpoint - startpoint) *
                                           (j - (int)startpoint);
          else
            value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                           (endpoint - startpoint) *
                                           (j - (int)startpoint);
        else if (values.at(i - 1) > values.at(i))
          value = values.at(i - 1) -
                  ((values.at(i - 1) - (values.at(i))) /
                   (endpoint - startpoint) * (j - (int)startpoint));
        else if (values.at(i - 1) == values.at(i))
          value = values.at(i);
        functionOfT.at(j) = value;
      }
    }
  }
  /* NOISE(VO VA TSTEP TD) */
  else if (str.find("NOISE") != std::string::npos) {
    if (tokens.size() < 2)
      Errors::function_errors(static_cast<int>(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS),
                              std::to_string(tokens.size()));
    if (tokens.size() > 4)
      Errors::function_errors(static_cast<int>(FunctionErrors::NOISE_TOO_MANY_ARGUMENTS),
                              std::to_string(tokens.size()));
    double VO = 0.0, VA = 0.0, TD = 0.0, TSTEP = 0.0;
    VO = modifier(tokens.at(0));
    if(VO != 0.0 && tokens.size() == 4) {
      Errors::function_errors(static_cast<int>(FunctionErrors::NOISE_VO_ZERO), tokens.at(0));
      VA = modifier(tokens.at(1));
      if (VA == 0.0)
        if (iObj.argVerb)
          Errors::function_errors(static_cast<int>(FunctionErrors::NOISE_VA_ZERO), tokens.at(1));
      TD = modifier(tokens.at(3));
    } else {
      VA = modifier(tokens.at(0));
      if (VA == 0.0)
        if (iObj.argVerb)
          Errors::function_errors(static_cast<int>(FunctionErrors::NOISE_VA_ZERO), tokens.at(1));
      TD = modifier(tokens.at(2));
    }
    TSTEP = iObj.transSim.prstep;
    int beginTime;
    double currentTimestep, value;
    beginTime = TD / iObj.transSim.prstep;
    for (int i = beginTime; i < iObj.transSim.simsize(); i++) {
      currentTimestep = i * iObj.transSim.prstep;
      value = VA * grand() / sqrt(2.0 * TSTEP);
      functionOfT.at(i) = value;
    }
  }
  /* PWS(0 0 T1 V1 T2 V2 .. TN VN) */
  else if (str.find("PWS") != std::string::npos) {
    if (std::stod(tokens.at(0)) != 0.0 || std::stod(tokens.at(1)) != 0.0)
      Errors::function_errors(static_cast<int>(FunctionErrors::INITIAL_VALUES), tokens.at(0) + " & " + tokens.at(1));
    std::vector<double> timesteps, values;
    for (int i = 0; i < tokens.size(); i = i + 2) {
      timesteps.push_back(modifier(tokens.at(i)));
    }
    for (int i = 1; i < tokens.size(); i = i + 2) {
      if (iObj.parameters.parsedParams.count(
              JoSIM::ParameterName(tokens.at(i), subckt)) != 0)
        values.push_back(iObj.parameters.parsedParams.at(
            JoSIM::ParameterName(tokens.at(i), subckt)));
      else if (iObj.parameters.parsedParams.count(
              JoSIM::ParameterName(tokens.at(i), "")) != 0)
        values.push_back(iObj.parameters.parsedParams.at(
            JoSIM::ParameterName(tokens.at(i), "")));
      else
        // values.push_back(modifier(tokens.at(i)));
        values.push_back(Parser::parse_param(
            tokens.at(i), iObj.parameters.parsedParams, subckt));
    }
    if (timesteps.size() < values.size())
      Errors::function_errors(static_cast<int>(FunctionErrors::TOO_FEW_TIMESTEPS),
                              std::to_string(timesteps.size()) +
                                  " timesteps & " +
                                  std::to_string(timesteps.size()) + " values");
    if (timesteps.size() > values.size())
      Errors::function_errors(
          static_cast<int>(FunctionErrors::TOO_FEW_VALUES), std::to_string(timesteps.size()) + " timesteps & " +
                              std::to_string(timesteps.size()) + " values");
    if ((timesteps.back() > iObj.transSim.tstop) &&
        (values.back() > values.at(values.size() - 2))) {
      values.at(values.size() - 1) = (iObj.transSim.tstop / timesteps.back()) *
                                  (values.back() - values.at(values.size() - 2));
      timesteps.at(timesteps.size() - 1) = iObj.transSim.tstop;
    } else if ((timesteps.back() > iObj.transSim.tstop) &&
               (values.back() == values.at(values.size() - 2))) {
      timesteps.at(timesteps.size() - 1) = iObj.transSim.tstop;
    }
    if (values.at(values.size() - 1) != 0.0) {
      std::fill(functionOfT.begin() +
                    timesteps.at(timesteps.size() - 1) / iObj.transSim.prstep,
                functionOfT.end(), values.at(values.size() - 1));
    }
    double startpoint, endpoint, value, period, ba = 0.0;
    if((timesteps.back() / iObj.transSim.prstep) > iObj.transSim.simsize()) functionOfT.resize(int(floor(timesteps.back() / iObj.transSim.prstep)), 0.0);
    for (int i = 1; i < timesteps.size(); i++) {
      startpoint = floor(timesteps.at(i - 1) / iObj.transSim.prstep);
      endpoint = floor(timesteps.at(i) / iObj.transSim.prstep);
      period = (endpoint - startpoint) * 2;
      functionOfT.at(startpoint) = values.at(i - 1);
      for (int j = (int)startpoint + 1; j < (int)endpoint; j++) {
        if (values.at(i - 1) < values.at(i)) {
          ba = (values.at(i) - values.at(i - 1)) / 2;
          value = values.at(i - 1) + ba * sin((2 * JoSIM::Constants::PI * (j - (period/4)))/period) + ba;
        } else if (values.at(i - 1) > values.at(i)) {
          ba = (values.at(i - 1) - values.at(i)) / 2;
          value = values.at(i - 1) - ba * sin((2 * JoSIM::Constants::PI * (j + (period/4)))/period) - ba;
        } else if (values.at(i - 1) == values.at(i)) {
          value = values.at(i);
        }
        functionOfT.at(j) = value;
      }
    }
  }
  return functionOfT;
}

bool Misc::findX(const std::vector<std::string> &segment, std::string &theLine,
                 int &linePos) {
  for (int i = linePos; i < segment.size(); i++) {
    if (segment.at(i).at(0) == 'X') {
      theLine = segment.at(i);
      if (i < segment.size() - 1)
        linePos = i + 1;
      else
        linePos = segment.size() - 1;
      return true;
    }
  }
  return false;
}

int Misc::numDigits(int number) {
  int digits = 0;
  if (number <= 0)
    digits = 1;
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}

double Misc::grand() {
  double r,v1,v2,fac;
  r=2;
  while (r>=1) {
    v1=(2*((double)rand()/(double)RAND_MAX)-1);
    v2=(2*((double)rand()/(double)RAND_MAX)-1);
    r=v1*v1+v2*v2;
  }
  fac=sqrt(-2*log(r)/r);
  return(v2*fac);
}