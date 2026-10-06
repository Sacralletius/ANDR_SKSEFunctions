#include "RE/A/Actor.h"
#include "RE/M/MagicItem.h"
#include "RE/N/NiPoint3.h"
#include "RE/V/VirtualMachine.h"
#include <limits>
#include <random>

/*Added by Ivy*/

#include <chrono>

#include "RE/B/BookMenu.h"
#include "RE/G/GFxMovieView.h"
#include "RE/G/GFxValue.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/M/MiddleHighProcessData.h"
#include "RE/M/MenuTopicManager.h"
#include "RE/T/TESObjectBOOK.h"
#include "RE/T/TESTopicInfo.h"
#include "RE/U/UI.h"
#include "SKSE/API.h"
#include "SKSE/Events.h"

/*Added By Ivy - Ends*/

///// Actual new functions /////

// Papyrus: String Function GetAndrealphusExtenderVersion() Global Native
// Returns the version number of the mod.
RE::BSFixedString GetAndrealphusExtenderVersion(RE::StaticFunctionTag*) { return "1.9.0"; }

///// Added by Ivy /////

// Papyrus: Int Function GetCurrentBookPage() Global Native
// Returns the current page of the book, when in the book menu. Otherwise, returns -1
int GetCurrentBookPage(RE::StaticFunctionTag*) {
    auto ui = RE::UI::GetSingleton();

    if (!ui) {
        return -1;
    }

    if (!ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
        return -1;
    }

    auto menu = ui->GetMenu<RE::BookMenu>();

    if (!menu) {
        return -1;
    }

    auto movie = menu->GetRuntimeData().book.get();

    if (!movie) {
        return -1;
    }

    RE::GFxValue val;

    if (movie->GetVariable(&val, "_root.BookMenu_mc.iLeftPageNumber") && val.IsNumber()) {
        return static_cast<int>(val.GetNumber());
    }

    if (movie->GetVariable(&val, "_root.iLeftPageNumber") && val.IsNumber()) {
        return static_cast<int>(val.GetNumber());
    }

    if (movie->GetVariable(&val, "_root.BookMenu_mc.iPageSetIndex") && val.IsNumber()) {
        return static_cast<int>(val.GetNumber());
    }

    if (movie->GetVariable(&val, "_root.iPageSetIndex") && val.IsNumber()) {
        return static_cast<int>(val.GetNumber());
    }

    return -1;
}

// Papyrus: String Function GetBookText(Book akBook) Global Native
// Returns the text of a book as a string.
RE::BSFixedString GetBookText(RE::StaticFunctionTag*, RE::TESObjectBOOK* akBook) {
    if (!akBook) {
        return "";
    }

    RE::BSString text;
    akBook->GetDescription(text, akBook);

    return RE::BSFixedString(text.c_str());
}

constexpr float g_bookFullyReadSeconds = 5.0f;

static bool TryGetNumber(RE::GFxMovieView* a_movie, const char* a_path, int& a_out) {
    RE::GFxValue val;

    if (!a_movie) {
        return false;
    }

    if (!a_movie->GetVariable(&val, a_path)) {
        return false;
    }

    if (!val.IsNumber()) {
        return false;
    }

    a_out = static_cast<int>(val.GetNumber());
    return true;
}

namespace BookFullyRead {
    using clock = std::chrono::steady_clock;

    constexpr float kOffEndDebounceSeconds = 0.25f;

    static bool firedThisOpen = false;
    static bool onLastPage = false;
    static bool offEnd = false;
    static clock::time_point enteredLastPage{};
    static clock::time_point leftLastPage{};

    static void Reset() {
        firedThisOpen = false;
        onLastPage = false;
        offEnd = false;
    }

