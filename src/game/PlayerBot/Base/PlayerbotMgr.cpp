/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Config/Config.h"
#include "Server/WorldPacket.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "../config.h"
#include "../../Chat/Chat.h"
#include "../../Entities/GossipDef.h"
#include "../../Entities/Player.h"
#include "../../Globals/ObjectMgr.h"
#include "../../Globals/ObjectAccessor.h"
#include "../../Guilds/Guild.h"
#include "../../Loot/LootMgr.h"
#include "../../MotionGenerators/WaypointMovementGenerator.h"
#include "../../Spells/SpellMgr.h"
#include "../../Tools/Language.h"
#include "../../World/World.h"

class LoginQueryHolder;
class CharacterHandler;

Config botConfig;

void PlayerbotMgr::SetInitialWorldSettings()
{
    //Get playerbot configuration file
    if (!botConfig.SetSource(_PLAYERBOT_CONFIG))
        sLog.outError("Playerbot: Unable to open configuration file. Database will be unaccessible. Configuration values will use default.");
    else
        sLog.outString("Playerbot: Using configuration file %s", _PLAYERBOT_CONFIG.c_str());

    //Check playerbot config file version
    if (botConfig.GetIntDefault("ConfVersion", 0) != PLAYERBOT_CONF_VERSION)
        sLog.outError("Playerbot: Configuration file version doesn't match expected version. Some config variables may be wrong or missing.");
}

PlayerbotMgr::PlayerbotMgr(Player* const master) : m_master(master)
{
    // load config variables
    m_confMaxNumBots = botConfig.GetIntDefault("PlayerbotAI.MaxNumBots", 9);
    m_confDebugWhisper = botConfig.GetBoolDefault("PlayerbotAI.DebugWhisper", false);
    m_confFollowDistance[0] = botConfig.GetFloatDefault("PlayerbotAI.FollowDistanceMin", 0.5f);
    m_confFollowDistance[1] = botConfig.GetFloatDefault("PlayerbotAI.FollowDistanceMax", 1.0f);
    m_confCollectCombat = botConfig.GetBoolDefault("PlayerbotAI.Collect.Combat", true);
    m_confCollectQuest = botConfig.GetBoolDefault("PlayerbotAI.Collect.Quest", true);
    m_confCollectProfession = botConfig.GetBoolDefault("PlayerbotAI.Collect.Profession", true);
    m_confCollectLoot = botConfig.GetBoolDefault("PlayerbotAI.Collect.Loot", true);
    m_confCollectSkin = botConfig.GetBoolDefault("PlayerbotAI.Collect.Skin", true);
    m_confCollectObjects = botConfig.GetBoolDefault("PlayerbotAI.Collect.Objects", true);
    m_confCollectDistanceMax = botConfig.GetIntDefault("PlayerbotAI.Collect.DistanceMax", 50);
    gConfigSellLevelDiff = botConfig.GetIntDefault("PlayerbotAI.SellAll.LevelDiff", 10);
    if (m_confCollectDistanceMax > 100)
    {
        sLog.outError("Playerbot: PlayerbotAI.Collect.DistanceMax higher than allowed. Using 100");
        m_confCollectDistanceMax = 100;
    }
    m_confCollectDistance = botConfig.GetIntDefault("PlayerbotAI.Collect.Distance", 25);
    if (m_confCollectDistance > m_confCollectDistanceMax)
    {
        sLog.outError("Playerbot: PlayerbotAI.Collect.Distance higher than PlayerbotAI.Collect.DistanceMax. Using DistanceMax value");
        m_confCollectDistance = m_confCollectDistanceMax;
    }
}

PlayerbotMgr::~PlayerbotMgr()
{
    LogoutAllBots(true);
}

void PlayerbotMgr::UpdateAI(const uint32 /*diff*/) {}

