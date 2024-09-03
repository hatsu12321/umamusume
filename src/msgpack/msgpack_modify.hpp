#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <queue>
#include <SQLiteCpp/SQLiteCpp.h>
#include <msgpack11.hpp>

#include "il2cpp/il2cpp_symbols.hpp"
#include "il2cpp/il2cpp-api-functions.hpp"
#include "local/local.hpp"
#include "game.hpp"

#include "masterdb/masterdb.hpp"

#include "hook.h"

#include "notification/DesktopNotificationManagerCompat.h"

#include "masterdb/masterdb.hpp"

#include "msgpack_data.hpp"

using namespace std;
using namespace msgpack11;

namespace MsgPackModify
{
	unordered_map<int, MsgPack::object> liveTheaterSaveInfoMap;

	unordered_map<int, bool> availableCharaIds;
	unordered_map<int, bool> defaultAvailableDressIds;
	unordered_map<int, bool> availableDressIds;
	unordered_map<int, bool> availableMusicIds;

	vector<int> mobIds;
	queue<int> mobIdQueue;

	MsgPack::array charaList;

	bool charaListInit = false;

	int64_t current_time() {
		auto ms = chrono::duration_cast<chrono::seconds>(
			chrono::system_clock::now().time_since_epoch());
		return ms.count();
	}

	void SuffleMobIds()
	{
		vector<int> ids = mobIds;

		random_device device;
		shuffle(ids.begin(), ids.end(), default_random_engine(device()));

		queue<int> q;

		for (auto id : ids)
		{
			q.push(id);
		}

		mobIdQueue = q;
	}

	int GetRandomMobId()
	{
		auto num = mobIdQueue.front();
		mobIdQueue.pop();
		mobIdQueue.push(num);
		return num;
	}

