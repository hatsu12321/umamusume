#pragma once
#include <string>
#include <tuple>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include "il2cpp/il2cpp_symbols.hpp"
#include "string_utils.hpp"
#include "game.hpp"

namespace MasterDB
{
	SQLite::Database* replacementMasterDB;
	SQLite::Database* masterDB;
	SQLite::Database* metaDB;

	string masterDBPath;

	void InitMasterDB()
	{
		auto path = wide_u8(il2cpp_symbols::get_method_pointer<Il2CppString * (*)()>("Cute.Core.Assembly.dll", "Cute.Core", "Device", "GetPersistentDataPath", IgnoreNumberOfArguments)()->chars);
		auto metaDBPath = path + R"(\meta)";
		masterDBPath = path + R"(\master\master.mdb)";

		metaDB = new SQLite::Database(":memory:", SQLite::OPEN_READWRITE);
		metaDB->backup(metaDBPath.data(), SQLite::Database::Load);

		masterDB = new SQLite::Database(":memory:", SQLite::OPEN_READWRITE);
		masterDB->backup(masterDBPath.data(), SQLite::Database::Load);
	}

	void InitReplacementMasterDB(string path)
	{
		replacementMasterDB = new SQLite::Database(path.data(), SQLite::OPEN_READONLY);
	}

	void ReloadMasterDB()
	{
		masterDB->backup(masterDBPath.data(), SQLite::Database::Load);
	}

	vector<string> GetChampionsResources()
	{
		if (!masterDB)
		{
			InitMasterDB();
		}

		vector<string> pairs;

		auto statement = new SQLite::Statement(*masterDB, R"(SELECT c.resource_id, t.text FROM champions_schedule c LEFT OUTER JOIN text_data t on t.category = 206 AND t."index" = c.id GROUP BY c.resource_id)");

		while (statement->executeStep())
		{
			pairs.emplace_back(statement->getColumn(1).getString());
		}

		delete statement;

		return pairs;
	}

	string GetTextData(int category, int index)
	{
		if (!masterDB)
		{
			InitMasterDB();
		}

		if (replacementMasterDB)
		{
			auto statement = new SQLite::Statement(*replacementMasterDB, R"(SELECT text FROM text_data WHERE "category" = ?1 AND "index" = ?2)");
			statement->bind(1, category);
			statement->bind(2, index);

			while (statement->executeStep())
			{
				auto text = statement->getColumn(0).getString();
				delete statement;
				return text;
			}
		}

		auto statement = new SQLite::Statement(*masterDB, R"(SELECT text FROM text_data WHERE "category" = ?1 AND "index" = ?2)");
		statement->bind(1, category);
		statement->bind(2, index);

		while (statement->executeStep())
		{
			auto text = statement->getColumn(0).getString();
			delete statement;
			return text;
		}

		delete statement;

		return "";
	}

	tuple<int, int> GetJobsInfo(int rewardId)
	{
		if (!masterDB)
		{
			InitMasterDB();
		}

		auto statement = new SQLite::Statement(*masterDB, R"(SELECT place_id, genre_id FROM jobs_reward WHERE "id" = ?1)");
		statement->bind(1, rewardId);

		while (statement->executeStep())
		{
			auto placeId = statement->getColumn(0).getInt();
			auto genreId = statement->getColumn(1).getInt();
			delete statement;
			return tuple{ placeId, genreId };
		}

		delete statement;

		return tuple{ 0, 0 };
	}

	int GetJobsPlaceRaceTrackId(int placeId)
	{
		if (!masterDB)
		{
			InitMasterDB();
		}

		auto statement = new SQLite::Statement(*masterDB, R"(SELECT race_track_id FROM jobs_place WHERE "id" = ?1)");
		statement->bind(1, placeId);

		while (statement->executeStep())
		{
			auto raceTrackId = statement->getColumn(0).getInt();
			delete statement;
			return raceTrackId;
		}

		delete statement;

		return 0;
	}
}
