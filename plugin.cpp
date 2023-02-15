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

// ============ Step 1 ============
// copied from FenixProjectilesAPI.h in https://github.com/fenix31415/MagicDeflectionAPI.
struct ProjectileRot {
    float x, z;
};

typedef uint32_t (*cast_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                           const ProjectileRot& rot);
typedef uint32_t (*cast_CustomPos_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                                     const ProjectileRot& rot);

// ============ Step 2 ============

static cast_CustomPos_t _API_cast_CustomPos_t;

uint32_t API_cast_CustomPos_t(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                              const ProjectileRot& rot) {
    return (*_API_cast_CustomPos_t)(caster, spel, start_pos, rot);
}

// ============ Step 3 ============
// This apparently needs another DLL?
// I'm not sure how this will work. AFAIK this references to typedefs in step 1.

namespace Subscribers {
    void init() {
        _API_cast_CustomPos_t = (cast_CustomPos_t)GetProcAddress(pluginHandle, "FenixProjectilesAPI_cast_CustomPos");
    }
}

// ============ Step 4 ============
static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            Subscribers::init();

            break;
    }
}

// ============ Step 7 ============
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

// This line is needed for CastSpellFromHand() to compile, might no longer be needed in the future? ---> This is a dtor.
// RE::Projectile::LaunchData::~LaunchData() {}

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
void CastSpellFromHand(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, RE::TESObjectREFR* akTarget,
                       std::int32_t PositionInt, RE::BGSProjectile* akProjectile) {
    
    auto NodePosition = akSource
                            ->GetMagicCaster(PositionInt == 0 ? RE::MagicSystem::CastingSource::kLeftHand
                                                              : RE::MagicSystem::CastingSource::kRightHand)
                            ->GetMagicNode()
                            ->world.translate;

    auto rot = rot_at(NodePosition);

    API_cast_CustomPos_t(akSource, akSpell, NodePosition, rot);

//    old code, kept as backup.

//    RE::Projectile::LaunchData ldata;

//    ldata.origin = NodePosition;
//    ldata.shooter = akSource;
//    ldata.desiredTarget = akTarget;
//    ldata.spell = akSpell;
//    ldata.castingSource =
//        PositionInt == 0 ? RE::MagicSystem::CastingSource::kLeftHand : RE::MagicSystem::CastingSource::kRightHand;
//    ldata.projectileBase = akProjectile;
// 
// 
//   remove all nullptr's.
// 
//   ldata.contactNormal = nullptr; // doesn't like nullptr
//    ldata.combatController = nullptr;
//    ldata.weaponSource = nullptr;
//    ldata.ammoSource = nullptr; //Might be useful to experiment with ammo...
//    ldata.unk50 = nullptr;
//    ldata.unk60 = 0.f;
//    ldata.unk64 = 0.f;
//    ldata.enchantItem = nullptr;
//    ldata.poison = nullptr;
//    ldata.area = 0;
//    ldata.power = 1.f;
//    ldata.scale = 1.f;
//    ldata.alwaysHit = false;
//    ldata.noDamageOutsideCombat = false;
//    ldata.autoAim = false;
//    ldata.unk9F = false;
//    ldata.useOrigin = true;
//    ldata.deferInitialization = false;
//    ldata.forceConeOfFire = false;
    
//     RE::BSPointerHandle<RE::Projectile> handle;
//     RE::Projectile::Launch(&handle, ldata);
}

bool PapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("CastEnchantment", "ANDR_PapyrusFunctions", CastEnchantment);
    vm->RegisterFunction("CastPotion", "ANDR_PapyrusFunctions", CastPotion);
    vm->RegisterFunction("CastIngredient", "ANDR_PapyrusFunctions", CastIngredient);
    vm->RegisterFunction("CastSpellFromHand", "ANDR_PapyrusFunctions", CastSpellFromHand);
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();
    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    return true;
}