void PlayerbotMgr::HandleMasterIncomingPacket(const WorldPacket& packet)
{
    switch (packet.GetOpcode())
    {
        case CMSG_OFFER_PETITION:
        {
            WorldPacket p(packet);
            p.rpos(0);    // reset reader
            ObjectGuid petitionGuid;
            ObjectGuid playerGuid;
            uint32 junk;

            p >> junk;                                      // this is not petition type!
            p >> petitionGuid;                              // petition guid
            p >> playerGuid;                                // player guid

            Player* player = ObjectAccessor::FindPlayer(playerGuid);
            if (!player)
                return;

            uint32 petitionLowGuid = petitionGuid.GetCounter();

            auto queryResult = CharacterDatabase.PQuery("SELECT * FROM petition_sign WHERE playerguid = '%u' AND petitionguid = '%u'", player->GetGUIDLow(), petitionLowGuid);

            if (queryResult)
            {
                ChatHandler(m_master).PSendSysMessage("%s has already signed the petition", player->GetName());
                return;
            }

            CharacterDatabase.PExecute("INSERT INTO petition_sign (ownerguid,petitionguid, playerguid, player_account) VALUES ('%u', '%u', '%u','%u')",
                                       GetMaster()->GetGUIDLow(), petitionLowGuid, player->GetGUIDLow(), GetMaster()->GetSession()->GetAccountId());

            p.Initialize(SMSG_PETITION_SIGN_RESULTS, (8 + 8 + 4));
            p << ObjectGuid(petitionGuid);
            p << ObjectGuid(playerGuid);
            p << uint32(PETITION_SIGN_OK);

            // close at signer side
            GetMaster()->GetSession()->SendPacket(p);

            return;
        }

        case CMSG_ACTIVATETAXI:
        {
            WorldPacket p(packet);
            p.rpos(0); // reset reader

            ObjectGuid guid;
            std::vector<uint32> nodes;
            nodes.resize(2);
            uint8 delay = 9;
            p >> guid >> nodes[0] >> nodes[1];

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_ACTIVATETAXI from %d to %d", nodes[0], nodes[1]);

            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {

                delay = delay + 3;
                Player* const bot = it->second;
                if (!bot)
                    return;

                Group* group = bot->GetGroup();
                if (!group)
                    continue;

                Unit* target = ObjectAccessor::GetUnit(*bot, guid);

                bot->GetPlayerbotAI()->SetIgnoreUpdateTime(delay);

                bot->GetMotionMaster()->Clear(true);
                bot->GetMotionMaster()->MoveFollow(target, INTERACTION_DISTANCE, bot->GetOrientation());
                bot->GetPlayerbotAI()->GetTaxi(guid, nodes);
            }
            return;
        }

        case CMSG_ACTIVATETAXIEXPRESS:
        {
            WorldPacket p(packet);
            p.rpos(0); // reset reader

            ObjectGuid guid;
            uint32 node_count;
            uint8 delay = 9;
            p >> guid;
            p >> node_count;

            std::vector<uint32> nodes;
            for (uint32 i = 0; i < node_count; ++i)
            {
                uint32 node;
                p >> node;
                nodes.push_back(node);
            }

            if (nodes.empty())
                return;

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_ACTIVATETAXIEXPRESS from %d to %d", nodes.front(), nodes.back());

            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {

                delay = delay + 3;
                Player* const bot = it->second;
                if (!bot)
                    return;
                Group* group = bot->GetGroup();
                if (!group)
                    continue;
                Unit* target = ObjectAccessor::GetUnit(*bot, guid);

                bot->GetPlayerbotAI()->SetIgnoreUpdateTime(delay);

                bot->GetMotionMaster()->Clear(true);
                bot->GetMotionMaster()->MoveFollow(target, INTERACTION_DISTANCE, bot->GetOrientation());
                bot->GetPlayerbotAI()->GetTaxi(guid, nodes);
            }
            return;
        }

        // if master is logging out, log out all bots
        case CMSG_LOGOUT_REQUEST:
        {
            LogoutAllBots();
            return;
        }

        // If master inspects one of his bots, give the master useful info in chat window
        // such as inventory that can be equipped
        case CMSG_INSPECT:
        {
            WorldPacket p(packet);
            p.rpos(0); // reset reader
            ObjectGuid guid;
            p >> guid;
            Player* const bot = GetPlayerBot(guid);
            if (bot) bot->GetPlayerbotAI()->SendNotEquipList(*bot);
            return;
        }

        // handle emotes from the master
        //case CMSG_EMOTE:
        case CMSG_TEXT_EMOTE:
        {
            WorldPacket p(packet);
            p.rpos(0); // reset reader
            uint32 emoteNum;
            p >> emoteNum;

            /* std::ostringstream out;
            out << "emote is: " << emoteNum;
            ChatHandler ch(m_master);
            ch.SendSysMessage(out.str().c_str()); */

            switch (emoteNum)
            {
                case TEXTEMOTE_BOW:
                {
                    // Buff anyone who bows before me. Useful for players not in bot's group
                    // How do I get correct target???
                    //Player* const pPlayer = GetPlayerBot(m_master->GetSelection());
                    //if (pPlayer->GetPlayerbotAI()->GetClassAI())
                    //    pPlayer->GetPlayerbotAI()->GetClassAI()->BuffPlayer(pPlayer);
                    return;
                }
                /*
                case TEXTEMOTE_BONK:
                {
                Player* const pPlayer = GetPlayerBot(m_master->GetSelection());
                if (!pPlayer || !pPlayer->GetPlayerbotAI())
                return;
                PlayerbotAI* const pBot = pPlayer->GetPlayerbotAI();

                ChatHandler ch(m_master);
                {
                std::ostringstream out;
                out << "CurrentTime: " << CurrentTime()
                << " m_ignoreAIUpdatesUntilTime: " << pBot->m_ignoreAIUpdatesUntilTime;
                ch.SendSysMessage(out.str().c_str());
                }
                {
                std::ostringstream out;
                out << "m_CurrentlyCastingSpellId: " << pBot->m_CurrentlyCastingSpellId;
                ch.SendSysMessage(out.str().c_str());
                }
                {
                std::ostringstream out;
                out << "IsBeingTeleported() " << pBot->GetPlayer()->IsBeingTeleported();
                ch.SendSysMessage(out.str().c_str());
                }
                {
                std::ostringstream out;
                bool tradeActive = (pBot->GetPlayer()->GetTrader()) ? true : false;
                out << "tradeActive: " << tradeActive;
                ch.SendSysMessage(out.str().c_str());
                }
                {
                std::ostringstream out;
                out << "IsCharmed() " << pBot->getPlayer()->isCharmed();
                ch.SendSysMessage(out.str().c_str());
                }
                return;
                }
                */

                case TEXTEMOTE_EAT:
                case TEXTEMOTE_DRINK:
                    return;

                // emote to attack selected target
                case TEXTEMOTE_POINT:
                {
                    ObjectGuid attackOnGuid = m_master->GetSelectionGuid();
                    if (!attackOnGuid)
                        return;

                    Unit* thingToAttack = ObjectAccessor::GetUnit(*m_master, attackOnGuid);
                    if (!thingToAttack) return;

                    Player* bot = 0;
                    for (PlayerBotMap::iterator itr = m_playerBots.begin(); itr != m_playerBots.end(); ++itr)
                    {
                        bot = itr->second;
                        if (bot->CanAttack(thingToAttack))
                        {
                            if (!bot->IsWithinLOSInMap(thingToAttack))
                                bot->GetPlayerbotAI()->DoTeleport(*m_master);
                            if (bot->IsWithinLOSInMap(thingToAttack))
                                bot->GetPlayerbotAI()->Attack(thingToAttack);
                        }
                    }
                    return;
                }

                // emote to stay
                case TEXTEMOTE_STAND:
                {
                    Player* const bot = GetPlayerBot(m_master->GetSelectionGuid());
                    if (bot)
                        bot->GetPlayerbotAI()->SetMovementOrder(PlayerbotAI::MOVEMENT_STAY);
                    else
                        for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
                        {
                            Player* const bot = it->second;
                            bot->GetPlayerbotAI()->SetMovementOrder(PlayerbotAI::MOVEMENT_STAY);
                        }
                    return;
                }

                // 324 is the followme emote (not defined in enum)
                // if master has bot selected then only bot follows, else all bots follow
                case 324:
                case TEXTEMOTE_WAVE:
                {
                    Player* const bot = GetPlayerBot(m_master->GetSelectionGuid());
                    if (bot)
                        bot->GetPlayerbotAI()->SetMovementOrder(PlayerbotAI::MOVEMENT_FOLLOW, m_master);
                    else
                        for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
                        {
                            Player* const bot = it->second;
                            bot->GetPlayerbotAI()->SetMovementOrder(PlayerbotAI::MOVEMENT_FOLLOW, m_master);
                        }
                    return;
                }
            }
            return;
            } /* EMOTE ends here */

        case CMSG_GAMEOBJ_USE: // not sure if we still need this one
        case CMSG_GAMEOBJ_REPORT_USE:
        {
            WorldPacket p(packet);
            p.rpos(0);     // reset reader
            ObjectGuid objGUID;
            p >> objGUID;
            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;

                // If player and bot are on different maps: then player was teleported by GameObject
                // let's return and let playerbot summon do its job by teleporting bots
                Map* masterMap = m_master->IsInWorld() ? m_master->GetMap() : nullptr;
                if (!masterMap || bot->GetMap() != masterMap || m_master->IsBeingTeleported())
                    return;

                bot->GetPlayerbotAI()->FollowAutoReset();
                GameObject* obj = masterMap->GetGameObject(objGUID);
                if (!obj)
                    return;

                // add other go types here, i.e.:
                // GAMEOBJECT_TYPE_CHEST - loot quest items of chest
                if (obj->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
                {
                    bot->GetPlayerbotAI()->TurnInQuests(obj);

                    // auto accept every available quest this NPC has
                    bot->PrepareQuestMenu(objGUID);
                    QuestMenu& questMenu = bot->GetPlayerMenu()->GetQuestMenu();
                    for (uint32 iI = 0; iI < questMenu.MenuItemCount(); ++iI)
                    {
                        QuestMenuItem const& qItem = questMenu.GetItem(iI);
                        uint32 questID = qItem.m_qId;
                        if (!bot->GetPlayerbotAI()->AddQuest(questID, obj))
                            DEBUG_LOG("Couldn't take quest");
                    }
                }
            }
        }
        break;

        case CMSG_QUESTGIVER_HELLO:
        {
            WorldPacket p(packet);
            p.rpos(0);    // reset reader
            ObjectGuid npcGUID;
            p >> npcGUID;
            WorldObject* pNpc = m_master->GetMap()->GetWorldObject(npcGUID);
            if (!pNpc)
                return;

            // for all master's bots
            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;
                bot->GetPlayerbotAI()->FollowAutoReset();
                bot->GetPlayerbotAI()->TurnInQuests(pNpc);
            }

            return;
        }

        // if master accepts a quest, bots should also try to accept quest
        case CMSG_QUESTGIVER_ACCEPT_QUEST:
        {
            WorldPacket p(packet);
            p.rpos(0);    // reset reader
            ObjectGuid guid;
            uint32 quest;
            uint32 unk1;
            p >> guid >> quest >> unk1;

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_QUESTGIVER_ACCEPT_QUEST npc = %s, quest = %u, unk1 = %u", guid.GetString().c_str(), quest, unk1);

            Quest const* qInfo = sObjectMgr.GetQuestTemplate(quest);
            if (qInfo)
                for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
                {
                    Player* const bot = it->second;
                    bot->GetPlayerbotAI()->FollowAutoReset();
                    if (bot->GetQuestStatus(quest) == QUEST_STATUS_COMPLETE)
                        bot->GetPlayerbotAI()->TellMaster("I already completed that quest.");
                    else if (!bot->CanTakeQuest(qInfo, false))
                    {
                        if (!bot->SatisfyQuestStatus(qInfo, false))
                            bot->GetPlayerbotAI()->TellMaster("I already have that quest.");
                        else
                            bot->GetPlayerbotAI()->TellMaster("I can't take that quest.");
                    }
                    else if (!bot->SatisfyQuestLog(false))
                        bot->GetPlayerbotAI()->TellMaster("My quest log is full.");
                    else if (!bot->CanAddQuest(qInfo, false))
                        bot->GetPlayerbotAI()->TellMaster("I can't take that quest because it requires that I take items, but my bags are full!");

                    else
                    {
                        p.rpos(0);         // reset reader
                        bot->GetSession()->HandleQuestgiverAcceptQuestOpcode(p);
                        bot->GetPlayerbotAI()->TellMaster("Got the quest.");

                        // build needed items if quest contains any
                        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
                            if (qInfo->ReqItemCount[i] > 0)
                            {
                                bot->GetPlayerbotAI()->SetQuestNeedItems();
                                break;
                            }

                        // build needed creatures if quest contains any
                        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
                            if (qInfo->ReqCreatureOrGOCount[i] > 0)
                            {
                                bot->GetPlayerbotAI()->SetQuestNeedCreatures();
                                break;
                            }
                    }
                }
            return;
        }

        case CMSG_AREATRIGGER:
        {
            WorldPacket p(packet);

            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;
                if (!bot)
                    continue;

                if (bot->IsWithinDistInMap(GetMaster(), 50))
                {
                    p.rpos(0);         // reset reader
                    bot->GetSession()->HandleAreaTriggerOpcode(p);
                }
            }
            return;
        }

        case CMSG_QUESTGIVER_COMPLETE_QUEST:
        {
            WorldPacket p(packet);
            p.rpos(0);    // reset reader
            uint32 quest;
            ObjectGuid npcGUID;
            p >> npcGUID >> quest;

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_QUESTGIVER_COMPLETE_QUEST npc = %s, quest = %u", npcGUID.GetString().c_str(), quest);

            WorldObject* pNpc = m_master->GetMap()->GetWorldObject(npcGUID);
            if (!pNpc)
                return;

            // for all master's bots
            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;
                bot->GetPlayerbotAI()->FollowAutoReset();
                bot->GetPlayerbotAI()->TurnInQuests(pNpc);
            }
            return;
        }

        case CMSG_LOOT_ROLL:
        {
            WorldPacket p(packet);  //WorldPacket packet for CMSG_LOOT_ROLL, (8+4+1)
            ObjectGuid Guid;
            uint32 itemSlot;
            uint8 rollType;

            p.rpos(0);              //reset packet pointer
            p >> Guid;              //guid of the lootable target
            p >> itemSlot;          //loot index
            p >> rollType;          //need,greed or pass on roll

            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                uint32 choice = 0;

                Player* const bot = it->second;
                if (!bot)
                    return;

                Group* group = bot->GetGroup();
                if (!group)
                    return;

                // check that the bot did not already vote
                if (rollType >= ROLL_NOT_EMITED_YET)
                    return;

                Loot* loot = sLootMgr.GetLoot(bot, Guid);

                if (!loot)
                {
                    sLog.outError("LootMgr::PlayerVote> Error cannot get loot object info!");
                    return;
                }

                LootItem* lootItem = loot->GetLootItemInSlot(itemSlot);

                ItemPrototype const* pProto = lootItem->itemProto;
                if (!pProto)
                    return;

                if (bot->GetPlayerbotAI()->CanStore())
                {
                    if (bot->CanUseItem(pProto) == EQUIP_ERR_OK && bot->GetPlayerbotAI()->IsItemUseful(lootItem->itemId))
                        choice = 1;     // Need
                    else if (bot->HasSkill(SKILL_ENCHANTING))
                        choice = 3;     // Disenchant
                    else
                        choice = 2;     // Greed
                }
                else
                    choice = 0;         // Pass

                sLootMgr.PlayerVote(bot, Guid, itemSlot, RollVote(choice));

                switch (choice)
                {
                    case ROLL_NEED:
                        bot->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED, 1);
                        break;
                    case ROLL_GREED:
                    case ROLL_DISENCHANT:
                        bot->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED, 1);
                        break;
                }
            }
            return;
        }

        // Handle GOSSIP activate actions, prior to GOSSIP select menu actions
        case CMSG_GOSSIP_HELLO:
        {
            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_GOSSIP_HELLO");

            WorldPacket p(packet);    //WorldPacket packet for CMSG_GOSSIP_HELLO, (8)
            ObjectGuid guid;
            p.rpos(0);                //reset packet pointer
            p >> guid;
            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;
                if (!bot)
                    continue;
                bot->GetPlayerbotAI()->FollowAutoReset();
                Creature* pCreature = bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
                if (!pCreature)
                {
                    DEBUG_LOG("[PlayerbotMgr]: HandleMasterIncomingPacket - Received  CMSG_GOSSIP_HELLO %s not found or you can't interact with him.", guid.GetString().c_str());
                    continue;
                }

                GossipMenuItemsMapBounds pMenuItemBounds = sObjectMgr.GetGossipMenuItemsMapBounds(pCreature->GetCreatureInfo()->GossipMenuId);
                for (GossipMenuItemsMap::const_iterator itr = pMenuItemBounds.first; itr != pMenuItemBounds.second; ++itr)
                {
                    uint32 npcflags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);

                    if (!(itr->second.npc_option_npcflag & npcflags))
                        continue;

                    switch (itr->second.option_id)
                    {
                        case GOSSIP_OPTION_TAXIVENDOR:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_TAXIVENDOR");
                            bot->GetSession()->SendLearnNewTaxiNode(pCreature);
                            break;
                        }
                        case GOSSIP_OPTION_QUESTGIVER:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_QUESTGIVER");
                            bot->GetPlayerbotAI()->TurnInQuests(pCreature);
                            break;
                        }
                        case GOSSIP_OPTION_VENDOR:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_VENDOR");
                            if (!botConfig.GetBoolDefault("PlayerbotAI.SellGarbage", true))
                                continue;

                            // changed the SellGarbage() function to support ch.SendSysMessaage()
                            bot->GetPlayerbotAI()->SellGarbage(*bot);
                            break;
                        }
                        case GOSSIP_OPTION_STABLEPET:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_STABLEPET");
                            break;
                        }
                        case GOSSIP_OPTION_AUCTIONEER:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_AUCTIONEER");
                            break;
                        }
                        case GOSSIP_OPTION_BANKER:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_BANKER");
                            break;
                        }
                        case GOSSIP_OPTION_INNKEEPER:
                        {
                            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_INNKEEPER");
                            break;
                        }
                    }
                }
            }
            return;
        }

        case CMSG_SPIRIT_HEALER_ACTIVATE:
        {
            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_SPIRIT_HEALER_ACTIVATE SpiritHealer is resurrecting the Player %s",m_master->GetName());
            for (PlayerBotMap::iterator itr = m_playerBots.begin(); itr != m_playerBots.end(); ++itr)
            {
                Player* const bot = itr->second;
                Group* grp = bot->GetGroup();
                if (grp)
                    grp->RemoveMember(bot->GetObjectGuid(), 1);
            }
            return;
        }

        case CMSG_LIST_INVENTORY:
        {
            if (!botConfig.GetBoolDefault("PlayerbotAI.SellGarbage", true))
                return;

            WorldPacket p(packet);
            p.rpos(0);  // reset reader
            ObjectGuid npcGUID;
            p >> npcGUID;
            Object* const pNpc = (WorldObject*) m_master->GetObjectByTypeMask(npcGUID, TYPEMASK_CREATURE_OR_GAMEOBJECT);
            if (!pNpc)
                return;

            // for all master's bots
            for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;
                if (!bot->IsInMap(static_cast<WorldObject*>(pNpc)))
                {
                    bot->GetPlayerbotAI()->TellMaster("I'm too far away to sell items!");
                    continue;
                }
                else
                {
                    // changed the SellGarbage() function to support ch.SendSysMessaage()
                    bot->GetPlayerbotAI()->FollowAutoReset();
                    bot->GetPlayerbotAI()->SellGarbage(*bot);
                }
            }
            return;
        }

        /*
        case CMSG_NAME_QUERY:
        case MSG_MOVE_START_FORWARD:
        case MSG_MOVE_STOP:
        case MSG_MOVE_SET_FACING:
        case MSG_MOVE_START_STRAFE_LEFT:
        case MSG_MOVE_START_STRAFE_RIGHT:
        case MSG_MOVE_STOP_STRAFE:
        case MSG_MOVE_START_BACKWARD:
        case MSG_MOVE_HEARTBEAT:
        case CMSG_STANDSTATECHANGE:
        case CMSG_QUERY_TIME:
        case CMSG_CREATURE_QUERY:
        case CMSG_GAMEOBJECT_QUERY:
        case MSG_MOVE_JUMP:
        case MSG_MOVE_FALL_LAND:
        return;*/

        default:
        {
            /*const char* oc = LookupOpcodeName(packet.GetOpcode());
            // ChatHandler ch(m_master);
            // ch.SendSysMessage(oc);

            std::ostringstream out;
            out << "masterin: " << oc;
            sLog.outError(out.str().c_str()); */
        }
    }
}

