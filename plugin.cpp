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
    
    RE::Projectile::LaunchData ldata;

    auto NodePosition = akSource
                            ->GetMagicCaster(PositionInt == 0 ? RE::MagicSystem::CastingSource::kLeftHand
                                                              : RE::MagicSystem::CastingSource::kRightHand)
                            ->GetMagicNode()
                            ->world.translate;

    auto rot = rot_at(NodePosition, akTarget->GetPosition());

    ldata.origin = NodePosition;
    ldata.contactNormal = {0.0f, 0.0f, 0.0f};
    ldata.projectileBase = akProjectile;
    ldata.shooter = akSource;
    ldata.combatController = akSource->GetActorRuntimeData().combatController;
    ldata.weaponSource = nullptr;
    ldata.ammoSource = nullptr;
    ldata.angleZ = rot.z;      // this needs the z value of Projectile rot, afaik. Not sure if the syntax is correct.
    ldata.angleX = rot.x;      // this needs the x value of Projectile rot, afaik. Not sure if the syntax is correct.
    ldata.unk50 = nullptr;
    ldata.desiredTarget = akTarget;
    ldata.unk60 = 0.0f;
    ldata.unk64 = 0.0f;
    ldata.parentCell = akSource->GetParentCell();
    ldata.spell = akSpell;
    ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
    ldata.unk7C = 0;                                   
    ldata.enchantItem = nullptr;
    ldata.poison = nullptr;
    ldata.area = 0;
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

// With the dtor from line 51 commented out, I get this error when trying to build. How can I fix this?
// 
//>------ Build All started: Project: SKSE_Template_HelloWorld, Configuration: debug ------
//  [1/2] Building CXX object CMakeFiles\ANDR_PapyrusFunctions.dir\plugin.cpp.obj
//  [2/2] Linking CXX shared library ANDR_PapyrusFunctions.dll
//  FAILED: ANDR_PapyrusFunctions.dll ANDR_PapyrusFunctions.lib 
//  cmd.exe /C "cd . && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -E vs_link_dll --intdir=CMakeFiles\ANDR_PapyrusFunctions.dir --rc=C:\PROGRA~2\WI3CF2~1\10\bin\100220~1.0\x64\rc.exe --mt=C:\PROGRA~2\WI3CF2~1\10\bin\100220~1.0\x64\mt.exe --manifests  -- C:\PROGRA~1\MIB055~1\2022\COMMUN~1\VC\Tools\MSVC\1434~1.319\bin\Hostx64\x64\link.exe /nologo CMakeFiles\ANDR_PapyrusFunctions.dir\cmake_pch.cxx.obj CMakeFiles\ANDR_PapyrusFunctions.dir\plugin.cpp.obj CMakeFiles\ANDR_PapyrusFunctions.dir\__ANDR_PapyrusFunctionsPlugin.cpp.obj  /out:ANDR_PapyrusFunctions.dll /implib:ANDR_PapyrusFunctions.lib /pdb:ANDR_PapyrusFunctions.pdb /dll /version:0.0 /machine:x64 /debug /INCREMENTAL  vcpkg_installed\x64-windows-static-md\debug\lib\CommonLibSSE.lib  vcpkg_installed\x64-windows-static-md\debug\lib\spdlogd.lib  vcpkg_installed\x64-windows-static-md\debug\lib\fmtd.lib  Version.lib  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib  && cmd.exe /C "cd /D "H:\Github Code\SKSE_Template_HelloWorld\build\debug" && C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -noprofile -executionpolicy Bypass -file "H:/Github Code/vcpkg/scripts/buildsystems/msbuild/applocal.ps1" -targetBinary "H:/Github Code/SKSE_Template_HelloWorld/build/debug/ANDR_PapyrusFunctions.dll" -installedDir "H:/Github Code/SKSE_Template_HelloWorld/build/debug/vcpkg_installed/x64-windows-static-md/debug/bin" -OutVariable out && cd /D "H:\Github Code\SKSE_Template_HelloWorld\build\debug" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -E make_directory "G:\SSE Mods\SSE MO\Base Directory\mods/ANDR_PapyrusFunctions/SKSE/Plugins" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -E copy_if_different "H:/Github Code/SKSE_Template_HelloWorld/build/debug/ANDR_PapyrusFunctions.dll" "G:\SSE Mods\SSE MO\Base Directory\mods/ANDR_PapyrusFunctions/SKSE/Plugins/ANDR_PapyrusFunctions.dll" && cd /D "H:\Github Code\SKSE_Template_HelloWorld\build\debug" && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -E copy_if_different "H:/Github Code/SKSE_Template_HelloWorld/build/debug/ANDR_PapyrusFunctions.pdb" "G:\SSE Mods\SSE MO\Base Directory\mods/ANDR_PapyrusFunctions/SKSE/Plugins/ANDR_PapyrusFunctions.pdb"""
//  LINK Pass 1: command "C:\PROGRA~1\MIB055~1\2022\COMMUN~1\VC\Tools\MSVC\1434~1.319\bin\Hostx64\x64\link.exe /nologo CMakeFiles\ANDR_PapyrusFunctions.dir\cmake_pch.cxx.obj CMakeFiles\ANDR_PapyrusFunctions.dir\plugin.cpp.obj CMakeFiles\ANDR_PapyrusFunctions.dir\__ANDR_PapyrusFunctionsPlugin.cpp.obj /out:ANDR_PapyrusFunctions.dll /implib:ANDR_PapyrusFunctions.lib /pdb:ANDR_PapyrusFunctions.pdb /dll /version:0.0 /machine:x64 /debug /INCREMENTAL vcpkg_installed\x64-windows-static-md\debug\lib\CommonLibSSE.lib vcpkg_installed\x64-windows-static-md\debug\lib\spdlogd.lib vcpkg_installed\x64-windows-static-md\debug\lib\fmtd.lib Version.lib kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib /MANIFEST /MANIFESTFILE:CMakeFiles\ANDR_PapyrusFunctions.dir/intermediate.manifest CMakeFiles\ANDR_PapyrusFunctions.dir/manifest.res" failed (exit code 1120) with the following output:
//     Creating library ANDR_PapyrusFunctions.lib and object ANDR_PapyrusFunctions.exp
// H:\Github Code\SKSE_Template_HelloWorld\build\debug\plugin.cpp.obj : error LNK2019: unresolved external symbol "public: virtual __cdecl RE::Projectile::LaunchData::~LaunchData(void)" (??1LaunchData@Projectile@RE@@UEAA@XZ) referenced in function "void __cdecl CastSpellFromHand(struct RE::StaticFunctionTag *,class RE::Actor *,class RE::SpellItem *,class RE::TESObjectREFR *,int,class RE::BGSProjectile *)" (?CastSpellFromHand@@YAXPEAUStaticFunctionTag@RE@@PEAVActor@2@PEAVSpellItem@2@PEAVTESObjectREFR@2@HPEAVBGSProjectile@2@@Z)
//  
// H:\Github Code\SKSE_Template_HelloWorld\build\debug\ANDR_PapyrusFunctions.dll : fatal error LNK1120: 1 unresolved externals
//  
//  ninja: build stopped: subcommand failed.
//
// Build All failed.