    static void Update(RE::BookMenu* a_menu) {
        if (firedThisOpen) {
            return;
        }

        auto movie = a_menu->GetRuntimeData().book.get();
        if (!movie) {
            return;
        }

        int totalPages = -1;
        if (!(TryGetNumber(movie, "_root.BookMenu_mc.PageInfoA.length", totalPages) ||
              TryGetNumber(movie, "_root.PageInfoA.length", totalPages))) {
            return;
        }

        if (totalPages <= 0) {
            return;
        }

        int leftPage = -1;
        int pageSetIndex = -1;

        bool haveLeft = TryGetNumber(movie, "_root.BookMenu_mc.iLeftPageNumber", leftPage) ||
                        TryGetNumber(movie, "_root.iLeftPageNumber", leftPage);

        bool haveSet = TryGetNumber(movie, "_root.BookMenu_mc.iPageSetIndex", pageSetIndex) ||
                       TryGetNumber(movie, "_root.iPageSetIndex", pageSetIndex);

        if (!haveLeft && haveSet) {
            leftPage = pageSetIndex * 2;
            haveLeft = true;
        }

        if (!haveLeft) {
            return;
        }

        int lastSet = (totalPages - 1) / 2;
        int lastLeft = lastSet * 2;

        bool atEnd = false;

        if (haveSet && pageSetIndex == lastSet) {
            atEnd = true;
        }

        if (leftPage == lastLeft || (leftPage - 1) == lastLeft) {
            atEnd = true;
        }

        auto now = clock::now();

        if (!atEnd) {
            if (onLastPage) {
                if (!offEnd) {
                    offEnd = true;
                    leftLastPage = now;
                } else if (std::chrono::duration<float>(now - leftLastPage).count() >= kOffEndDebounceSeconds) {
                    onLastPage = false;
                    offEnd = false;
                }
            }
            return;
        }

        offEnd = false;

        if (!onLastPage) {
            onLastPage = true;
            enteredLastPage = now;
            return;
        }

        if (std::chrono::duration<float>(now - enteredLastPage).count() < g_bookFullyReadSeconds) {
            return;
        }

        auto src = SKSE::GetModCallbackEventSource();
        if (src) {
            SKSE::ModCallbackEvent ev;
            ev.eventName = "ANDR_OnBookFullyRead";
            ev.strArg = "";
            ev.numArg = static_cast<float>(leftPage);

            RE::TESForm* sender = RE::BookMenu::GetTargetForm();
            if (!sender) {
                sender = RE::BookMenu::GetTargetReference().get();
            }

            ev.sender = sender;
            src->SendEvent(&ev);
        }

        firedThisOpen = true;
    }

    struct AdvanceMovieHook {
        static void thunk(RE::BookMenu* a_this, float a_interval, std::uint32_t a_currentTime) {
            func(a_this, a_interval, a_currentTime);
            Update(a_this);
        }

        static inline REL::Relocation<decltype(thunk)> func;
    };

    class MenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MenuWatcher* GetSingleton() {
            static MenuWatcher singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (a_event && a_event->menuName == RE::BookMenu::MENU_NAME) {
                Reset();
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    static void Install() {
        REL::Relocation<std::uintptr_t> vtbl{RE::BookMenu::VTABLE[0]};
        AdvanceMovieHook::func = vtbl.write_vfunc(0x5, AdvanceMovieHook::thunk);

        auto messaging = SKSE::GetMessagingInterface();
        if (!messaging) {
            return;
        }

        messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_message) {
            if (!a_message || a_message->type != SKSE::MessagingInterface::kDataLoaded) {
                return;
            }

            auto ui = RE::UI::GetSingleton();
            if (ui) {
                ui->AddEventSink<RE::MenuOpenCloseEvent>(MenuWatcher::GetSingleton());
            }
        });
    }
}

RE::TESObjectBOOK* GetOpenedBook(RE::StaticFunctionTag*) {
    auto ui = RE::UI::GetSingleton();

    if (!ui) {
        return nullptr;
    }

    if (!ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
        return nullptr;
    }

    if (auto* form = RE::BookMenu::GetTargetForm()) {
        if (auto* book = form->As<RE::TESObjectBOOK>()) {
            return book;
        }
    }

    if (auto ref = RE::BookMenu::GetTargetReference()) {
        if (auto* base = ref->GetBaseObject()) {
            if (auto* book = base->As<RE::TESObjectBOOK>()) {
                return book;
            }
        }
    }

    return nullptr;
}

// Papyrus: TopicInfo Function GetCurrentTopicInfo() Global Native
// Returns the TopicInfo the NPC is currently speaking. Otherwise, returns None.
RE::TESTopicInfo* GetCurrentTopicInfo(RE::StaticFunctionTag*) {
    auto topicManager = RE::MenuTopicManager::GetSingleton();

    if (!topicManager) {
        return nullptr;
    }

    return topicManager->currentTopicInfo;
}




//   commander: a CommandedActorData entry in middleHigh->commandedActors
//   commanded: middleHigh->commandingActor, plus the kIsCommandedActor flag
static RE::MiddleHighProcessData* EnsureMiddleHighProcess(RE::Actor* a_actor) {
    if (!a_actor) {
        return nullptr;
    }

    if (auto data = a_actor->GetMiddleHighProcess()) {
        return data;
    }

    a_actor->MoveToMiddleHigh();

    return a_actor->GetMiddleHighProcess();
}