void PlayerbotMgr::HandleMasterOutgoingPacket(const WorldPacket& packet)
{
    switch (packet.GetOpcode())
    {
        // If a change in speed was detected for the master
        // make sure we have the same mount status
        case SMSG_FORCE_RUN_SPEED_CHANGE:
        {
            WorldPacket p(packet);
            ObjectGuid guid;

            // Only adjust speed and mount if this is master that did so
            p >> guid.ReadAsPacked();
            if (guid != GetMaster()->GetObjectGuid())
                return;

            for (auto it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                Player* const bot = it->second;
                if (GetMaster()->IsMounted() && !bot->IsMounted())
                {
                    // Player Part
                    if (!GetMaster()->GetAurasByType(SPELL_AURA_MOUNTED).empty())
                    {
                        int32 master_speed1 = 0;
                        int32 master_speed2 = 0;
                        master_speed1 = GetMaster()->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetSpellProto()->EffectBasePoints[1];
                        master_speed2 = GetMaster()->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetSpellProto()->EffectBasePoints[2];

                        // Bot Part
                        // Step 1: find spell in bot spellbook that matches the speed change from master
                        uint32 spellMount = 0;
                        for (auto & itr : bot->GetSpellMap())
                        {
                            uint32 spellId = itr.first;
                            if (itr.second.state == PLAYERSPELL_REMOVED || itr.second.disabled || IsPassiveSpell(spellId))
                                continue;
                            const SpellEntry* pSpellInfo = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
                            if (!pSpellInfo)
                                continue;

                            if (pSpellInfo->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED)
                            {
                                if (pSpellInfo->EffectApplyAuraName[1] == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
                                {
                                    if (pSpellInfo->EffectBasePoints[1] == master_speed1)
                                    {
                                        spellMount = spellId;
                                        break;
                                    }
                                }
                                else if ((pSpellInfo->EffectApplyAuraName[1] == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
                                         && (pSpellInfo->EffectApplyAuraName[2] == SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED))
                                {
                                    if ((pSpellInfo->EffectBasePoints[1] == master_speed1)
                                        && (pSpellInfo->EffectBasePoints[2] == master_speed2))
                                    {
                                        spellMount = spellId;
                                        break;
                                    }
                                }
                                else if ((pSpellInfo->EffectApplyAuraName[2] == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
                                         && (pSpellInfo->EffectApplyAuraName[1] == SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED))
                                    if ((pSpellInfo->EffectBasePoints[2] == master_speed2)
                                        && (pSpellInfo->EffectBasePoints[1] == master_speed1))
                                    {
                                        spellMount = spellId;
                                        break;
                                    }
                            }
                        }
                        if (spellMount > 0 && bot->CastSpell(bot, spellMount, TRIGGERED_NONE) == SPELL_CAST_OK)
                            return;

                        // Step 2: no spell found or cast failed -> search for an item in inventory (mount)
                        // We start with the fastest mounts as bot will not be able to outrun its master since it is following him/her
                        uint32 skillLevels[] = { 375, 300, 225, 150, 75 };
                        for (uint32 level : skillLevels)
                        {
                            Item* mount = bot->GetPlayerbotAI()->FindMount(level);
                            if (mount)
                            {
                                bot->GetPlayerbotAI()->UseItem(mount);
                                return;
                            }
                        }
                    }
                }
                // If master dismounted, do so
                else if (!GetMaster()->IsMounted() && bot->IsMounted())    // only execute code if master is the one who dismounted
                {
                    WorldPacket emptyPacket;
                    bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);  //updated code
                }
            }
        }
        // maybe our bots should only start looting after the master loots?
        //case SMSG_LOOT_RELEASE_RESPONSE: {}
        case SMSG_NAME_QUERY_RESPONSE:
        case SMSG_MONSTER_MOVE:
        case SMSG_COMPRESSED_UPDATE_OBJECT:
        case SMSG_DESTROY_OBJECT:
        case SMSG_UPDATE_OBJECT:
        case SMSG_STANDSTATE_UPDATE:
        case MSG_MOVE_HEARTBEAT:
        case SMSG_QUERY_TIME_RESPONSE:
        case SMSG_AURA_UPDATE_ALL:
        case SMSG_CREATURE_QUERY_RESPONSE:
        case SMSG_GAMEOBJECT_QUERY_RESPONSE:
            return;
        default:
        {
/*            const char* oc = LookupOpcodeName(packet.GetOpcode());

            std::ostringstream out;
            out << "masterout: " << oc;
            sLog.outError(out.str().c_str());
*/
        }

    }
}

void PlayerbotMgr::RemoveBots()
{
    for (auto& guid : m_botToRemove)
    {
        Player* bot = GetPlayerBot(guid);
        if (bot)
        {
            WorldSession* botWorldSessionPtr = bot->GetSession();
            m_playerBots.erase(guid);                               // deletes bot player ptr inside this WorldSession PlayerBotMap
            botWorldSessionPtr->LogoutPlayer();                     // this will delete the bot Player object and PlayerbotAI object
            delete botWorldSessionPtr;                              // finally delete the bot's WorldSession
        }
    }

    m_botToRemove.clear();
}

void PlayerbotMgr::LogoutAllBots(bool fullRemove /*= false*/)
{
    for (auto itr : m_playerBots)
        m_botToRemove.insert(itr.first);

    if (fullRemove)
        RemoveBots();
}

void PlayerbotMgr::Stay()
{
    for (PlayerBotMap::const_iterator itr = GetPlayerBotsBegin(); itr != GetPlayerBotsEnd(); ++itr)
    {
        Player* bot = itr->second;
        bot->GetMotionMaster()->Clear();
    }
}

// Playerbot mod: logs out a Playerbot.
void PlayerbotMgr::LogoutPlayerBot(ObjectGuid guid)
{
    m_botToRemove.insert(guid);
}

// Playerbot mod: Gets a player bot Player object for this WorldSession master
Player* PlayerbotMgr::GetPlayerBot(ObjectGuid playerGuid) const
{
    PlayerBotMap::const_iterator it = m_playerBots.find(playerGuid);
    return (it == m_playerBots.end()) ? 0 : it->second;
}

void PlayerbotMgr::OnBotLogin(Player* const bot)
{
    // simulate client taking control
    WorldPacket* const pCMSG_SET_ACTIVE_MOVER = new WorldPacket(CMSG_SET_ACTIVE_MOVER, 8);
    *pCMSG_SET_ACTIVE_MOVER << bot->GetObjectGuid();
    bot->GetSession()->QueuePacket(std::unique_ptr<WorldPacket>(pCMSG_SET_ACTIVE_MOVER));

    WorldPacket* const pMSG_MOVE_FALL_LAND = new WorldPacket(MSG_MOVE_FALL_LAND, 64);
    pMSG_MOVE_FALL_LAND->appendPackGUID(bot->GetObjectGuid());
    *pMSG_MOVE_FALL_LAND << bot->GetMover()->m_movementInfo;
    bot->GetSession()->QueuePacket(std::unique_ptr<WorldPacket>(pMSG_MOVE_FALL_LAND));

    // give the bot some AI, object is owned by the player class
    PlayerbotAI* ai = new PlayerbotAI(*this, bot, m_confDebugWhisper);
    bot->SetPlayerbotAI(ai);

    // tell the world session that they now manage this new bot
    m_playerBots[bot->GetObjectGuid()] = bot;

    // if bot is in a group and master is not in group then
    // have bot leave their group
    if (bot->GetGroup() &&
            (m_master->GetGroup() == nullptr ||
             m_master->GetGroup()->IsMember(bot->GetObjectGuid()) == false))
        bot->RemoveFromGroup();

    // sometimes master can lose leadership, pass leadership to master check
    const ObjectGuid masterGuid = m_master->GetObjectGuid();
    if (m_master->GetGroup() &&
            !m_master->GetGroup()->IsLeader(masterGuid))
        // But only do so if one of the master's bots is leader
        for (PlayerBotMap::const_iterator itr = GetPlayerBotsBegin(); itr != GetPlayerBotsEnd(); itr++)
        {
            Player* bot = itr->second;
            if (m_master->GetGroup()->IsLeader(bot->GetObjectGuid()))
            {
                m_master->GetGroup()->ChangeLeader(masterGuid);
                break;
            }
        }
}

void PlayerbotMgr::RemoveAllBotsFromGroup()
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); m_master->GetGroup() && it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (bot->IsInGroup(m_master))
            m_master->GetGroup()->RemoveMember(bot->GetObjectGuid(), 0);
    }
}

