#include <spdlog/sinks/basic_file_sink.h>
#include <warning.h>

#include "RE/A/Actor.h"
#include "RE/A/Array.h"
#include "RE/B/BGSPerk.h"
#include "RE/B/BGSPerkRankArray.h"
#include "RE/M/MagicItem.h"
#include "RE/N/NiPoint3.h"
#include "RE/O/Object.h"
#include "RE/T/TESSpellList.h"
#include "RE/V/VirtualMachine.h"
#include <random>

/*Added by Ivy*/

#include <atomic>
#include <chrono>
#include <thread>

#include "RE/B/BookMenu.h"
#include "RE/G/GFxMovieView.h"
#include "RE/G/GFxValue.h"
#include "RE/T/TESObjectBOOK.h"
#include "RE/U/UI.h"
#include "SKSE/API.h"
#include "SKSE/Events.h"

/*Added By Ivy - Ends*/

/*
namespace logger = SKSE::log;

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
}
*/
///// Actual new functions /////

// Papyrus: String Function GetAndrealphusExtenderVersion() Global Native
// Returns the version number of the mod.
RE::BSFixedString GetAndrealphusExtenderVersion(RE::StaticFunctionTag*) { return "1.7.1"; }

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

static std::atomic<float> g_bookFullyReadSeconds{5.0f};
static std::atomic<bool> g_bookFullyReadMonitorStarted{false};

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

static bool TryGetBool(RE::GFxMovieView* a_movie, const char* a_path, bool& a_out) {
    RE::GFxValue val;

    if (!a_movie) {
        return false;
    }

    if (!a_movie->GetVariable(&val, a_path)) {
        return false;
    }

    if (!val.IsBool()) {
        return false;
    }

    a_out = val.GetBool();
    return true;
}

static void StartBookFullyReadMonitor() {
    bool expected = false;

    if (!g_bookFullyReadMonitorStarted.compare_exchange_strong(expected, true)) {
        return;
    }

    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            auto task = []() {
                using clock = std::chrono::steady_clock;

                static bool firedThisOpen = false;
                static bool onLastPage = false;
                static clock::time_point enteredLastPage{};
                static clock::time_point lastAtEnd{};
                static int offEndTicks = 0;

                auto ui = RE::UI::GetSingleton();

                if (!ui || !ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
                    firedThisOpen = false;
                    onLastPage = false;
                    offEndTicks = 0;
                    return;
                }

                auto menu = ui->GetMenu<RE::BookMenu>();
                if (!menu) {
                    firedThisOpen = false;
                    onLastPage = false;
                    offEndTicks = 0;
                    return;
                }

                auto movie = menu->GetRuntimeData().book.get();
                if (!movie) {
                    firedThisOpen = false;
                    onLastPage = false;
                    offEndTicks = 0;
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

                if (firedThisOpen) {
                    return;
                }

                if (atEnd) {
                    offEndTicks = 0;

                    auto now = clock::now();
                    lastAtEnd = now;

                    if (!onLastPage) {
                        onLastPage = true;
                        enteredLastPage = now;
                        return;
                    }

                    float seconds = g_bookFullyReadSeconds.load();
                    if (seconds < 0.0f) {
                        seconds = 0.0f;
                    }

                    auto elapsed = std::chrono::duration<float>(now - enteredLastPage).count();

                    if (elapsed >= seconds) {
                        auto src = SKSE::GetModCallbackEventSource();
                        if (src) {
                            SKSE::ModCallbackEvent ev;
                            ev.eventName = "ANDR_OnBookFullyRead";
                            ev.strArg = "";
                            ev.numArg = static_cast<float>(leftPage);

                            RE::TESForm* sender = RE::BookMenu::GetTargetForm();
                            if (!sender) {
                                sender = RE::BookMenu::GetTargetReference();
                            }

                            ev.sender = sender;
                            src->SendEvent(&ev);
                        }

                        firedThisOpen = true;
                    }
                } else {
                    if (onLastPage) {
                        offEndTicks++;
                        if (offEndTicks >= 10) {
                            onLastPage = false;
                            offEndTicks = 0;
                        }
                    } else {
                        offEndTicks = 0;
                    }
                }
            };

            auto ti = SKSE::GetTaskInterface();
            if (ti) {
                ti->AddTask(task);
            }
        }
    }).detach();
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

    if (auto* ref = RE::BookMenu::GetTargetReference()) {
        if (auto* base = ref->GetBaseObject()) {
            if (auto* book = base->As<RE::TESObjectBOOK>()) {
                return book;
            }
        }
    }

    return nullptr;
}