static void ForgetCommandedActor(RE::Actor* a_commander, RE::ActorHandle a_commanded) {
    auto commanderData = EnsureMiddleHighProcess(a_commander);

    if (!commanderData) {
        return;
    }

    auto& commandedActors = commanderData->commandedActors;

    for (auto it = commandedActors.begin(); it != commandedActors.end(); ++it) {
        if (it->commandedActor == a_commanded) {
            commandedActors.erase(it);
            return;
        }
    }
}

#ifdef GetObject
#    undef GetObject
#endif

static RE::SpellItem* GetCommandedActorAbility() {
    auto defaults = RE::BGSDefaultObjectManager::GetSingleton();

    if (!defaults) {
        return nullptr;
    }

    return defaults->GetObject<RE::SpellItem>(RE::DEFAULT_OBJECTS::kCommandedActorAbility);
}

// Papyrus: Bool Function ApplyCommandEffect(Actor akCaster, Actor akTarget) Global Native
// Makes akTarget a commanded actor of akCaster. Unlike vanilla, works on living, non-summoned actors
// and for non-player casters. Any previous commander is released first.
bool ApplyCommandEffect(RE::StaticFunctionTag*, RE::Actor* akCaster, RE::Actor* akTarget) {
    if (!akCaster || !akTarget || akCaster == akTarget) {
        return false;
    }

    auto casterData = EnsureMiddleHighProcess(akCaster);
    auto targetData = EnsureMiddleHighProcess(akTarget);

    if (!casterData || !targetData) {
        return false;
    }

    auto casterHandle = akCaster->GetHandle();
    auto targetHandle = akTarget->GetHandle();

    // An actor answers to one commander, so take it off any previous commander's list.
    if (auto previous = akTarget->GetCommandingActor()) {
        if (previous.get() != akCaster) {
            ForgetCommandedActor(previous.get(), targetHandle);
        }
    }

    auto& commandedActors = casterData->commandedActors;
    bool alreadyListed = false;

    for (auto& entry : commandedActors) {
        if (entry.commandedActor == targetHandle) {
            alreadyListed = true;
            break;
        }
    }

    if (!alreadyListed) {
        RE::CommandedActorData entry{};

        entry.commandedActor = targetHandle;
        entry.activeEffect = nullptr;

        commandedActors.push_back(entry);
    }

    targetData->commandingActor = casterHandle;
    akTarget->GetActorRuntimeData().boolFlags.set(RE::Actor::BOOL_FLAGS::kIsCommandedActor);

    if (auto ability = GetCommandedActorAbility()) {
        if (!akTarget->HasSpell(ability)) {
            akTarget->AddSpell(ability);
        }
    }
	
    akTarget->StopCombat();
    akTarget->EvaluatePackage();

    return true;
}

// Papyrus: Function EndCommandEffect(Actor akCaster, Actor akTarget) Global Native
// Releases akTarget from akCaster's command. Does nothing if akCaster is not its commander.
void EndCommandEffect(RE::StaticFunctionTag*, RE::Actor* akCaster, RE::Actor* akTarget) {
    if (!akCaster || !akTarget) {
        return;
    }

    auto targetData = EnsureMiddleHighProcess(akTarget);

    if (!targetData) {
        return;
    }

    if (targetData->commandingActor != akCaster->GetHandle()) {
        return;
    }

    ForgetCommandedActor(akCaster, akTarget->GetHandle());

    targetData->commandingActor = RE::ActorHandle{};
    akTarget->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kIsCommandedActor);

    if (auto ability = GetCommandedActorAbility()) {
        akTarget->RemoveSpell(ability);
    }
	
    akTarget->EvaluatePackage();
}