void Creature::LoadBotMenu(Player* pPlayer)
{

    if (pPlayer->GetPlayerbotAI()) return;
    ObjectGuid guid = pPlayer->GetObjectGuid();
    uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    auto queryResult = CharacterDatabase.PQuery("SELECT guid, name FROM characters WHERE account='%d'", accountId);
    do
    {
        Field* fields = queryResult->Fetch();
        ObjectGuid guidlo = ObjectGuid(fields[0].GetUInt64());
        std::string name = fields[1].GetString();
        std::string word = "";

        if ((guid == ObjectGuid()) || (guid == guidlo))
        {
            //not found or himself
        }
        else
        {
            // if(sConfig.GetBoolDefault("PlayerbotAI.DisableBots", false)) return;
            // create the manager if it doesn't already exist
            if (!pPlayer->GetPlayerbotMgr())
                pPlayer->SetPlayerbotMgr(new PlayerbotMgr(pPlayer));
            if (pPlayer->GetPlayerbotMgr()->GetPlayerBot(guidlo) == nullptr) // add (if not already in game)
            {
                word += "Recruit ";
                word += name;
                word += " as a Bot.";
                pPlayer->GetPlayerMenu()->GetGossipMenu().AddMenuItem((uint8) 9, word, guidlo, GOSSIP_OPTION_BOT, word, false);
            }
            else if (pPlayer->GetPlayerbotMgr()->GetPlayerBot(guidlo) != nullptr) // remove (if in game)
            {
                word += "Dismiss ";
                word += name;
                word += " from duty.";
                pPlayer->GetPlayerMenu()->GetGossipMenu().AddMenuItem((uint8) 0, word, guidlo, GOSSIP_OPTION_BOT, word, false);
            }
        }
    }
    while (queryResult->NextRow());
}

