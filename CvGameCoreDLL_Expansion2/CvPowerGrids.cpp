#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"
#include "ICvDLLUserInterface.h"
#include "CvGameCoreUtils.h"
#include "CvInfosSerializationHelper.h"
#include "cvStopWatch.h"
#include "CvPowerGrids.h"

// must be included after all other headers
#include "LintFree.h"

#if defined(MOD_GLOBAL_POWER)
//--------------------------------------------------------------------------------
/// Constructor
CvPlayerPowerGrids::CvPlayerPowerGrids(void) :

m_pPlayer(NULL),
m_powerGrids(),
m_cities()
{
}
//--------------------------------------------------------------------------------
/// Destructor
CvPlayerPowerGrids::~CvPlayerPowerGrids(void)
{
	Uninit();
}
//--------------------------------------------------------------------------------
/// Init
void CvPlayerPowerGrids::Init(CvPlayer* pPlayer)
{
	Reset();

	m_pPlayer = pPlayer;
}
//--------------------------------------------------------------------------------
/// Cleanup
void CvPlayerPowerGrids::Uninit()
{
	m_pPlayer = NULL;
	m_powerGrids.clear();
	m_cities.clear();
}
//--------------------------------------------------------------------------------
/// Reset
void CvPlayerPowerGrids::Reset(void)
{
}
//--------------------------------------------------------------------------------
/// Serialization read
void CvPlayerPowerGrids::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	size_t nItems;
	MOD_SERIALIZE_READ(89, kStream, nItems, 0);
	m_powerGrids.clear();
	for (size_t i = 0; i < nItems; i++)
	{
		int temp;
		MOD_SERIALIZE_READ(89, kStream, temp, 0);
		size_t nItems2;
		MOD_SERIALIZE_READ(89, kStream, nItems2, 0);
		for (size_t j = 0; j < nItems2; j++)
		{
			int temp2;
			MOD_SERIALIZE_READ(89, kStream, temp2, 0);
			m_powerGrids[temp].aCityList.push_back(temp2);
		}
		int temp2;
		MOD_SERIALIZE_READ(89, kStream, temp2, 0);
		m_powerGrids[temp].iGeneration = temp2;
		MOD_SERIALIZE_READ(89, kStream, temp2, 0);
		m_powerGrids[temp].iConsumption = temp2;
	}

	MOD_SERIALIZE_READ(89, kStream, nItems, 0);
	m_cities.clear();
	for (size_t i = 0; i < nItems; i++)
	{
		int iKey;
		int iValue;
		MOD_SERIALIZE_READ(89, kStream, iKey, 0);
		MOD_SERIALIZE_READ(89, kStream, iValue, 0);
		m_cities[iKey] = iValue;
	}
}
//--------------------------------------------------------------------------------
/// Serialization write
void CvPlayerPowerGrids::Write(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion = 1;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	MOD_SERIALIZE_WRITE(kStream, m_powerGrids.size());
	for (AllPowerGridsStore::const_iterator it = m_powerGrids.begin(); it != m_powerGrids.end(); ++it)
	{
		MOD_SERIALIZE_WRITE(kStream, it->first);
		MOD_SERIALIZE_WRITE(kStream, it->second.aCityList.size());
		for (std::vector<int>::const_iterator it2 = it->second.aCityList.begin(); it2 != it->second.aCityList.end(); ++it2)
		{
			MOD_SERIALIZE_WRITE(kStream, *it2);
		}
		MOD_SERIALIZE_WRITE(kStream, it->second.iGeneration);
		MOD_SERIALIZE_WRITE(kStream, it->second.iConsumption);
	}
	MOD_SERIALIZE_WRITE(kStream, m_cities.size());
	for (AllCitiesStore::const_iterator it = m_cities.begin(); it != m_cities.end(); ++it)
	{
		MOD_SERIALIZE_WRITE(kStream, it->first);
		MOD_SERIALIZE_WRITE(kStream, it->second);
	}
}
//--------------------------------------------------------------------------------
// Fun stuff below
//--------------------------------------------------------------------------------
/// Assign the player's cities to the correct power grids. Cities can import/export power between cities in one grid only. Should call right after city connections are updated.
void CvPlayerPowerGrids::UpdateCitiesInPowerGrids()
{
	CvTeam& pTeam = GET_TEAM(m_pPlayer->getTeam());

	if (pTeam.CanUsePower())
	{
		if (pTeam.CanTrasmitPowerByLand() || pTeam.CanTrasmitPowerByWater())
		{
			int iPowerGridNew = 0;
			int iPowerGridA = 0;

			CvCity* pLoopCityA;
			CvCity* pLoopCityB;
			int iCityLoopA;
			int iCityLoopB;
			int iCityA;
			int iCityB;

			m_powerGrids.clear();
			m_cities.clear();

			for (pLoopCityA = m_pPlayer->firstCity(&iCityLoopA); pLoopCityA != NULL; pLoopCityA = m_pPlayer->nextCity(&iCityLoopA))
			{
				if (
					pLoopCityA->getOwner() != m_pPlayer->GetID()
					)
				{
					continue;
				}
				iCityA = pLoopCityA->GetID();
				if (m_cities.find(iCityA) == m_cities.end()) // assign a power grid if city doesnt havent one
				{
					m_powerGrids[iPowerGridNew].aCityList.push_back(iCityA);
					m_cities[iCityA] = iPowerGridNew;
					iPowerGridA = iPowerGridNew;
					iPowerGridNew++;
				}
				else
				{
					iPowerGridA = m_cities.find(iCityA)->second;
				}

				iCityLoopB = iCityLoopA;
				for (pLoopCityB = m_pPlayer->nextCity(&iCityLoopB); pLoopCityB != NULL; pLoopCityB = m_pPlayer->nextCity(&iCityLoopB))
				{
					if (
						pLoopCityA == pLoopCityB
						|| pLoopCityB->getOwner() != m_pPlayer->GetID()
						)
					{
						continue;
					}

					if (
						(pTeam.CanTrasmitPowerByLand() && m_pPlayer->GetCityConnections()->AreCitiesDirectlyConnected(pLoopCityA, pLoopCityB, CvCityConnections::CONNECTION_ANY_LAND))
						|| (pTeam.CanTrasmitPowerByWater() && m_pPlayer->GetCityConnections()->AreCitiesDirectlyConnected(pLoopCityA, pLoopCityB, CvCityConnections::CONNECTION_HARBOR))
						|| (pLoopCityA->GetCityPower()->CanTransmitPowerByWater() && pLoopCityB->GetCityPower()->CanTransmitPowerByWater() && m_pPlayer->GetCityConnections()->AreCitiesDirectlyConnected(pLoopCityA, pLoopCityB, CvCityConnections::CONNECTION_HARBOR))
						)
					{
						iCityB = pLoopCityB->GetID();
						AllCitiesStore::const_iterator it1 = m_cities.find(iCityB);
						if (it1 == m_cities.end()) // does not belong in a power grid yet, we only need to assign this city to the current A's grid
						{
							m_powerGrids[iPowerGridA].aCityList.push_back(pLoopCityB->GetID());
							m_cities[iCityB] = iPowerGridA;
						}
						else if (it1->second != iPowerGridA) // belongs in a grid other than the current A's already, we need to move all the cities from that grid into the current A's grid
						{
							AllPowerGridsStore::const_iterator itGrid = m_powerGrids.find(it1->second);
							if (itGrid != m_powerGrids.end())
							{
								for (std::vector<int>::const_iterator itCity = itGrid->second.aCityList.begin(); itCity != itGrid->second.aCityList.end(); ++itCity)
								{
									m_powerGrids[iPowerGridA].aCityList.push_back(*itCity);
									m_powerGrids.erase(it1->second);
									m_cities[*itCity] = iPowerGridA;
								}
							}
						}
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------------
/// Iterate through each power grid, and update imports and exports
void CvPlayerPowerGrids::UpdatePowerInPowerGrids()
{
	for (AllPowerGridsStore::const_iterator itGrid = m_powerGrids.begin(); itGrid != m_powerGrids.end(); ++itGrid)
	{
		UpdatePowerInPowerGrid(itGrid->first);
	}
}
//--------------------------------------------------------------------------------
/// Update imports and exports for one power grid
void CvPlayerPowerGrids::UpdatePowerInPowerGrid(int iPowerGrid)
{
	// reset power values for the power grid before re-calculating everything
	m_powerGrids[iPowerGrid].iGeneration = 0;
	m_powerGrids[iPowerGrid].iConsumption = 0;

	// calculate power for each city
	if (m_powerGrids[iPowerGrid].aCityList.size() > 1)
	{
		int iExcessSupply = 0; // excess supply = generation - consumption for each city
		std::map<int, int> piExcessSupply; // same as above, except we record the excess for each city
		std::map<int, int> piExcessDemand; // excess demand = consumption - generation for each city, first int is city ID, second int is the city's excess demand
		for (std::vector<int>::const_iterator itCity = m_powerGrids[iPowerGrid].aCityList.begin(); itCity != m_powerGrids[iPowerGrid].aCityList.end(); ++itCity)
		{
			int iCity = *itCity;
			CvCity* pCity = m_pPlayer->getCity(iCity);

			// reset imports and exports
			pCity->GetCityPower()->SetPowerExport(0);
			pCity->GetCityPower()->SetPowerImport(0);

			// calculate excess supply and demand
			int iGeneration = pCity->GetCityPower()->GetPowerGeneration();
			int iConsumption = pCity->GetCityPower()->GetPowerConsumption();
			if (iGeneration > iConsumption)
			{
				iExcessSupply += iGeneration - iConsumption;
				piExcessSupply[iCity] = iGeneration - iConsumption;
			}
			else if (iConsumption > iGeneration)
			{
				piExcessDemand[iCity] = iConsumption - iGeneration;
			}

			// write into memory for the power grid
			m_powerGrids[iPowerGrid].iGeneration += iGeneration;
			m_powerGrids[iPowerGrid].iConsumption += iConsumption;
		}

		// now we try to evenly distribute our excess supply to cities which need power imports; also evenly distribute the exporting cities
		std::map<int, int>::iterator itSupply = piExcessSupply.begin();
		while (iExcessSupply > 0 && piExcessDemand.size() > 0)
		{
			for (std::map<int, int>::iterator itDemand = piExcessDemand.begin(); itDemand != piExcessDemand.end(); /*increment purpose left blank here*/)
			{
				if (itDemand->second > 0)
				{
					itDemand->second--;
					m_pPlayer->getCity(itDemand->first)->GetCityPower()->ChangePowerImport(1);
					++itDemand;

					if (itSupply == piExcessSupply.end())
					{
						itSupply = piExcessSupply.begin();
					}

					iExcessSupply--;
					itSupply->second--;
					m_pPlayer->getCity(itSupply->first)->GetCityPower()->ChangePowerExport(1);

					if (itSupply->second <= 0)
					{
						piExcessDemand.erase(itSupply++);
					}
					else
					{
						++itSupply;
					}
				}
				else
				{
					piExcessDemand.erase(itDemand++);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------------
/// Loop through all power grids to perform the power shortage function.
void CvPlayerPowerGrids::DoPowerShortagePenalty()
{
	// If we have power grids
	if (GetNumberOfPowerGrids() > 0)
	{
		for (AllPowerGridsStore::const_iterator itGrid = m_powerGrids.begin(); itGrid != m_powerGrids.end(); ++itGrid)
		{
			if (itGrid->second.iConsumption > itGrid->second.iGeneration)
			{
				DoPowerShortagePenalty(itGrid->first);
			}
		}
	}
	// Else, maybe we have not researched power transmission yet?
	else if (!GET_TEAM(m_pPlayer->getTeam()).CanTrasmitPowerByLand() && !GET_TEAM(m_pPlayer->getTeam()).CanTrasmitPowerByWater())
	{
		int iLoop;
		for (CvCity* pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
		{
			pLoopCity->GetCityPower()->DoPowerShortagePenalty();
		}
	}
}
//--------------------------------------------------------------------------------
/// Do power shortage penalties for this power grid if there is a lack of power.
void CvPlayerPowerGrids::DoPowerShortagePenalty(int iPowerGrid)
{
	if (m_powerGrids[iPowerGrid].iConsumption > m_powerGrids[iPowerGrid].iGeneration)
	{
		for (std::vector<int>::const_iterator itCity = m_powerGrids[iPowerGrid].aCityList.begin(); itCity != m_powerGrids[iPowerGrid].aCityList.end(); ++itCity)
		{
			m_pPlayer->getCity(*itCity)->GetCityPower()->DoPowerShortagePenalty();
		}
	}
}

//--------------------------------------------------------------------------------
/// How many power grids does the player have?
int CvPlayerPowerGrids::GetNumberOfPowerGrids() const
{
	return m_powerGrids.size();
}

//--------------------------------------------------------------------------------
/// Get the first power grid ID for this player.
int CvPlayerPowerGrids::GetFirstPowerGrid() const
{
	if (GetNumberOfPowerGrids() > 0)
	{
		return m_powerGrids.begin()->first;
	}
	return -1;
}

//--------------------------------------------------------------------------------
/// Get the subsequent power grid ID.
int CvPlayerPowerGrids::GetNextPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		++it;
		if (it != m_powerGrids.end())
		{
			return it->first;
		}
	}
	
	return -1;
}

//--------------------------------------------------------------------------------
/// How many cities belong in this power grid?
int CvPlayerPowerGrids::GetNumberOfCitiesInPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		return it->second.aCityList.size();
	}

	return 0;
}
//--------------------------------------------------------------------------------
/// Obtains the list of cities which belong in a power grid.
std::vector<int> CvPlayerPowerGrids::GetListOfCitiesInPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		return it->second.aCityList;
	}

	return std::vector<int>();
}
//--------------------------------------------------------------------------------
/// What power grid does a city belong to?
int CvPlayerPowerGrids::GetPowerGridID(const CvCity* pCity) const
{
	int iCity = pCity->GetID();

	AllCitiesStore::const_iterator itCity = m_cities.find(iCity);
	if (itCity != m_cities.end())
	{
		return itCity->second;
	}

	return -1;
}
//--------------------------------------------------------------------------------
/// How much power does a power grid generate?
int CvPlayerPowerGrids::GetPowerGenerationInPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		return MAX(it->second.iGeneration, 0);
	}

	return 0;
}
//--------------------------------------------------------------------------------
/// How much power does a power grid consume?
int CvPlayerPowerGrids::GetPowerConsumptionInPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		return MAX(it->second.iConsumption, 0);
	}

	return 0;
}
//--------------------------------------------------------------------------------
/// How much spare power does a power grid have?
int CvPlayerPowerGrids::GetPowerSurplusInPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		return MAX(it->second.iGeneration - it->second.iConsumption, 0);
	}

	return -1;
}
//--------------------------------------------------------------------------------
/// How much power does a power grid lack?
int CvPlayerPowerGrids::GetPowerShortageInPowerGrid(int iPowerGrid) const
{
	AllPowerGridsStore::const_iterator it = m_powerGrids.find(iPowerGrid);
	if (it != m_powerGrids.end())
	{
		return MAX(it->second.iConsumption - it->second.iGeneration, 0);
	}

	return -1;
}
//--------------------------------------------------------------------------------
// CvCityPower
//--------------------------------------------------------------------------------
/// Constructor
CvCityPower::CvCityPower(void) :

	m_pCity(NULL),
	m_piNumPowerConsumingBuilding(),
	m_iPowerGenerationFromBuildings(0),
	m_iPowerConsumptionFromBuildings(0),
	m_iPowerGenerationFromTerrain(0),
	m_iPowerConsumptionFromTerrain(0),
	m_iPowerExport(0),
	m_iPowerImport(0),
	m_iAllowsWaterPowerTransmission(0)
{
}
//--------------------------------------------------------------------------------
/// Destructor
CvCityPower::~CvCityPower(void)
{
	Uninit();
}
//--------------------------------------------------------------------------------
/// Init
void CvCityPower::Init(CvCity* pCity)
{
	Reset();

	m_pCity = pCity;
}
//--------------------------------------------------------------------------------
/// Cleanup
void CvCityPower::Uninit()
{
	m_pCity = NULL;
	m_piNumPowerConsumingBuilding.clear();
	m_iPowerGenerationFromBuildings = 0;
	m_iPowerConsumptionFromBuildings = 0;
	m_iPowerGenerationFromTerrain = 0;
	m_iPowerConsumptionFromTerrain = 0;
	m_iPowerExport = 0;
	m_iPowerImport = 0;
	m_iAllowsWaterPowerTransmission = 0;
}
//--------------------------------------------------------------------------------
/// Reset
void CvCityPower::Reset()
{
}
//--------------------------------------------------------------------------------
/// Serialization read
void CvCityPower::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	MOD_SERIALIZE_INIT_READ(kStream);

	size_t nItems;
	MOD_SERIALIZE_READ(89, kStream, nItems, 0);
	m_piNumPowerConsumingBuilding.clear();
	for (size_t i = 0; i < nItems; i++)
	{
		int key;
		SinglePowerBuildingStore value;
		MOD_SERIALIZE_READ(89, kStream, key, 0);
		MOD_SERIALIZE_READ(89, kStream, value.iNumberTotal, 0);
		m_piNumPowerConsumingBuilding[(BuildingTypes)key] = value;
	}
	MOD_SERIALIZE_READ(89, kStream, m_iPowerGenerationFromBuildings, 0);
	MOD_SERIALIZE_READ(89, kStream, m_iPowerConsumptionFromBuildings, 0);
	MOD_SERIALIZE_READ(89, kStream, m_iPowerGenerationFromTerrain, 0);
	MOD_SERIALIZE_READ(89, kStream, m_iPowerConsumptionFromTerrain, 0);
	MOD_SERIALIZE_READ(89, kStream, m_iPowerExport, 0);
	MOD_SERIALIZE_READ(89, kStream, m_iPowerImport, 0);
	MOD_SERIALIZE_READ(89, kStream, m_iAllowsWaterPowerTransmission, 0);
}
//--------------------------------------------------------------------------------
/// Serialization write
void CvCityPower::Write(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion = 1;
	kStream << uiVersion;
	MOD_SERIALIZE_INIT_WRITE(kStream);

	MOD_SERIALIZE_WRITE(kStream, m_piNumPowerConsumingBuilding.size());
	for (AllPowerBuildingsStore::const_iterator it = m_piNumPowerConsumingBuilding.begin(); it != m_piNumPowerConsumingBuilding.end(); ++it)
	{
		MOD_SERIALIZE_WRITE(kStream, (int)it->first);
		MOD_SERIALIZE_WRITE(kStream, it->second.iNumberTotal);
	}
	MOD_SERIALIZE_WRITE(kStream, m_iPowerGenerationFromBuildings);
	MOD_SERIALIZE_WRITE(kStream, m_iPowerConsumptionFromBuildings);
	MOD_SERIALIZE_WRITE(kStream, m_iPowerGenerationFromTerrain);
	MOD_SERIALIZE_WRITE(kStream, m_iPowerConsumptionFromTerrain);
	MOD_SERIALIZE_WRITE(kStream, m_iPowerExport);
	MOD_SERIALIZE_WRITE(kStream, m_iPowerImport);
	MOD_SERIALIZE_WRITE(kStream, m_iAllowsWaterPowerTransmission);
}
//--------------------------------------------------------------------------------
// Fun stuff below
//--------------------------------------------------------------------------------
/// Set power generation (by buildings)
void CvCityPower::SetPowerGenerationFromBuildings(int iValue)
{
	if (m_iPowerGenerationFromBuildings != iValue)
	{
		m_iPowerGenerationFromBuildings = iValue;
	}
}
/// Change power generation (by buildings)
void CvCityPower::ChangePowerGenerationFromBuildings(int iChange)
{
	m_iPowerGenerationFromBuildings += iChange;
}
/// Get power generation (by buildings)
int CvCityPower::GetPowerGenerationFromBuildings() const
{
	if (GET_TEAM(m_pCity->getTeam()).CanUsePower())
	{
		return m_iPowerGenerationFromBuildings;
	}
	else
	{
		return 0;
	}
}
//	--------------------------------------------------------------------------------
/// Set power consumption (by buildings)
void CvCityPower::SetPowerConsumptionFromBuildings(int iValue)
{
	if (m_iPowerConsumptionFromBuildings != iValue)
	{
		m_iPowerConsumptionFromBuildings = iValue;
	}
}
/// Change power consumption (by buildings)
void CvCityPower::ChangePowerConsumptionFromBuildings(int iChange)
{
	m_iPowerConsumptionFromBuildings += iChange;
}
/// Get power consumption (by buildings)
int CvCityPower::GetPowerConsumptionFromBuildings() const
{
	return m_iPowerConsumptionFromBuildings;
}
//--------------------------------------------------------------------------------
/// Set power generation (by improvements)
void CvCityPower::SetPowerGenerationFromTerrain(int iValue)
{
	if (m_iPowerGenerationFromTerrain != iValue)
	{
		m_iPowerGenerationFromTerrain = iValue;
	}
}
/// Change power generation (by improvements)
void CvCityPower::ChangePowerGenerationFromTerrain(int iChange)
{
	m_iPowerGenerationFromTerrain += iChange;
}
/// Get power generation (by improvements)
int CvCityPower::GetPowerGenerationFromTerrain() const
{
	if (GET_TEAM(m_pCity->getTeam()).CanUsePower())
	{
		return m_iPowerGenerationFromTerrain;
	}
	else
	{
		return 0;
	}
}
//	--------------------------------------------------------------------------------
/// Set power consumption (by improvements)
void CvCityPower::SetPowerConsumptionFromTerrain(int iValue)
{
	if (m_iPowerConsumptionFromTerrain != iValue)
	{
		m_iPowerConsumptionFromTerrain = iValue;
	}
}
/// Change power consumption (by improvements)
void CvCityPower::ChangePowerConsumptionFromTerrain(int iChange)
{
	m_iPowerConsumptionFromTerrain += iChange;
}
/// Get power consumption (by improvements)
int CvCityPower::GetPowerConsumptionFromTerrain() const
{
	return m_iPowerConsumptionFromTerrain;
}
//	--------------------------------------------------------------------------------
/// Get power generation (by all sources)
int CvCityPower::GetPowerGeneration() const
{
	return GetPowerGenerationFromBuildings() + GetPowerGenerationFromTerrain();
}
/// Get power consumption (by all sources)
int CvCityPower::GetPowerConsumption() const
{
	return GetPowerConsumptionFromBuildings() + GetPowerConsumptionFromTerrain();
}
//	--------------------------------------------------------------------------------
/// Set power exported to grid
void CvCityPower::SetPowerExport(int iValue)
{
	if (m_iPowerExport != iValue)
	{
		m_iPowerExport = iValue;
	}
}
/// Change power exported to grid
void CvCityPower::ChangePowerExport(int iChange)
{
	m_iPowerExport += iChange;
}
/// Get power exported to grid
int CvCityPower::GetPowerExport() const
{
	return m_iPowerExport;
}
//	--------------------------------------------------------------------------------
/// Set power imported from grid
void CvCityPower::SetPowerImport(int iValue)
{
	if (m_iPowerImport != iValue)
	{
		m_iPowerImport = iValue;
	}
}
/// Change power imported from grid
void CvCityPower::ChangePowerImport(int iChange)
{
	m_iPowerImport += iChange;
}
/// Get power imported from grid
int CvCityPower::GetPowerImport() const
{
	return m_iPowerImport;
}
//--------------------------------------------------------------------------------
/// Change count for allowing power transmission via water city connetions.
void CvCityPower::ChangeAllowsWaterPowerTransmissionCount(int iChange)
{
	m_iAllowsWaterPowerTransmission += iChange;
}
/// Can this city transmit power by water city connections?
bool CvCityPower::CanTransmitPowerByWater() const
{
	return m_iAllowsWaterPowerTransmission > 0;
}
//	--------------------------------------------------------------------------------
/// Get the number of buildings in the city which consumes power
int CvCityPower::GetNumPowerConsumingBuilding(BuildingTypes eIndex) const
{
	AllPowerBuildingsStore::const_iterator it = m_piNumPowerConsumingBuilding.find(eIndex);
	if (it != m_piNumPowerConsumingBuilding.end())
	{
		return it->second.iNumberTotal;
	}
	return 0;
}
//	--------------------------------------------------------------------------------
/// Set the number of buildings in the city which consumes power
void CvCityPower::SetNumPowerConsumingBuilding(BuildingTypes eIndex, int iNewValue)
{
	if (iNewValue == 0)
	{
		m_piNumPowerConsumingBuilding.erase(eIndex);
	}
	else
	{
		m_piNumPowerConsumingBuilding[eIndex].iNumberTotal = iNewValue;
	}
}
//	--------------------------------------------------------------------------------
/// Set the number of buildings in the city which consumes power
void CvCityPower::ChangeNumPowerConsumingBuilding(BuildingTypes eIndex, int iChange)
{
	if (iChange != 0)
	{
		int iNewValue = m_piNumPowerConsumingBuilding[eIndex].iNumberTotal + iChange;
		if (iNewValue == 0)
		{
			m_piNumPowerConsumingBuilding.erase(eIndex);
		}
		else
		{
			m_piNumPowerConsumingBuilding[eIndex].iNumberTotal = iNewValue;
		}
	}
}
//	--------------------------------------------------------------------------------
/// Penalty for consuming more power than generation and imports in a city
void CvCityPower::DoPowerShortagePenalty()
{
	ResetPowerShortagePenalty();
	int iShortage = (GetPowerGeneration() + GetPowerImport() - GetPowerConsumption() - GetPowerExport()) * -1;
#if defined(MOD_BUILDINGS_DEACTIVATION)
	if (MOD_BUILDINGS_DEACTIVATION && iShortage > 0)
	{
		int iIterations = 0;
		std::map<BuildingTypes, int> piNumDeactivateableBuilding; // need a separate copy to work with

		for (AllPowerBuildingsStore::const_iterator it = m_piNumPowerConsumingBuilding.begin(); it != m_piNumPowerConsumingBuilding.end(); ++it)
		{
			if (it->second.iNumberTotal > 0)
			{
				// don't try to deactivate buildings which have been deactivated for other reasons
				int iNumAlreadyDeactived = m_pCity->GetCityBuildings()->GetNumDeactivatedBuilding((BuildingTypes)it->first);
				if (it->second.iNumberTotal - iNumAlreadyDeactived > 0)
				{
					piNumDeactivateableBuilding[it->first] = it->second.iNumberTotal - iNumAlreadyDeactived;
				}
			}
		}

		// keep deactivating buildings until we are positive again
		while (iShortage > 0 && iIterations < 500)
		{
			// pick a random building to deactivate
			int iRandom = GC.getGame().getSmallFakeRandNum(piNumDeactivateableBuilding.size(), m_pCity->plot()->GetPlotIndex() + GET_PLAYER(m_pCity->getOwner()).GetPseudoRandomSeed());
			BuildingTypes eBuilding = NO_BUILDING;
			for (std::map<BuildingTypes, int>::iterator it = piNumDeactivateableBuilding.begin(); it != piNumDeactivateableBuilding.end(); ++it)
			{
				iRandom--;
				if (iRandom < 0)
				{
					eBuilding = it->first;
					it->second--;
					break;
				}
			}
			if (eBuilding != NO_BUILDING)
			{
				m_pCity->GetCityBuildings()->SetNumDeactivatedBuilding(eBuilding, DEACTIVATION_POWER, m_pCity->GetCityBuildings()->GetNumDeactivatedBuilding(eBuilding, DEACTIVATION_POWER) + 1);
				if (piNumDeactivateableBuilding[eBuilding] <= 0)
				{
					piNumDeactivateableBuilding.erase(eBuilding);
				}
				int iPowerRequirement = GC.getBuildingInfo(eBuilding)->GetPowerChange() * -1;
				if (iPowerRequirement > 0)
				{
					iShortage -= iPowerRequirement;
				}
				else
				{
					// really shouldn't reach here
					piNumDeactivateableBuilding.erase(eBuilding);
				}
			}
			iIterations++; // prevent infinite loops
		}
	}
#endif
}
//	--------------------------------------------------------------------------------
/// Reset penalty for consuming more power than generation and imports in a city
void CvCityPower::ResetPowerShortagePenalty()
{
#if defined(MOD_BUILDINGS_DEACTIVATION)
	if (MOD_BUILDINGS_DEACTIVATION)
	{
		for (AllPowerBuildingsStore::iterator it = m_piNumPowerConsumingBuilding.begin(); it != m_piNumPowerConsumingBuilding.end(); ++it)
		{
			int iNumUnpowered = m_pCity->GetCityBuildings()->GetNumDeactivatedBuilding(it->first, DEACTIVATION_POWER);

			if (iNumUnpowered > 0)
			{
				m_pCity->GetCityBuildings()->SetNumDeactivatedBuilding(it->first, DEACTIVATION_POWER, 0);
			}
		}
	}
#endif
}
#endif