	void InitMasterDB()
	{
		MasterDB::InitMasterDB();

		auto db = new SQLite::Database(MasterDB::masterDBPath, SQLite::OPEN_READWRITE);

		db->exec("UPDATE dress_data SET use_live = 1, use_live_theater = 1");

		db->exec("UPDATE dress_data SET start_time = 1483196400 WHERE start_time > "s.append(to_string(current_time())).data());

		db->exec("UPDATE dress_data SET general_purpose = 1, costume_type = 1 WHERE id >= 200000 AND id <= 299999 AND body_type = 100");

		db->exec("UPDATE dress_data SET body_type = 230 WHERE id > 299999 AND body_type = 100");

		db->exec("UPDATE dress_data SET body_type = 230 WHERE id LIKE '1___60'");

		/*db->exec("UPDATE fan_raid_data SET end_date = 2524575600 WHERE fan_raid_id = 1001");

		db->exec("UPDATE fan_raid_top_data SET start_date = 2524575600, end_date = 2524575600");

		db->exec("UPDATE fan_raid_top_data SET start_date = 1648782000, end_date = 2524575600 WHERE id = 1");*/

		db->exec("UPDATE live_data SET start_date = 1483196400 WHERE has_live = 1 AND start_date > "s.append(to_string(current_time())).data());

		db->exec("UPDATE chara_data SET start_date = 1483196400 WHERE start_date > "s.append(to_string(current_time())).data());

		db->exec("UPDATE chara_data SET shape = 1 WHERE id = 9001");

		unordered_map<int, bool> masterCardIds;

		auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT id FROM card_data");
		while (statement.executeStep())
		{
			masterCardIds[statement.getColumn(0).getInt()] = true;
		}

		auto statement1 = SQLite::Statement(*MasterDB::masterDB, "SELECT id FROM dress_data WHERE (condition_type = 1 OR condition_type = 4 OR condition_type = 5) AND use_live_theater = 1 AND id < 999");
		while (statement1.executeStep())
		{
			defaultAvailableDressIds[statement1.getColumn(0).getInt()] = true;
		}

		auto statement2 = SQLite::Statement(*MasterDB::masterDB, "SELECT mob_id FROM mob_data WHERE use_live = 1");
		while (statement2.executeStep())
		{
			mobIds.emplace_back(statement2.getColumn(0).getInt());
		}

		vector<int> metaDressIds;

		auto mstatement = SQLite::Statement(*MasterDB::metaDB, "SELECT n FROM a WHERE n LIKE '%pfb_bdy1____0_'");
		while (mstatement.executeStep())
		{
			auto name = mstatement.getColumn(0).getString();
			auto chara_id = name.substr(32, 4);
			auto body_type = name.substr(37, 2);

			if (stoi(body_type) <= 1)
			{
				body_type = "01";
				metaDressIds.emplace_back(stoi(chara_id + body_type));
			}
		}

		auto mstatement1 = SQLite::Statement(*MasterDB::metaDB, "SELECT n FROM a WHERE n LIKE '%pfb_bdy2____0_'");
		while (mstatement1.executeStep())
		{
			auto name = mstatement1.getColumn(0).getString();
			auto chara_id = name.substr(32, 4);
			auto body_type = name.substr(37, 2);

			if (stoi(body_type) <= 1)
			{
				body_type = "01";
				metaDressIds.emplace_back(stoi(chara_id + body_type));
			}
		}

		for (auto dressId : metaDressIds)
		{
			if (!masterCardIds.contains(dressId))
			{
				auto chara_id = to_string(dressId).substr(0, 4);
				auto sub_type = stoi(to_string(dressId).substr(3, 2));
				stringstream ss;
				ss << to_string(dressId) << ",";
				ss << chara_id << ",";
				ss << "3, 0, 100101, 0, 20, 0, 0, 10, 100101, 1, 100101, 3";

				if (Game::CurrentGameRegion == Game::Region::KOR)
				{
					ss << ", 1483196400";
				}

				db->exec("INSERT INTO card_data VALUES("s.append(ss.str()).append(")").data());

				stringstream ss1;
				ss1 << to_string(dressId);
				ss1 << "03, ";
				ss1 << to_string(dressId);
				ss1 << ", 3, ";
				ss1 << to_string(dressId);
				ss1 << ", 10010103, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 5, 7, 7, 1, 7, 7, 5, 7, 1, 101,";
				ss1 << to_string(dressId);

				db->exec("INSERT INTO card_rarity_data VALUES("s.append(ss1.str()).append(")").data());
				availableDressIds[dressId] = true;
			}
		}

		delete db;

		MasterDB::ReloadMasterDB();

		SuffleMobIds();
	}

