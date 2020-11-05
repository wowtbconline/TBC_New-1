#include "ScriptMgr.h"
#include "EscortAI.h"
#include "Group.h"

enum eMatureNetherwing
{
    SAY_JUST_EATEN              = -1000175,

    SPELL_PLACE_CARCASS         = 38439,
    SPELL_JUST_EATEN            = 38502,
    SPELL_NETHER_BREATH         = 38467,
    POINT_ID                    = 1,

    GO_CARCASS                  = 185155,

    QUEST_KINDNESS              = 10804,
    NPC_EVENT_PINGER            = 22131
};

class mob_mature_netherwing_drake : public CreatureScript
{
public:
    mob_mature_netherwing_drake() : CreatureScript("mob_mature_netherwing_drake") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_mature_netherwing_drakeAI(pCreature);
    }

    struct mob_mature_netherwing_drakeAI : public ScriptedAI
    {
        mob_mature_netherwing_drakeAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        ObjectGuid uiPlayerGUID;

        bool bCanEat;
        bool bIsEating;

        uint32 EatTimer;
        uint32 CastTimer;

        void Reset()
        {
            uiPlayerGUID.Clear();

            bCanEat = false;
            bIsEating = false;

            EatTimer = 5000;
            CastTimer = 5000;
        }

        void SpellHit(Unit* pCaster, SpellInfo const* pSpell)
        {
            if(bCanEat || bIsEating)
                return;

            if(pCaster->GetTypeId() == TYPEID_PLAYER && pSpell->Id == SPELL_PLACE_CARCASS && !me->HasAura(SPELL_JUST_EATEN))
            {
                uiPlayerGUID.Set(pCaster->GetGUID());
                bCanEat = true;
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if(type != POINT_MOTION_TYPE)
                return;

            if(id == POINT_ID)
            {
                bIsEating = true;
                EatTimer = 7000;
                //me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_UNARMED);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(bCanEat || bIsEating)
            {
                if(EatTimer <= diff)
                {
                    if(bCanEat && !bIsEating)
                    {
                        if(Unit* pUnit = ObjectAccessor::GetUnit(*me, uiPlayerGUID))
                        {
                            if(GameObject* pGo = pUnit->FindNearestGameObject(GO_CARCASS, 10))
                            {
                                // @todo
                                // if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                                    ;// me->GetMotionMaster()->MovementExpired();

                                me->GetMotionMaster()->MoveIdle();
                                me->StopMoving();

                                me->GetMotionMaster()->MovePoint(POINT_ID, pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ());
                            }
                        }
                        bCanEat = false;
                    }
                    else if(bIsEating)
                    {
                        DoCast(me, SPELL_JUST_EATEN);
                        DoScriptText(SAY_JUST_EATEN, me);

                        if(Player* pPlr = ObjectAccessor::GetPlayer(*me, uiPlayerGUID))
                        {
                            pPlr->KilledMonsterCredit(NPC_EVENT_PINGER);

                            if(GameObject* pGo = pPlr->FindNearestGameObject(GO_CARCASS, 10))
                                pGo->Delete();
                        }

                        Reset();
                        me->GetMotionMaster()->Clear();
                    }
                }
                else
                    EatTimer -= diff;

            return;
            }

            if(!UpdateVictim())
                return;

            if(CastTimer <= diff)
            {
                DoCast(me->GetVictim(), SPELL_NETHER_BREATH);
                CastTimer = 5000;
            } else CastTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

#define FACTION_DEFAULT     62
#define FACTION_FRIENDLY    1840                            // Not sure if this is correct, it was taken off of Mordenai.

#define SPELL_HIT_FORCE_OF_NELTHARAKU   38762
#define SPELL_FORCE_OF_NELTHARAKU       38775

#define CREATURE_DRAGONMAW_SUBJUGATOR   21718
#define CREATURE_ESCAPE_DUMMY           22317

class mob_enslaved_netherwing_drake : public CreatureScript
{
public:
    mob_enslaved_netherwing_drake() : CreatureScript("mob_enslaved_netherwing_drake") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_enslaved_netherwing_drakeAI(pCreature);
    }

    struct mob_enslaved_netherwing_drakeAI : public ScriptedAI
    {
        mob_enslaved_netherwing_drakeAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            PlayerGUID.Clear();
            Tapped = false;
            Reset();
        }

        ObjectGuid PlayerGUID;
        uint32 FlyTimer;
        bool Tapped;

        void Reset()
        {
            if(!Tapped)
                me->SetFaction(FACTION_DEFAULT);

            FlyTimer = 10000;
            me->RemoveUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
            me->SetVisible(true);
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if(!caster)
                return;

            if(caster->GetTypeId() == TYPEID_PLAYER && spell->Id == SPELL_HIT_FORCE_OF_NELTHARAKU && !Tapped)
            {
                Tapped = true;
                PlayerGUID.Set(caster->GetGUID());

                me->SetFaction(FACTION_FRIENDLY);
                DoCast(caster, SPELL_FORCE_OF_NELTHARAKU, true);

                Unit* Dragonmaw = me->FindNearestCreature(CREATURE_DRAGONMAW_SUBJUGATOR, 50);

                if(Dragonmaw)
                {
                    me->GetThreatManager().AddThreat(Dragonmaw, 100000.0f);
                    AttackStart(Dragonmaw);
                }

                //HostileReference* ref = me->getThreatManager().getOnlineContainer().getReferenceByTarget(caster);
                //if(ref)
                //    ref->removeReference();
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if(type != POINT_MOTION_TYPE)
                return;

            if(id == 1)
            {
                if(PlayerGUID)
                {
                    Unit* player = ObjectAccessor::GetUnit((*me), PlayerGUID);
                    if(player)
                        DoCast(player, SPELL_FORCE_OF_NELTHARAKU, true);

                    PlayerGUID.Clear();
                }
                me->SetVisible(false);
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
                // me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                me->RemoveCorpse();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(!UpdateVictim())
            {
                if(Tapped)
                {
                    if(FlyTimer <= diff)
                    {
                        Tapped = false;
                        if(PlayerGUID)
                        {
                            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                            if(player && player->GetQuestStatus(10854) == QUEST_STATUS_INCOMPLETE)
                            {
                                DoCast(player, SPELL_FORCE_OF_NELTHARAKU, true);

                                Position pos;
                                if(Unit* EscapeDummy = me->FindNearestCreature(CREATURE_ESCAPE_DUMMY, 30))
                                    EscapeDummy->GetPosition();
                                else
                                {
                                    me->GetRandomNearPosition(20);
                                    pos.m_positionZ += 25;
                                }

                                me->AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
                                me->GetMotionMaster()->MovePoint(1, pos);
                            }
                        }
                    } else FlyTimer -= diff;
                }
                return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class mob_dragonmaw_peon : public CreatureScript
{
public:
    mob_dragonmaw_peon() : CreatureScript("mob_dragonmaw_peon") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_dragonmaw_peonAI(pCreature);
    }

    struct mob_dragonmaw_peonAI : public ScriptedAI
    {
        mob_dragonmaw_peonAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        ObjectGuid PlayerGUID;
        bool Tapped;
        uint32 PoisonTimer;

        void Reset()
        {
            PlayerGUID.Clear();
            Tapped = false;
            PoisonTimer = 0;
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if(!caster)
                return;

            if(caster->GetTypeId() == TYPEID_PLAYER && spell->Id == 40468 && !Tapped)
            {
                PlayerGUID.Set(caster->GetGUID());

                Tapped = true;
                float x, y, z;
                caster->GetClosePoint(x, y, z, me->GetObjectScale());

                me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                me->GetMotionMaster()->MovePoint(1, x, y, z);
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if(type != POINT_MOTION_TYPE)
                return;

            if(id)
            {
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_EAT);
                PoisonTimer = 15000;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(PoisonTimer)
            {
                if(PoisonTimer <= diff)
                {
                    if(PlayerGUID)
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                        if(player && player->GetQuestStatus(11020) == QUEST_STATUS_INCOMPLETE)
                            player->KilledMonsterCredit(23209);
                    }
                    PoisonTimer = 0;
                    // me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                } else PoisonTimer -= diff;
            }
        }
    };
};

class npc_drake_dealer_hurlunk : public CreatureScript
{
public:
    npc_drake_dealer_hurlunk() : CreatureScript("npc_drake_dealer_hurlunk") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        ClearGossipMenuFor(player);
        if(uiAction == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if(creature->IsVendor() && player->GetReputationRank(1015) == REP_EXALTED)
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

#define GOSSIP_HSK1 "Take Flanis's Pack"
#define GOSSIP_HSK2 "Take Kagrosh's Pack"

class npcs_flanis_swiftwing_and_kagrosh : public CreatureScript
{
public:
    npcs_flanis_swiftwing_and_kagrosh() : CreatureScript("npcs_flanis_swiftwing_and_kagrosh") { }

    bool OnGossipSelect(Player* player, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        ClearGossipMenuFor(player);
        if(uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            ItemPosCountVec dest;
            uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 30658, 1, 0);
            if(msg == EQUIP_ERR_OK)
            {
                player->StoreNewItem(dest, 30658, 1, true);
                ClearGossipMenuFor(player);
            }
        }
        if(uiAction == GOSSIP_ACTION_INFO_DEF+2)
        {
            ItemPosCountVec dest;
            uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 30659, 1, 0);
            if(msg == EQUIP_ERR_OK)
            {
                player->StoreNewItem(dest, 30659, 1, true);
                ClearGossipMenuFor(player);
            }
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if(player->GetQuestStatus(10583) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30658, 1, true))
            AddGossipItemFor(player, 0, GOSSIP_HSK1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        if(player->GetQuestStatus(10601) == QUEST_STATUS_INCOMPLETE && !player->HasItemCount(30659, 1, true))
            AddGossipItemFor(player, 0, GOSSIP_HSK2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

#define QUEST_11082     11082

#define GOSSIP_HMO "I am here for you, overseer."
#define GOSSIP_SMO1 "How dare you question an overseer of the Dragonmaw!"
#define GOSSIP_SMO2 "Who speaks of me? What are you talking about, broken?"
#define GOSSIP_SMO3 "Continue please."
#define GOSSIP_SMO4 "Who are these bidders?"
#define GOSSIP_SMO5 "Well... yes."

class npc_murkblood_overseer : public CreatureScript
{
public:
    npc_murkblood_overseer() : CreatureScript("npc_murkblood_overseer") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        ClearGossipMenuFor(player);
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SMO1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                                                                //correct id not known
                SendGossipMenuFor(player, 10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SMO2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                                                                //correct id not known
                SendGossipMenuFor(player, 10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SMO3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                                                                //correct id not known
                SendGossipMenuFor(player, 10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SMO4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                                                                //correct id not known
                SendGossipMenuFor(player, 10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SMO5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                                                                //correct id not known
                SendGossipMenuFor(player, 10940, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                                                                //correct id not known
                SendGossipMenuFor(player, 10940, creature->GetGUID());
                creature->CastSpell(player, 41121, false);
                player->AreaExploredOrEventHappens(QUEST_11082);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if(player->GetQuestStatus(QUEST_11082) == QUEST_STATUS_INCOMPLETE)
            AddGossipItemFor(player, 0, GOSSIP_HMO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        SendGossipMenuFor(player, 10940, creature->GetGUID());
        return true;
    }
};

#define GOSSIP_ORONOK1 "I am ready to hear your story, Oronok."
#define GOSSIP_ORONOK2 "How do I find the cipher?"
#define GOSSIP_ORONOK3 "How do you know all of this?"
#define GOSSIP_ORONOK4 "Yet what? What is it, Oronok?"
#define GOSSIP_ORONOK5 "Continue, please."
#define GOSSIP_ORONOK6 "So what of the cipher now? And your boys?"
#define GOSSIP_ORONOK7 "I will find your boys and the cipher, Oronok."

class npc_oronok_tornheart : public CreatureScript
{
public:
    npc_oronok_tornheart() : CreatureScript("npc_oronok_tornheart") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        ClearGossipMenuFor(player);
        switch(uiAction)
        {
            case GOSSIP_ACTION_TRADE:
                player->GetSession()->SendListInventory(creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF:
                AddGossipItemFor(player, 0, GOSSIP_ORONOK2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                SendGossipMenuFor(player, 10313, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+1:
                AddGossipItemFor(player, 0, GOSSIP_ORONOK3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                SendGossipMenuFor(player, 10314, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                AddGossipItemFor(player, 0, GOSSIP_ORONOK4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                SendGossipMenuFor(player, 10315, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                AddGossipItemFor(player, 0, GOSSIP_ORONOK5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                SendGossipMenuFor(player, 10316, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                AddGossipItemFor(player, 0, GOSSIP_ORONOK6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                SendGossipMenuFor(player, 10317, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                AddGossipItemFor(player, 0, GOSSIP_ORONOK7, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                SendGossipMenuFor(player, 10318, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                CloseGossipMenuFor(player);
                player->AreaExploredOrEventHappens(10519);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if(creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());
        if(creature->IsVendor())
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        if(player->GetQuestStatus(10519) == QUEST_STATUS_INCOMPLETE)
        {
            AddGossipItemFor(player, 0, GOSSIP_ORONOK1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            SendGossipMenuFor(player, 10312, creature->GetGUID());
        } else
        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

enum eKarynaku
{
    QUEST_ALLY_OF_NETHER    = 10870,

    TAXI_PATH_ID            = 649
};

class npc_karynaku : public CreatureScript
{
public:
    npc_karynaku() : CreatureScript("npc_karynaku") { }

    bool OnQuestAccept(Player* player, Creature* /*pCreature*/, Quest const* quest)
    {
        if(quest->GetQuestId() == QUEST_ALLY_OF_NETHER)
            player->ActivateTaxiPathTo(TAXI_PATH_ID);        //player->ActivateTaxiPathTo(649);

        return true;
    }
};

enum eOverlordData
{
    QUEST_LORD_ILLIDAN_STORMRAGE    = 11108,

    C_ILLIDAN                       = 22083,
    C_YARZILL                       = 23141,

    SPELL_ONE                       = 39990, // Red Lightning Bolt
    SPELL_TWO                       = 41528, // Mark of Stormrage
    SPELL_THREE                     = 40216, // Dragonaw Faction
    SPELL_FOUR                      = 42016, // Dragonaw Trasform

    OVERLORD_SAY_1                  = -1000606,
    OVERLORD_SAY_2                  = -1000607,
    OVERLORD_SAY_3                  = -1000608, //signed for 28315
    OVERLORD_SAY_4                  = -1000609,
    OVERLORD_SAY_5                  = -1000610,
    OVERLORD_SAY_6                  = -1000611,

    OVERLORD_YELL_1                 = -1000612,
    OVERLORD_YELL_2                 = -1000613,

    LORD_ILLIDAN_SAY_1              = -1000614,
    LORD_ILLIDAN_SAY_2              = -1000615,
    LORD_ILLIDAN_SAY_3              = -1000616,
    LORD_ILLIDAN_SAY_4              = -1000617,
    LORD_ILLIDAN_SAY_5              = -1000618,
    LORD_ILLIDAN_SAY_6              = -1000619,
    LORD_ILLIDAN_SAY_7              = -1000620,

    YARZILL_THE_MERC_SAY            = -1000621,
};

class npc_overlord_morghor : public CreatureScript
{
public:
    npc_overlord_morghor() : CreatureScript("npc_overlord_morghor") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest *_Quest)
    {
        if(_Quest->GetQuestId() == QUEST_LORD_ILLIDAN_STORMRAGE)
        {
            CAST_AI(npc_overlord_morghor::npc_overlord_morghorAI, creature->AI())->PlayerGUID.Set(player->GetGUID());
            CAST_AI(npc_overlord_morghor::npc_overlord_morghorAI, creature->AI())->StartEvent();
            return true;
        }
        return false;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
    return new npc_overlord_morghorAI(pCreature);
    }

    struct npc_overlord_morghorAI : public ScriptedAI
    {
        npc_overlord_morghorAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        ObjectGuid PlayerGUID;
        ObjectGuid IllidanGUID;

        uint32 ConversationTimer;
        uint32 Step;

        bool Event;

        void Reset()
        {
            PlayerGUID.Clear();
            IllidanGUID.Clear();

            ConversationTimer = 0;
            Step = 0;

            Event = false;
            me->SetUInt32Value(UNIT_NPC_FLAGS, 2);
        }

        void StartEvent()
        {
            me->SetUInt32Value(UNIT_NPC_FLAGS, 0);
            me->SetUInt32Value(UNIT_FIELD_BYTES_1, 0);
            Unit* Illidan = me->SummonCreature(C_ILLIDAN, -5107.83f, 602.584f, 85.2393f, 4.92598f, TEMPSUMMON_CORPSE_DESPAWN, 0);
            if(Illidan)
            {
                IllidanGUID.Set(Illidan->GetGUID());
                Illidan->SetVisible(false);
            }
            if(PlayerGUID)
            {
                Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                if(player)
                    DoScriptText(OVERLORD_SAY_1, me, player);
            }
            ConversationTimer = 4200;
            Step = 0;
            Event = true;
        }

        uint32 NextStep(uint32 Step)
        {
            Unit* player = ObjectAccessor::GetUnit((*me), PlayerGUID);

            Unit* Illi = ObjectAccessor::GetUnit((*me), IllidanGUID);

            if(!player || !Illi)
            {
                EnterEvadeMode();
                return 0;
            }

            switch(Step)
            {
                case 0: return 0; break;
                case 1: me->GetMotionMaster()->MovePoint(0, -5104.41f, 595.297f, 85.6838f); return 9000; break;
                case 2: DoScriptText(OVERLORD_YELL_1, me, player); return 4500; break;
                case 3: me->SetInFront(player); return 3200;  break;
                case 4: DoScriptText(OVERLORD_SAY_2, me, player); return 2000; break;
                case 5: Illi->SetVisible(true);
                     Illi->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE); return 350; break;
                case 6:
                    Illi->CastSpell(Illi, SPELL_ONE, true);
                    Illi->SetTarget(me->GetGUID());
                    me->SetTarget(IllidanGUID);
                    return 2000; break;
                case 7: DoScriptText(OVERLORD_YELL_2, me); return 4500; break;
                case 8: me->SetUInt32Value(UNIT_FIELD_BYTES_1, 8); return 2500; break;
                case 9: DoScriptText(OVERLORD_SAY_3, me); return 6500; break;
                case 10: DoScriptText(LORD_ILLIDAN_SAY_1, Illi); return 5000;  break;
                case 11: DoScriptText(OVERLORD_SAY_4, me, player); return 6000; break;
                case 12: DoScriptText(LORD_ILLIDAN_SAY_2, Illi); return 5500; break;
                case 13: DoScriptText(LORD_ILLIDAN_SAY_3, Illi); return 4000; break;
                case 14: Illi->SetTarget(PlayerGUID); return 1500; break;
                case 15: DoScriptText(LORD_ILLIDAN_SAY_4, Illi); return 1500; break;
                case 16:
                    if(player)
                    {
                        Illi->CastSpell(player, SPELL_TWO, true);
                        player->RemoveAurasDueToSpell(SPELL_THREE);
                        player->RemoveAurasDueToSpell(SPELL_FOUR);
                        return 5000;
                    } else {
                        if(Player* plr = player->ToPlayer())
                            plr->FailQuest(QUEST_LORD_ILLIDAN_STORMRAGE); Step = 30; return 100;
                    }
                    break;
                case 17: DoScriptText(LORD_ILLIDAN_SAY_5, Illi); return 5000; break;
                case 18: DoScriptText(LORD_ILLIDAN_SAY_6, Illi); return 5000; break;
                case 19: DoScriptText(LORD_ILLIDAN_SAY_7, Illi); return 5000; break;
                case 20:
                    Illi->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                    Illi->AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
                    return 500; break;
                case 21: DoScriptText(OVERLORD_SAY_5, me); return 500; break;
                case 22:
                    Illi->SetVisible(false);
                    Illi->SetDeathState(JUST_DIED);
                    return 1000; break;
                case 23: me->SetUInt32Value(UNIT_FIELD_BYTES_1, 0); return 2000; break;
                case 24: me->SetTarget(PlayerGUID); return 5000; break;
                case 25: DoScriptText(OVERLORD_SAY_6, me); return 2000; break;
                case 26:
                    if(player)
                        if (Player* plr = player->ToPlayer())
                            plr->GroupEventHappens(QUEST_LORD_ILLIDAN_STORMRAGE, me);
                    return 6000; break;
                case 27:
                {
                    Unit* Yarzill = me->FindNearestCreature(C_YARZILL, 50);
                    if(Yarzill)
                        Yarzill->SetTarget(PlayerGUID);
                    return 500;
                }
                    break;
                case 28:
                    player->RemoveAurasDueToSpell(SPELL_TWO);
                    player->RemoveAurasDueToSpell(41519);
                    player->CastSpell(player, SPELL_THREE, true);
                    player->CastSpell(player, SPELL_FOUR, true);
                    return 1000; break;
                case 29:
                {
                    Unit* Yarzill = me->FindNearestCreature(C_YARZILL, 50);
                    if(Yarzill)
                        DoScriptText(YARZILL_THE_MERC_SAY, Yarzill, player);
                    return 5000;
                }
                    break;
                case 30:
                {
                    Unit* Yarzill = me->FindNearestCreature(C_YARZILL, 50);
                    if(Yarzill)
                        Yarzill->SetTarget(ObjectGuid::Empty);
                    return 5000;
                }
                    break;
                case 31:
                {
                    Unit* Yarzill = me->FindNearestCreature(C_YARZILL, 50);
                    if(Yarzill)
                        Yarzill->CastSpell(player, 41540, true);
                    return 1000;
                }
                    break;
                case 32: me->GetMotionMaster()->MovePoint(0, -5085.77f, 577.231f, 86.6719f); return 5000; break;
                case 33: Reset(); return 100; break;
                default : return 0;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(!ConversationTimer)
                return;

            if(ConversationTimer <= diff)
            {
                if(Event && IllidanGUID && PlayerGUID)
                    ConversationTimer = NextStep(++Step);
            } else ConversationTimer -= diff;
        }
    };
};

enum eEarthmender
{
    SAY_WIL_START               = -1000381,
    SAY_WIL_AGGRO1              = -1000382,
    SAY_WIL_AGGRO2              = -1000383,
    SAY_WIL_PROGRESS1           = -1000384,
    SAY_WIL_PROGRESS2           = -1000385,
    SAY_WIL_FIND_EXIT           = -1000386,
    SAY_WIL_PROGRESS4           = -1000387,
    SAY_WIL_PROGRESS5           = -1000388,
    SAY_WIL_JUST_AHEAD          = -1000389,
    SAY_WIL_END                 = -1000390,

    SPELL_CHAIN_LIGHTNING       = 16006,
    SPELL_EARTHBING_TOTEM       = 15786,
    SPELL_FROST_SHOCK           = 12548,
    SPELL_HEALING_WAVE          = 12491,

    QUEST_ESCAPE_COILSCAR       = 10451,
    NPC_COILSKAR_ASSASSIN       = 21044,
    FACTION_EARTHEN             = 1726                      //guessed
};

class npc_earthmender_wilda : public CreatureScript
{
public:
    npc_earthmender_wilda() : CreatureScript("npc_earthmender_wilda") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* pQuest)
    {
        if(pQuest->GetQuestId() == QUEST_ESCAPE_COILSCAR)
        {
            DoScriptText(SAY_WIL_START, creature, player);
            creature->SetFaction(FACTION_EARTHEN);

            if(npc_earthmender_wildaAI* pEscortAI = CAST_AI(npc_earthmender_wilda::npc_earthmender_wildaAI, creature->AI()))
                pEscortAI->Start(false, false, player->GetGUID(), pQuest);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_earthmender_wildaAI(pCreature);
    }

    struct npc_earthmender_wildaAI : public EscortAI
    {
        npc_earthmender_wildaAI(Creature* pCreature) : EscortAI(pCreature) { }

        uint32 m_uiHealingTimer;

        void Reset()
        {
            m_uiHealingTimer = 0;
        }

        void WaypointReached(uint32 uiPointId, uint32)
        {
            Player* player = GetPlayerForEscort();

            if(!player)
                return;

            switch(uiPointId)
            {
                case 13:
                    DoScriptText(SAY_WIL_PROGRESS1, me, player);
                    DoSpawnAssassin();
                    break;
                case 14:
                    DoSpawnAssassin();
                    break;
                case 15:
                    DoScriptText(SAY_WIL_FIND_EXIT, me, player);
                    break;
                case 19:
                    DoRandomSay();
                    break;
                case 20:
                    DoSpawnAssassin();
                    break;
                case 26:
                    DoRandomSay();
                    break;
                case 27:
                    DoSpawnAssassin();
                    break;
                case 33:
                    DoRandomSay();
                    break;
                case 34:
                    DoSpawnAssassin();
                    break;
                case 37:
                    DoRandomSay();
                    break;
                case 38:
                    DoSpawnAssassin();
                    break;
                case 39:
                    DoScriptText(SAY_WIL_JUST_AHEAD, me, player);
                    break;
                case 43:
                    DoRandomSay();
                    break;
                case 44:
                    DoSpawnAssassin();
                    break;
                case 50:
                    DoScriptText(SAY_WIL_END, me, player);

                    if(Player* player = GetPlayerForEscort())
                        player->GroupEventHappens(QUEST_ESCAPE_COILSCAR, me);
                    break;
            }
        }

        void JustSummoned(Creature* summoned)
        {
            if(summoned->GetEntry() == NPC_COILSKAR_ASSASSIN)
                summoned->AI()->AttackStart(me);
        }

        //this is very unclear, random say without no real relevance to script/event
        void DoRandomSay()
        {
            DoScriptText(RAND(SAY_WIL_PROGRESS2, SAY_WIL_PROGRESS4, SAY_WIL_PROGRESS5), me);
        }

        void DoSpawnAssassin()
        {
            //unknown where they actually appear
            DoSummon(NPC_COILSKAR_ASSASSIN, me, 15.0f, 5000, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT);
        }

        void EnterCombat(Unit* who)
        {
            //don't always use
            if(rand()%5)
                return;

            //only aggro text if not player
            if(who->GetTypeId() != TYPEID_PLAYER)
            {
                //appears to be random
                if(urand(0, 1))
                    DoScriptText(RAND(SAY_WIL_AGGRO1, SAY_WIL_AGGRO2), who);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            EscortAI::UpdateAI(diff);

            if(!UpdateVictim())
                return;

            //TODO: add more abilities
            if(!HealthAbovePct(30))
            {
                if(m_uiHealingTimer <= diff)
                {
                    DoCast(me, SPELL_HEALING_WAVE);
                    m_uiHealingTimer = 15000;
                }
                else
                    m_uiHealingTimer -= diff;
            }
        }
    };
};

/* ContentData
Battle of the crimson watch - creatures, gameobjects and defines
mob_illidari_spawn : Adds that are summoned in the Crimson Watch battle.
mob_torloth_the_magnificent : Final Creature that players have to face before quest is completed
npc_lord_illidan_stormrage : Creature that controls the event.
go_crystal_prison : GameObject that begins the event and hands out quest
EndContentData */

#define END_TEXT -1000366 //signed for 10646

#define QUEST_BATTLE_OF_THE_CRIMSON_WATCH 10781
#define EVENT_AREA_RADIUS 65 //65yds
#define EVENT_COOLDOWN 30000 //in ms. appear after event completed or failed (should be = Adds despawn time)

struct TorlothCinematic
{
    int32 TextId;
    uint32 creature, Timer;
};

// Creature 0 - Torloth, 1 - Illidan
static TorlothCinematic TorlothAnim[]=
{
    {-1000367, 0, 2000},
    {-1000368, 1, 7000},
    {-1000369, 0, 3000},
    {0, 0, 2000}, // Torloth stand
    {-1000370, 0, 1000},
    {0, 0, 3000},
    {0, 0, 0}
};

struct Location
{
    float x, y, z, o;
};

//Cordinates for Spawns
static Location SpawnLocation[]=
{
    //Cords used for:
    {-4615.8556f, 1342.2532f, 139.9f, 1.612f}, //Illidari Soldier
    {-4598.9365f, 1377.3182f, 139.9f, 3.917f}, //Illidari Soldier
    {-4598.4697f, 1360.8999f, 139.9f, 2.427f}, //Illidari Soldier
    {-4589.3599f, 1369.1061f, 139.9f, 3.165f}, //Illidari Soldier
    {-4608.3477f, 1386.0076f, 139.9f, 4.108f}, //Illidari Soldier
    {-4633.1889f, 1359.8033f, 139.9f, 0.949f}, //Illidari Soldier
    {-4623.5791f, 1351.4574f, 139.9f, 0.971f}, //Illidari Soldier
    {-4607.2988f, 1351.6099f, 139.9f, 2.416f}, //Illidari Soldier
    {-4633.7764f, 1376.0417f, 139.9f, 5.608f}, //Illidari Soldier
    {-4600.2461f, 1369.1240f, 139.9f, 3.056f}, //Illidari Mind Breaker
    {-4631.7808f, 1367.9459f, 139.9f, 0.020f}, //Illidari Mind Breaker
    {-4600.2461f, 1369.1240f, 139.9f, 3.056f}, //Illidari Highlord
    {-4631.7808f, 1367.9459f, 139.9f, 0.020f}, //Illidari Highlord
    {-4615.5586f, 1353.0031f, 139.9f, 1.540f}, //Illidari Highlord
    {-4616.4736f, 1384.2170f, 139.9f, 4.971f}, //Illidari Highlord
    {-4627.1240f, 1378.8752f, 139.9f, 2.544f} //Torloth The Magnificent
};

struct WaveData
{
    uint8 SpawnCount, UsedSpawnPoint;
    uint32 CreatureId, SpawnTimer, YellTimer;
    int32 WaveTextId;
};

static WaveData WavesInfo[]=
{
    {9, 0, 22075, 10000, 7000, -1000371},   //Illidari Soldier
    {2, 9, 22074, 10000, 7000, -1000372},   //Illidari Mind Breaker
    {4, 11, 19797, 10000, 7000, -1000373},  //Illidari Highlord
    {1, 15, 22076, 10000, 7000, -1000374}   //Torloth The Magnificent
};

struct SpawnSpells
{
 uint32 Timer1, Timer2, SpellId;
};

static SpawnSpells SpawnCast[]=
{
    {10000, 15000, 35871},  // Illidari Soldier Cast - Spellbreaker
    {10000, 10000, 38985},  // Illidari Mind Breake Cast - Focused Bursts
    {35000, 35000, 22884},  // Illidari Mind Breake Cast - Psychic Scream
    {20000, 20000, 17194},  // Illidari Mind Breake Cast - Mind Blast
    {8000, 15000, 38010},   // Illidari Highlord Cast - Curse of Flames
    {12000, 20000, 16102},  // Illidari Highlord Cast - Flamestrike
    {10000, 15000, 15284},  // Torloth the Magnificent Cast - Cleave
    {18000, 20000, 39082},  // Torloth the Magnificent Cast - Shadowfury
    {25000, 28000, 33961}   // Torloth the Magnificent Cast - Spell Reflection
};

class mob_torloth_the_magnificent : public CreatureScript
{
public:
    mob_torloth_the_magnificent() : CreatureScript("mob_torloth_the_magnificent") { }

    CreatureAI* GetAI(Creature* c) const
    {
        return new mob_torloth_the_magnificentAI(c);
    }

    struct mob_torloth_the_magnificentAI : public ScriptedAI
    {
        mob_torloth_the_magnificentAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 AnimationTimer, SpellTimer1, SpellTimer2, SpellTimer3;

        uint8 AnimationCount;

        ObjectGuid LordIllidanGUID;
        ObjectGuid AggroTargetGUID;

        bool Timers;

        void Reset()
        {
            AnimationTimer = 4000;
            AnimationCount = 0;
            LordIllidanGUID.Clear();
            AggroTargetGUID.Clear();
            Timers = false;

            me->AddUnitState(UNIT_STATE_ROOT);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetTarget(ObjectGuid::Empty);
        }

        void EnterCombat(Unit* /*pWho*/){ }

        void HandleAnimation()
        {
            Creature* creature = me;

            if(TorlothAnim[AnimationCount].creature == 1)
            {
                creature = (ObjectAccessor::GetCreature(*me, LordIllidanGUID));

                if(!creature)
                    return;
            }

            if(TorlothAnim[AnimationCount].TextId)
                DoScriptText(TorlothAnim[AnimationCount].TextId, creature);

            AnimationTimer = TorlothAnim[AnimationCount].Timer;

            switch(AnimationCount)
            {
            case 0:
                me->SetUInt32Value(UNIT_FIELD_BYTES_1, 8);
                break;
            case 3:
                me->RemoveFlag(UNIT_FIELD_BYTES_1, 8);
                break;
            case 5:
                if(Player* AggroTarget = (ObjectAccessor::GetPlayer(*me, AggroTargetGUID)))
                {
                    me->SetTarget(AggroTarget->GetGUID());
                    me->GetThreatManager().AddThreat(AggroTarget, 1);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                }
                break;
            case 6:
                if(Player* AggroTarget = (ObjectAccessor::GetPlayer(*me, AggroTargetGUID)))
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->ClearUnitState(UNIT_STATE_ROOT);

                    float x, y, z;
                    AggroTarget->GetPosition(x, y, z);
                    me->GetMotionMaster()->MovePoint(0, x, y, z);
                }
                break;
            }
            ++AnimationCount;
        }

        void UpdateAI(const uint32 diff)
        {
            if(AnimationTimer)
            {
                if(AnimationTimer <= diff)
                {
                    HandleAnimation();
                } else AnimationTimer -= diff;
            }

            if(AnimationCount < 6)
            {
                me->CombatStop();
            } else if(!Timers)
            {
                SpellTimer1 = SpawnCast[6].Timer1;
                SpellTimer2 = SpawnCast[7].Timer1;
                SpellTimer3 = SpawnCast[8].Timer1;
                Timers = true;
            }

            if(Timers)
            {
                if(SpellTimer1 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[6].SpellId);//Cleave
                    SpellTimer1 = SpawnCast[6].Timer2 + (rand()%10 * 1000);
                } else SpellTimer1 -= diff;

                if(SpellTimer2 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[7].SpellId);//Shadowfury
                    SpellTimer2 = SpawnCast[7].Timer2 + (rand()%5 * 1000);
                } else SpellTimer2 -= diff;

                if(SpellTimer3 <= diff)
                {
                    DoCast(me, SpawnCast[8].SpellId);
                    SpellTimer3 = SpawnCast[8].Timer2 + (rand()%7 * 1000);//Spell Reflection
                } else SpellTimer3 -= diff;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* slayer)
        {
            if(slayer)
                switch(slayer->GetTypeId())
                {
                    case TYPEID_UNIT:
                        if(Unit* owner = slayer->GetOwner())
                            if(owner->GetTypeId() == TYPEID_PLAYER)
                                if(Player* plr = owner->ToPlayer())
                                    plr->GroupEventHappens(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, me);
                        break;
                    case TYPEID_PLAYER:
                        if (Player* plr = slayer->ToPlayer())
                            plr->GroupEventHappens(QUEST_BATTLE_OF_THE_CRIMSON_WATCH, me);
                        break;
                    default:
                        break;
                }

            if(Creature* LordIllidan = (ObjectAccessor::GetCreature(*me, LordIllidanGUID)))
            {
                DoScriptText(END_TEXT, LordIllidan, slayer);
                LordIllidan->AI()->EnterEvadeMode();
            }
        }
    };
};

class npc_lord_illidan_stormrage : public CreatureScript
{
public:
    npc_lord_illidan_stormrage() : CreatureScript("npc_lord_illidan_stormrage") { }

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_lord_illidan_stormrageAI(c);
    }

    struct npc_lord_illidan_stormrageAI : public ScriptedAI
    {
        npc_lord_illidan_stormrageAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        ObjectGuid PlayerGUID;

        uint32 WaveTimer;
        uint32 AnnounceTimer;

        int8 LiveCount;
        uint8 WaveCount;

        bool EventStarted;
        bool Announced;
        bool Failed;

        void Reset()
        {
            PlayerGUID.Clear();

            WaveTimer = 10000;
            AnnounceTimer = 7000;
            LiveCount = 0;
            WaveCount = 0;

            EventStarted = false;
            Announced = false;
            Failed = false;

            me->SetVisible(false);
        }

        void EnterCombat(Unit* /*pWho*/) { }
        void MoveInLineOfSight(Unit* /*pWho*/) { }
        void AttackStart(Unit* /*pWho*/) { }

        void SummonNextWave();

        void CheckEventFail()
        {
            Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);

            if(!player)
                return;

            if(Group *EventGroup = player->GetGroup())
            {
                Player* GroupMember;

                uint8 GroupMemberCount = 0;
                uint8 DeadMemberCount = 0;
                uint8 FailedMemberCount = 0;

                const Group::MemberSlotList members = EventGroup->GetMemberSlots();

                for(Group::member_citerator itr = members.begin(); itr!= members.end(); ++itr)
                {
                    GroupMember = (ObjectAccessor::GetPlayer(*me, itr->guid));
                    if(!GroupMember)
                        continue;
                    if(!GroupMember->IsWithinDistInMap(me, EVENT_AREA_RADIUS) && GroupMember->GetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) == QUEST_STATUS_INCOMPLETE)
                    {
                        GroupMember->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                        ++FailedMemberCount;
                    }
                    ++GroupMemberCount;

                    if(GroupMember->IsDead())
                    {
                        ++DeadMemberCount;
                    }
                }

                if(GroupMemberCount == FailedMemberCount)
                {
                    Failed = true;
                }

                if(GroupMemberCount == DeadMemberCount)
                {
                    for(Group::member_citerator itr = members.begin(); itr!= members.end(); ++itr)
                    {
                        GroupMember = ObjectAccessor::GetPlayer(*me, itr->guid);

                        if(GroupMember && GroupMember->GetQuestStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) == QUEST_STATUS_INCOMPLETE)
                        {
                            GroupMember->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                        }
                    }
                    Failed = true;
                }
            } else if(player->IsDead() || !player->IsWithinDistInMap(me, EVENT_AREA_RADIUS))
            {
                player->FailQuest(QUEST_BATTLE_OF_THE_CRIMSON_WATCH);
                Failed = true;
            }
        }

        void LiveCounter()
        {
            --LiveCount;
            if(!LiveCount)
                Announced = false;
        }

        void UpdateAI(const uint32 diff)
        {
            if(!PlayerGUID || !EventStarted)
                return;

            if(!LiveCount && WaveCount < 4)
            {
                if(!Announced && AnnounceTimer <= diff)
                {
                    DoScriptText(WavesInfo[WaveCount].WaveTextId, me);
                    Announced = true;
                } else AnnounceTimer -= diff;

                if(WaveTimer <= diff)
                {
                    SummonNextWave();
                } else WaveTimer -= diff;
            }
            CheckEventFail();

            if(Failed)
                EnterEvadeMode();
        }
    };
};

class mob_illidari_spawn : public CreatureScript
{
public:
    mob_illidari_spawn() : CreatureScript("mob_illidari_spawn") { }

    CreatureAI* GetAI(Creature* c) const
    {
        return new mob_illidari_spawnAI(c);
    }

    struct mob_illidari_spawnAI : public ScriptedAI
    {
        mob_illidari_spawnAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        ObjectGuid LordIllidanGUID;
        uint32 SpellTimer1, SpellTimer2, SpellTimer3;
        bool Timers;

        void Reset()
        {
            LordIllidanGUID.Clear();
            Timers = false;
        }

        void EnterCombat(Unit* /*pWho*/) { }
        void JustDied(Unit* /*slayer*/)
        {
            me->RemoveCorpse();
            if(Creature* LordIllidan = (ObjectAccessor::GetCreature(*me, LordIllidanGUID)))
                if(LordIllidan)
                    CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, LordIllidan->AI())->LiveCounter();
        }

        void UpdateAI(const uint32 diff)
        {
            if(!UpdateVictim())
                return;

            if(!Timers)
            {
                if(me->GetEntry() == 22075)//Illidari Soldier
                {
                    SpellTimer1 = SpawnCast[0].Timer1 + (rand()%4 * 1000);
                }
                if(me->GetEntry() == 22074)//Illidari Mind Breaker
                {
                    SpellTimer1 = SpawnCast[1].Timer1 + (rand()%10 * 1000);
                    SpellTimer2 = SpawnCast[2].Timer1 + (rand()%4 * 1000);
                    SpellTimer3 = SpawnCast[3].Timer1 + (rand()%4 * 1000);
                }
                if(me->GetEntry() == 19797)// Illidari Highlord
                {
                    SpellTimer1 = SpawnCast[4].Timer1 + (rand()%4 * 1000);
                    SpellTimer2 = SpawnCast[5].Timer1 + (rand()%4 * 1000);
                }
                Timers = true;
            }
            //Illidari Soldier
            if(me->GetEntry() == 22075)
            {
                if(SpellTimer1 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[0].SpellId);//Spellbreaker
                    SpellTimer1 = SpawnCast[0].Timer2 + (rand()%5 * 1000);
                } else SpellTimer1 -= diff;
            }
            //Illidari Mind Breaker
            if(me->GetEntry() == 22074)
            {
                if(SpellTimer1 <= diff)
                {
                    if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        if(target->GetTypeId() == TYPEID_PLAYER)
                        {
                            DoCast(target, SpawnCast[1].SpellId); //Focused Bursts
                            SpellTimer1 = SpawnCast[1].Timer2 + (rand()%5 * 1000);
                        } else SpellTimer1 = 2000;
                    }
                } else SpellTimer1 -= diff;

                if(SpellTimer2 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[2].SpellId);//Psychic Scream
                    SpellTimer2 = SpawnCast[2].Timer2 + (rand()%13 * 1000);
                } else SpellTimer2 -= diff;

                if(SpellTimer3 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[3].SpellId);//Mind Blast
                    SpellTimer3 = SpawnCast[3].Timer2 + (rand()%8 * 1000);
                } else SpellTimer3 -= diff;
            }
            //Illidari Highlord
            if(me->GetEntry() == 19797)
            {
                if(SpellTimer1 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[4].SpellId);//Curse Of Flames
                    SpellTimer1 = SpawnCast[4].Timer2 + (rand()%10 * 1000);
                } else SpellTimer1 -= diff;

                if(SpellTimer2 <= diff)
                {
                    DoCast(me->GetVictim(), SpawnCast[5].SpellId);//Flamestrike
                    SpellTimer2 = SpawnCast[5].Timer2 + (rand()%7 * 13000);
                } else SpellTimer2 -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI::SummonNextWave()
{
    uint8 count = WavesInfo[WaveCount].SpawnCount;
    uint8 locIndex = WavesInfo[WaveCount].UsedSpawnPoint;
    uint8 FelguardCount = 0;
    uint8 DreadlordCount = 0;

    for(uint8 i = 0; i < count; ++i)
    {
        Creature* Spawn = NULL;
        float X = SpawnLocation[locIndex + i].x;
        float Y = SpawnLocation[locIndex + i].y;
        float Z = SpawnLocation[locIndex + i].z;
        float O = SpawnLocation[locIndex + i].o;
        Spawn = me->SummonCreature(WavesInfo[WaveCount].CreatureId, X, Y, Z, O, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000);
        ++LiveCount;

        if(Spawn)
        {
            Spawn->LoadCreatureAddon();

            if(WaveCount == 0)//1 Wave
            {
                if(rand()%3 == 1 && FelguardCount<2)
                {
                    Spawn->SetDisplayId(18654);
                    ++FelguardCount;
                }
                else if(DreadlordCount < 3)
                {
                    Spawn->SetDisplayId(19991);
                    ++DreadlordCount;
                }
                else if(FelguardCount<2)
                {
                    Spawn->SetDisplayId(18654);
                    ++FelguardCount;
                }
            }

            if(WaveCount < 3)//1-3 Wave
            {
                if(PlayerGUID)
                {
                    if(Player* target = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                    {
                        float x, y, z;
                        target->GetPosition(x, y, z);
                        Spawn->GetMotionMaster()->MovePoint(0, x, y, z);
                    }
                }
                CAST_AI(mob_illidari_spawn::mob_illidari_spawnAI, Spawn->AI())->LordIllidanGUID.Set(me->GetGUID());
            }

            if(WavesInfo[WaveCount].CreatureId == 22076) // Torloth
            {
                CAST_AI(mob_torloth_the_magnificent::mob_torloth_the_magnificentAI, Spawn->AI())->LordIllidanGUID.Set(me->GetGUID());
                if(PlayerGUID)
                    CAST_AI(mob_torloth_the_magnificent::mob_torloth_the_magnificentAI, Spawn->AI())->AggroTargetGUID.Set(PlayerGUID);
            }
        }
    }
    ++WaveCount;
    WaveTimer = WavesInfo[WaveCount].SpawnTimer;
    AnnounceTimer = WavesInfo[WaveCount].YellTimer;
}

class go_crystal_prison : public GameObjectScript
{
public:
    go_crystal_prison() : GameObjectScript("go_crystal_prison") { }

    bool OnQuestAccept(Player* player, GameObject* /*pGo*/, Quest const* quest)
    {
        if(quest->GetQuestId() == QUEST_BATTLE_OF_THE_CRIMSON_WATCH)
        {
            Creature* Illidan = player->FindNearestCreature(22083, 50);

            if(Illidan && !CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->EventStarted)
            {
                CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->PlayerGUID.Set(player->GetGUID());
                CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->LiveCount = 0;
                CAST_AI(npc_lord_illidan_stormrage::npc_lord_illidan_stormrageAI, Illidan->AI())->EventStarted=true;
            }
        }
        return true;
    }
};

/* QUESTS */
#define QUEST_ENRAGED_SPIRITS_FIRE_EARTH 10458
#define QUEST_ENRAGED_SPIRITS_AIR 10481
#define QUEST_ENRAGED_SPIRITS_WATER 10480

/* Totem */
#define ENTRY_TOTEM_OF_SPIRITS 21071
#define RADIUS_TOTEM_OF_SPIRITS 15

/* SPIRITS */
#define ENTRY_ENRAGED_EARTH_SPIRIT 21050
#define ENTRY_ENRAGED_FIRE_SPIRIT 21061
#define ENTRY_ENRAGED_AIR_SPIRIT 21060
#define ENTRY_ENRAGED_WATER_SPIRIT 21059

/* SOULS */
#define ENTRY_EARTHEN_SOUL 21073
#define ENTRY_FIERY_SOUL 21097
#define ENTRY_ENRAGED_AIRY_SOUL 21116
#define ENTRY_ENRAGED_WATERY_SOUL 21109  // wrong model

/* SPELL KILLCREDIT - not working!?! - using KilledMonsterCredit */
#define SPELL_EARTHEN_SOUL_CAPTURED_CREDIT 36108
#define SPELL_FIERY_SOUL_CAPTURED_CREDIT 36117
#define SPELL_AIRY_SOUL_CAPTURED_CREDIT 36182
#define SPELL_WATERY_SOUL_CAPTURED_CREDIT 36171

/* KilledMonsterCredit Workaround */
#define CREDIT_FIRE 21094
#define CREDIT_WATER 21095
#define CREDIT_AIR 21096
#define CREDIT_EARTH 21092

/* Captured Spell/Buff */
#define SPELL_SOUL_CAPTURED 36115

/* Factions */
#define ENRAGED_SOUL_FRIENDLY 35
#define ENRAGED_SOUL_HOSTILE 14

class npc_enraged_spirit : public CreatureScript
{
public:
    npc_enraged_spirit() : CreatureScript("npc_enraged_spirit") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_enraged_spiritAI(pCreature);
    }

    struct npc_enraged_spiritAI : public ScriptedAI
    {
        npc_enraged_spiritAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        void Reset()   { }

        void EnterCombat(Unit* /*pWho*/){ }

        void JustDied(Unit* /*pKiller*/)
        {
            // always spawn spirit on death
            // if totem around
            // move spirit to totem and cast kill count
            uint32 entry = 0;
            uint32 credit = 0;

            switch(me->GetEntry()) {
                case ENTRY_ENRAGED_FIRE_SPIRIT:
                    entry  = ENTRY_FIERY_SOUL;
                    credit = CREDIT_FIRE;
                    break;
                case ENTRY_ENRAGED_EARTH_SPIRIT:
                    entry  = ENTRY_EARTHEN_SOUL;
                    credit = CREDIT_EARTH;
                    break;
                case ENTRY_ENRAGED_AIR_SPIRIT:
                    entry  = ENTRY_ENRAGED_AIRY_SOUL;
                    credit = CREDIT_AIR;
                    break;
                case ENTRY_ENRAGED_WATER_SPIRIT:
                    entry  = ENTRY_ENRAGED_WATERY_SOUL;
                    credit = CREDIT_WATER;
                break;
            }

            // Spawn Soul on Kill ALWAYS!
            Creature* Summoned = NULL;
            Unit* totemOspirits = NULL;

            if(entry != 0)
                Summoned = DoSpawnCreature(entry, 0, 0, 1, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 5000);

            // FIND TOTEM, PROCESS QUEST
            if(Summoned)
            {
                 totemOspirits = me->FindNearestCreature(ENTRY_TOTEM_OF_SPIRITS, RADIUS_TOTEM_OF_SPIRITS);
                 if(totemOspirits)
                 {
                     Summoned->SetFaction(ENRAGED_SOUL_FRIENDLY);
                     Summoned->GetMotionMaster()->MovePoint(0, totemOspirits->GetPositionX(), totemOspirits->GetPositionY(), Summoned->GetPositionZ());

                     Unit* Owner = totemOspirits->GetOwner();
                     if(Owner && Owner->GetTypeId() == TYPEID_PLAYER)
                         if (Player* plr = Owner->ToPlayer())
                             plr->KilledMonsterCredit(credit);
                     DoCast(totemOspirits, SPELL_SOUL_CAPTURED);
                 }
            }
        }
    };
};

/********************
An Innocent Disguise
*********************/
float Wyrm_SomePositions[5][3] =
{
    {-3560.477f, 512.431f, 19.2f},      //Vagath ... Shadowlords
    {-3572.920f, 662.789f, -1.0f},      //Start Akama
    {-3572.825f, 621.806f,  6.3f},      //Stop Akama
    {-3561.880f, 598.995f,  9.3f},      //Stop Maiev
    {-3603.344f, 383.222f, 33.2f}       //EndPoint
};

enum XiriPhases
{
    PHASE_SUMMON_DEMONS,
    PHASE_SUMMON_AKAMA,
    PHASE_ATTACK,
    PHASE_KILL_WAIT,
    PHASE_END,
    PHASE_WAIT
};

#define ENTRY_VAGATH                    23152
#define ENTRY_ILLIDARI_SHADOWLORD       22988
#define ENTRY_AKAMA                     23191
#define ENTRY_MAIEV                     22989

class npc_xiri : public CreatureScript
{
public:
    npc_xiri() : CreatureScript("npc_xiri") { }

    CreatureAI* GetAI(Creature *_Creature) const
    {
        return new npc_xiriAI (_Creature);
    }

struct npc_xiriAI : public ScriptedAI
{
    npc_xiriAI(Creature *c) : ScriptedAI(c)
    {
        EventStarted = false;
    }

    ObjectGuid EventStarter;
    bool EventStarted;

    XiriPhases Phase;
    XiriPhases NextPhase;

    uint32 Wait_Timer;
    Creature* temp;

    ObjectGuid Akama;
    ObjectGuid Maiev;
    ObjectGuid Vagath;
    ObjectGuid Ashtongue[5];
    ObjectGuid Demons[6];

    void Reset()
    {
        //EventStarter = 0;
        //EventStarted = false;
    }

    void MoveInLineOfSight(Unit* who){}

    void StartEvent(Player* starter)
    {
        EventStarted = true;
        EventStarter.Set(starter->GetGUID());

        me->setActive(true);

        Phase = PHASE_WAIT;
        NextPhase = PHASE_SUMMON_DEMONS;
        Wait_Timer = 10000;

        me->SetUInt32Value(UNIT_NPC_FLAGS,0);
    }

    bool AreDemonsDead()
    {
        for(int i = 0; i < 6; i++)
        {
            Creature* c_Demon = ObjectAccessor::GetCreature((*me), Demons[i]);
            if(c_Demon && c_Demon->IsAlive())
                return false;
        }
        return true;
    }

    void ForceDespawn(Creature* c_creature)
    {
        c_creature->GetMotionMaster()->MoveIdle();
        c_creature->SetVisible(false);
        // c_creature->DealDamage(c_creature, c_creature->GetHealth());
        c_creature->RemoveCorpse();

    }

    void EnterCombat(Unit* who) { }

    //not used in Trinityd, function for external script library
    void EventHappens(Player* player, uint32 questId)
    {
        if(Group *pGroup = player->GetGroup() )
        {
            for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player *pGroupGuy = itr->GetSource();

                // for any leave or dead (with not released body) group member at appropriate distance
                if(pGroupGuy && pGroupGuy->IsWithinDistInMap(me,500) && !pGroupGuy->GetCorpse() )
                    pGroupGuy->AreaExploredOrEventHappens(questId);
            }
        }
        else
            player->AreaExploredOrEventHappens(questId);
    }


    void UpdateAI(const uint32 diff)
    {
        if(!EventStarted)
            return;
        {

            Creature* c_Vagath;
            Creature* c_Akama;
            Creature* c_Maiev;

            switch(Phase)
            {
            case PHASE_SUMMON_DEMONS:
                temp = me->SummonCreature(ENTRY_VAGATH,Wyrm_SomePositions[0][0],Wyrm_SomePositions[0][1],Wyrm_SomePositions[0][2],M_PI*0.5,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Vagath.Set(temp->GetGUID());

                temp = me->SummonCreature(ENTRY_ILLIDARI_SHADOWLORD,Wyrm_SomePositions[0][0]-17,Wyrm_SomePositions[0][1]+5,Wyrm_SomePositions[0][2]+1,M_PI*0.5 ,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Demons[0].Set(temp->GetGUID());
                temp = me->SummonCreature(ENTRY_ILLIDARI_SHADOWLORD,Wyrm_SomePositions[0][0]-12,Wyrm_SomePositions[0][1]+5,Wyrm_SomePositions[0][2]+1,M_PI*0.5,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Demons[1].Set(temp->GetGUID());
                temp = me->SummonCreature(ENTRY_ILLIDARI_SHADOWLORD,Wyrm_SomePositions[0][0]-7,Wyrm_SomePositions[0][1]+5,Wyrm_SomePositions[0][2]+1,M_PI*0.5,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Demons[2].Set(temp->GetGUID());

                temp = me->SummonCreature(ENTRY_ILLIDARI_SHADOWLORD,Wyrm_SomePositions[0][0]+17,Wyrm_SomePositions[0][1]+5,Wyrm_SomePositions[0][2]+1,M_PI*0.5,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Demons[3].Set(temp->GetGUID());
                temp = me->SummonCreature(ENTRY_ILLIDARI_SHADOWLORD,Wyrm_SomePositions[0][0]+12,Wyrm_SomePositions[0][1]+5,Wyrm_SomePositions[0][2]+1,M_PI*0.5,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Demons[4].Set(temp->GetGUID());
                temp = me->SummonCreature(ENTRY_ILLIDARI_SHADOWLORD,Wyrm_SomePositions[0][0]+7,Wyrm_SomePositions[0][1]+5,Wyrm_SomePositions[0][2]+1,M_PI*0.5,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                Demons[5].Set(temp->GetGUID());

                Phase = PHASE_WAIT;
                NextPhase = PHASE_SUMMON_AKAMA;
                Wait_Timer = 5000;
                break;
            case PHASE_SUMMON_AKAMA:
                temp = me->SummonCreature(ENTRY_AKAMA,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[2][0],Wyrm_SomePositions[2][1],Wyrm_SomePositions[2][2]);
                Akama.Set(temp->GetGUID());

                temp = me->SummonCreature(ENTRY_MAIEV,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                //temp->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY, 44850);
                //temp->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY + 1, 0);
                //temp->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY + 2, 45738);
                SetEquipmentSlots(false, 44850, EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                // @todo me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, 45738);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[3][0],Wyrm_SomePositions[3][1],Wyrm_SomePositions[3][2]);
                Maiev.Set(temp->GetGUID());

                temp = me->SummonCreature(21701,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[3][0],Wyrm_SomePositions[3][1]+5,Wyrm_SomePositions[3][2]);
                Ashtongue[0].Set(temp->GetGUID());

                temp = me->SummonCreature(21701,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[3][0]+5,Wyrm_SomePositions[3][1]+10,Wyrm_SomePositions[3][2]);
                Ashtongue[1].Set(temp->GetGUID());

                temp = me->SummonCreature(21701,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[3][0]-5,Wyrm_SomePositions[3][1]+10,Wyrm_SomePositions[3][2]);
                Ashtongue[2].Set(temp->GetGUID());

                temp = me->SummonCreature(21701,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[3][0]+10,Wyrm_SomePositions[3][1]+15,Wyrm_SomePositions[3][2]);
                Ashtongue[3].Set(temp->GetGUID());

                temp = me->SummonCreature(21701,Wyrm_SomePositions[1][0],Wyrm_SomePositions[1][1],Wyrm_SomePositions[1][2],M_PI*1.5f,TEMPSUMMON_TIMED_DESPAWN,300000);
                temp->setActive(true);
                temp->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                temp->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[3][0]-10,Wyrm_SomePositions[3][1]+15,Wyrm_SomePositions[3][2]);
                Ashtongue[4].Set(temp->GetGUID());

                Phase = PHASE_WAIT;
                NextPhase = PHASE_ATTACK;
                Wait_Timer = 20000;
                break;
            case PHASE_ATTACK:
                
                c_Vagath = ObjectAccessor::GetCreature((*me),Vagath);
                c_Akama = ObjectAccessor::GetCreature((*me),Akama);
                c_Maiev = ObjectAccessor::GetCreature((*me),Maiev);
                if(c_Vagath && c_Akama && c_Maiev)
                {
                    c_Akama->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[0][0],Wyrm_SomePositions[0][1]+10,Wyrm_SomePositions[0][2]);
                    c_Akama->SetHomePosition(Wyrm_SomePositions[0][0],Wyrm_SomePositions[0][1]+10,Wyrm_SomePositions[0][2],M_PI*1.5f);
                    c_Maiev->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[0][0],Wyrm_SomePositions[0][1],Wyrm_SomePositions[0][2]);
                    c_Maiev->SetHomePosition(Wyrm_SomePositions[0][0],Wyrm_SomePositions[0][1],Wyrm_SomePositions[0][2],M_PI*1.5f);
                
                    for(int i = 0; i < 5; i++)
                    {
                        Creature* c_Ashtongue;
                        c_Ashtongue = ObjectAccessor::GetCreature((*me),Ashtongue[i]);
                        if(c_Ashtongue)
                        {
                            c_Ashtongue->GetMotionMaster()->MovePoint(0,float(Wyrm_SomePositions[0][0]+(rand()%5-2)),float(Wyrm_SomePositions[0][1]+10),float(Wyrm_SomePositions[0][2]+0.5));
                            c_Ashtongue->SetHomePosition(float(Wyrm_SomePositions[0][0]+(rand()%5-2)),float(Wyrm_SomePositions[0][1]+10),float(Wyrm_SomePositions[0][2]+0.5),M_PI*1.5f);
                        }
                    }

                }
                Phase = PHASE_KILL_WAIT;
                break;
            case PHASE_KILL_WAIT:
                c_Vagath = ObjectAccessor::GetCreature((*me),Vagath);
                if((!c_Vagath || c_Vagath->IsDead()) && AreDemonsDead())
                {
                    for(int i = 0; i < 5; i++)
                    {
                        Creature* c_Ashtongue;
                        c_Ashtongue = ObjectAccessor::GetCreature((*me),Ashtongue[i]);
                        if(c_Ashtongue)
                        {
                            c_Ashtongue->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                            c_Ashtongue->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[4][0],Wyrm_SomePositions[4][1],Wyrm_SomePositions[4][2]);
                            c_Ashtongue->SetHomePosition(Wyrm_SomePositions[4][0],Wyrm_SomePositions[4][1],Wyrm_SomePositions[4][2],M_PI*1.5f);
                        }
                    }

                    c_Akama = ObjectAccessor::GetCreature((*me),Akama);
                    c_Maiev = ObjectAccessor::GetCreature((*me),Maiev);
                    if(c_Akama)
                    {
                        c_Akama->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                        c_Akama->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[4][0],Wyrm_SomePositions[4][1],Wyrm_SomePositions[4][2]);
                        c_Akama->SetHomePosition(Wyrm_SomePositions[4][0],Wyrm_SomePositions[4][1],Wyrm_SomePositions[4][2],M_PI*1.5f);

                    }
                    if(c_Maiev)
                    {
                        c_Maiev->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                        c_Maiev->GetMotionMaster()->MovePoint(0,Wyrm_SomePositions[4][0],Wyrm_SomePositions[4][1],Wyrm_SomePositions[4][2]);
                        c_Maiev->SetHomePosition(Wyrm_SomePositions[4][0],Wyrm_SomePositions[4][1],Wyrm_SomePositions[4][2],M_PI*1.5f);
                    }

                    Phase = PHASE_WAIT;
                    NextPhase = PHASE_END;
                    Wait_Timer = 15000;
                }
                break;
            case PHASE_END:
                c_Akama = ObjectAccessor::GetCreature((*me),Akama);
                c_Maiev = ObjectAccessor::GetCreature((*me),Maiev);

                if(c_Akama) ForceDespawn(c_Akama);
                if(c_Maiev) ForceDespawn(c_Maiev);
                for(int i = 0; i < 5; i++)
                {
                    Creature* c_Ashtongue;
                    c_Ashtongue = ObjectAccessor::GetCreature((*me),Ashtongue[i]);
                    if(c_Ashtongue) ForceDespawn(c_Ashtongue);
                }

                if(Player* starter = ObjectAccessor::GetPlayer(*me,EventStarter))
                {
                    EventHappens(starter, 10985);
                    EventHappens(starter, 13429);
                }

                me->SetUInt32Value(UNIT_NPC_FLAGS,3);
                EventStarted = false;
                break;
            case PHASE_WAIT:
                if(Wait_Timer <= diff)
                {
                    Phase = NextPhase;
                    Wait_Timer = 0;
                } else Wait_Timer -= diff;
                break;
            }

        }
    }
};

#define XIRI_OPTION      "Давай начнем"

    bool OnGossipHello(Player *player, Creature *_Creature)
    {
        if(_Creature->IsQuestGiver())
            player->PrepareQuestMenu( _Creature->GetGUID() );

        if((player->GetQuestStatus(10985) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(13429) == QUEST_STATUS_INCOMPLETE ))
            AddGossipItemFor(player,  0, XIRI_OPTION, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        SendGossipMenuFor(player, player->GetGossipTextId(_Creature),_Creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player *player, Creature *_Creature, uint32 sender, uint32 action)
    {
            if(action == GOSSIP_ACTION_INFO_DEF+1)
            {
                CloseGossipMenuFor(player);
                CAST_AI(npc_xiri::npc_xiriAI,_Creature->AI())->StartEvent(player);
            }

        return true;
    }
};

#define SPELL_SHADOW_STRIKE                 40685
#define SPELL_THROW_DAGGERS                 41152

class npc_preeven_maiev : public CreatureScript
{
public:
    npc_preeven_maiev() : CreatureScript("npc_preeven_maiev") { }

    CreatureAI* GetAI(Creature *_Creature) const
    {
        return new npc_preeven_maievAI (_Creature);
    }

    struct npc_preeven_maievAI : public ScriptedAI
    {
        npc_preeven_maievAI(Creature *c) : ScriptedAI(c)
        {
            Reset();
        }

        uint32 strike_Timer;
        uint32 throw_Timer;

        // void DamageTaken(Unit* done_by, uint32 &damage)
        //{
        //    damage = 0;
        //}

        void Reset()
        {
                strike_Timer = 10000 + rand()%10000;
                throw_Timer = 10000 + rand()%10000;
        }

        void EnterCombat(Unit* who) { }
        void UpdateAI(const uint32 diff)
        {
            if(!UpdateVictim() )
                return;

            if(strike_Timer <= diff)
            {
                Unit* ptarget = SelectTarget(SELECT_TARGET_RANDOM,0);
                DoCast(ptarget,SPELL_SHADOW_STRIKE);
                strike_Timer = 10000 + rand()%10000;
            } else strike_Timer -= diff;

        
            if(throw_Timer <= diff)
            {
                DoCast(me->GetVictim(),SPELL_THROW_DAGGERS);
                throw_Timer = 10000 + rand()%10000;
            } else throw_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_shadowmoon_valley()
{
    new mob_mature_netherwing_drake;
    new mob_enslaved_netherwing_drake;
    new mob_dragonmaw_peon;
    new npc_drake_dealer_hurlunk;
    new npcs_flanis_swiftwing_and_kagrosh;
    new npc_murkblood_overseer;
    new npc_karynaku;
    new npc_oronok_tornheart;
    new npc_overlord_morghor;
    new npc_earthmender_wilda;
    new npc_lord_illidan_stormrage;
    new go_crystal_prison;
    new mob_illidari_spawn;
    new mob_torloth_the_magnificent;
    new npc_enraged_spirit;
    new npc_xiri;
    new npc_preeven_maiev;
}