void Player::skill(std::list<uint32>& m_spellsToLearn)
{
    for (SkillStatusMap::const_iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        if (itr->second.uState == SKILL_DELETED)
            continue;

        uint32 pskill = itr->first;

        m_spellsToLearn.push_back(pskill);
    }
}

void Player::MakeTalentGlyphLink(std::ostringstream& out)
{
    // |cff4e96f7|Htalent:1396:4|h[Unleashed Fury]|h|r
    // |cff66bbff|Hglyph:23:460|h[Glyph of Fortitude]|h|r

    if (m_specsCount)
        // loop through all specs (only 1 for now)
        for (uint32 specIdx = 0; specIdx < m_specsCount; ++specIdx)
        {
            // find class talent tabs (all players have 3 talent tabs)
            uint32 const* talentTabIds = GetTalentTabPages(getClass());

            out << "\n" << "Active Talents ";

            for (uint32 i = 0; i < 3; ++i)
            {
                uint32 talentTabId = talentTabIds[i];
                for (PlayerTalentMap::iterator iter = m_talents[specIdx].begin(); iter != m_talents[specIdx].end(); ++iter)
                {
                    PlayerTalent talent = (*iter).second;

                    if (talent.state == PLAYERSPELL_REMOVED)
                        continue;

                    // skip another tab talents
                    if (talent.talentEntry->TalentTab != talentTabId)
                        continue;

                    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talent.talentEntry->TalentID);

                    SpellEntry const* spell_entry = sSpellTemplate.LookupEntry<SpellEntry>(talentInfo->RankID[talent.currentRank]);

                    out << "|cff4e96f7|Htalent:" << talent.talentEntry->TalentID << ":" << talent.currentRank
                        << " |h[" << spell_entry->SpellName[GetSession()->GetSessionDbcLocale()] << "]|h|r";
                }
            }

            uint32 freepoints = 0;

            out << " Unspent points : ";

            if ((freepoints = GetFreeTalentPoints()) > 0)
                out << "|h|cff00ff00" << freepoints << "|h|r";
            else
                out << "|h|cffff0000" << freepoints << "|h|r";

            out << "\n" << "Active Glyphs ";
            // GlyphProperties.dbc
            for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
            {
                GlyphPropertiesEntry const* glyph = sGlyphPropertiesStore.LookupEntry(m_glyphs[specIdx][i].GetId());
                if (!glyph)
                    continue;

                SpellEntry const* spell_entry = sSpellTemplate.LookupEntry<SpellEntry>(glyph->SpellId);

                out << "|cff66bbff|Hglyph:" << GetGlyphSlot(i) << ":" << m_glyphs[specIdx][i].GetId()
                    << " |h[" << spell_entry->SpellName[GetSession()->GetSessionDbcLocale()] << "]|h|r";

            }
        }
}