	string ModifyRequest(const char* data, size_t size)
	{
		string error;
		auto parsed = MsgPack::parse(data, size, error);

		string dump;

		if (error.empty())
		{
			if (parsed.is_object())
			{
				MsgPack::object object = parsed.object_items();

				if (object["live_theater_save_info"].is_object())
				{
					MsgPack::object info = object["live_theater_save_info"].object_items();
					auto music_id = info["music_id"].int32_value();
					MsgPack::array member_info_array = info["member_info_array"].array_items();

					if (liveTheaterSaveInfoMap.contains(music_id))
					{
						bool valid = true;

						for (auto& member : member_info_array)
						{
							int chara_id = member["chara_id"].int32_value();

							if (chara_id > 0 && !availableCharaIds.contains(chara_id))
							{
								cout << "Invalid Chara: " << chara_id << endl;
								valid = false;
								break;
							}

							int dress_id = member["dress_id"].int32_value();
							int dress_id2 = member["dress_id2"].int32_value();

							if (dress_id > 0 &&
								!(availableDressIds.contains(dress_id) || defaultAvailableDressIds.contains(dress_id)))
							{
								cout << "Invalid Dress: " << dress_id << endl;
								valid = false;
								break;
							}

							if (dress_id2 > 0 &&
								!(availableDressIds.contains(dress_id2) || defaultAvailableDressIds.contains(dress_id2)))
							{
								cout << "Invalid Dress 2: " << dress_id2 << endl;
								valid = false;
								break;
							}
						}

						if (!valid)
						{
							// rollback data
							object["live_theater_save_info"] = liveTheaterSaveInfoMap[music_id];
						}
					}
					else if (availableMusicIds[music_id])
					{
						bool valid = true;

						for (auto& member : member_info_array)
						{
							int chara_id = member["chara_id"].int32_value();

							if (chara_id > 0 && !availableCharaIds.contains(chara_id))
							{
								cout << "Invalid Chara: " << chara_id << endl;
								valid = false;
								break;
							}

							int dress_id = member["dress_id"].int32_value();
							int dress_id2 = member["dress_id2"].int32_value();

							if (dress_id > 0 &&
								!(availableDressIds.contains(dress_id) || defaultAvailableDressIds.contains(dress_id)))
							{
								cout << "Invalid Dress: " << dress_id << endl;
								valid = false;
								break;
							}

							if (dress_id2 > 0 &&
								!(availableDressIds.contains(dress_id2) || defaultAvailableDressIds.contains(dress_id2)))
							{
								cout << "Invalid Dress 2: " << dress_id2 << endl;
								valid = false;
								break;
							}
						}

						if (!valid)
						{
							if (liveTheaterSaveInfoMap.empty())
							{
								MsgPack::array new_member_info_array;
								for (auto& member : member_info_array)
								{
									MsgPack::object new_member;

									int chara_id = member["chara_id"].int32_value();
									int mob_id = member["mob_id"].int32_value();

									if (chara_id > 0 && !availableCharaIds.contains(chara_id))
									{
										new_member["chara_id"] = 0;
										new_member["mob_id"] = GetRandomMobId();
									}
									else if (chara_id == 0 && mob_id == 0)
									{
										new_member["mob_id"] = GetRandomMobId();
									}

									int dress_id = member["dress_id"].int32_value();
									int dress_id2 = member["dress_id2"].int32_value();

									if (!(availableDressIds.contains(dress_id) || defaultAvailableDressIds.contains(dress_id)))
									{
										new_member["dress_id"] = 7;
									}
									else if (!availableCharaIds.contains(chara_id) &&
										(availableDressIds.contains(dress_id) || defaultAvailableDressIds.contains(dress_id)))
									{
										new_member["dress_id"] = 7;
									}

									if (dress_id2 > 0)
									{
										if (!(availableDressIds.contains(dress_id2) || defaultAvailableDressIds.contains(dress_id2)))
										{
											new_member["dress_id2"] = 7;
										}
										else if (!availableCharaIds.contains(chara_id) &&
											(availableDressIds.contains(dress_id2) || defaultAvailableDressIds.contains(dress_id2)))
										{
											new_member["dress_id2"] = 7;
										}
									}

									new_member_info_array.emplace_back(new_member);
								}
								info["member_info_array"] = new_member_info_array;
							}
							else
							{
								// fallback data
								pair<int, MsgPack::object> first = *liveTheaterSaveInfoMap.begin();
								object["live_theater_save_info"] = first.second;
							}
						}
					}
					else
					{
						if (liveTheaterSaveInfoMap.empty())
						{
							if (!availableMusicIds.empty())
							{
								pair<int, bool> first = *availableMusicIds.begin();

								auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT live_member_number FROM live_data WHERE music_id = ?");
								statement.bind(0, first.first);

								int memberCount;

								while (statement.executeStep())
								{
									memberCount = statement.getColumn(0).getInt();
								}

								MsgPack::array new_member_info_array;
								for (int i = 0; i < memberCount; i++)
								{
									MsgPack::object new_member;

									new_member["mob_id"] = GetRandomMobId();
									new_member["dress_id"] = 7;


									new_member_info_array.emplace_back(new_member);
								}
								info["member_info_array"] = new_member_info_array;
							}
						}
						else
						{
							// fallback data
							pair<int, MsgPack::object> first = *liveTheaterSaveInfoMap.begin();
							object["live_theater_save_info"] = first.second;
						}
					}
				}

				MsgPack{ object }.dump(dump);
			}
		}

		return dump;
	}

	MsgPack::array GetCharaList()
	{
		return charaList;
	}

