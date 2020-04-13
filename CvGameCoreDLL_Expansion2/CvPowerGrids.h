#pragma once

#ifndef CIV5_POWER_GRIDS_H
#define CIV5_POWER_GRIDS_H

#if defined(MOD_GLOBAL_POWER)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvPlayerPowerGrids
//!  \brief		Maintains the multiple power grids for a player
//
//!  Key Attributes:
//!  - This object is created inside the CvPlayer object and accessed through CvPlayer
//!  - One grid is a list of cities which are considered connected for power transmission
//!  - A player might have 1 power grid if all his/her cities are connected to each other
//!  - On the other extreme, if there are zero connections, then each city is its own power grid
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvPlayerPowerGrids
{
public:
	CvPlayerPowerGrids(void);
	~CvPlayerPowerGrids(void);

	void Init(CvPlayer* pPlayer);
	void Uninit();
	void Reset(void);

	//// Serialization routines
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream);

	void UpdateCitiesInPowerGrids();
	void UpdatePowerInPowerGrids();
	void UpdatePowerInPowerGrid(int iPowerGrid);
	void DoPowerShortagePenalty();
	void DoPowerShortagePenalty(int iPowerGrid);

	struct SinglePowerGridStore
	{
		SinglePowerGridStore() :
			aCityList(),
			iGeneration(0),
			iConsumption(0)
		{};

		std::vector<int> aCityList; // vector of city IDs in the power grid
		// by our definition, power cannot be transferred between grids, so there are no imports and exports at this level
		int iGeneration; // total generation, sum of all cities
		int iConsumption; // total consumption, sum of all cities
	};

	typedef std::map<int, SinglePowerGridStore> AllPowerGridsStore; // int is the power grid ID
	typedef std::map<int, int> AllCitiesStore; // key is city ID, value is power grid ID

	int GetNumberOfPowerGrids() const;
	int GetFirstPowerGrid() const;
	int GetNextPowerGrid(int iPowerGrid) const;
	int GetNumberOfCitiesInPowerGrid(int iPowerGrid) const;
	std::vector<int> GetListOfCitiesInPowerGrid(int iPowerGrid) const;
	int GetPowerGridID(const CvCity* pCity) const;
	int GetPowerGenerationInPowerGrid(int iPowerGrid) const;
	int GetPowerConsumptionInPowerGrid(int iPowerGrid) const;
	int GetPowerSurplusInPowerGrid(int iPowerGrid) const;
	int GetPowerShortageInPowerGrid(int iPowerGrid) const;

private:
	CvPlayer* m_pPlayer;
	AllPowerGridsStore m_powerGrids;
	AllCitiesStore m_cities;
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvCityPower
//!  \brief		Information about power in a single city
//
//!  Key Attributes:
//!  - This object is created inside the CvCity object and accessed through CvCity
//!  - One instance for each city
//!  - Accessed by any class that needs to check power status
//!  - Separate class for tidiness
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCityPower
{
public:
	CvCityPower(void);
	~CvCityPower(void);

	void Init(CvCity* pCity);
	void Uninit();

	void Reset();
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream);

	void SetPowerGenerationFromBuildings(int iValue);
	void ChangePowerGenerationFromBuildings(int iChange);
	int GetPowerGenerationFromBuildings() const;

	void SetPowerConsumptionFromBuildings(int iValue);
	void ChangePowerConsumptionFromBuildings(int iChange);
	int GetPowerConsumptionFromBuildings() const;

	void SetPowerGenerationFromTerrain(int iValue);
	void ChangePowerGenerationFromTerrain(int iChange);
	int GetPowerGenerationFromTerrain() const;

	void SetPowerConsumptionFromTerrain(int iValue);
	void ChangePowerConsumptionFromTerrain(int iChange);
	int GetPowerConsumptionFromTerrain() const;

	int GetPowerGeneration() const;
	int GetPowerConsumption() const;

	void SetPowerExport(int iValue);
	void ChangePowerExport(int iChange);
	int GetPowerExport() const;

	void SetPowerImport(int iValue);
	void ChangePowerImport(int iChange);
	int GetPowerImport() const;

	void ChangeAllowsWaterPowerTransmissionCount(int iChange);
	bool CanTransmitPowerByWater() const;

	int GetNumPowerConsumingBuilding(BuildingTypes eIndex) const;
	void SetNumPowerConsumingBuilding(BuildingTypes eIndex, int iNewValue);
	void ChangeNumPowerConsumingBuilding(BuildingTypes eIndex, int iChange);

	struct SinglePowerBuildingStore
	{
		SinglePowerBuildingStore() :
			iNumberTotal(0)
		{};

		int iNumberTotal; // total copies of this building
	};

	typedef std::map<BuildingTypes, SinglePowerBuildingStore> AllPowerBuildingsStore;

	void DoPowerShortagePenalty();
	void ResetPowerShortagePenalty();

private:
	CvCity* m_pCity;
	AllPowerBuildingsStore m_piNumPowerConsumingBuilding;
	int m_iPowerGenerationFromBuildings;
	int m_iPowerConsumptionFromBuildings;
	int m_iPowerGenerationFromTerrain;
	int m_iPowerConsumptionFromTerrain;
	int m_iPowerExport;
	int m_iPowerImport;
	int m_iAllowsWaterPowerTransmission;
};
#endif
#endif