// Papyrus: Function CastEnchantment(Actor akSource, Enchantment akEnchantment, Actor akTarget)
// Cast the akEnchantment from the akSource to the akTarget.
void CastEnchantment(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment,
                     RE::Actor* akTarget) {
    if (!akSource || !akEnchantment) {
        return;
    }

    auto caster = akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);

    if (!caster) {
        return;
    }

    caster->CastSpellImmediate(akEnchantment, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastPotion(Actor akSource, Potion akPotion, Actor akTarget)
// Cast the akPotion from the akSource to the akTarget.
void CastPotion(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion, RE::Actor* akTarget) {
    if (!akSource || !akPotion) {
        return;
    }

    auto caster = akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);

    if (!caster) {
        return;
    }

    caster->CastSpellImmediate(akPotion, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastIngredient(Actor akSource, Ingredient akIngredient, Actor akTarget)
// Cast the akIngredient from the akSource to the akTarget.
void CastIngredient(RE::StaticFunctionTag*, RE::Actor* akSource, RE::IngredientItem* akIngredient,
                    RE::Actor* akTarget) {
    if (!akSource || !akIngredient) {
        return;
    }

    auto caster = akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);

    if (!caster) {
        return;
    }

    caster->CastSpellImmediate(akIngredient, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Float Function GetEffectiveEnchantmentCost(Actor akSource, Enchantment akEnchantment)
// Get the total effect cost of the enchantment.
float GetEffectiveEnchantmentCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment) {
    if (!akEnchantment) {
        return 0.0f;
    }

    return akEnchantment->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectivePotionCost(Actor akSource, Potion akPotion)
// Get the total effect cost of the potion.
float GetEffectivePotionCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion) {
    if (!akPotion) {
        return 0.0f;
    }

    return akPotion->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectiveIngredientCost(Actor akSource, Ingredient akIngredient)
// Get the total effect cost of the ingredient.
float GetEffectiveIngredientCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::IngredientItem* akIngredient) {
    if (!akIngredient) {
        return 0.0f;
    }

    return akIngredient->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectiveScrollCost(Actor akSource, Scroll akScroll)
// Get the total effect cost of the scroll.
float GetEffectiveScrollCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::ScrollItem* akScroll) {
    if (!akScroll) {
        return 0.0f;
    }

    return akScroll->CalculateMagickaCost(akSource);
}

// ActiveMagicEffect Function GetActiveMagicEffectFromActor(Actor akActor, MagicEffect akMagicEffect) native global
// Get the instance of akMagicEffect on akActor
RE::ActiveEffect* GetActiveMagicEffectFromActor(RE::StaticFunctionTag*, RE::Actor* akActor,
                                                RE::EffectSetting* akMagicEffect) {
    if (akMagicEffect == nullptr) {
        return nullptr;
    }

    if (akActor == nullptr) {
        return nullptr;
    }

    auto magicTarget = akActor->AsMagicTarget();

    if (!magicTarget) {
        return nullptr;
    }

    auto EffectsList = magicTarget->GetActiveEffectList();

    if (!EffectsList) {
        return nullptr;
    }

    for (const auto& effect : *EffectsList) {
        const auto& setting = effect ? effect->GetBaseObject() : nullptr;

        if (setting) {
            if (setting == akMagicEffect) {
                return effect;
            }
        }
    }
    return nullptr;
}

void SetRefAsNoAIAcquire(RE::StaticFunctionTag*, RE::TESObjectREFR* akObject, bool SetNoAIAquire) {
    if (!akObject) {
        return;
    }

    if (SetNoAIAquire == true) {
        akObject->formFlags |= RE::TESObjectREFR::RecordFlags::RecordFlag::kNoAIAcquire;
    } else {
        akObject->formFlags &= ~RE::TESObjectREFR::RecordFlags::RecordFlag::kNoAIAcquire;
    }
}

// START: Extra requirements for CastSpellFromRef and CastSpellFromPointToPoint -> instructions from Fenix

struct ProjectileRotCustom {
    float x, z;
};

RE::EffectSetting* getAVEffectSetting(RE::MagicItem* mgitem) {
    using func_t = decltype(getAVEffectSetting);
    REL::Relocation<func_t> func{REL::RelocationID(11194, 11302)};
    return func(mgitem);
}

float SkyrimSE_c51f70(RE::NiPoint3* dir) {
    using func_t = decltype(SkyrimSE_c51f70);
    REL::Relocation<func_t> func{REL::RelocationID(68820, 70172)};
    return func(dir);
}

auto rot_at_custom(RE::NiPoint3 dir) {
    ProjectileRotCustom rotcustom;
    auto len = dir.Unitize();
    if (len == 0) {
        rotcustom = {0, 0};
    } else {
        float polar_angle = SkyrimSE_c51f70(&dir);
        rotcustom = {-asin(dir.z), polar_angle};
    }
    return rotcustom;
}

auto rot_at_custom(const RE::NiPoint3& from, const RE::NiPoint3& to) { return rot_at_custom(to - from); }

// END: Extra requirements for CastSpellFromRef and CastSpellFromPointToPoint -> instructions from Fenix

void CastSpellFromRef(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, RE::TESObjectREFR* akTarget,
                      RE::TESObjectREFR* akOriginRef) {
    if (!akSource || !akSpell || !akTarget || !akOriginRef) {
        return;
    }

    auto sourceHandle = akSource->GetHandle();
    auto targetHandle = akTarget->GetHandle();
    auto originHandle = akOriginRef->GetHandle();

    SKSE::GetTaskInterface()->AddTask([sourceHandle, targetHandle, originHandle, akSpell]() {
        auto source = sourceHandle.get();
        auto target = targetHandle.get();
        auto originRef = originHandle.get();

        if (!source || !target || !originRef) {
            return;
        }

        auto NodePosition = originRef->GetPosition();

        auto rotcustom = rot_at_custom(NodePosition, target->GetPosition());

        auto eff = akSpell->GetCostliestEffectItem();

        auto mgef = getAVEffectSetting(akSpell);

        if (!eff || !mgef || !mgef->data.projectileBase) {
            return;
        }

        RE::Projectile::LaunchData ldata{};

        ldata.origin = NodePosition;
        ldata.contactNormal = {0.0f, 0.0f, 0.0f};
        ldata.projectileBase = mgef->data.projectileBase;
        ldata.shooter = source.get();
        ldata.combatController = source->GetActorRuntimeData().combatController;
        ldata.weaponSource = nullptr;
        ldata.ammoSource = nullptr;
        ldata.angleZ = rotcustom.z;
        ldata.angleX = rotcustom.x;
        ldata.unk50 = nullptr;
        ldata.desiredTarget = nullptr;
        ldata.unk60 = 0.0f;
        ldata.unk64 = 0.0f;
        ldata.parentCell = source->GetParentCell();
        ldata.spell = akSpell;
        ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
        ldata.enchantItem = nullptr;
        ldata.poison = nullptr;
        ldata.area = eff->GetArea();
        ldata.power = 1.0f;
        ldata.scale = 1.0f;
        ldata.alwaysHit = false;
        ldata.noDamageOutsideCombat = false;
        ldata.autoAim = false;
        ldata.useOrigin = true;
        ldata.deferInitialization = false;
        ldata.forceConeOfFire = false;

        RE::BSPointerHandle<RE::Projectile> handle;
        RE::Projectile::Launch(&handle, ldata);
    });
}

void CastSpellFromPointToPoint(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, float StartPoint_X,
                               float StartPoint_Y, float StartPoint_Z, float EndPoint_X, float EndPoint_Y,
                               float EndPoint_Z) {
    if (!akSource || !akSpell) {
        return;
    }

    auto sourceHandle = akSource->GetHandle();

    SKSE::GetTaskInterface()->AddTask([sourceHandle, akSpell, StartPoint_X, StartPoint_Y, StartPoint_Z, EndPoint_X,
                                       EndPoint_Y, EndPoint_Z]() {
        auto source = sourceHandle.get();

        if (!source) {
            return;
        }

        RE::NiPoint3 NodePosition;

        NodePosition.x = StartPoint_X;
        NodePosition.y = StartPoint_Y;
        NodePosition.z = StartPoint_Z;

        RE::NiPoint3 DestinationPosition;

        DestinationPosition.x = EndPoint_X;
        DestinationPosition.y = EndPoint_Y;
        DestinationPosition.z = EndPoint_Z;

        auto rotcustom = rot_at_custom(NodePosition, DestinationPosition);

        auto eff = akSpell->GetCostliestEffectItem();

        auto mgef = getAVEffectSetting(akSpell);

        if (!eff || !mgef || !mgef->data.projectileBase) {
            return;
        }

        RE::Projectile::LaunchData ldata{};

        ldata.origin = NodePosition;
        ldata.contactNormal = {0.0f, 0.0f, 0.0f};
        ldata.projectileBase = mgef->data.projectileBase;
        ldata.shooter = source.get();
        ldata.combatController = source->GetActorRuntimeData().combatController;
        ldata.weaponSource = nullptr;
        ldata.ammoSource = nullptr;
        ldata.angleZ = rotcustom.z;
        ldata.angleX = rotcustom.x;
        ldata.unk50 = nullptr;
        ldata.desiredTarget = nullptr;
        ldata.unk60 = 0.0f;
        ldata.unk64 = 0.0f;
        ldata.parentCell = source->GetParentCell();
        ldata.spell = akSpell;
        ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
        ldata.enchantItem = nullptr;
        ldata.poison = nullptr;
        ldata.area = eff->GetArea();
        ldata.power = 1.0f;
        ldata.scale = 1.0f;
        ldata.alwaysHit = false;
        ldata.noDamageOutsideCombat = false;
        ldata.autoAim = false;
        ldata.useOrigin = true;
        ldata.deferInitialization = false;
        ldata.forceConeOfFire = false;

        RE::BSPointerHandle<RE::Projectile> handle;
        RE::Projectile::Launch(&handle, ldata);
    });
}

inline void LaunchAmmo(RE::StaticFunctionTag*, RE::Actor* a_actor, RE::TESAmmo* a_ammo, RE::TESObjectWEAP* a_weapon,
                       RE::BSFixedString a_nodeName, RE::TESObjectREFR* a_target, RE::BGSProjectile* a_projbase,
                       RE::TESObjectREFR* a_secondOrigin) {
    //   a_projbase needs to be assigned through Papyrus. Using a_ammo->data.projectile gives an invalid value. As does
    //   getting it through launchData.

    if (!a_actor || !a_projbase) {
        return;
    }

    auto actorHandle = a_actor->GetHandle();
    auto targetHandle = a_target ? a_target->GetHandle() : RE::ObjectRefHandle{};
    auto originHandle = a_secondOrigin ? a_secondOrigin->GetHandle() : RE::ObjectRefHandle{};

    SKSE::GetTaskInterface()->AddTask([actorHandle, a_ammo, a_weapon, a_nodeName, targetHandle, a_projbase,
                                       originHandle]() {
        auto a_actor = actorHandle.get();
        if (!a_actor) {
            return;
        }

        auto a_target = targetHandle.get();
        auto a_secondOrigin = originHandle.get();

        RE::NiAVObject* root = nullptr;
        if (a_secondOrigin) {
            root = a_secondOrigin->GetCurrent3D();
        } else {
            root = a_actor->GetCurrent3D();
        }

        RE::NiAVObject* fireNode = nullptr;

        if (!a_nodeName.empty()) {
            if (root) {
                fireNode = root->GetObjectByName(a_nodeName);
            }
        } else {
            if (root) {
                fireNode = root;
            }
        }

        RE::NiPoint3 origin;
        RE::Projectile::ProjectileRot angles{};

        if (fireNode && a_target) {
            origin = fireNode->world.translate;

            RE::NiPoint3 targetcoords;

            RE::Actor* a_targetactor = skyrim_cast<RE::Actor*>(a_target.get());

            if (a_actor->IsPlayerRef()) {
                targetcoords = a_target->GetPosition();
            } else {
                if (a_targetactor) {
                    targetcoords = a_target->GetPosition();
                    float heightincrease = static_cast<float>(a_target->GetHeight() * 0.6);
                    targetcoords.z += heightincrease;

                } else {
                    targetcoords = a_target->GetPosition();
                }
            }

            float dx = targetcoords.x - origin.x;
            float dy = targetcoords.y - origin.y;
            float dz = targetcoords.z - origin.z;

            float horizontalDist = (dx * dx + dy * dy);

            float angleX = std::atan2(dz, std::sqrt(horizontalDist));
            float angleZ = std::atan2(dx, dy);

            angles.x = -angleX;
            angles.z = angleZ;

        } else {
            origin = a_actor->GetPosition();
            origin.z += 96.0f;

            angles.x = a_actor->GetAimAngle();
            angles.z = a_actor->GetAimHeading();
        }

        RE::ProjectileHandle handle{};
        RE::Projectile::LaunchData launchData(a_actor.get(), origin, angles, a_ammo, a_weapon);

        launchData.autoAim = false;
        launchData.projectileBase = a_projbase;

        RE::Projectile::Launch(&handle, launchData);
    });
}

inline void LaunchMagicSpell(RE::StaticFunctionTag*, RE::Actor* a_actor, RE::SpellItem* a_spell,
                             RE::BSFixedString a_nodeName, RE::TESObjectREFR* a_target, RE::BGSProjectile* a_projbase,
                             RE::TESObjectREFR* a_secondOrigin) {
    if (!a_actor || !a_projbase) {
        return;
    }

    auto actorHandle = a_actor->GetHandle();
    auto targetHandle = a_target ? a_target->GetHandle() : RE::ObjectRefHandle{};
    auto originHandle = a_secondOrigin ? a_secondOrigin->GetHandle() : RE::ObjectRefHandle{};

    SKSE::GetTaskInterface()->AddTask([actorHandle, a_spell, a_nodeName, targetHandle, a_projbase, originHandle]() {
        auto a_actor = actorHandle.get();
        if (!a_actor) {
            return;
        }

        auto a_target = targetHandle.get();
        auto a_secondOrigin = originHandle.get();

        RE::NiAVObject* root = nullptr;
        if (a_secondOrigin) {
            root = a_secondOrigin->GetCurrent3D();
        } else {
            root = a_actor->GetCurrent3D();
        }

        RE::NiAVObject* fireNode = nullptr;

        if (!a_nodeName.empty()) {
            if (root) {
                fireNode = root->GetObjectByName(a_nodeName);
            }
        } else {
            if (root) {
                fireNode = root;
            }
        }

        RE::NiPoint3 origin;
        RE::Projectile::ProjectileRot angles{};

        if (fireNode && a_target) {
            origin = fireNode->world.translate;
            RE::NiPoint3 targetcoords;

            RE::Actor* a_targetactor = skyrim_cast<RE::Actor*>(a_target.get());

            if (a_actor->IsPlayerRef()) {
                targetcoords = a_target->GetPosition();
            } else {
                if (a_targetactor) {
                    targetcoords = a_target->GetPosition();
                    float heightincrease = static_cast<float>(a_target->GetHeight() * 0.6);
                    targetcoords.z += heightincrease;

                } else {
                    targetcoords = a_target->GetPosition();
                }
            }

            float dx = targetcoords.x - origin.x;
            float dy = targetcoords.y - origin.y;
            float dz = targetcoords.z - origin.z;

            float horizontalDist = (dx * dx + dy * dy);

            float angleX = std::atan2(dz, std::sqrt(horizontalDist));
            float angleZ = std::atan2(dx, dy);

            angles.x = -angleX;
            angles.z = angleZ;

        } else {
            origin = a_actor->GetPosition();
            origin.z += 96.0f;

            angles.x = a_actor->GetAimAngle();
            angles.z = a_actor->GetAimHeading();
        }

        RE::ProjectileHandle handle{};
        RE::Projectile::LaunchData launchData(a_actor.get(), origin, angles, a_spell);

        launchData.autoAim = false;
        launchData.projectileBase = a_projbase;

        RE::Projectile::Launch(&handle, launchData);
    });
}

void MoveRefToCrosshairLoc(RE::StaticFunctionTag*, RE::Actor* originRef, RE::TESObjectREFR* markerRef, float fDistance,
                           float fHeight, bool UseLeftRightOffsets, bool isLeft) {
    if (!originRef || !markerRef) {
        return;
    }

    float gameX = RE::rad_to_deg(originRef->GetAngleX());  // pitch
    float gameZ = RE::rad_to_deg(originRef->GetAngleZ());  // yaw

    float angleX = RE::deg_to_rad(90.0f + gameX);
    float angleZ = 0.0f;

    if (gameZ < 90.0f)
        angleZ = RE::deg_to_rad(90.0f - gameZ);
    else
        angleZ = RE::deg_to_rad(450.0f - gameZ);

    float extraX = 0.0f;
    float extraY = 0.0f;
    float extraZ = 0.0f;

    auto camera = RE::PlayerCamera::GetSingleton();
    bool isFirstPerson = camera && camera->IsInFirstPerson();

    if (isFirstPerson) {
        extraZ = 10.0f;
        if (UseLeftRightOffsets) {
            extraX = isLeft ? -10.0f : 10.0f;
        }
    } else {
        extraZ = 40.0f;
        if (UseLeftRightOffsets) {
            extraX = isLeft ? -15.0f : 15.0f;
        }
    }

    float ArgumentMathSinX = sin(angleX);
    float ArgumentMathCosZ = cos(angleZ);
    float ArgumentMathSinZ = sin(angleZ);
    float ArgumentMathCosX = cos(angleX);

    float offsetX = fDistance * ArgumentMathSinX * ArgumentMathCosZ + extraX;
    float offsetY = fDistance * ArgumentMathSinX * ArgumentMathSinZ + extraY;
    float offsetZ = fDistance * ArgumentMathCosX + fHeight + extraZ;

    RE::NiPoint3 originPos = originRef->GetPosition();
    RE::NiPoint3 finalPos = {originPos.x + offsetX, originPos.y + offsetY, originPos.z + offsetZ};

    markerRef->MoveTo(originRef);
    markerRef->SetPosition(finalPos);
}

int MakeDiceRoll(RE::StaticFunctionTag*, int iNumberOfDice, int iNumberOfSides, int iModifier) {

    // Guard against invalid inputs
    if (iNumberOfDice <= 0 || iNumberOfSides < 1) 
        return iModifier;

    // Thread safety "static std::mt19937 is not thread-safe"
    thread_local std::mt19937 gen(std::random_device{}());

    std::uniform_int_distribution<> dist(1, iNumberOfSides);

    std::int64_t total = iModifier;

    for (int i = 0; i < iNumberOfDice; ++i) {
        total += dist(gen);
    }

    // Papyrus Int is 32-bit, so saturate rather than wrap on absurd inputs
    if (total > (std::numeric_limits<int>::max)()) {
        return (std::numeric_limits<int>::max)();
    }

    if (total < (std::numeric_limits<int>::min)()) {
        return (std::numeric_limits<int>::min)();
    }

    return static_cast<int>(total);
}

int GetFactionCrimeGoldValue(RE::StaticFunctionTag*, RE::TESFaction* CrimeFaction, int iCrimeType) {
    
    if (!CrimeFaction) {
        return 0;
    }

    switch (iCrimeType) {
        case 1:
            return CrimeFaction->crimeData.crimevalues.murderCrimeGold;

        case 2:
            return CrimeFaction->crimeData.crimevalues.assaultCrimeGold;

        case 3:
            return CrimeFaction->crimeData.crimevalues.trespassCrimeGold;

        case 4:
            return CrimeFaction->crimeData.crimevalues.stealCrimeGoldMult;

        case 5:
            return CrimeFaction->crimeData.crimevalues.escapeCrimeGold;

        case 6:
            return CrimeFaction->crimeData.crimevalues.werewolfCrimeGold;

        default:
            return 0;
    }
}

RE::TESObjectREFR* GetOpenedContainerRef(RE::StaticFunctionTag*) {
    auto handle = RE::ContainerMenu::GetTargetRefHandle();

    if (handle) {
        auto container = RE::TESObjectREFR::LookupByHandle(handle);

        if (container) {
            return container.get();
        }
    }

    return nullptr;
}

bool PapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("GetAndrealphusExtenderVersion", "ANDR_PapyrusFunctions", GetAndrealphusExtenderVersion);
    vm->RegisterFunction("CastEnchantment", "ANDR_PapyrusFunctions", CastEnchantment);
    vm->RegisterFunction("CastPotion", "ANDR_PapyrusFunctions", CastPotion);
    vm->RegisterFunction("CastIngredient", "ANDR_PapyrusFunctions", CastIngredient);
    vm->RegisterFunction("GetEffectiveEnchantmentCost", "ANDR_PapyrusFunctions", GetEffectiveEnchantmentCost);
    vm->RegisterFunction("GetEffectivePotionCost", "ANDR_PapyrusFunctions", GetEffectivePotionCost);
    vm->RegisterFunction("GetEffectiveIngredientCost", "ANDR_PapyrusFunctions", GetEffectiveIngredientCost);
    vm->RegisterFunction("GetEffectiveScrollCost", "ANDR_PapyrusFunctions", GetEffectiveScrollCost);
    vm->RegisterFunction("GetActiveMagicEffectFromActor", "ANDR_PapyrusFunctions", GetActiveMagicEffectFromActor);
    vm->RegisterFunction("SetRefAsNoAIAcquire", "ANDR_PapyrusFunctions", SetRefAsNoAIAcquire);
    vm->RegisterFunction("CastSpellFromRef", "ANDR_PapyrusFunctions", CastSpellFromRef);
    vm->RegisterFunction("CastSpellFromPointToPoint", "ANDR_PapyrusFunctions", CastSpellFromPointToPoint);
    vm->RegisterFunction("LaunchAmmo", "ANDR_PapyrusFunctions", LaunchAmmo);
    vm->RegisterFunction("LaunchMagicSpell", "ANDR_PapyrusFunctions", LaunchMagicSpell);
    vm->RegisterFunction("MoveRefToCrosshairLoc", "ANDR_PapyrusFunctions", MoveRefToCrosshairLoc);
    vm->RegisterFunction("MakeDiceRoll", "ANDR_PapyrusFunctions", MakeDiceRoll);
    vm->RegisterFunction("GetFactionCrimeGoldValue", "ANDR_PapyrusFunctions", GetFactionCrimeGoldValue);
    vm->RegisterFunction("GetOpenedContainerRef", "ANDR_PapyrusFunctions", GetOpenedContainerRef);

    /*Added by Ivy*/
    vm->RegisterFunction("GetCurrentBookPage", "ANDR_PapyrusFunctions", GetCurrentBookPage);
    vm->RegisterFunction("GetBookText", "ANDR_PapyrusFunctions", GetBookText);
    vm->RegisterFunction("GetOpenedBook", "ANDR_PapyrusFunctions", GetOpenedBook);
    vm->RegisterFunction("GetCurrentTopicInfo", "ANDR_PapyrusFunctions", GetCurrentTopicInfo);
    vm->RegisterFunction("ApplyCommandEffect", "ANDR_PapyrusFunctions", ApplyCommandEffect);
    vm->RegisterFunction("EndCommandEffect", "ANDR_PapyrusFunctions", EndCommandEffect);

    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    BookFullyRead::Install();  // Added by Ivy
    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    return true;
}
