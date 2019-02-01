// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_matrix.h"

void
Matrix::create_matrix(Input &iObj) {
	if(iObj.argAnal == VANALYSIS) Matrix::create_A_volt(iObj);
	else if (iObj.argAnal == PANALYSIS) Matrix::create_A_phase(iObj);
	Matrix::create_CSR();
}

void
Matrix::create_A_volt(Input &iObj) {
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt = "";
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter, expStart, expEnd;
	bool pGND, nGND;
	rowCounter = colCounter = 0;
	for (auto i : iObj.expNetlist) {
		expStart = expEnd = -1;
		devicetokens = Misc::tokenize_space(i.first);
		for (int t = 0; t < devicetokens.size(); t++) {
			if(devicetokens[t].find('{') != std::string::npos) {
				expStart = t;
			}
			if(devicetokens[t].find('}') != std::string::npos) {
				expEnd = t;
			}
		}
		if(expStart == -1 && expEnd != -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		else if(expStart != -1 && expEnd == -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		if (expStart != -1 && expStart == expEnd) {
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		else if (expStart != -1 && expEnd != -1) {
			int d = expStart + 1;
			while (expStart != expEnd) {
				devicetokens[expStart] += devicetokens[d];
				devicetokens.erase(devicetokens.begin()+d);
				expEnd--;
			}
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		double value = 0.0;
		try {
			label = devicetokens[0];
			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
				componentLabels.end())
				componentLabels.push_back(label);
			else {
				Errors::invalid_component_errors(DUPLICATE_LABEL, label);
			}
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
			nodeP = devicetokens[1];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_PNODE, i.first);
		}
		try {
			nodeN = devicetokens[2];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_NNODE, i.first);
		}
		/**************/
		/** RESISTOR **/
		/**************/
		if (i.first[0] == 'R') {
			std::string R = label;
			matrix_element e;
			try {
				if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					value = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(RES_ERROR, i.first);
			}
			components.voltRes[R].label = R;
			components.voltRes[R].value = value;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltRes[R].posNodeR = rNameP;
				components.voltRes[R].posNodeC = cNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltRes[R].negNodeR = rNameN;
				components.voltRes[R].negNodeC = cNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				components.voltRes[R].posNRow = rowMap[rNameP];
				components.voltRes[R].posNCol = columnMap[cNameP];
				e.value = 1 / value;
				components.voltRes[R].ppPtr = mElements.size();
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					components.voltRes[R].posNRow = rowMap[rNameP];
					components.voltRes[R].negNCol = columnMap[cNameN];
					e.value = -1 / value;
					components.voltRes[R].pnPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					components.voltRes[R].negNRow = rowMap[rNameN];
					components.voltRes[R].posNCol = columnMap[cNameP];
					e.value = -1 / value;
					components.voltRes[R].npPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameN];
					components.voltRes[R].negNRow = rowMap[rNameN];
					components.voltRes[R].negNCol = columnMap[cNameN];
					e.value = 1 / value;
					components.voltRes[R].nnPtr = mElements.size();
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				components.voltRes[R].negNRow = rowMap[rNameN];
				components.voltRes[R].negNCol = columnMap[cNameN];
				e.value = 1 / value;
				components.voltRes[R].nnPtr = mElements.size();
				mElements.push_back(e);
			}
		}
		/***************/
		/** CAPACITOR **/
		/***************/
		else if (i.first[0] == 'C') {
			std::string C = label;
			matrix_element e;
			try {
				if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					value = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(CAP_ERROR, i.first);
			}
			components.voltCap[C].label = devicetokens[0];
			components.voltCap[C].value = value;
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			components.voltCap[C].curNodeC = cName;
			components.voltCap[C].curNodeR = rName;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltCap[C].posNodeC = cNameP; 
				components.voltCap[C].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltCap[C].negNodeC = cNameN;
				components.voltCap[C].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.voltCap[C].posNCol = e.colIndex;
				components.voltCap[C].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.voltCap[C].curNCol = e.colIndex;
				components.voltCap[C].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rName];
					components.voltCap[C].negNCol = e.colIndex;
					components.voltCap[C].curNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cName];
					e.rowIndex = rowMap[rNameN];
					components.voltCap[C].curNCol = e.colIndex;
					components.voltCap[C].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.voltCap[C].negNCol = e.colIndex;
				components.voltCap[C].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.voltCap[C].curNCol = e.colIndex;
				components.voltCap[C].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.voltCap[C].curNCol = e.colIndex;
			components.voltCap[C].curNRow = e.rowIndex;
			e.value = iObj.transSim.prstep / (2 * value);
			components.voltCap[C].capPtr = mElements.size();
			mElements.push_back(e);
		}
		/**************/
		/** INDUCTOR **/
		/**************/
		else if (i.first[0] == 'L') {
			std::string L = label;
			matrix_element e;
			try {
				if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					value = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(IND_ERROR, i.first);
			}
			components.voltInd[L].label = label;
			components.voltInd[L].value = value;
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			components.voltInd[L].curNodeC = cName;
			components.voltInd[L].curNodeR = rName;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltInd[L].posNodeC = cNameP;
				components.voltInd[L].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltInd[L].negNodeC = cNameN;
				components.voltInd[L].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.voltInd[L].curNCol = e.colIndex;
				components.voltInd[L].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.voltInd[L].posNCol = e.colIndex;
				components.voltInd[L].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap[cName];
					e.rowIndex = rowMap[rNameN];
					components.voltInd[L].curNCol = e.colIndex;
					components.voltInd[L].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rName];
					components.voltInd[L].negNCol = e.colIndex;
					components.voltInd[L].curNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.voltInd[L].curNCol = e.colIndex;
				components.voltInd[L].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.voltInd[L].negNCol = e.colIndex;
				components.voltInd[L].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.voltInd[L].curNCol = e.colIndex;
			components.voltInd[L].curNRow = e.rowIndex;
			e.value = (-2 * value) / iObj.transSim.prstep;
			components.voltInd[L].indPtr = mElements.size();
			mElements.push_back(e);
		}
		/********************/
		/** VOLTAGE SOURCE **/
		/********************/
		else if (i.first[0] == 'V') {
			std::string V = label;
			matrix_element e;
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
			cName = "C_" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			components.voltVs[V].curNodeC = cName;
			components.voltVs[V].curNodeR = rName;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltVs[V].posNodeC = cNameP;
				components.voltVs[V].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltVs[V].negNodeC = cNameN;
				components.voltVs[V].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.voltVs[V].curNCol = e.colIndex;
				components.voltVs[V].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.voltVs[V].posNCol = e.colIndex;
				components.voltVs[V].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap[cName];
					e.rowIndex = rowMap[rNameN];
					components.voltVs[V].curNCol = e.colIndex;
					components.voltVs[V].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rName];
					components.voltVs[V].negNCol = e.colIndex;
					components.voltVs[V].curNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.voltVs[V].curNCol = e.colIndex;
				components.voltVs[V].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.voltVs[V].negNCol = e.colIndex;
				components.voltVs[V].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
		}
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i.first[0] == 'I') {
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
		}
		/************************/
		/** JOSEPHSON JUNCTION **/
		/************************/
		else if (i.first[0] == 'B') {
			std::string jj = label;
			matrix_element e;
			std::string modelstring = "", area = "";
			for (int t = devicetokens.size() - 1; t > 2; t--) {
				if (devicetokens[t].find("=") == std::string::npos) {
					if (iObj.netlist.models.count(std::make_pair(devicetokens[t], i.second)) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], i.second));
						break;
					}
					else if (iObj.netlist.models.count(std::make_pair(devicetokens[t], "")) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], ""));
						break;
					}
					else { Errors::invalid_component_errors(MODEL_NOT_DEFINED,
														  devicetokens[t]);
							break;
					}
				}
				if (devicetokens[t].find("AREA") != std::string::npos) {
					area = devicetokens[t].substr(
						devicetokens[t].find_first_of('=') + 1,
						devicetokens[t].size() - 1);
				}
			}
			if(area == "" && iObj.argVerb) Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, "");
			components.voltJJ[jj].label = jj;
			components.jj_model(modelstring, area, jj, iObj, i.second);
			components.voltJJ[jj].label = label;
			cName = "C_P" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.voltJJ[jj].phaseNodeC = cName;
			components.voltJJ[jj].phaseNodeR = rName;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltJJ[jj].posNodeC = cNameP;
				components.voltJJ[jj].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltJJ[jj].negNodeC = cNameN;
				components.voltJJ[jj].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				components.voltJJ[jj].posNCol = e.colIndex;
				components.voltJJ[jj].posNRow = e.rowIndex;
				e.value = ((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0);
				components.voltJJ[jj].ppPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.voltJJ[jj].posNCol = e.colIndex;
				components.voltJJ[jj].phaseNRow = e.rowIndex;
				e.value = (-iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					components.voltJJ[jj].negNCol = e.colIndex;
					components.voltJJ[jj].posNRow = e.rowIndex;
					e.value = -(((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0));
					components.voltJJ[jj].npPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					components.voltJJ[jj].posNCol = e.colIndex;
					components.voltJJ[jj].negNRow = e.rowIndex;
					e.value = -(((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0));
					components.voltJJ[jj].pnPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameN];
					components.voltJJ[jj].negNCol = e.colIndex;
					components.voltJJ[jj].negNRow = e.rowIndex;
					e.value = ((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0);
					components.voltJJ[jj].nnPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rName];
					components.voltJJ[jj].negNCol = e.colIndex;
					components.voltJJ[jj].phaseNRow = e.rowIndex;
					e.value = (iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameP];
				components.voltJJ[jj].negNCol = e.colIndex;
				components.voltJJ[jj].posNRow = e.rowIndex;
				e.value = -(((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0));
				components.voltJJ[jj].npPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameN];
				components.voltJJ[jj].posNCol = e.colIndex;
				components.voltJJ[jj].negNRow = e.rowIndex;
				e.value = -(((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0));
				components.voltJJ[jj].pnPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				components.voltJJ[jj].negNCol = e.colIndex;
				components.voltJJ[jj].negNRow = e.rowIndex;
				e.value = ((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0);
				components.voltJJ[jj].nnPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.voltJJ[jj].negNCol = e.colIndex;
				components.voltJJ[jj].phaseNRow = e.rowIndex;
				e.value = (iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.voltJJ[jj].phaseNCol = e.colIndex;
			components.voltJJ[jj].phaseNRow = e.rowIndex;
			e.value = 1;
			mElements.push_back(e);
			components.voltJJ[jj].gLarge = components.voltJJ.at(jj).iC / (components.voltJJ.at(jj).iCFact * components.voltJJ.at(jj).delV);
			components.voltJJ[jj].lowerB = components.voltJJ.at(jj).vG - 0.5*components.voltJJ.at(jj).delV;
			components.voltJJ[jj].upperB = components.voltJJ.at(jj).vG + 0.5 * components.voltJJ.at(jj).delV;
			components.voltJJ[jj].subCond = 1 / components.voltJJ.at(jj).r0 + ((2*components.voltJJ.at(jj).C) / iObj.transSim.prstep);
			components.voltJJ[jj].transCond = components.voltJJ.at(jj).gLarge + ((2*components.voltJJ.at(jj).C) / iObj.transSim.prstep);
			components.voltJJ[jj].normalCond = 1 / components.voltJJ.at(jj).rN + ((2*components.voltJJ.at(jj).C) / iObj.transSim.prstep);
			components.voltJJ[jj].Del0 = 1.76 * BOLTZMANN * components.voltJJ.at(jj).tC;
			components.voltJJ[jj].Del = components.voltJJ.at(jj).Del0 * sqrt(cos((M_PI/2) * 
				(components.voltJJ.at(jj).T/components.voltJJ.at(jj).tC) * (components.voltJJ.at(jj).T/components.voltJJ.at(jj).tC)));
			components.voltJJ[jj].rNCalc = ((M_PI * components.voltJJ.at(jj).Del) / (2 * EV * components.voltJJ.at(jj).iC)) * 
				tanh(components.voltJJ.at(jj).Del / (2 * BOLTZMANN * components.voltJJ.at(jj).T));
			components.voltJJ[jj].iS = -components.voltJJ.at(jj).iC * sin(components.voltJJ.at(jj).phi0);
		}
		/***********************/
		/** TRANSMISSION LINE **/
		/***********************/
		else if (i.first[0] == 'T') {
			std::string Tx = label, cName2, rName2, cNameP2, rNameP2, cNameN2, 
				rNameN2, nodeP2, nodeN2;
			bool pGND2, nGND2;
			matrix_element e;
			double z0 = 10, tD = 0.0;
			for (int t = 5; t < devicetokens.size(); t++) {
				if(devicetokens[t].find("TD") != std::string::npos)
					tD = Parser::parse_param(
						devicetokens[t].substr(devicetokens[t].find("TD=") + 3,
						devicetokens[t].size() - 1), iObj.parameters.parsedParams, i.second);
				else if(devicetokens[t].find("Z0") != std::string::npos)
					z0 = Parser::parse_param(
						devicetokens[t].substr(devicetokens[t].find("Z0=") + 3,
						devicetokens[t].size() - 1), iObj.parameters.parsedParams, i.second);
				else if(devicetokens[t].find("LOSSLESS") != std::string::npos) {}
				else Errors::invalid_component_errors(INVALID_TX_DEFINED, i.first);
			}
			components.txLine[Tx].label = label;
			components.txLine[Tx].k = tD / iObj.transSim.prstep;
			components.txLine[Tx].value = z0;
			cName = "C_I1" + devicetokens[0];
			rName = "R_" + devicetokens[0] + "-I1";
			cName2 = "C_I2" + devicetokens[0];
			rName2 = "R_" + devicetokens[0] + "-I2";
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (rowMap.count(rName2) == 0) {
				rowMap[rName2] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName2) == 0) {
				columnMap[cName2] = colCounter;
				colCounter++;
			}
			components.txLine[Tx].curNode1C = cName;
			components.txLine[Tx].curNode1R = rName;
			components.txLine[Tx].curNode2C = cName2;
			components.txLine[Tx].curNode2R = rName2;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.txLine[Tx].posNodeC = cNameP;
				components.txLine[Tx].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.txLine[Tx].negNodeC = cNameN;
				components.txLine[Tx].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			try {
				nodeP2 = devicetokens[3];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_PNODE, i.first);
			}
			/* Check if negative node exists, if not it's a bad device line definition
			*/
			try {
				nodeN2 = devicetokens[4];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_NNODE, i.first);
			}
			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
				cNameP2 = "C_NV" + nodeP2;
				rNameP2 = "R_N" + nodeP2;
				components.txLine[Tx].posNode2C = cNameP2;
				components.txLine[Tx].posNode2R = rNameP2;
				if (rowMap.count(rNameP2) == 0) {
					rowMap[rNameP2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP2) == 0) {
					columnMap[cNameP2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP2].push_back(label);
				pGND2 = false;
			}
			else
				pGND2 = true;
			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
				cNameN2 = "C_NV" + nodeN2;
				rNameN2 = "R_N" + nodeN2;
				components.txLine[Tx].negNode2C = cNameN2;
				components.txLine[Tx].negNode2R = rNameN2;
				if (rowMap.count(rNameN2) == 0) {
					rowMap[rNameN2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN2) == 0) {
					columnMap[cNameN2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN2].push_back(label);
				nGND2 = false;
			}
			else
				nGND2 = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.txLine[Tx].curN1Col = e.colIndex;
				components.txLine[Tx].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.txLine[Tx].posNCol = e.colIndex;
				components.txLine[Tx].curN1Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap[cName];
					e.rowIndex = rowMap[rNameN];
					components.txLine[Tx].curN1Col = e.colIndex;
					components.txLine[Tx].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rName];
					components.txLine[Tx].negNCol = e.colIndex;
					components.txLine[Tx].curN1Row = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.txLine[Tx].curN1Col = e.colIndex;
				components.txLine[Tx].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.txLine[Tx].negNCol = e.colIndex;
				components.txLine[Tx].curN1Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.txLine[Tx].curN1Col = e.colIndex;
			components.txLine[Tx].curN1Row = e.rowIndex;
			e.value = -z0;
			mElements.push_back(e);
			if (!pGND2) {
				e.label = label;
				e.colIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameP2];
				components.txLine[Tx].curN2Col = e.colIndex;
				components.txLine[Tx].posN2Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP2];
				e.rowIndex = rowMap[rName2];
				components.txLine[Tx].posN2Col = e.colIndex;
				components.txLine[Tx].curN2Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND2) {
					e.label = label;
					e.colIndex = columnMap[cName2];
					e.rowIndex = rowMap[rNameN2];
					components.txLine[Tx].curN2Col = e.colIndex;
					components.txLine[Tx].negN2Row = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap[cNameN2];
					e.rowIndex = rowMap[rName2];
					components.txLine[Tx].negN2Col = e.colIndex;
					components.txLine[Tx].curN2Row = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND2) {
				e.label = label;
				e.colIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameN2];
				components.txLine[Tx].curN2Col = e.colIndex;
				components.txLine[Tx].negN2Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN2];
				e.rowIndex = rowMap[rName2];
				components.txLine[Tx].negN2Col = e.colIndex;
				components.txLine[Tx].curN2Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName2];
			e.rowIndex = rowMap[rName2];
			components.txLine[Tx].curN2Col = e.colIndex;
			components.txLine[Tx].curN2Row = e.rowIndex;
			e.value = -z0;
			mElements.push_back(e);
		}
		/*********************/
		/** MUTUAL COUPLING **/
		/*********************/
		else if (i.first[0] == 'K') {
			components.mutualInductanceLines.push_back(i);
		}
	}
	double mutualL = 0.0, cf = 0.0;
	for (auto i : components.mutualInductanceLines) {
		devicetokens = Misc::tokenize_space(i.first);
		try {
			label = devicetokens[0];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
			if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					cf = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
			else
				cf = Misc::modifier(devicetokens[3]);
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MUT_ERROR, i.first);
		}
		std::string ind1, ind2;
		ind1 = devicetokens[1];
		ind2 = devicetokens[2];
		if(components.voltInd.count(ind1) == 0) Errors::invalid_component_errors(MISSING_INDUCTOR, ind1);
		if(components.voltInd.count(ind2) == 0) Errors::invalid_component_errors(MISSING_INDUCTOR, ind2);
		cf = cf * sqrt(components.voltInd.at(ind1).value * components.voltInd.at(ind2).value);
		mutualL = ((2 * cf) / iObj.transSim.prstep);
		components.voltInd.at(ind1).mut[ind2] = mutualL;
		components.voltInd.at(ind2).mut[ind1] = mutualL;
		matrix_element e;
		e.label = label;
		e.colIndex = components.voltInd.at(ind1).curNCol;
		e.rowIndex = components.voltInd.at(ind2).curNRow;
		e.value = -mutualL;
		components.voltInd.at(ind1).mutPtr[ind2] = mElements.size();
		mElements.push_back(e);
		e.label = label;
		e.colIndex = components.voltInd.at(ind2).curNCol;
		e.rowIndex = components.voltInd.at(ind1).curNRow;
		e.value = -mutualL;
		components.voltInd.at(ind2).mutPtr[ind1] = mElements.size();
		mElements.push_back(e);
	}
	std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
	std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
	std::transform(
		rowMapFlip.begin(),
		rowMapFlip.end(),
		back_inserter(rowNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
	std::transform(
		columnMapFlip.begin(),
		columnMapFlip.end(),
		back_inserter(columnNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
}

void
Matrix::create_A_phase(Input &iObj) {
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt;
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter, expStart, expEnd;
	bool pGND, nGND;
	rowCounter = colCounter = 0;
	/* Main circuit node identification */
	for (auto i : iObj.expNetlist) {
		expStart = expEnd = -1;
		devicetokens = Misc::tokenize_space(i.first);
				for (int t = 0; t < devicetokens.size(); t++) {
			if(devicetokens[t].find('{') != std::string::npos) {
				expStart = t;
			}
			if(devicetokens[t].find('}') != std::string::npos) {
				expEnd = t;
			}
		}
		if(expStart == -1 && expEnd != -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		else if(expStart != -1 && expEnd == -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		if (expStart != -1 && expStart == expEnd) {
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		else if (expStart != -1 && expEnd != -1) {
			int d = expStart + 1;
			while (expStart != expEnd) {
				devicetokens[expStart] += devicetokens[d];
				devicetokens.erase(devicetokens.begin()+d);
				expEnd--;
			}
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		double value = 0.0;
		/* Check if label exists, if not there is a bug in the program */
		try {
			label = devicetokens[0];
			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
				componentLabels.end())
				componentLabels.push_back(label);
			else {
				Errors::invalid_component_errors(DUPLICATE_LABEL, label);
			}
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
			nodeP = devicetokens[1];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_PNODE, i.first);
		}
		/* Check if negative node exists, if not it's a bad device line definition
		 */
		try {
			nodeN = devicetokens[2];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_NNODE, i.first);
		}
		/********************/
		/** PHASE RESISTOR **/
		/********************/
		if (i.first[0] == 'R') {
			std::string R = devicetokens[0];
			matrix_element e;
			try {
				if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					value = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(RES_ERROR, i.first);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseRes[R].curNodeC = cName;
			components.phaseRes[R].curNodeR = rName;
			components.phaseRes[R].value = value;
			components.phaseRes[R].label = R;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseRes[R].posNodeC = cNameP;
				components.phaseRes[R].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseRes[R].negNodeC = cNameN;
				components.phaseRes[R].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.phaseRes[R].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.phaseRes[R].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.phaseRes[R].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.phaseRes[R].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.phaseRes[R].curNCol = e.colIndex;
			components.phaseRes[R].curNRow = e.rowIndex;
			e.value = -(M_PI * components.phaseRes[R].value * iObj.transSim.prstep) / PHI_ZERO;
			components.phaseRes[R].resPtr = mElements.size();
			mElements.push_back(e);
		}
		/*********************/
		/** PHASE CAPACITOR **/
		/*********************/
		else if (i.first[0] == 'C') {
			std::string C = devicetokens[0];
			matrix_element e;
			try {
				if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					value = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(CAP_ERROR, i.first);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseCap[C].curNodeC = cName;
			components.phaseCap[C].curNodeR = rName;
			components.phaseCap[C].value = value;
			components.phaseCap[C].label = C;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseCap[C].posNodeC = cNameP;
				components.phaseCap[C].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseCap[C].negNodeC = cNameN;
				components.phaseCap[C].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.phaseCap[C].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.phaseCap[C].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.phaseCap[C].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.phaseCap[C].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.phaseCap[C].curNCol = e.colIndex;
			components.phaseCap[C].curNRow = e.rowIndex;
			e.value = (-2 * M_PI * iObj.transSim.prstep * iObj.transSim.prstep) / (PHI_ZERO * 4 * components.phaseCap[C].value);
			components.phaseCap[C].capPtr = mElements.size();
			mElements.push_back(e);
		}
		/********************/
		/** PHASE INDUCTOR **/
		/********************/
		else if (i.first[0] == 'L') {
			std::string L = devicetokens[0];
			matrix_element e;
			try {
				if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
					value = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(IND_ERROR, i.first);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseInd[L].curNodeC = cName;
			components.phaseInd[L].curNodeR = rName;
			components.phaseInd[L].value = value;
			components.phaseInd[L].label = L;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseInd[L].posNodeC = cNameP;
				components.phaseInd[L].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseInd[L].negNodeC = cNameN;
				components.phaseInd[L].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.phaseInd[L].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.phaseInd[L].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.phaseInd[L].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.phaseInd[L].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			components.phaseInd[L].curNCol = e.colIndex;
			components.phaseInd[L].curNRow = e.rowIndex;
			e.value = -(components.phaseInd[L].value * 2 * M_PI) / PHI_ZERO;
			components.phaseInd[L].indPtr = mElements.size();
			mElements.push_back(e);
		}
		/**************************/
		/** PHASE VOLTAGE SOURCE **/
		/**************************/
		else if (i.first[0] == 'V') {
			std::string VS = devicetokens[0];
			matrix_element e;
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
			cName = "C_" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseVs[VS].curNodeC = cName;
			components.phaseVs[VS].curNodeR = rName;
			components.phaseVs[VS].label = VS;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseVs[VS].posNodeC = cNameP;
				components.phaseVs[VS].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseVs[VS].negNodeC = cNameN;
				components.phaseVs[VS].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				components.phaseVs[VS].curNCol = e.colIndex;
				components.phaseVs[VS].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				components.phaseVs[VS].posNCol = e.colIndex;
				components.phaseVs[VS].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				components.phaseVs[VS].curNCol = e.colIndex;
				components.phaseVs[VS].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				components.phaseVs[VS].negNCol = e.colIndex;
				components.phaseVs[VS].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
		}
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i.first[0] == 'I') {
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
		}
		/******************************/
		/** PHASE JOSEPHSON JUNCTION **/
		/******************************/
		else if (i.first[0] == 'B') {
			std::string cVolt, rVolt, jj;
			jj = devicetokens[0];
			matrix_element e;
			std::string modelstring = "", area = "";
			for (int t = devicetokens.size() - 1; t > 2; t--) {
				if (devicetokens[t].find("=") == std::string::npos) {
					if (iObj.netlist.models.count(std::make_pair(devicetokens[t], i.second)) != 0) {
						modelstring = devicetokens[t];
						break;
					}
					else if (iObj.netlist.models.count(std::make_pair(devicetokens[t], "")) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], ""));
						break;
					}
					else {
						Errors::invalid_component_errors(MODEL_NOT_DEFINED,
														  devicetokens[t]);
						break;
					}
				}
				if (devicetokens[t].find("AREA") != std::string::npos) {
					area = devicetokens[t].substr(
						devicetokens[t].find_first_of('=') + 1,
						devicetokens[t].size() - 1);
				}
			}
			if(area == "") Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, "");
			components.phaseJJ[jj].label = jj;
			components.jj_model_phase(modelstring, area, jj, iObj, subckt);
			cVolt = "C_V" + devicetokens[0];
			rVolt = "R_" + devicetokens[0];
			components.phaseJJ[jj].voltNodeC = cVolt;
			components.phaseJJ[jj].voltNodeR = rVolt;
			if (rowMap.count(rVolt) == 0) {
				rowMap[rVolt] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cVolt) == 0) {
				columnMap[cVolt] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseJJ[jj].posNodeC = cNameP;
				components.phaseJJ[jj].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseJJ[jj].negNodeC = cNameN;
				components.phaseJJ[jj].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap[cVolt];
				e.rowIndex = rowMap[rNameP];
				components.phaseJJ[jj].posNRow = e.rowIndex;;
				e.value = 1 / components.phaseJJ[jj].r0 + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
				components.phaseJJ[jj].pPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rVolt];
				components.phaseJJ[jj].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap[cVolt];
				e.rowIndex = rowMap[rNameN];
				components.phaseJJ[jj].negNRow = e.rowIndex;				
				e.value = -1 / components.phaseJJ[jj].r0 - ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
				components.phaseJJ[jj].nPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rVolt];	
				components.phaseJJ[jj].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cVolt];
			e.rowIndex = rowMap[rVolt];
			components.phaseJJ[jj].voltNCol = e.colIndex;
			components.phaseJJ[jj].voltNRow = e.rowIndex;
			e.value = -(iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
			mElements.push_back(e);
			components.phaseJJ[jj].gLarge = components.phaseJJ[jj].iC / (components.phaseJJ[jj].iCFact * components.phaseJJ[jj].delV);
			components.phaseJJ[jj].lower = components.phaseJJ[jj].vG - 0.5*components.phaseJJ[jj].delV;
			components.phaseJJ[jj].upper = components.phaseJJ[jj].vG + 0.5 * components.phaseJJ[jj].delV;
			components.phaseJJ[jj].subCond = 1 / components.phaseJJ[jj].r0 + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
			components.phaseJJ[jj].transCond = components.phaseJJ[jj].gLarge + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
			components.phaseJJ[jj].normalCond = 1 / components.phaseJJ[jj].rN + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
			components.phaseJJ[jj].Del0 = 1.76 * BOLTZMANN * components.phaseJJ[jj].tC;
			components.phaseJJ[jj].Del = components.phaseJJ[jj].Del0 * sqrt(cos((M_PI/2) * (components.phaseJJ[jj].T/components.phaseJJ[jj].tC) * (components.phaseJJ[jj].T/components.phaseJJ[jj].tC)));
			components.phaseJJ[jj].rNCalc = ((M_PI * components.phaseJJ[jj].Del) / (2 * EV * components.phaseJJ[jj].iC)) * tanh(components.phaseJJ[jj].Del / (2 * BOLTZMANN * components.phaseJJ[jj].T));
			components.phaseJJ[jj].iS = -components.phaseJJ[jj].iC * sin(components.phaseJJ[jj].phi0);
		}
		/*****************************/
		/** PHASE TRANSMISSION LINE **/
		/*****************************/
		else if (i.first[0] == 'T') {
			std::string nodeP2, nodeN2, cNameP2, rNameP2, cNameN2,
				rNameN2, cName1, rName1, cName2,
				rName2, tl;
			bool p2GND, n2GND;
			tl = devicetokens[0];
			try {
				nodeP2 = devicetokens[3];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_PNODE, i.first);
			}
			try {
				nodeN2 = devicetokens[4];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_NNODE, i.first);
			}
			matrix_element e;
			if (devicetokens.size() < 7) {
				Errors::invalid_component_errors(TIME_ERROR, i.first);
			}
			for (size_t l = 5; l < devicetokens.size(); l++) {
				if (devicetokens[l].find("TD") != std::string::npos)
					components.txPhase[tl].tD = Misc::modifier((devicetokens[l]).substr(3));
				else if (devicetokens[l].find("Z0") != std::string::npos)
					components.txPhase[tl].value = Misc::modifier((devicetokens[l]).substr(3));
			}
			components.txPhase[tl].k = components.txPhase[tl].tD / iObj.transSim.prstep;
			cName1 = "C_I1" + label;
			rName1 = "R_" + label + "-I1";
			components.txPhase[tl].curNode1C = cName1;
			components.txPhase[tl].curNode1R = rName1;
			if (rowMap.count(rName1) == 0) {
				rowMap[rName1] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName1) == 0) {
				columnMap[cName1] = colCounter;
				colCounter++;
			}
			cName2 = "C_I2" + label;
			rName2 = "R_" + label + "-I2";
			components.txPhase[tl].curNode2C = cName2;
			components.txPhase[tl].curNode2R = rName2;
			if (rowMap.count(rName2) == 0) {
				rowMap[rName2] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName2) == 0) {
				columnMap[cName2] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.txPhase[tl].posNodeC = cNameP;
				components.txPhase[tl].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.txPhase[tl].negNodeC = cNameN;
				components.txPhase[tl].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
				cNameP2 = "C_NP" + nodeP2;
				rNameP2 = "R_N" + nodeP2;
				components.txPhase[tl].posNode2C = cNameP2;
				components.txPhase[tl].posNode2R = rNameP2;
				if (rowMap.count(rNameP2) == 0) {
					rowMap[rNameP2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP2) == 0) {
					columnMap[cNameP2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP2].push_back(label);
				p2GND = false;
			}
			else
				p2GND = true;
			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
				cNameN2 = "C_NP" + nodeN2;
				rNameN2 = "R_N" + nodeN2;
				components.txPhase[tl].negNode2C = cNameN2;
				components.txPhase[tl].negNode2R = rNameN2;
				if (rowMap.count(rNameN2) == 0) {
					rowMap[rNameN2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN2) == 0) {
					columnMap[cNameN2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN2].push_back(label);
				n2GND = false;
			}
			else
				n2GND = true;
			if (!pGND) {
				e.colIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName1];
				components.txPhase[tl].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
				e.colIndex = columnMap[cName1];
				e.rowIndex = rowMap[rNameP];
				components.txPhase[tl].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if(!nGND) {
				e.colIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName1];
				components.txPhase[tl].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
				e.colIndex = columnMap[cName1];
				e.rowIndex = rowMap[rNameN];
				components.txPhase[tl].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			if (!p2GND) {
				e.colIndex = columnMap[cNameP2];
				e.rowIndex = rowMap[rName2];
				components.txPhase[tl].posN2Col = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
				e.colIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameP2];
				components.txPhase[tl].posN2Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if(!n2GND) {
				e.colIndex = columnMap[cNameN2];
				components.txPhase[tl].negN2Col = e.colIndex;
				e.rowIndex = rowMap[rName2];
				e.value = -1;
				mElements.push_back(e);
				e.colIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameN2];
				components.txPhase[tl].negN2Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap[cName1];
			e.rowIndex = rowMap[rName1];
			components.txPhase[tl].curN1Col = e.colIndex;
			components.txPhase[tl].curN1Row = e.rowIndex;
			e.value = -(M_PI * iObj.transSim.prstep * components.txPhase[tl].value) / (PHI_ZERO);
			mElements.push_back(e);
			e.label = label;
			e.colIndex = columnMap[cName2];
			e.rowIndex = rowMap[rName2];
			components.txPhase[tl].curN2Col = e.colIndex;
			components.txPhase[tl].curN2Row = e.rowIndex;			
			e.value = -(M_PI * iObj.transSim.prstep * components.txPhase[tl].value) / (PHI_ZERO);
			mElements.push_back(e);
		}
		/*********************/
		/** MUTUAL COUPLING **/
		/*********************/
		else if (i.first[0] == 'K') {
			components.mutualInductanceLines.push_back(i);
		}
	}
	double mutualL = 0.0, cf = 0.0;
	for (auto i : components.mutualInductanceLines) {
		devicetokens = Misc::tokenize_space(i.first);
		try {
			label = devicetokens[0];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
			if (iObj.parameters.parsedParams.count(std::make_pair(devicetokens[3], i.second)) != 0)
				cf = iObj.parameters.parsedParams.at(std::make_pair(devicetokens[3], i.second));
			else
				cf = Misc::modifier(devicetokens[3]);
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MUT_ERROR, i.first);
		}
		std::string ind1, ind2;
		ind1 = devicetokens[1];
		ind2 = devicetokens[2];
		mutualL = cf * sqrt(components.phaseInd[ind1].value * components.phaseInd[ind2].value);
		matrix_element e;
		e.label = label;
		e.colIndex = components.phaseInd[ind2].curNCol;
		e.rowIndex = components.phaseInd[ind1].curNRow;
		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
		components.phaseInd[ind1].mutPtr = mElements.size();
		mElements.push_back(e);
		e.label = label;
		e.colIndex = components.phaseInd[ind1].curNCol;
		e.rowIndex = components.phaseInd[ind2].curNRow;
		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
		components.phaseInd[ind2].mutPtr = mElements.size();
		mElements.push_back(e);
	}
	std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
	std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
	std::transform(
		rowMapFlip.begin(),
		rowMapFlip.end(),
		back_inserter(rowNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
	std::transform(
		columnMapFlip.begin(),
		columnMapFlip.end(),
		back_inserter(columnNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
}

void
Matrix::create_CSR() {
	Nsize = rowNames.size();
	Msize = columnNames.size();
	std::vector<std::map<int,double>> aMat(rowNames.size());
	colind.clear();
	nzval.clear();
	rowptr.clear();
	rowptr.push_back(0);
	for (auto i : mElements){
		aMat[i.rowIndex][i.colIndex] += i.value;
	}
	for(auto i : aMat) {
		for (auto j : i) {
			nzval.push_back(j.second);
			colind.push_back(j.first);
		}
		rowptr.push_back(rowptr.back() + i.size());
	}
}

// void
// Matrix::matrix_A(InputFile& iFile) {
// 	if(cArg.analysisT == VANALYSIS) Matrix::create_A_matrix_volt(iFile);
// 	else if(cArg.analysisT == PANALYSIS) Matrix::create_A_matrix_phase(iFile);
// 	if (iFile.matA.rowNames.size() != iFile.matA.columnNames.size())
// 		Errors::matrix_errors(NON_SQUARE,
// 			std::to_string(iFile.matA.columnNames.size()) + "x" +
// 			std::to_string(iFile.matA.rowNames.size()));
// 	if (cArg.verbose)
// 		std::cout << std::setw(35) << std::left
// 		<< "A matrix dimensions: " << iFile.matA.columnNames.size() << "\n\n";
// 	csr_A_matrix(iFile);
// }

// void
// Matrix::create_A_matrix_volt(InputFile& iFile) {
// 	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
// 	std::vector<std::string> devicetokens, componentLabels;
// 	devicetokens.clear();
// 	componentLabels.clear();
// 	std::string label, nodeP, nodeN, subckt = "";
// 	std::unordered_map<std::string, int> rowMap, columnMap;
// 	rowMap.clear();
// 	columnMap.clear();
// 	int rowCounter, colCounter, expStart, expEnd;
// 	bool pGND, nGND;
// 	rowCounter = colCounter = 0;
// 	for (auto i : iFile.maincircuitSegment) {
// 		expStart = expEnd = -1;
// 		devicetokens = Misc::tokenize_space(i);
// 		for (int t = 0; t < devicetokens.size(); t++) {
// 			if(devicetokens[t].find('{') != std::string::npos) {
// 				expStart = t;
// 			}
// 			if(devicetokens[t].find('}') != std::string::npos) {
// 				expEnd = t;
// 			}
// 		}
// 		if(expStart == -1 && expEnd != -1) Errors::invalid_component_errors(INVALID_EXPR, i);
// 		else if(expStart != -1 && expEnd == -1) Errors::invalid_component_errors(INVALID_EXPR, i);
// 		if (expStart != -1 && expStart == expEnd) {
// 			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
// 			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
// 			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_return_expression(devicetokens[expStart]), 25);
// 		}
// 		else if (expStart != -1 && expEnd != -1) {
// 			int d = expStart + 1;
// 			while (expStart != expEnd) {
// 				devicetokens[expStart] += devicetokens[d];
// 				devicetokens.erase(devicetokens.begin()+d);
// 				expEnd--;
// 			}
// 			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
// 			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
// 			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_return_expression(devicetokens[expStart]), 25);
// 		}
// 		double value = 0.0;
// 		try {
// 			label = devicetokens[0];
// 			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
// 				componentLabels.end())
// 				componentLabels.push_back(label);
// 			else {
// 				Errors::invalid_component_errors(DUPLICATE_LABEL, label);
// 			}
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_LABEL, i);
// 		}
// 		if (label.find_first_of("|") != std::string::npos) {
// 			subckt = iFile.subcircuitNameMap.at(label.substr(label.find_first_of("|") + 1));
// 		}
// 		else subckt = "";
// 		try {
// 			nodeP = devicetokens[1];
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_PNODE, i);
// 		}
// 		try {
// 			nodeN = devicetokens[2];
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_NNODE, i);
// 		}
// 		/**************/
// 		/** RESISTOR **/
// 		/**************/
// 		if (i[0] == 'R') {
// 			std::string R = label;
// 			matrix_element e;
// 			try {
// 				if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					value = Misc::modifier(devicetokens[3]);
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(RES_ERROR, i);
// 			}
// 			iFile.voltRes[R].label = R;
// 			iFile.voltRes[R].value = value;
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.voltRes[R].posNodeR = rNameP;
// 				iFile.voltRes[R].posNodeC = cNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.voltRes[R].negNodeR = rNameN;
// 				iFile.voltRes[R].negNodeC = cNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.voltRes[R].posNRow = rowMap[rNameP];
// 				iFile.voltRes[R].posNCol = columnMap[cNameP];
// 				e.value = 1 / value;
// 				iFile.voltRes[R].ppPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND) {
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rNameP];
// 					iFile.voltRes[R].posNRow = rowMap[rNameP];
// 					iFile.voltRes[R].negNCol = columnMap[cNameN];
// 					e.value = -1 / value;
// 					iFile.voltRes[R].pnPtr = iFile.matA.mElements.size();
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameP];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltRes[R].negNRow = rowMap[rNameN];
// 					iFile.voltRes[R].posNCol = columnMap[cNameP];
// 					e.value = -1 / value;
// 					iFile.voltRes[R].npPtr = iFile.matA.mElements.size();
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltRes[R].negNRow = rowMap[rNameN];
// 					iFile.voltRes[R].negNCol = columnMap[cNameN];
// 					e.value = 1 / value;
// 					iFile.voltRes[R].nnPtr = iFile.matA.mElements.size();
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.voltRes[R].negNRow = rowMap[rNameN];
// 				iFile.voltRes[R].negNCol = columnMap[cNameN];
// 				e.value = 1 / value;
// 				iFile.voltRes[R].nnPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 			}
// 		}
// 		/***************/
// 		/** CAPACITOR **/
// 		/***************/
// 		else if (i[0] == 'C') {
// 			std::string C = label;
// 			matrix_element e;
// 			try {
// 				if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					value = Misc::modifier(devicetokens[3]);
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(CAP_ERROR, i);
// 			}
// 			iFile.voltCap[C].label = devicetokens[0];
// 			iFile.voltCap[C].value = value;
// 			cName = "C_I" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			iFile.voltCap[C].curNodeC = cName;
// 			iFile.voltCap[C].curNodeR = rName;
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.voltCap[C].posNodeC = cNameP; 
// 				iFile.voltCap[C].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.voltCap[C].negNodeC = cNameN;
// 				iFile.voltCap[C].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltCap[C].posNCol = e.colIndex;
// 				iFile.voltCap[C].curNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.voltCap[C].curNCol = e.colIndex;
// 				iFile.voltCap[C].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND) {
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rName];
// 					iFile.voltCap[C].negNCol = e.colIndex;
// 					iFile.voltCap[C].curNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cName];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltCap[C].curNCol = e.colIndex;
// 					iFile.voltCap[C].negNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltCap[C].negNCol = e.colIndex;
// 				iFile.voltCap[C].curNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.voltCap[C].curNCol = e.colIndex;
// 				iFile.voltCap[C].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.voltCap[C].curNCol = e.colIndex;
// 			iFile.voltCap[C].curNRow = e.rowIndex;
// 			e.value = iFile.tsim.prstep / (2 * value);
// 			iFile.voltCap[C].capPtr = iFile.matA.mElements.size();
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/**************/
// 		/** INDUCTOR **/
// 		/**************/
// 		else if (i[0] == 'L') {
// 			std::string L = label;
// 			matrix_element e;
// 			try {
// 				if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					value = Misc::modifier(devicetokens[3]);
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(IND_ERROR, i);
// 			}
// 			iFile.voltInd[L].label = label;
// 			iFile.voltInd[L].value = value;
// 			cName = "C_I" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			iFile.voltInd[L].curNodeC = cName;
// 			iFile.voltInd[L].curNodeR = rName;
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.voltInd[L].posNodeC = cNameP;
// 				iFile.voltInd[L].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.voltInd[L].negNodeC = cNameN;
// 				iFile.voltInd[L].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.voltInd[L].curNCol = e.colIndex;
// 				iFile.voltInd[L].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltInd[L].posNCol = e.colIndex;
// 				iFile.voltInd[L].curNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND) {
// 					e.label = label;
// 					e.colIndex = columnMap[cName];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltInd[L].curNCol = e.colIndex;
// 					iFile.voltInd[L].negNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rName];
// 					iFile.voltInd[L].negNCol = e.colIndex;
// 					iFile.voltInd[L].curNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.voltInd[L].curNCol = e.colIndex;
// 				iFile.voltInd[L].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltInd[L].negNCol = e.colIndex;
// 				iFile.voltInd[L].curNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.voltInd[L].curNCol = e.colIndex;
// 			iFile.voltInd[L].curNRow = e.rowIndex;
// 			e.value = (-2 * value) / iFile.tsim.prstep;
// 			iFile.voltInd[L].indPtr = iFile.matA.mElements.size();
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/********************/
// 		/** VOLTAGE SOURCE **/
// 		/********************/
// 		else if (i[0] == 'V') {
// 			std::string V = label;
// 			matrix_element e;
// 			iFile.matA.sources[label] = Misc::function_parse(i, iFile, subckt);
// 			cName = "C_" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			iFile.voltVs[V].curNodeC = cName;
// 			iFile.voltVs[V].curNodeR = rName;
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.voltVs[V].posNodeC = cNameP;
// 				iFile.voltVs[V].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.voltVs[V].negNodeC = cNameN;
// 				iFile.voltVs[V].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.voltVs[V].curNCol = e.colIndex;
// 				iFile.voltVs[V].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltVs[V].posNCol = e.colIndex;
// 				iFile.voltVs[V].curNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND) {
// 					e.label = label;
// 					e.colIndex = columnMap[cName];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltVs[V].curNCol = e.colIndex;
// 					iFile.voltVs[V].negNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rName];
// 					iFile.voltVs[V].negNCol = e.colIndex;
// 					iFile.voltVs[V].curNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.voltVs[V].curNCol = e.colIndex;
// 				iFile.voltVs[V].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltVs[V].negNCol = e.colIndex;
// 				iFile.voltVs[V].curNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 		}
// 		/********************/
// 		/** CURRENT SOURCE **/
// 		/********************/
// 		else if (i[0] == 'I') {
// 			iFile.matA.sources[label] = Misc::function_parse(i, iFile, subckt);
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 		}
// 		/************************/
// 		/** JOSEPHSON JUNCTION **/
// 		/************************/
// 		else if (i[0] == 'B') {
// 			std::string jj = label;
// 			matrix_element e;
// 			std::string modName = "", area = "";
// 			for (int t = devicetokens.size() - 1; t > 2; t--) {
// 				if (devicetokens[t].find("=") == std::string::npos) {
// 					if (iFile.models.count(devicetokens[t]) != 0) {
// 						modName = devicetokens[t];
// 						break;
// 					}
// 					else if (iFile.models.count(
// 							devicetokens[t].substr(devicetokens[t].find("|") + 1,
// 													devicetokens[t].size() - 1))
// 							 != 0) { 
// 						modName = devicetokens[t].substr(devicetokens[t].find("|") + 1,
// 													devicetokens[t].size() - 1);
// 						break;
// 					}
// 					else { Errors::invalid_component_errors(MODEL_NOT_DEFINED,
// 														  devicetokens[t]);
// 							break;
// 					}
// 				}
// 				if (devicetokens[t].find("AREA") != std::string::npos) {
// 					area = devicetokens[t].substr(
// 						devicetokens[t].find_first_of('=') + 1,
// 						devicetokens[t].size() - 1);
// 				}
// 			}
// 			if(area == "" && cArg.verbose) Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, "");
// 			iFile.voltJJ[jj].label = jj;
// 			Component::jj_comp(modName, area, jj, subckt);
// 			iFile.voltJJ[jj].label = label;
// 			cName = "C_P" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			iFile.voltJJ[jj].phaseNodeC = cName;
// 			iFile.voltJJ[jj].phaseNodeR = rName;
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.voltJJ[jj].posNodeC = cNameP;
// 				iFile.voltJJ[jj].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.voltJJ[jj].negNodeC = cNameN;
// 				iFile.voltJJ[jj].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.voltJJ[jj].posNCol = e.colIndex;
// 				iFile.voltJJ[jj].posNRow = e.rowIndex;
// 				e.value = ((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0);
// 				iFile.voltJJ[jj].ppPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltJJ[jj].posNCol = e.colIndex;
// 				iFile.voltJJ[jj].phaseNRow = e.rowIndex;
// 				e.value = (-iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND) {
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rNameP];
// 					iFile.voltJJ[jj].negNCol = e.colIndex;
// 					iFile.voltJJ[jj].posNRow = e.rowIndex;
// 					e.value = -(((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0));
// 					iFile.voltJJ[jj].npPtr = iFile.matA.mElements.size();
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameP];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltJJ[jj].posNCol = e.colIndex;
// 					iFile.voltJJ[jj].negNRow = e.rowIndex;
// 					e.value = -(((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0));
// 					iFile.voltJJ[jj].pnPtr = iFile.matA.mElements.size();
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.voltJJ[jj].negNCol = e.colIndex;
// 					iFile.voltJJ[jj].negNRow = e.rowIndex;
// 					e.value = ((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0);
// 					iFile.voltJJ[jj].nnPtr = iFile.matA.mElements.size();
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rName];
// 					iFile.voltJJ[jj].negNCol = e.colIndex;
// 					iFile.voltJJ[jj].phaseNRow = e.rowIndex;
// 					e.value = (iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.voltJJ[jj].negNCol = e.colIndex;
// 				iFile.voltJJ[jj].posNRow = e.rowIndex;
// 				e.value = -(((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0));
// 				iFile.voltJJ[jj].npPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.voltJJ[jj].posNCol = e.colIndex;
// 				iFile.voltJJ[jj].negNRow = e.rowIndex;
// 				e.value = -(((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0));
// 				iFile.voltJJ[jj].pnPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.voltJJ[jj].negNCol = e.colIndex;
// 				iFile.voltJJ[jj].negNRow = e.rowIndex;
// 				e.value = ((2 * iFile.voltJJ.at(jj).C) / iFile.tsim.prstep) + (1 / iFile.voltJJ.at(jj).r0);
// 				iFile.voltJJ[jj].nnPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.voltJJ[jj].negNCol = e.colIndex;
// 				iFile.voltJJ[jj].phaseNRow = e.rowIndex;
// 				e.value = (iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.voltJJ[jj].phaseNCol = e.colIndex;
// 			iFile.voltJJ[jj].phaseNRow = e.rowIndex;
// 			e.value = 1;
// 			iFile.matA.mElements.push_back(e);
// 			iFile.voltJJ[jj].gLarge = iFile.voltJJ.at(jj).iC / (iFile.voltJJ.at(jj).iCFact * iFile.voltJJ.at(jj).delV);
// 			iFile.voltJJ[jj].lowerB = iFile.voltJJ.at(jj).vG - 0.5*iFile.voltJJ.at(jj).delV;
// 			iFile.voltJJ[jj].upperB = iFile.voltJJ.at(jj).vG + 0.5 * iFile.voltJJ.at(jj).delV;
// 			iFile.voltJJ[jj].subCond = 1 / iFile.voltJJ.at(jj).r0 + ((2*iFile.voltJJ.at(jj).C) / iFile.tsim.prstep);
// 			iFile.voltJJ[jj].transCond = iFile.voltJJ.at(jj).gLarge + ((2*iFile.voltJJ.at(jj).C) / iFile.tsim.prstep);
// 			iFile.voltJJ[jj].normalCond = 1 / iFile.voltJJ.at(jj).rN + ((2*iFile.voltJJ.at(jj).C) / iFile.tsim.prstep);
// 			iFile.voltJJ[jj].Del0 = 1.76 * BOLTZMANN * iFile.voltJJ.at(jj).tC;
// 			iFile.voltJJ[jj].Del = iFile.voltJJ.at(jj).Del0 * sqrt(cos((M_PI/2) * 
// 				(iFile.voltJJ.at(jj).T/iFile.voltJJ.at(jj).tC) * (iFile.voltJJ.at(jj).T/iFile.voltJJ.at(jj).tC)));
// 			iFile.voltJJ[jj].rNCalc = ((M_PI * iFile.voltJJ.at(jj).Del) / (2 * EV * iFile.voltJJ.at(jj).iC)) * 
// 				tanh(iFile.voltJJ.at(jj).Del / (2 * BOLTZMANN * iFile.voltJJ.at(jj).T));
// 			iFile.voltJJ[jj].iS = -iFile.voltJJ.at(jj).iC * sin(iFile.voltJJ.at(jj).phi0);
// 		}
// 		/***********************/
// 		/** TRANSMISSION LINE **/
// 		/***********************/
// 		else if (i[0] == 'T') {
// 			std::string Tx = label, cName2, rName2, cNameP2, rNameP2, cNameN2, 
// 				rNameN2, nodeP2, nodeN2;
// 			bool pGND2, nGND2;
// 			matrix_element e;
// 			double z0 = 10, tD = 0.0;
// 			for (int t = 5; t < devicetokens.size(); t++) {
// 				if(devicetokens[t].find("TD") != std::string::npos)
// 					tD = Parser::parse_return_expression(
// 						devicetokens[t].substr(devicetokens[t].find("TD=") + 3,
// 						devicetokens[t].size() - 1));
// 				else if(devicetokens[t].find("Z0") != std::string::npos)
// 					z0 = Parser::parse_return_expression(
// 						devicetokens[t].substr(devicetokens[t].find("Z0=") + 3,
// 						devicetokens[t].size() - 1));
// 				else if(devicetokens[t].find("LOSSLESS") != std::string::npos) {}
// 				else Errors::invalid_component_errors(INVALID_TX_DEFINED, i);
// 			}
// 			iFile.txLine[Tx].label = label;
// 			iFile.txLine[Tx].k = tD / iFile.tsim.prstep;
// 			iFile.txLine[Tx].value = z0;
// 			cName = "C_I1" + devicetokens[0];
// 			rName = "R_" + devicetokens[0] + "-I1";
// 			cName2 = "C_I2" + devicetokens[0];
// 			rName2 = "R_" + devicetokens[0] + "-I2";
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			if (rowMap.count(rName2) == 0) {
// 				rowMap[rName2] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName2) == 0) {
// 				columnMap[cName2] = colCounter;
// 				colCounter++;
// 			}
// 			iFile.txLine[Tx].curNode1C = cName;
// 			iFile.txLine[Tx].curNode1R = rName;
// 			iFile.txLine[Tx].curNode2C = cName2;
// 			iFile.txLine[Tx].curNode2R = rName2;
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NV" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.txLine[Tx].posNodeC = cNameP;
// 				iFile.txLine[Tx].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NV" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.txLine[Tx].negNodeC = cNameN;
// 				iFile.txLine[Tx].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			try {
// 				nodeP2 = devicetokens[3];
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(MISSING_PNODE, i);
// 			}
// 			/* Check if negative node exists, if not it's a bad device line definition
// 			*/
// 			try {
// 				nodeN2 = devicetokens[4];
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(MISSING_NNODE, i);
// 			}
// 			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
// 				cNameP2 = "C_NV" + nodeP2;
// 				rNameP2 = "R_N" + nodeP2;
// 				iFile.txLine[Tx].posNode2C = cNameP2;
// 				iFile.txLine[Tx].posNode2R = rNameP2;
// 				if (rowMap.count(rNameP2) == 0) {
// 					rowMap[rNameP2] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP2) == 0) {
// 					columnMap[cNameP2] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP2].push_back(label);
// 				pGND2 = false;
// 			}
// 			else
// 				pGND2 = true;
// 			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
// 				cNameN2 = "C_NV" + nodeN2;
// 				rNameN2 = "R_N" + nodeN2;
// 				iFile.txLine[Tx].negNode2C = cNameN2;
// 				iFile.txLine[Tx].negNode2R = rNameN2;
// 				if (rowMap.count(rNameN2) == 0) {
// 					rowMap[rNameN2] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN2) == 0) {
// 					columnMap[cNameN2] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN2].push_back(label);
// 				nGND2 = false;
// 			}
// 			else
// 				nGND2 = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.txLine[Tx].curN1Col = e.colIndex;
// 				iFile.txLine[Tx].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.txLine[Tx].posNCol = e.colIndex;
// 				iFile.txLine[Tx].curN1Row = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND) {
// 					e.label = label;
// 					e.colIndex = columnMap[cName];
// 					e.rowIndex = rowMap[rNameN];
// 					iFile.txLine[Tx].curN1Col = e.colIndex;
// 					iFile.txLine[Tx].negNRow = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN];
// 					e.rowIndex = rowMap[rName];
// 					iFile.txLine[Tx].negNCol = e.colIndex;
// 					iFile.txLine[Tx].curN1Row = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.txLine[Tx].curN1Col = e.colIndex;
// 				iFile.txLine[Tx].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.txLine[Tx].negNCol = e.colIndex;
// 				iFile.txLine[Tx].curN1Row = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.txLine[Tx].curN1Col = e.colIndex;
// 			iFile.txLine[Tx].curN1Row = e.rowIndex;
// 			e.value = -z0;
// 			iFile.matA.mElements.push_back(e);
// 			if (!pGND2) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName2];
// 				e.rowIndex = rowMap[rNameP2];
// 				iFile.txLine[Tx].curN2Col = e.colIndex;
// 				iFile.txLine[Tx].posN2Row = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP2];
// 				e.rowIndex = rowMap[rName2];
// 				iFile.txLine[Tx].posN2Col = e.colIndex;
// 				iFile.txLine[Tx].curN2Row = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				if (!nGND2) {
// 					e.label = label;
// 					e.colIndex = columnMap[cName2];
// 					e.rowIndex = rowMap[rNameN2];
// 					iFile.txLine[Tx].curN2Col = e.colIndex;
// 					iFile.txLine[Tx].negN2Row = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 					e.label = label;
// 					e.colIndex = columnMap[cNameN2];
// 					e.rowIndex = rowMap[rName2];
// 					iFile.txLine[Tx].negN2Col = e.colIndex;
// 					iFile.txLine[Tx].curN2Row = e.rowIndex;
// 					e.value = -1;
// 					iFile.matA.mElements.push_back(e);
// 				}
// 			}
// 			else if (!nGND2) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName2];
// 				e.rowIndex = rowMap[rNameN2];
// 				iFile.txLine[Tx].curN2Col = e.colIndex;
// 				iFile.txLine[Tx].negN2Row = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN2];
// 				e.rowIndex = rowMap[rName2];
// 				iFile.txLine[Tx].negN2Col = e.colIndex;
// 				iFile.txLine[Tx].curN2Row = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName2];
// 			e.rowIndex = rowMap[rName2];
// 			iFile.txLine[Tx].curN2Col = e.colIndex;
// 			iFile.txLine[Tx].curN2Row = e.rowIndex;
// 			e.value = -z0;
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/*********************/
// 		/** MUTUAL COUPLING **/
// 		/*********************/
// 		else if (i[0] == 'K') {
// 			iFile.mutualInductanceLines.push_back(i);
// 		}
// 	}
// 	double mutualL = 0.0, cf = 0.0;
// 	for (auto i : iFile.mutualInductanceLines) {
// 		devicetokens = Misc::tokenize_space(i);
// 		try {
// 			label = devicetokens[0];
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_LABEL, i);
// 		}
// 		if (label.find_first_of("|") != std::string::npos) {
// 			subckt = iFile.subcircuitNameMap.at(label.substr(label.find_first_of("|") + 1));
// 		}
// 		try {
// 			if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 				cf = iFile.paramValues.paramMap.at(devicetokens[3]);
// 			else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					cf = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 			else
// 				cf = Misc::modifier(devicetokens[3]);
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MUT_ERROR, i);
// 		}
// 		std::string ind1, ind2;
// 		ind1 = devicetokens[1];
// 		ind2 = devicetokens[2];
// 		if(iFile.voltInd.count(ind1) == 0) Errors::invalid_component_errors(MISSING_INDUCTOR, ind1);
// 		if(iFile.voltInd.count(ind2) == 0) Errors::invalid_component_errors(MISSING_INDUCTOR, ind2);
// 		cf = cf * sqrt(iFile.voltInd.at(ind1).value * iFile.voltInd.at(ind2).value);
// 		mutualL = ((2 * cf) / iFile.tsim.prstep);
// 		iFile.voltInd.at(ind1).mut[ind2] = mutualL;
// 		iFile.voltInd.at(ind2).mut[ind1] = mutualL;
// 		matrix_element e;
// 		e.label = label;
// 		e.colIndex = iFile.voltInd.at(ind1).curNCol;
// 		e.rowIndex = iFile.voltInd.at(ind2).curNRow;
// 		e.value = -mutualL;
// 		iFile.voltInd.at(ind1).mutPtr[ind2] = iFile.matA.mElements.size();
// 		iFile.matA.mElements.push_back(e);
// 		e.label = label;
// 		e.colIndex = iFile.voltInd.at(ind2).curNCol;
// 		e.rowIndex = iFile.voltInd.at(ind1).curNRow;
// 		e.value = -mutualL;
// 		iFile.voltInd.at(ind2).mutPtr[ind1] = iFile.matA.mElements.size();
// 		iFile.matA.mElements.push_back(e);
// 	}
// 	std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
// 	std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
// 	std::transform(
// 		rowMapFlip.begin(),
// 		rowMapFlip.end(),
// 		back_inserter(iFile.matA.rowNames),
// 		[](std::pair<int, std::string> const& pair) { return pair.second; });
// 	std::transform(
// 		columnMapFlip.begin(),
// 		columnMapFlip.end(),
// 		back_inserter(iFile.matA.columnNames),
// 		[](std::pair<int, std::string> const& pair) { return pair.second; });
// }