// Papyrus: Function CastEnchantment(Actor akSource, Enchantment akEnchantment, Actor akTarget)
// Cast the akEnchantment from the akSource to the akTarget.
void CastEnchantment(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment,
                     RE::Actor* akTarget) {
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akEnchantment, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastPotion(Actor akSource, Potion akPotion, Actor akTarget)
// Cast the akPotion from the akSource to the akTarget.
void CastPotion(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion, RE::Actor* akTarget) {
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akPotion, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastIngredient(Actor akSource, Ingredient akIngredient, Actor akTarget)
// Cast the akIngredient from the akSource to the akTarget.
void CastIngredient(RE::StaticFunctionTag*, RE::Actor* akSource, RE::IngredientItem* akIngredient,
                    RE::Actor* akTarget) {
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akIngredient, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Float Function GetEffectiveEnchantmentCost(Actor akSource, Enchantment akEnchantment)
// Get the total effect cost of the enchantment.
float GetEffectiveEnchantmentCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment) {
    return akEnchantment->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectivePotionCost(Actor akSource, Potion akPotion)
// Get the total effect cost of the potion.
float GetEffectivePotionCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion) {
    return akPotion->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectiveIngredientCost(Actor akSource, Ingredient akIngredient)
// Get the total effect cost of the ingredient.
float GetEffectiveIngredientCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::IngredientItem* akIngredient) {
    return akIngredient->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectiveScrollCost(Actor akSource, Scroll akScroll)
// Get the total effect cost of the scroll.
float GetEffectiveScrollCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::ScrollItem* akScroll) {
    return akScroll->CalculateMagickaCost(akSource);
}

// ActiveMagicEffect Function GetActiveMagicEffectFromActor(Actor akActor, MagicEffect akMagicEffect) native global
// Get the instance of akMagicEffect on akActor
RE::ActiveEffect* GetActiveMagicEffectFromActor(RE::StaticFunctionTag*, RE::Actor* akActor,
                                                RE::EffectSetting* akMagicEffect) {
    if (akMagicEffect == nullptr) {
        //        logger::info("akMagicEffect is none.");
        return nullptr;
    }

    auto EffectsList = akActor->AsMagicTarget()->GetActiveEffectList();

    if (!EffectsList) {
        //        logger::info("Effects List is none.");
        return nullptr;
    }

    for (const auto& effect : *EffectsList) {
        const auto& setting = effect ? effect->GetBaseObject() : nullptr;

        if (setting) {
            //            logger::info("The effect we're checking is {}. The effect we're looking for is {}.",
            //            setting->formID,
            //                         akMagicEffect->formID);
            if (setting == akMagicEffect) {
                return effect;
            }
        } else {
            //            logger::info("The setting is nullptr");
        }
    }
    //    logger::info("Effect is none.");
    return nullptr;
}

void SetRefAsNoAIAcquire(RE::StaticFunctionTag*, RE::TESObjectREFR* akObject, bool SetNoAIAquire) {
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

typedef uint32_t (*cast_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                           const ProjectileRotCustom& rotcustom);
typedef uint32_t (*cast_CustomPos_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                                     const ProjectileRotCustom& rotcustom);

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
    auto NodePosition = akOriginRef->GetPosition();

    //	logger::info("Position: X is {}, Y is {}, Z is {}.", NodePosition.x, NodePosition.y, NodePosition.z);

    auto rotcustom = rot_at_custom(NodePosition, akTarget->GetPosition());

    //	logger::info("Rotation: X is {}, Z is {}.", rotcustom.x, rotcustom.z);

    auto eff = akSpell->GetCostliestEffectItem();

    auto mgef = getAVEffectSetting(akSpell);

    RE::Projectile::LaunchData ldata;

    ldata.origin = NodePosition;
    ldata.contactNormal = {0.0f, 0.0f, 0.0f};
    ldata.projectileBase = mgef->data.projectileBase;
    ldata.shooter = akSource;
    ldata.combatController = akSource->GetActorRuntimeData().combatController;
    ldata.weaponSource = nullptr;
    ldata.ammoSource = nullptr;
    ldata.angleZ = rotcustom.z;
    ldata.angleX = rotcustom.x;
    ldata.unk50 = nullptr;
    ldata.desiredTarget = nullptr;
    ldata.unk60 = 0.0f;
    ldata.unk64 = 0.0f;
    ldata.parentCell = akSource->GetParentCell();
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
}

void CastSpellFromPointToPoint(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, float StartPoint_X,
                               float StartPoint_Y, float StartPoint_Z, float EndPoint_X, float EndPoint_Y,
                               float EndPoint_Z) {
    RE::NiPoint3 NodePosition;

    NodePosition.x = StartPoint_X;
    NodePosition.y = StartPoint_Y;
    NodePosition.z = StartPoint_Z;

    //	 logger::info("NodePosition: X = {}, Y = {}, Z = {}.", NodePosition.x, NodePosition.y, NodePosition.z);

    RE::NiPoint3 DestinationPosition;

    DestinationPosition.x = EndPoint_X;
    DestinationPosition.y = EndPoint_Y;
    DestinationPosition.z = EndPoint_Z;

    //	 logger::info("DestinationPosition: X = {}, Y = {}, Z = {}.", DestinationPosition.x, DestinationPosition.y,
    //				  DestinationPosition.z);

    auto rotcustom = rot_at_custom(NodePosition, DestinationPosition);

    auto eff = akSpell->GetCostliestEffectItem();

    auto mgef = getAVEffectSetting(akSpell);

    RE::Projectile::LaunchData ldata;

    ldata.origin = NodePosition;
    ldata.contactNormal = {0.0f, 0.0f, 0.0f};
    ldata.projectileBase = mgef->data.projectileBase;
    ldata.shooter = akSource;
    ldata.combatController = akSource->GetActorRuntimeData().combatController;
    ldata.weaponSource = nullptr;
    ldata.ammoSource = nullptr;
    ldata.angleZ = rotcustom.z;
    ldata.angleX = rotcustom.x;
    ldata.unk50 = nullptr;
    ldata.desiredTarget = nullptr;
    ldata.unk60 = 0.0f;
    ldata.unk64 = 0.0f;
    ldata.parentCell = akSource->GetParentCell();
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
}

inline void LaunchAmmo(RE::StaticFunctionTag*, RE::Actor* a_actor, RE::TESAmmo* a_ammo, RE::TESObjectWEAP* a_weapon,
                       RE::BSFixedString a_nodeName, RE::TESObjectREFR* a_target, RE::BGSProjectile* a_projbase,
                       RE::TESObjectREFR* a_secondOrigin) {
    //   a_projbase needs to be assigned through Papyrus. Using a_ammo->data.projectile gives an invalid value. As does
    //   getting it through launchData.

    SKSE::GetTaskInterface()->AddTask([a_actor, a_ammo, a_weapon, a_nodeName, a_target, a_projbase, a_secondOrigin]() {
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

            RE::Actor* a_targetactor = skyrim_cast<RE::Actor*>(a_target);

            if (a_actor->IsPlayerRef()) {
                targetcoords = a_target->GetPosition();
            } else {
                if (a_targetactor) {
                    targetcoords = a_target->GetPosition();
                    float heightincrease = a_target->GetHeight() * 0.6;
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

            /*
                        logger::info("origin: x={}, y={}, z={}", origin.x, origin.y, origin.z);
                        logger::info("targetcoords: x={}, y={}, z={}", targetcoords.x, targetcoords.y, targetcoords.z);
                        logger::info("horizontalDist={}", horizontalDist);
                        logger::info("dx={}", dx);
                        logger::info("dy={}", dy);
                        logger::info("dz={}", dz);
                        logger::info("angleX={}", angles.x);
                        logger::info("angleZ={}", angles.z);
            */

        } else {
            origin = a_actor->GetPosition();
            origin.z += 96.0f;

            angles.x = a_actor->GetAimAngle();
            angles.z = a_actor->GetAimHeading();
        }

        RE::ProjectileHandle handle{};
        RE::Projectile::LaunchData launchData(a_actor, origin, angles, a_ammo, a_weapon);

        launchData.autoAim = false;
        launchData.projectileBase = a_projbase;

        RE::Projectile::Launch(&handle, launchData);
    });
}

inline void LaunchMagicSpell(RE::StaticFunctionTag*, RE::Actor* a_actor, RE::SpellItem* a_spell,
                             RE::BSFixedString a_nodeName, RE::TESObjectREFR* a_target, RE::BGSProjectile* a_projbase,
                             RE::TESObjectREFR* a_secondOrigin) {
    SKSE::GetTaskInterface()->AddTask([a_actor, a_spell, a_nodeName, a_target, a_projbase, a_secondOrigin]() {
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

            RE::Actor* a_targetactor = skyrim_cast<RE::Actor*>(a_target);

            if (a_actor->IsPlayerRef()) {
                targetcoords = a_target->GetPosition();
            } else {
                if (a_targetactor) {
                    targetcoords = a_target->GetPosition();
                    float heightincrease = a_target->GetHeight() * 0.6;
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

            /*
                        logger::info("origin: x={}, y={}, z={}", origin.x, origin.y, origin.z);
                        logger::info("targetcoords: x={}, y={}, z={}", targetcoords.x, targetcoords.y, targetcoords.z);
                        logger::info("horizontalDist={}", horizontalDist);
                        logger::info("dx={}", dx);
                        logger::info("dy={}", dy);
                        logger::info("dz={}", dz);
                        logger::info("angleX={}", angles.x);
                        logger::info("angleZ={}", angles.z);
            */
        } else {
            origin = a_actor->GetPosition();
            origin.z += 96.0f;

            angles.x = a_actor->GetAimAngle();
            angles.z = a_actor->GetAimHeading();
        }

        RE::ProjectileHandle handle{};
        RE::Projectile::LaunchData launchData(a_actor, origin, angles, a_spell);

        launchData.autoAim = false;
        launchData.projectileBase = a_projbase;

        RE::Projectile::Launch(&handle, launchData);
    });
}

void MoveRefToCrosshairLoc(RE::StaticFunctionTag*, RE::Actor* originRef, RE::TESObjectREFR* markerRef, float fDistance,
                           float fHeight, bool UseLeftRightOffsets, bool isLeft) {
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

    /*  logger::info("GameX = {}, GameZ = {}, AngleX = {}, AngleZ = {}", gameX, gameZ, angleX, angleZ);

        logger::info("TotalOffsetX: fDistance = {}, ArgumentMathSinX = {}, ArgumentMathCosZ = {}, ExtraOffsetX = {}",
       fDistance, ArgumentMathSinX, ArgumentMathCosZ, extraX); logger::info("TotalOffsetY: fDistance = {},
       ArgumentMathSinX = {}, ArgumentMathSinZ = {}, ExtraOffsetY = {}", fDistance, ArgumentMathSinX, ArgumentMathSinZ,
       extraY); logger::info("TotalOffsetZ: fDistance = {}, ArgumentMathCosX = {}, fHeight = {}, ExtraOffsetZ = {}",
       fDistance, ArgumentMathCosX, fHeight, extraZ);

        logger::info("finalPos: x = {}, y = {}, z = {}", finalPos.x, finalPos.y, finalPos.z);
    */

    markerRef->MoveTo(originRef);
    markerRef->SetPosition(finalPos);
}

int MakeDiceRoll(RE::StaticFunctionTag*, int iNumberOfDice, int iNumberOfSides, int iModifier) {

    // Guard against invalid inputs
    if (iNumberOfDice <= 0 || iNumberOfSides < 1) 
        return iModifier;

/*
    static std::random_device rd;
    static std::mt19937 gen(rd());
*/

    // Thread safety "static std::mt19937 is not thread-safe"
    thread_local std::mt19937 gen(std::random_device{}());

    std::uniform_int_distribution<> dist(1, iNumberOfSides);

    int total = 0;

    for (int i = 0; i < iNumberOfDice; ++i) {
        total += dist(gen);
    }

    return total + iModifier;
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

    /*Added by Ivy*/
    vm->RegisterFunction("GetCurrentBookPage", "ANDR_PapyrusFunctions", GetCurrentBookPage);
    vm->RegisterFunction("GetBookText", "ANDR_PapyrusFunctions", GetBookText);
    vm->RegisterFunction("GetOpenedBook", "ANDR_PapyrusFunctions", GetOpenedBook);


    /*  depreciated functions
        vm->RegisterFunction("CastSpellFromHand", "ANDR_PapyrusFunctions", CastSpellFromHand);
        vm->RegisterFunction("MoveRefToCrosshairLocation", "ANDR_PapyrusFunctions", MoveRefToCrosshairLocation);
        vm->RegisterFunction("AddSpellsToActor", "ANDR_PapyrusFunctions", AddSpellsToActor);
        vm->RegisterFunction("AddPerksToActor", "ANDR_PapyrusFunctions", AddPerksToActor);
        vm->RegisterFunction("DumpActorSpellsToFormList", "ANDR_PapyrusFunctions", DumpActorSpellsToFormList);
        vm->RegisterFunction("DumpActorPerksToFormList", "ANDR_PapyrusFunctions", DumpActorPerksToFormList);
    */
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    StartBookFullyReadMonitor();  // Added by Ivy
    //    SetupLog();
    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    return true;
}

/*
    Old depreciated code -> kept as backup and reference.


    // Math for this function seems to be different than through papyrus, as such it's done as a non-native function,
    that calls CastSpellFromPointToPoint() for now.
    // Papyrus: Function CastSpellFromHand(Actor akSource, Spell akSpell, Bool IsLeftHand, Float DistanceVar = 2000.0,
    Float HeightVar = 100.0,
    // Bool UseCustomObject = False, Form akObjectBase = None,
    // Float Offset_NoSneak_Left_X = 30.0, Float Offset_NoSneak_Left_Y = 30.0, Float Offset_NoSneak_Left_Z = 110.0,
    // Float Offset_NoSneak_Right_X = 30.0, Float Offset_NoSneak_Right_Y = -30.0, Float Offset_NoSneak_Right_Z = 110.0,
    // Float Offset_Sneak_Left_X = 30.0, Float Offset_Sneak_Left_Y = 30.0, Float Offset_Sneak_Left_Z = 70.0,
    // Float Offset_Sneak_Right_X = 30.0, Float Offset_Sneak_Right_Y = -30.0, Float Offset_Sneak_Right_Z = 70.0)
    void CastSpellFromHand(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, bool IsLeftHand,
                             float DistanceVar, float HeightVar, float Offset_NoSneak_Left_X, float
    Offset_NoSneak_Left_Y, float Offset_NoSneak_Left_Z, float Offset_NoSneak_Right_X, float Offset_NoSneak_Right_Y,
                             float Offset_NoSneak_Right_Z, float Offset_Sneak_Left_X, float Offset_Sneak_Left_Y,
                             float Offset_Sneak_Left_Z, float Offset_Sneak_Right_X, float Offset_Sneak_Right_Y,
                             float Offset_Sneak_Right_Z) {

          float GameX = akSource->GetAngle().x;
          float GameZ = akSource->GetAngle().z;
          float AngleX = 90.0 + GameX;
          float AngleZ;

          float SourceMarkerXOffset_Standard;
          float SourceMarkerYOffset_Standard;
          float SourceMarkerZOffset_Standard;

          if (GameZ < 90.0) {
              AngleZ = (90.0 - GameZ);
          } else {
              AngleZ = (450.0 - GameZ);
          }

        if (akSource->IsSneaking())
          {
                  if (IsLeftHand == true) {
                      SourceMarkerXOffset_Standard = Offset_Sneak_Left_X;
                      SourceMarkerYOffset_Standard = Offset_Sneak_Left_Y;
                      SourceMarkerZOffset_Standard = Offset_Sneak_Left_Z;
                  } else {
                      SourceMarkerXOffset_Standard = Offset_Sneak_Right_X;
                      SourceMarkerYOffset_Standard = Offset_Sneak_Right_Y;
                      SourceMarkerZOffset_Standard = Offset_Sneak_Right_Z;
                  }

          } else {
                  if (IsLeftHand == true) {
                          SourceMarkerXOffset_Standard = Offset_NoSneak_Left_X;
                          SourceMarkerYOffset_Standard = Offset_NoSneak_Left_Y;
                          SourceMarkerZOffset_Standard = Offset_NoSneak_Left_Z;
                  } else {
                          SourceMarkerXOffset_Standard = Offset_NoSneak_Right_X;
                          SourceMarkerYOffset_Standard = Offset_NoSneak_Right_Y;
                          SourceMarkerZOffset_Standard = Offset_NoSneak_Right_Z;
                  }
          }

          RE::NiPoint3 NodePosition;

          NodePosition.x = (akSource->GetPositionX() + (cos(AngleZ) * SourceMarkerXOffset_Standard - sin(AngleZ) *
          SourceMarkerYOffset_Standard));
          NodePosition.y = (akSource->GetPositionY() + (cos(AngleZ) * SourceMarkerYOffset_Standard + sin(AngleZ) *
          SourceMarkerXOffset_Standard));
          NodePosition.z = (akSource->GetPositionZ() + SourceMarkerZOffset_Standard);

          logger::info("NodePosition: X = {}, Y = {}, Z = {}.", NodePosition.x, NodePosition.y, NodePosition.z);

          RE::NiPoint3 DestinationPosition;

          DestinationPosition.x = (akSource->GetPositionX() + (DistanceVar * sin(AngleX) * cos(AngleZ)));
          DestinationPosition.y = (akSource->GetPositionY() + (DistanceVar * sin(AngleX) * sin(AngleZ)));
          DestinationPosition.z = (akSource->GetPositionZ() + (DistanceVar * cos(AngleX) + HeightVar));

          logger::info("DestinationPosition: X = {}, Y = {}, Z = {}.", DestinationPosition.x, DestinationPosition.y,
                       DestinationPosition.z);

          auto rot = rot_at(NodePosition, DestinationPosition);

          auto eff = akSpell->GetCostliestEffectItem();

          auto mgef = getAVEffectSetting(akSpell);

          RE::Projectile::LaunchData ldata;

          ldata.origin = NodePosition;
          ldata.contactNormal = {0.0f, 0.0f, 0.0f};
          ldata.projectileBase = mgef->data.projectileBase;
          ldata.shooter = akSource;
          ldata.combatController = akSource->GetActorRuntimeData().combatController;
          ldata.weaponSource = nullptr;
          ldata.ammoSource = nullptr;
          ldata.angleZ = rot.z;
          ldata.angleX = rot.x;
          ldata.unk50 = nullptr;
          ldata.desiredTarget = nullptr;
          ldata.unk60 = 0.0f;
          ldata.unk64 = 0.0f;
          ldata.parentCell = akSource->GetParentCell();
          ldata.spell = akSpell;
          ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
          ldata.unk7C = 0;
          ldata.enchantItem = nullptr;
          ldata.poison = nullptr;
          ldata.area = eff->GetArea();
          ldata.power = 1.0f;
          ldata.scale = 1.0f;
          ldata.alwaysHit = false;
          ldata.noDamageOutsideCombat = false;
          ldata.autoAim = false;
          ldata.unk9F = false;
          ldata.useOrigin = true;
          ldata.deferInitialization = false;
          ldata.forceConeOfFire = false;

          RE::BSPointerHandle<RE::Projectile> handle;
          RE::Projectile::Launch(&handle, ldata);
    }

    // This line is needed for CastSpellFromRef(), CastSpellFromHand() and CastSpellFromPointToPoint() to compile, might
    no longer be needed in the future? ---> This is a dtor.

    RE::Projectile::LaunchData::~LaunchData() {}


void MoveObjectWithOffset(RE::TESObjectREFR* ObjectToMove, RE::TESObjectREFR* TargetObject, const RE::NiPoint3&
localOffset)
{
// NOT a papyrus function -> just needed for MoveRefToCrosshairLocation()

    if (ObjectToMove && TargetObject) {

        const RE::NiPoint3& TargetObjectPos = TargetObject->GetPosition();
        float angleZ = TargetObject->GetAngleZ();  // yaw angle (radians)

        float cosYaw = std::cos(angleZ);
        float sinYaw = std::sin(angleZ);

        RE::NiPoint3 worldOffset;
        worldOffset.x = localOffset.x * cosYaw - localOffset.y * sinYaw;
        worldOffset.y = localOffset.x * sinYaw + localOffset.y * cosYaw;
        worldOffset.z = localOffset.z;

        RE::NiPoint3 newWorldPos = TargetObjectPos + worldOffset;

        ObjectToMove->SetPosition(newWorldPos);
    }
}

void MoveRefToCrosshairLocation(RE::StaticFunctionTag*, RE::TESObjectREFR* a_originactor,
                                RE::TESObjectREFR* a_targetref, float DistanceVar, float HeightVar) {

    a_targetref->MoveTo(a_originactor);
    RE::NiPoint3 origincoords = a_originactor->GetPosition();

    float GameX = a_originactor->GetAngleX();
    float GameZ = a_originactor->GetAngleZ();
    float AngleX = (90 + GameX);
    float AngleZ;

    if (GameZ < 90) {
        AngleZ = (90 - GameZ);
    } else {
        AngleZ = (450 - GameZ);
    }

    RE::NiPoint3 TargetPosition;

    TargetPosition.x = (origincoords.x + (DistanceVar * sin(AngleX) * cos(AngleZ)));
    TargetPosition.y = (origincoords.y + (DistanceVar * sin(AngleX) * sin(AngleZ)));
    TargetPosition.z = (origincoords.z + (DistanceVar * cos(AngleX) + HeightVar));

    auto* player = RE::PlayerCharacter::GetSingleton();
    MoveObjectWithOffset(a_targetref, player, TargetPosition);

}


void EndDialogue(RE::StaticFunctionTag*, RE::Actor* a_actor) {
    a_actor->EndDialogue();
}

RE::BGSDialogueBranch* GetCurrentDialogueBranch(RE::StaticFunctionTag*, RE::Actor* a_actor) {
    return a_actor->GetExclusiveBranch();
}


void AddSpellsToActor(RE::StaticFunctionTag*, RE::Actor* actorRef, RE::BSScript::VMArray<RE::SpellItem*> spells)
{
    if (!actorRef || !spells)
        return;

    for (uint32_t i = 0; i < spells->size(); ++i) {
        const auto& var = (*spells)[i];
        RE::BSScript::Object* obj = var.GetObject().get();

        if (obj && obj->IsValid()) {
            RE::TESForm* form = reinterpret_cast<RE::TESForm*>(obj);
            if (form) {
                auto* spell = form->As<RE::SpellItem>();
                if (spell) {
                    actorRef->AddSpell(spell);
                }
            }
        }
    }
}

void AddPerksToActor(RE::StaticFunctionTag*, RE::Actor* actorRef, RE::BSScript::Array* perks) {
    if (!actorRef || !perks)
        return;

    for (uint32_t i = 0; i < perks->size(); ++i) {
        const auto& var = (*perks)[i];
        RE::BSScript::Object* obj = var.GetObject().get();

        if (obj && obj->IsValid()) {
            RE::TESForm* form = reinterpret_cast<RE::TESForm*>(obj);
            if (form) {
                auto* perk = form->As<RE::BGSPerk>();
                if (perk) {
                    actorRef->AddPerk(perk);
                }
            }
        }
    }
}

void AddShoutsToActor(RE::StaticFunctionTag*, RE::Actor* actorRef, RE::BSScript::Array* shouts) {
    if (!actorRef || !shouts)
        return;

    for (uint32_t i = 0; i < shouts->size(); ++i) {
        const auto& var = (*shouts)[i];
        RE::BSScript::Object* obj = var.GetObject().get();

        if (obj && obj->IsValid()) {
            RE::TESForm* form = reinterpret_cast<RE::TESForm*>(obj);
            if (form) {
                auto* shout = form->As<RE::TESShout>();
                if (shout) {
                    actorRef->AddShout(shout);
                }
            }
        }
    }
}


void AddSpellsToActor(RE::StaticFunctionTag*, RE::Actor* actorRef, RE::BGSListForm* spellList)
{
    for (auto* form : spellList->forms) {
        if (auto* spell = form->As<RE::SpellItem>()) {
            actorRef->AddSpell(spell);
        }
    }
}

void AddPerksToActor(RE::StaticFunctionTag*, RE::Actor* actorRef, RE::BGSListForm* perkList)
{
    for (auto* form : perkList->forms) {
        if (auto* perk = form->As<RE::BGSPerk>()) {
            actorRef->AddPerk(perk);
        }
    }
}

void DumpActorSpellsToFormList(RE::StaticFunctionTag*, RE::Actor* actor, RE::BGSListForm* outList)
{
    if (!actor || !outList)
        return;

    outList->forms.clear();

    // From actor base (racial/class spells)
    if (auto* base = actor->GetActorBase()) {
        if (auto* npc = base->As<RE::TESNPC>()) {
            if (auto* spellData = npc->GetSpellList()) {
                for (std::uint32_t i = 0; i < spellData->numSpells; ++i) {
                    if (auto* spell = spellData->spells[i]; spell && !outList->HasForm(spell)) {
                        outList->forms.push_back(spell);
                    }
                }
            }
        }
    }

    // From runtime (learned/given spells)
    for (auto* spell : actor->GetActorRuntimeData().addedSpells) {
        if (spell && !outList->HasForm(spell)) {
            outList->forms.push_back(spell);
        }
    }
}

void DumpActorPerksToFormList(RE::StaticFunctionTag*, RE::Actor* actor, RE::BGSListForm* outList)
{
    if (!actor || !outList)
        return;

    outList->forms.clear();

    if (auto* base = actor->GetActorBase()) {
        auto* perkArray = static_cast<RE::BGSPerkRankArray*>(base);
        auto* perks = perkArray->perks;
        auto  count = perkArray->perkCount;

        for (std::uint32_t i = 0; i < count; ++i) {
            auto& entry = perks[i];
            if (entry.perk && !outList->HasForm(entry.perk)) {
                outList->forms.push_back(entry.perk);
            }
        }
    }
}
*/