#include <spdlog/sinks/basic_file_sink.h>
#include <warning.h>

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

////// Instructions from Fenix to get the position and rotation for CastSpellFromHand() function.

struct ProjectileRot {
    float x, z;
};

typedef uint32_t (*cast_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                           const ProjectileRot& rot);
typedef uint32_t (*cast_CustomPos_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                                     const ProjectileRot& rot);

RE::EffectSetting* getAVEffectSetting(RE::MagicItem* mgitem) {
    using func_t = decltype(getAVEffectSetting);
    REL::Relocation<func_t> func {REL::ID(11194)};
    return func(mgitem);
}

float SkyrimSE_c51f70(RE::NiPoint3* dir) {
    using func_t = decltype(SkyrimSE_c51f70);
    REL::Relocation<func_t> func{REL::ID(68820)};
    return func(dir);
}

auto rot_at(RE::NiPoint3 dir) {
    ProjectileRot rot;
    auto len = dir.Unitize();
    if (len == 0) {
        rot = {0, 0};
    } else {
        float polar_angle = SkyrimSE_c51f70(&dir);
        rot = {-asin(dir.z), polar_angle};
    }
    return rot;
}

auto rot_at(const RE::NiPoint3& from, const RE::NiPoint3& to) { return rot_at(to - from); }

///// Actual new functions /////

// Papyrus: Function CastEnchantment(Actor akSource, Enchantment akEnchantment, Actor akTarget)
// Cast the akEnchantment from the akSource to the akTarget.
void CastEnchantment(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment,
                      RE::Actor* akTarget) {   
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akEnchantment, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastPotion(Actor akSource, Potion akPotion, Actor akTarget)
// Cast the akPotion from the akSource to the akTarget.
void CastPotion(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion,
                      RE::Actor* akTarget) {
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
// Papyrus: Function CastSpellFromHand(Actor akSource, Spell akSpell, ObjectReference akTarget, int PositionInt) global native
// Cast a spell from the hand defined in PositionInt at the akTarget.
void CastSpellFromRef(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, RE::TESObjectREFR* akTarget,
                      RE::TESObjectREFR* akOriginRef) {
    
//    auto NodePosition = akSource
//                            ->GetMagicCaster(PositionInt == 0 ? RE::MagicSystem::CastingSource::kLeftHand
//                                                              : RE::MagicSystem::CastingSource::kRightHand)
//                            ->GetMagicNode()
//                            ->world.translate;
    auto NodePosition = akOriginRef->GetPosition();

    logger::info("Position: X is {}, Y is {}, Z is {}.", NodePosition.x, NodePosition.y, NodePosition.z);

    auto rot = rot_at(NodePosition, akTarget->GetPosition());

    logger::info("Rotation: X is {}, Z is {}.", rot.x, rot.z);

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
//    ldata.desiredTarget = akTarget;
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

// This line is needed for CastSpellFromHand() to compile, might no longer be needed in the future? ---> This is a dtor.

RE::Projectile::LaunchData::~LaunchData() {}

bool PapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("CastEnchantment", "ANDR_PapyrusFunctions", CastEnchantment);
    vm->RegisterFunction("CastPotion", "ANDR_PapyrusFunctions", CastPotion);
    vm->RegisterFunction("CastIngredient", "ANDR_PapyrusFunctions", CastIngredient);
    vm->RegisterFunction("CastSpellFromRef", "ANDR_PapyrusFunctions", CastSpellFromRef);
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();
    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    return true;
}