	string ModifyResponse(const char* data, size_t size)
	{
		string error;
		auto parsed = MsgPack::parse(data, size, error);

		string dump;

		if (error.empty())
		{
			if (parsed.is_object())
			{
				MsgPack::object object = parsed.object_items();

				/*if (object["data_headers"].is_object())
				{
					MsgPack::object header = object["data_headers"].object_items();

					if (header["result_code"].int32_value() == 9151)
					{
						header["result_code"] = 1;

						MsgPack::object fakeFanRaidData = MsgPack::object
						{
							{
								"fan_raid_fans_info",
								MsgPack::object
								{
									{ "all_gained_fans", INT_MAX },
									{ "individual_gained_fans", INT_MAX }
								}
							},
							{ "checked_fan_raid_all_reward_id_array", MsgPack::array{} },
							{ "fan_raid_polling_interval", INT_MAX },
							{ "is_cleared_fan_raid_condition", 1 }
						};

						object["data"] = fakeFanRaidData;
					}

					object["data_headers"] = header;
				}*/

				if (object["data"].is_object())
				{
					MsgPack::object data = object["data"].object_items();

					if (data["common_define"].is_object())
					{
						if (!MasterDB::masterDB)
						{
							InitMasterDB();
						}
					}

					/*if (data["fan_raid_id"].is_int())
					{
						data["fan_raid_id"] = 1001;
					}*/

					if (data["chara_list"].is_array())
					{
						MsgPack::array chara_list = data["chara_list"].array_items();

						unordered_map<int, MsgPack::object> chara_map;

						availableCharaIds.clear();
						for (auto& chara : chara_list)
						{
							MsgPack::object charaObject = chara.object_items();

							int chara_id = charaObject["chara_id"].int32_value();

							availableCharaIds[chara_id] = true;
							chara_map[chara_id] = charaObject;
						}

						auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT id FROM chara_data");

						MsgPack::array charas;
						while (statement.executeStep())
						{
							int chara_id = statement.getColumn(0).getInt();

							if (chara_map.contains(chara_id))
							{
								charas.emplace_back(chara_map[chara_id]);
							}
							else
							{
								charas.emplace_back(
									MsgPack::object
									{
										{ "chara_id", statement.getColumn(0).getInt() },
										{ "training_num", 0 },
										{ "love_point", 0 },
										{ "fan", 1 },
										{ "max_grade", 0 },
										{ "dress_id", 2 },
										{ "mini_dress_id", 2 },
										{ "love_point_pool", 0 },
									}
								);
							}
						}

						charaList = charas;
					}

					if (data["chara_profile_array"].is_array())
					{
						MsgPack::array chara_profile_array = data["chara_profile_array"].array_items();

						unordered_map<int, MsgPack::object> chara_profile_map;

						for (auto& chara_profile : chara_profile_array)
						{
							MsgPack::object charaProfileObject = chara_profile.object_items();

							int chara_id = charaProfileObject["chara_id"].int32_value();

							chara_profile_map[chara_id] = charaProfileObject;
						}

						auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT id FROM chara_data");

						MsgPack::array chara_profiles;
						while (statement.executeStep())
						{
							int chara_id = statement.getColumn(0).getInt();

							if (chara_profile_map.contains(chara_id))
							{
								chara_profiles.emplace_back(chara_profile_map[chara_id]);
							}
							else
							{
								chara_profiles.emplace_back(
									MsgPack::object
									{
										{ "chara_id", statement.getColumn(0).getInt() },
										{ "data_id", 1 },
										{ "new_flag", 0 },
									}
								);
							}
						}

						data["chara_profile_array"] = chara_profiles;
					}

					if (data["release_card_array"].is_array())
					{
						MsgPack::array release_card_array = data["release_card_array"].array_items();

						unordered_map<int, bool> release_card_map;

						for (auto& release_card : release_card_array)
						{
							auto release_card_id = release_card.int32_value();

							release_card_map[release_card_id] = true;
						}

						auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT id FROM card_data WHERE id <= 999999");

						MsgPack::array release_cards;
						while (statement.executeStep())
						{
							int release_card_id = statement.getColumn(0).getInt();

							release_cards.emplace_back(release_card_id);
						}

						data["release_card_array"] = release_cards;
					}

					if (data["card_list"].is_array())
					{
						MsgPack::array card_list = data["card_list"].array_items();

						unordered_map<int, MsgPack::object> card_map;

						for (auto& card : card_list)
						{
							MsgPack::object cardObject = card.object_items();

							int card_id = cardObject["card_id"].int32_value();

							card_map[card_id] = cardObject;
						}

						auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT id, default_rarity FROM card_data WHERE id <= 999999");

						MsgPack::array cards;
						while (statement.executeStep())
						{
							int card_id = statement.getColumn(0).getInt();

							if (card_map.contains(card_id))
							{
								MsgPack::object card = card_map[card_id];
								if (card["rarity"].int32_value() < 3)
								{
									card["rarity"] = 3;
								}

								cards.emplace_back(card);
							}
							else
							{
								cards.emplace_back(
									MsgPack::object
									{
										{ "null", 1 },
										{ "card_id", statement.getColumn(0).getInt() },
										{ "rarity", statement.getColumn(1).getInt() },
										{ "talent_level", 1 },
										{ "create_time", "2022-07-01 12:00:00" },
										{ "skill_data_array", MsgPack::array{} },
									}
								);
							}
						}

						data["card_list"] = cards;
					}

					if (data["cloth_list"].is_array())
					{
						MsgPack::array cloth_list = data["cloth_list"].array_items();

						availableDressIds.clear();
						for (auto& cloth : cloth_list)
						{
							MsgPack::object clothObject = cloth.object_items();
							availableDressIds[clothObject["cloth_id"].int32_value()] = true;
						}

						auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT id FROM dress_data");

						MsgPack::array dresses;
						while (statement.executeStep())
						{
							dresses.emplace_back(MsgPack::object{ {"cloth_id", statement.getColumn(0).getInt()} });
						}

						data["cloth_list"] = dresses;
					}

					if (data["music_list"].is_array())
					{
						MsgPack::array music_list = data["music_list"].array_items();

						availableMusicIds.clear();
						for (auto& music : music_list)
						{
							MsgPack::object musicObject = music.object_items();
							availableMusicIds[musicObject["music_id"].int32_value()] = true;
						}

						auto statement = SQLite::Statement(*MasterDB::masterDB, "SELECT music_id FROM live_data");

						MsgPack::array musicIds;
						while (statement.executeStep())
						{
							musicIds.emplace_back(
								MsgPack::object
								{
									{ "music_id", statement.getColumn(0).getInt() },
									{ "acquisition_time", "2022-07-01 12:00:00" }
								}
							);
						}

						data["music_list"] = musicIds;
					}

					if (data["live_theater_save_info_array"].is_array())
					{
						MsgPack::array live_theater_save_info_array = data["live_theater_save_info_array"].array_items();

						liveTheaterSaveInfoMap.clear();

						for (auto& live_theater_save_info : live_theater_save_info_array)
						{
							auto saveInfo = MsgPack::object{ live_theater_save_info.object_items() };
							liveTheaterSaveInfoMap.emplace(saveInfo["music_id"].int32_value(), saveInfo);
						}
					}

					/*if (data["summary_user_info_array"].is_array())
					{
						MsgPack::array summary_user_info_array = data["summary_user_info_array"].array_items();

						MsgPack::array userInfoArray;
						for (auto& user_info : summary_user_info_array)
						{
							auto userInfo = MsgPack::object{ user_info.object_items() };
							
							if (userInfo["viewer_id"].int64_value() != 000000000)
							{
								userInfo["name"] = "";
							}

							userInfoArray.emplace_back(userInfo);
						}

						data["summary_user_info_array"] = userInfoArray;
					}

					if (data["circle_chat_message_array"].is_array())
					{
						data["circle_chat_message_array"] = MsgPack::array{};
					}

					if (data["circle_item_request_array"].is_array())
					{
						data["circle_item_request_array"] = MsgPack::array{};
					}*/

					object["data"] = data;
				}

				MsgPack{ object }.dump(dump);
			}
		}

		return dump;
	}
}