void Player::chompAndTrim(std::string& str)
{
    while (str.length() > 0)
    {
        char lc = str[str.length() - 1];
        if (lc == '\r' || lc == '\n' || lc == ' ' || lc == '"' || lc == '\'')
            str = str.substr(0, str.length() - 1);
        else
            break;
    }

    while (str.length() > 0)
    {
        char lc = str[0];
        if (lc == ' ' || lc == '"' || lc == '\'')
            str = str.substr(1, str.length() - 1);
        else
            break;
    }
}

bool Player::getNextQuestId(const std::string& pString, unsigned int& pStartPos, unsigned int& pId)
{
    bool result = false;
    unsigned int i;
    for (i = pStartPos; i < pString.size(); ++i)
    {
        if (pString[i] == ',')
            break;
    }
    if (i > pStartPos)
    {
        std::string idString = pString.substr(pStartPos, i - pStartPos);
        pStartPos = i + 1;
        chompAndTrim(idString);
        pId = atoi(idString.c_str());
        result = true;
    }
    return (result);
}

bool Player::requiredQuests(const char* pQuestIdString)
{
    if (pQuestIdString != nullptr)
    {
        unsigned int pos = 0;
        unsigned int id;
        std::string confString(pQuestIdString);
        chompAndTrim(confString);
        while (getNextQuestId(confString, pos, id))
        {
            QuestStatus status = GetQuestStatus(id);
            if (status == QUEST_STATUS_COMPLETE)
                return true;
        }
    }
    return false;
}