// void
// Matrix::create_A_matrix_phase(InputFile& iFile) {
// 	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
// 	std::vector<std::string> devicetokens, componentLabels;
// 	devicetokens.clear();
// 	componentLabels.clear();
// 	std::string label, nodeP, nodeN, subckt;
// 	std::unordered_map<std::string, int> rowMap, columnMap;
// 	rowMap.clear();
// 	columnMap.clear();
// 	int rowCounter, colCounter, expStart, expEnd;
// 	bool pGND, nGND;
// 	rowCounter = colCounter = 0;
// 	/* Main circuit node identification */
// 	for (auto i : iFile.maincircuitSegment) {
// 		expStart = expEnd = -1;
// 		devicetokens = Misc::tokenize_space(i);
// 				for (int t = 0; t < devicetokens.size(); t++) {
// 			if(devicetokens[t].find('{') != std::string::npos) {
// 				expStart = t;
// 			}
// 			if(devicetokens[t].find('}') != std::string::npos) {
// 				expEnd = t;
// 			}
// 		}
// 		if(expStart == -1 && expEnd != -1) Errors::invalid_component_errors(INVALID_EXPR, i);
// 		else if(expStart != -1 && expEnd == -1) Errors::invalid_component_errors(INVALID_EXPR, i);
// 		if (expStart != -1 && expStart == expEnd) {
// 			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
// 			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
// 			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_return_expression(devicetokens[expStart]), 25);
// 		}
// 		else if (expStart != -1 && expEnd != -1) {
// 			int d = expStart + 1;
// 			while (expStart != expEnd) {
// 				devicetokens[expStart] += devicetokens[d];
// 				devicetokens.erase(devicetokens.begin()+d);
// 				expEnd--;
// 			}
// 			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
// 			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
// 			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_return_expression(devicetokens[expStart]), 25);
// 		}
// 		double value = 0.0;
// 		/* Check if label exists, if not there is a bug in the program */
// 		try {
// 			label = devicetokens[0];
// 			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
// 				componentLabels.end())
// 				componentLabels.push_back(label);
// 			else {
// 				Errors::invalid_component_errors(DUPLICATE_LABEL, label);
// 			}
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_LABEL, i);
// 		}
// 		if (label.find_first_of("|") != std::string::npos) {
// 			subckt = iFile.subcircuitNameMap.at(label.substr(label.find_first_of("|") + 1));
// 		}
// 		else subckt = "";
// 		/* Check if positive node exists, if not it's a bad device line definition
// 		 */
// 		try {
// 			nodeP = devicetokens[1];
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_PNODE, i);
// 		}
// 		/* Check if negative node exists, if not it's a bad device line definition
// 		 */
// 		try {
// 			nodeN = devicetokens[2];
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_NNODE, i);
// 		}
// 		/********************/
// 		/** PHASE RESISTOR **/
// 		/********************/
// 		if (i[0] == 'R') {
// 			std::string R = devicetokens[0];
// 			matrix_element e;
// 			try {
// 				if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					value = Misc::modifier(devicetokens[3]);
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(RES_ERROR, i);
// 			}
// 			cName = "C_I" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			iFile.phaseRes[R].curNodeC = cName;
// 			iFile.phaseRes[R].curNodeR = rName;
// 			iFile.phaseRes[R].value = value;
// 			iFile.phaseRes[R].label = R;
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.phaseRes[R].posNodeC = cNameP;
// 				iFile.phaseRes[R].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.phaseRes[R].negNodeC = cNameN;
// 				iFile.phaseRes[R].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.phaseRes[R].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseRes[R].posNCol = e.colIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.phaseRes[R].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseRes[R].negNCol = e.colIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.phaseRes[R].curNCol = e.colIndex;
// 			iFile.phaseRes[R].curNRow = e.rowIndex;
// 			e.value = -(M_PI * iFile.phaseRes[R].value * iFile.tsim.prstep) / PHI_ZERO;
// 			iFile.phaseRes[R].resPtr = iFile.matA.mElements.size();
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/*********************/
// 		/** PHASE CAPACITOR **/
// 		/*********************/
// 		else if (i[0] == 'C') {
// 			std::string C = devicetokens[0];
// 			matrix_element e;
// 			try {
// 				if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					value = Misc::modifier(devicetokens[3]);
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(CAP_ERROR, i);
// 			}
// 			cName = "C_I" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			iFile.phaseCap[C].curNodeC = cName;
// 			iFile.phaseCap[C].curNodeR = rName;
// 			iFile.phaseCap[C].value = value;
// 			iFile.phaseCap[C].label = C;
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.phaseCap[C].posNodeC = cNameP;
// 				iFile.phaseCap[C].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.phaseCap[C].negNodeC = cNameN;
// 				iFile.phaseCap[C].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.phaseCap[C].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseCap[C].posNCol = e.colIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.phaseCap[C].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseCap[C].negNCol = e.colIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.phaseCap[C].curNCol = e.colIndex;
// 			iFile.phaseCap[C].curNRow = e.rowIndex;
// 			e.value = (-2 * M_PI * iFile.tsim.prstep * iFile.tsim.prstep) / (PHI_ZERO * 4 * iFile.phaseCap[C].value);
// 			iFile.phaseCap[C].capPtr = iFile.matA.mElements.size();
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/********************/
// 		/** PHASE INDUCTOR **/
// 		/********************/
// 		else if (i[0] == 'L') {
// 			std::string L = devicetokens[0];
// 			matrix_element e;
// 			try {
// 				if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					value = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					value = Misc::modifier(devicetokens[3]);
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(IND_ERROR, i);
// 			}
// 			cName = "C_I" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			iFile.phaseInd[L].curNodeC = cName;
// 			iFile.phaseInd[L].curNodeR = rName;
// 			iFile.phaseInd[L].value = value;
// 			iFile.phaseInd[L].label = L;
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.phaseInd[L].posNodeC = cNameP;
// 				iFile.phaseInd[L].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.phaseInd[L].negNodeC = cNameN;
// 				iFile.phaseInd[L].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.phaseInd[L].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseInd[L].posNCol = e.colIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.phaseInd[L].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseInd[L].negNCol = e.colIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName];
// 			e.rowIndex = rowMap[rName];
// 			iFile.phaseInd[L].curNCol = e.colIndex;
// 			iFile.phaseInd[L].curNRow = e.rowIndex;
// 			e.value = -(iFile.phaseInd[L].value * 2 * M_PI) / PHI_ZERO;
// 			iFile.phaseInd[L].indPtr = iFile.matA.mElements.size();
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/**************************/
// 		/** PHASE VOLTAGE SOURCE **/
// 		/**************************/
// 		else if (i[0] == 'V') {
// 			std::string VS = devicetokens[0];
// 			matrix_element e;
// 			iFile.matA.sources[label] = Misc::function_parse(i, iFile, subckt);
// 			cName = "C_" + devicetokens[0];
// 			rName = "R_" + devicetokens[0];
// 			iFile.phaseVs[VS].curNodeC = cName;
// 			iFile.phaseVs[VS].curNodeR = rName;
// 			iFile.phaseVs[VS].label = VS;
// 			if (rowMap.count(rName) == 0) {
// 				rowMap[rName] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName) == 0) {
// 				columnMap[cName] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.phaseVs[VS].posNodeC = cNameP;
// 				iFile.phaseVs[VS].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.phaseVs[VS].negNodeC = cNameN;
// 				iFile.phaseVs[VS].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.phaseVs[VS].curNCol = e.colIndex;
// 				iFile.phaseVs[VS].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseVs[VS].posNCol = e.colIndex;
// 				iFile.phaseVs[VS].curNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cName];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.phaseVs[VS].curNCol = e.colIndex;
// 				iFile.phaseVs[VS].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName];
// 				iFile.phaseVs[VS].negNCol = e.colIndex;
// 				iFile.phaseVs[VS].curNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 		}
// 		/********************/
// 		/** CURRENT SOURCE **/
// 		/********************/
// 		else if (i[0] == 'I') {
// 			iFile.matA.sources[label] = Misc::function_parse(i, iFile, subckt);
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 		}
// 		/******************************/
// 		/** PHASE JOSEPHSON JUNCTION **/
// 		/******************************/
// 		else if (i[0] == 'B') {
// 			std::string cVolt, rVolt, jj;
// 			jj = devicetokens[0];
// 			matrix_element e;
// 			std::string modName = "", area = "";
// 			for (int t = devicetokens.size() - 1; t > 2; t--) {
// 				if (devicetokens[t].find("=") == std::string::npos) {
// 					if (iFile.models.count(devicetokens[t]) != 0) {
// 						modName = devicetokens[t];
// 						break;
// 					}
// 					else if (iFile.models.count(
// 							devicetokens[t].substr(devicetokens[t].find("|") + 1,
// 													devicetokens[t].size() - 1))
// 							 != 0) { 
// 						modName = devicetokens[t].substr(devicetokens[t].find("|") + 1,
// 													devicetokens[t].size() - 1);
// 						break;
// 					}
// 					else {
// 						Errors::invalid_component_errors(MODEL_NOT_DEFINED,
// 														  devicetokens[t]);
// 						break;
// 					}
// 				}
// 				if (devicetokens[t].find("AREA") != std::string::npos) {
// 					area = devicetokens[t].substr(
// 						devicetokens[t].find_first_of('=') + 1,
// 						devicetokens[t].size() - 1);
// 				}
// 			}
// 			if(area == "") Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, "");
// 			iFile.phaseJJ[jj].label = jj;
// 			Component::jj_comp_phase(modName, area, jj, subckt);
// 			cVolt = "C_V" + devicetokens[0];
// 			rVolt = "R_" + devicetokens[0];
// 			iFile.phaseJJ[jj].voltNodeC = cVolt;
// 			iFile.phaseJJ[jj].voltNodeR = rVolt;
// 			if (rowMap.count(rVolt) == 0) {
// 				rowMap[rVolt] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cVolt) == 0) {
// 				columnMap[cVolt] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.phaseJJ[jj].posNodeC = cNameP;
// 				iFile.phaseJJ[jj].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.phaseJJ[jj].negNodeC = cNameN;
// 				iFile.phaseJJ[jj].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (!pGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cVolt];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.phaseJJ[jj].posNRow = e.rowIndex;;
// 				e.value = 1 / iFile.phaseJJ[jj].r0 + ((2*iFile.phaseJJ[jj].C) / iFile.tsim.prstep);
// 				iFile.phaseJJ[jj].pPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rVolt];
// 				iFile.phaseJJ[jj].posNCol = e.colIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if (!nGND) {
// 				e.label = label;
// 				e.colIndex = columnMap[cVolt];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.phaseJJ[jj].negNRow = e.rowIndex;				
// 				e.value = -1 / iFile.phaseJJ[jj].r0 - ((2*iFile.phaseJJ[jj].C) / iFile.tsim.prstep);
// 				iFile.phaseJJ[jj].nPtr = iFile.matA.mElements.size();
// 				iFile.matA.mElements.push_back(e);
// 				e.label = label;
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rVolt];	
// 				iFile.phaseJJ[jj].negNCol = e.colIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cVolt];
// 			e.rowIndex = rowMap[rVolt];
// 			iFile.phaseJJ[jj].voltNCol = e.colIndex;
// 			iFile.phaseJJ[jj].voltNRow = e.rowIndex;
// 			e.value = -(iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
// 			iFile.matA.mElements.push_back(e);
// 			iFile.phaseJJ[jj].gLarge = iFile.phaseJJ[jj].iC / (iFile.phaseJJ[jj].iCFact * iFile.phaseJJ[jj].delV);
// 			iFile.phaseJJ[jj].lower = iFile.phaseJJ[jj].vG - 0.5*iFile.phaseJJ[jj].delV;
// 			iFile.phaseJJ[jj].upper = iFile.phaseJJ[jj].vG + 0.5 * iFile.phaseJJ[jj].delV;
// 			iFile.phaseJJ[jj].subCond = 1 / iFile.phaseJJ[jj].r0 + ((2*iFile.phaseJJ[jj].C) / iFile.tsim.prstep);
// 			iFile.phaseJJ[jj].transCond = iFile.phaseJJ[jj].gLarge + ((2*iFile.phaseJJ[jj].C) / iFile.tsim.prstep);
// 			iFile.phaseJJ[jj].normalCond = 1 / iFile.phaseJJ[jj].rN + ((2*iFile.phaseJJ[jj].C) / iFile.tsim.prstep);
// 			iFile.phaseJJ[jj].Del0 = 1.76 * BOLTZMANN * iFile.phaseJJ[jj].tC;
// 			iFile.phaseJJ[jj].Del = iFile.phaseJJ[jj].Del0 * sqrt(cos((M_PI/2) * (iFile.phaseJJ[jj].T/iFile.phaseJJ[jj].tC) * (iFile.phaseJJ[jj].T/iFile.phaseJJ[jj].tC)));
// 			iFile.phaseJJ[jj].rNCalc = ((M_PI * iFile.phaseJJ[jj].Del) / (2 * EV * iFile.phaseJJ[jj].iC)) * tanh(iFile.phaseJJ[jj].Del / (2 * BOLTZMANN * iFile.phaseJJ[jj].T));
// 			iFile.phaseJJ[jj].iS = -iFile.phaseJJ[jj].iC * sin(iFile.phaseJJ[jj].phi0);
// 		}
// 		/*****************************/
// 		/** PHASE TRANSMISSION LINE **/
// 		/*****************************/
// 		else if (i[0] == 'T') {
// 			std::string nodeP2, nodeN2, cNameP2, rNameP2, cNameN2,
// 				rNameN2, cName1, rName1, cName2,
// 				rName2, tl;
// 			bool p2GND, n2GND;
// 			tl = devicetokens[0];
// 			try {
// 				nodeP2 = devicetokens[3];
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(MISSING_PNODE, i);
// 			}
// 			try {
// 				nodeN2 = devicetokens[4];
// 			}
// 			catch (std::exception &e) {
// 				Errors::invalid_component_errors(MISSING_NNODE, i);
// 			}
// 			matrix_element e;
// 			if (devicetokens.size() < 7) {
// 				Errors::invalid_component_errors(TIME_ERROR, i);
// 			}
// 			for (size_t l = 5; l < devicetokens.size(); l++) {
// 				if (devicetokens[l].find("TD") != std::string::npos)
// 					iFile.txPhase[tl].tD = Misc::modifier((devicetokens[l]).substr(3));
// 				else if (devicetokens[l].find("Z0") != std::string::npos)
// 					iFile.txPhase[tl].value = Misc::modifier((devicetokens[l]).substr(3));
// 			}
// 			iFile.txPhase[tl].k = iFile.txPhase[tl].tD / iFile.tsim.prstep;
// 			cName1 = "C_I1" + label;
// 			rName1 = "R_" + label + "-I1";
// 			iFile.txPhase[tl].curNode1C = cName1;
// 			iFile.txPhase[tl].curNode1R = rName1;
// 			if (rowMap.count(rName1) == 0) {
// 				rowMap[rName1] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName1) == 0) {
// 				columnMap[cName1] = colCounter;
// 				colCounter++;
// 			}
// 			cName2 = "C_I2" + label;
// 			rName2 = "R_" + label + "-I2";
// 			iFile.txPhase[tl].curNode2C = cName2;
// 			iFile.txPhase[tl].curNode2R = rName2;
// 			if (rowMap.count(rName2) == 0) {
// 				rowMap[rName2] = rowCounter;
// 				rowCounter++;
// 			}
// 			if (columnMap.count(cName2) == 0) {
// 				columnMap[cName2] = colCounter;
// 				colCounter++;
// 			}
// 			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
// 				cNameP = "C_NP" + nodeP;
// 				rNameP = "R_N" + nodeP;
// 				iFile.txPhase[tl].posNodeC = cNameP;
// 				iFile.txPhase[tl].posNodeR = rNameP;
// 				if (rowMap.count(rNameP) == 0) {
// 					rowMap[rNameP] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP) == 0) {
// 					columnMap[cNameP] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP].push_back(label);
// 				pGND = false;
// 			}
// 			else
// 				pGND = true;
// 			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
// 				cNameN = "C_NP" + nodeN;
// 				rNameN = "R_N" + nodeN;
// 				iFile.txPhase[tl].negNodeC = cNameN;
// 				iFile.txPhase[tl].negNodeR = rNameN;
// 				if (rowMap.count(rNameN) == 0) {
// 					rowMap[rNameN] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN) == 0) {
// 					columnMap[cNameN] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN].push_back(label);
// 				nGND = false;
// 			}
// 			else
// 				nGND = true;
// 			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
// 				cNameP2 = "C_NP" + nodeP2;
// 				rNameP2 = "R_N" + nodeP2;
// 				iFile.txPhase[tl].posNode2C = cNameP2;
// 				iFile.txPhase[tl].posNode2R = rNameP2;
// 				if (rowMap.count(rNameP2) == 0) {
// 					rowMap[rNameP2] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameP2) == 0) {
// 					columnMap[cNameP2] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameP2].push_back(label);
// 				p2GND = false;
// 			}
// 			else
// 				p2GND = true;
// 			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
// 				cNameN2 = "C_NP" + nodeN2;
// 				rNameN2 = "R_N" + nodeN2;
// 				iFile.txPhase[tl].negNode2C = cNameN2;
// 				iFile.txPhase[tl].negNode2R = rNameN2;
// 				if (rowMap.count(rNameN2) == 0) {
// 					rowMap[rNameN2] = rowCounter;
// 					rowCounter++;
// 				}
// 				if (columnMap.count(cNameN2) == 0) {
// 					columnMap[cNameN2] = colCounter;
// 					colCounter++;
// 				}
// 				iFile.matA.nodeConnections[rNameN2].push_back(label);
// 				n2GND = false;
// 			}
// 			else
// 				n2GND = true;
// 			if (!pGND) {
// 				e.colIndex = columnMap[cNameP];
// 				e.rowIndex = rowMap[rName1];
// 				iFile.txPhase[tl].posNCol = e.colIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.colIndex = columnMap[cName1];
// 				e.rowIndex = rowMap[rNameP];
// 				iFile.txPhase[tl].posNRow = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if(!nGND) {
// 				e.colIndex = columnMap[cNameN];
// 				e.rowIndex = rowMap[rName1];
// 				iFile.txPhase[tl].negNCol = e.colIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.colIndex = columnMap[cName1];
// 				e.rowIndex = rowMap[rNameN];
// 				iFile.txPhase[tl].negNRow = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if (!p2GND) {
// 				e.colIndex = columnMap[cNameP2];
// 				e.rowIndex = rowMap[rName2];
// 				iFile.txPhase[tl].posN2Col = e.colIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 				e.colIndex = columnMap[cName2];
// 				e.rowIndex = rowMap[rNameP2];
// 				iFile.txPhase[tl].posN2Row = e.rowIndex;
// 				e.value = 1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			if(!n2GND) {
// 				e.colIndex = columnMap[cNameN2];
// 				iFile.txPhase[tl].negN2Col = e.colIndex;
// 				e.rowIndex = rowMap[rName2];
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 				e.colIndex = columnMap[cName2];
// 				e.rowIndex = rowMap[rNameN2];
// 				iFile.txPhase[tl].negN2Row = e.rowIndex;
// 				e.value = -1;
// 				iFile.matA.mElements.push_back(e);
// 			}
// 			e.label = label;
// 			e.colIndex = columnMap[cName1];
// 			e.rowIndex = rowMap[rName1];
// 			iFile.txPhase[tl].curN1Col = e.colIndex;
// 			iFile.txPhase[tl].curN1Row = e.rowIndex;
// 			e.value = -(M_PI * iFile.tsim.prstep * iFile.txPhase[tl].value) / (PHI_ZERO);
// 			iFile.matA.mElements.push_back(e);
// 			e.label = label;
// 			e.colIndex = columnMap[cName2];
// 			e.rowIndex = rowMap[rName2];
// 			iFile.txPhase[tl].curN2Col = e.colIndex;
// 			iFile.txPhase[tl].curN2Row = e.rowIndex;			
// 			e.value = -(M_PI * iFile.tsim.prstep * iFile.txPhase[tl].value) / (PHI_ZERO);
// 			iFile.matA.mElements.push_back(e);
// 		}
// 		/*********************/
// 		/** MUTUAL COUPLING **/
// 		/*********************/
// 		else if (i[0] == 'K') {
// 			iFile.mutualInductanceLines.push_back(i);
// 		}
// 	}
// 	double mutualL = 0.0, cf = 0.0;
// 	for (auto i : iFile.mutualInductanceLines) {
// 		devicetokens = Misc::tokenize_space(i);
// 		try {
// 			label = devicetokens[0];
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MISSING_LABEL, i);
// 		}
// 		if (label.find_first_of("|") != std::string::npos) {
// 			subckt = iFile.subcircuitNameMap.at(label.substr(label.find_first_of("|") + 1));
// 		}
// 		try {
// 			if (iFile.paramValues.paramMap.count(devicetokens[3]) != 0)
// 					cf = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else if (iFile.paramValues.paramMap.count(devicetokens[3] + "|" + subckt) != 0)
// 					cf = iFile.paramValues.returnParam(devicetokens[3], subckt);
// 				else
// 					cf = Misc::modifier(devicetokens[3]);
// 		}
// 		catch (std::exception &e) {
// 			Errors::invalid_component_errors(MUT_ERROR, i);
// 		}
// 		std::string ind1, ind2;
// 		ind1 = devicetokens[1];
// 		ind2 = devicetokens[2];
// 		mutualL = cf * sqrt(iFile.phaseInd[ind1].value * iFile.phaseInd[ind2].value);
// 		matrix_element e;
// 		e.label = label;
// 		e.colIndex = iFile.phaseInd[ind2].curNCol;
// 		e.rowIndex = iFile.phaseInd[ind1].curNRow;
// 		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
// 		iFile.phaseInd[ind1].mutPtr = iFile.matA.mElements.size();
// 		iFile.matA.mElements.push_back(e);
// 		e.label = label;
// 		e.colIndex = iFile.phaseInd[ind1].curNCol;
// 		e.rowIndex = iFile.phaseInd[ind2].curNRow;
// 		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
// 		iFile.phaseInd[ind2].mutPtr = iFile.matA.mElements.size();
// 		iFile.matA.mElements.push_back(e);
// 	}
// 	std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
// 	std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
// 	std::transform(
// 		rowMapFlip.begin(),
// 		rowMapFlip.end(),
// 		back_inserter(iFile.matA.rowNames),
// 		[](std::pair<int, std::string> const& pair) { return pair.second; });
// 	std::transform(
// 		columnMapFlip.begin(),
// 		columnMapFlip.end(),
// 		back_inserter(iFile.matA.columnNames),
// 		[](std::pair<int, std::string> const& pair) { return pair.second; });
// }


// void Matrix::csr_A_matrix(InputFile& iFile) {
// 	iFile.matA.Nsize = iFile.matA.rowNames.size();
// 	iFile.matA.Msize = iFile.matA.columnNames.size();
// 	std::vector<std::map<int,double>> aMat(iFile.matA.rowNames.size());
// 	iFile.matA.colind.clear();
// 	iFile.matA.nzval.clear();
// 	iFile.matA.rowptr.clear();
// 	iFile.matA.rowptr.push_back(0);
// 	for (auto i : iFile.matA.mElements){
// 		aMat[i.rowIndex][i.colIndex] += i.value;
// 	}
// 	for(auto i : aMat) {
// 		for (auto j : i) {
// 			iFile.matA.nzval.push_back(j.second);
// 			iFile.matA.colind.push_back(j.first);
// 		}
// 		iFile.matA.rowptr.push_back(iFile.matA.rowptr.back() + i.size());
// 	}	
// }