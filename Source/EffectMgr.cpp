#include "stdafx.h"

#include <GWCA/Packets/Opcodes.h>
#include <GWCA/Constants/Skills.h>

#include <GWCA/Utilities/Debug.h>
#include <GWCA/Utilities/Export.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Macros.h>
#include <GWCA/Utilities/Scanner.h>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameContainers/GamePos.h>

#include <GWCA/Packets/StoC.h>

#include <GWCA/GameEntities/NPC.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>

#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/WorldContext.h>

#include <GWCA/Managers/Module.h>

#include <GWCA/Managers/CtoSMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>

namespace {
    using namespace GW;

    uint32_t alcohol_level = 0;
    // post processing effects hook
    typedef void(__cdecl *PostProcessEffect_pt)(uint32_t intensity, uint32_t tint);
    PostProcessEffect_pt RetPostProcessEffect;
    PostProcessEffect_pt PostProcessEffect_Func;

    void __cdecl OnPostProcessEffect(uint32_t intensity, uint32_t tint) {
        HookBase::EnterHook();
        alcohol_level = intensity;
        RetPostProcessEffect(intensity, tint);
        HookBase::LeaveHook();
    }

    void Init() {
        PostProcessEffect_Func = (PostProcessEffect_pt)Scanner::Find(
            "\xD9\x5D\x0C\xD9\x45\x0C\x8D\x45\xF8", "xxxxxxxxx", -0x1C);
        GWCA_INFO("[SCAN] PostProcessEffect = %p\n", PostProcessEffect_Func);

        if (Verify(PostProcessEffect_Func))
            HookBase::CreateHook(PostProcessEffect_Func, OnPostProcessEffect, (void **)&RetPostProcessEffect);
    }

    void Exit() {
        if (PostProcessEffect_Func)
            HookBase::RemoveHook(PostProcessEffect_Func);
    }

}

namespace GW {
    Module EffectModule = {
        "EffectModule",     // name
        NULL,               // param
        ::Init,             // init_module
        ::Exit,             // exit_module
        NULL,               // enable_hooks
        NULL,               // disable_hooks
    };
    namespace Effects {

        uint32_t GetAlcoholLevel() {
            return alcohol_level;
        }

        void GetDrunkAf(uint32_t intensity, uint32_t tint) {
            RetPostProcessEffect(intensity, tint);
        }



        Effect* GetPlayerEffectBySkillId(Constants::SkillID skill_id) {
            auto* effects = GetPlayerEffects();
            if (!effects) return nullptr;
            for (auto& effect : *effects) {
                if (effect.skill_id == (uint32_t)skill_id)
                    return &effect;
            }
            return nullptr;
        }

        Buff* GetPlayerBuffBySkillId(Constants::SkillID skill_id) {
            auto* buffs = GetPlayerBuffs();
            if (!buffs) return nullptr;
            for (auto& buff : *buffs) {
                if (buff.skill_id == (uint32_t)skill_id)
                    return &buff;
            }
            return nullptr;
        }
        AgentEffectsArray* GetPartyEffectArray() {
            auto* w = WorldContext::instance();
            return w && w->party_effects.valid() ? &w->party_effects : nullptr;
        }
        AgentEffects* GetAgentEffectsArray(uint32_t agent_id) {
            AgentEffectsArray* agent_effects = GetPartyEffectArray();
            if (!agent_effects) return nullptr;
            for (auto& agent_effect : *agent_effects) {
                if (agent_effect.agent_id == agent_id)
                    return &agent_effect;
            }
            return nullptr;
        }
        EffectArray* GetAgentEffects(uint32_t agent_id) {
            auto* e = GetAgentEffectsArray(agent_id);
            return e && e->effects.valid() ? &e->effects : nullptr;
        }
        BuffArray* GetAgentBuffs(uint32_t agent_id) {
            auto* e = GetAgentEffectsArray(agent_id);
            return e && e->buffs.valid() ? &e->buffs : nullptr;
        }
        EffectArray* GetPlayerEffects() {
            return GetAgentEffects(Agents::GetPlayerId());
        }

        BuffArray* GetPlayerBuffs() {
            return GetAgentBuffs(Agents::GetPlayerId());
        }

        void DropBuff(uint32_t buff_id) {
            CtoS::SendPacket(0x8, GAME_CMSG_DROP_BUFF, buff_id);
        }
    }
} // namespace GW