void Player::UpdateMail()
{
    // save money,items and mail to prevent cheating
    CharacterDatabase.BeginTransaction();
    this->SaveGoldToDB();
    this->SaveInventoryAndGoldToDB();
    this->_SaveMail();
    CharacterDatabase.CommitTransaction();
}

//See MainSpec enum in PlayerbotAI.h for details on class return values
uint32 Player::GetSpec()
{
    uint32 row = 0, spec = 0;

    //Iterate through the 3 talent trees
    for (uint32 i = 0; i < 3; ++i)
    {
        for (PlayerTalentMap::iterator iter = m_talents[m_activeSpec].begin(); iter != m_talents[m_activeSpec].end(); ++iter)
        {
            PlayerTalent talent = (*iter).second;
            if (row == 0 && spec == 0)
                spec = talent.talentEntry->TalentTab;

            //If current talent is deeper into a tree, that is our new max talent
            if (talent.talentEntry->Row > row)
            {
                row = talent.talentEntry->Row;

                //Set the tree the deepest talent is on
                spec = talent.talentEntry->TalentTab;
            }
        }
    }

    //Return the tree with the deepest talent
    return spec;
}

bool ChatHandler::HandlePlayerbotCommand(char* args)
{
    if (!(m_session->GetSecurity() > SEC_PLAYER))
    {
        if (botConfig.GetBoolDefault("PlayerbotAI.DisableBots", false))
        {
            PSendSysMessage("|cffff0000Playerbot system is currently disabled!");
            SetSentErrorMessage(true);
            return false;
        }
    }

    if (!m_session)
    {
        PSendSysMessage("|cffff0000You may only add bots from an active session");
        SetSentErrorMessage(true);
        return false;
    }

    if (!*args)
    {
        PSendSysMessage("|cffff0000usage: add PLAYERNAME  or  remove PLAYERNAME");
        SetSentErrorMessage(true);
        return false;
    }

    // create the playerbot manager if it doesn't already exist
    PlayerbotMgr* mgr = m_session->GetPlayer()->GetPlayerbotMgr();
    if (!mgr)
    {
        mgr = new PlayerbotMgr(m_session->GetPlayer());
        m_session->GetPlayer()->SetPlayerbotMgr(mgr);
    }

    char* cmd = strtok((char*)args, " ");
    if (!cmd)
    {
        PSendSysMessage("|cffff0000usage: add PLAYERNAME, remove PLAYERNAME, removeall");
        SetSentErrorMessage(true);
        return false;
    }

    std::string cmdStr = cmd;

    if (cmdStr == "removeall")
    {
        mgr->LogoutAllBots();
        return true;
    }

    // commands that require botname
    char* charname = strtok(NULL, " ");
    if (!charname)
    {
        PSendSysMessage("|cffff0000usage: add PLAYERNAME, remove PLAYERNAME, removeall");
        SetSentErrorMessage(true);
        return false;
    }

    std::string charnameStr = charname;

    if (!normalizePlayerName(charnameStr))
        return false;

    ObjectGuid guid = sObjectMgr.GetPlayerGuidByName(charnameStr.c_str());
    if (guid == ObjectGuid() || (guid == m_session->GetPlayer()->GetObjectGuid()))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    if (accountId != m_session->GetAccountId())
    {
        PSendSysMessage("|cffff0000You may only add bots from the same account.");
        SetSentErrorMessage(true);
        return false;
    }

    auto resultchar = CharacterDatabase.PQuery("SELECT COUNT(*) FROM characters WHERE online = '1' AND account = '%u'", m_session->GetAccountId());
    if (resultchar)
    {
        Field* fields = resultchar->Fetch();
        int acctcharcount = fields[0].GetUInt32();
        int maxnum = botConfig.GetIntDefault("PlayerbotAI.MaxNumBots", 9);
        if (!(m_session->GetSecurity() > SEC_PLAYER))
            if (acctcharcount > maxnum && (cmdStr == "add" || cmdStr == "login"))
            {
                PSendSysMessage("|cffff0000You cannot summon anymore bots.(Current Max: |cffffffff%u)", maxnum);
                SetSentErrorMessage(true);
                return false;
            }
    }

    auto resultlvl = CharacterDatabase.PQuery("SELECT level, name, race, class, map FROM characters WHERE guid = '%u'", guid.GetCounter());
    if (resultlvl)
    {
        Field* fields = resultlvl->Fetch();
        int charlvl = fields[0].GetUInt32();
        int maxlvl = botConfig.GetIntDefault("PlayerbotAI.RestrictBotLevel", 80);
        uint8 race = fields[2].GetUInt8();
        uint8 charclass = fields[3].GetUInt8();
        uint32 mapid = fields[4].GetUInt32();
        uint32 team = 0;

        team = Player::TeamForRace(race);

        if (!(m_session->GetSecurity() > SEC_PLAYER))
        {
            if (charlvl > maxlvl)
            {
                PSendSysMessage("|cffff0000You cannot summon |cffffffff[%s]|cffff0000, it's level is too high.(Current Max:lvl |cffffffff%u)", fields[1].GetString(), maxlvl);
                SetSentErrorMessage(true);
                return false;
            }

            // Death Knight still in starting map
            if (mapid == 609 && charclass == CLASS_DEATH_KNIGHT)
            {
                PSendSysMessage("|cffff0000You cannot summon |cffffffff[%s]|cffff0000, the Death Knight is not free of the Lich King.", fields[1].GetString());
                SetSentErrorMessage(true);
                return false;
            }

            // Opposing team bot
            if (!sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_GROUP) && m_session->GetPlayer()->GetTeam() != team)
            {
                PSendSysMessage("|cffff0000You cannot summon |cffffffff[%s]|cffff0000, a member of the enemy side", fields[1].GetString());
                SetSentErrorMessage(true);
                return false;
            }
        }
    }
    // end of gmconfig patch
    if (cmdStr == "add" || cmdStr == "login")
    {
        if (mgr->GetPlayerBot(guid))
        {
            PSendSysMessage("Bot already exists in world.");
            SetSentErrorMessage(true);
            return false;
        }
        CharacterDatabase.DirectPExecute("UPDATE characters SET online = 1 WHERE guid = '%u'", guid.GetCounter());
        mgr->LoginPlayerBot(guid);
        PSendSysMessage("Bot added successfully.");
    }
    else if (cmdStr == "remove" || cmdStr == "logout")
    {
        if (!mgr->GetPlayerBot(guid))
        {
            PSendSysMessage("|cffff0000Bot can not be removed because bot does not exist in world.");
            SetSentErrorMessage(true);
            return false;
        }
        CharacterDatabase.DirectPExecute("UPDATE characters SET online = 0 WHERE guid = '%u'", guid.GetCounter());
        mgr->LogoutPlayerBot(guid);
        PSendSysMessage("Bot removed successfully.");
    }

    return true;
}
