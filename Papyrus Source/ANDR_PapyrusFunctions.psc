scriptName ANDR_PapyrusFunctions hidden

Import Math

; ============================= NATIVE FUNCTIONS =============================

Int Function GetAndrealphusExtenderVersion() Global Native
{
- Return value: an integer string that's the version number of this mod. (without the dots, so for instance 1.4.1 -> 141)
}

Function CastEnchantment(Actor akSource, Enchantment akEnchantment, Actor akTarget) global native
{ 
- akSource: The Actor from which to cast the Enchantment.
- akEnchantment: Enchantment to cast.
- akTarget: Actor at which to aim the Enchantment.
}

Function CastPotion(Actor akSource, Potion akPotion, Actor akTarget) global native
{ 
- akSource: The Actor from which to cast the Potion.
- akPotion: Potion to cast.
- akTarget: Actor at which to aim the Potion.
}

Function CastIngredient(Actor akSource, Ingredient akIngedient, Actor akTarget) global native
{ 
- akSource: The Actor from which to cast the Ingredient.
- akIngedient: Ingredient to cast.
- akTarget: Actor at which to aim the Ingredient.
}

Float Function GetEffectiveEnchantmentCost(Actor akSource, Enchantment akEnchantment) global native
{
- Return value: the total effective cost of all the enchantment's effects.
- akSource: the user of the enchantment.
- akEnchantment: the enchantment.
}

Float Function GetEffectivePotionCost(Actor akSource, Potion akPotion) global native
{
- Return value: the total effective cost of all the potion's effects.
- akSource: the user of the potion.
- akPotion: the potion.
}

Float Function GetEffectiveIngredientCost(Actor akSource, Ingredient akIngredient) global native
{
- Return value: the total effective cost of all the ingredient's effects.
- akSource: the user of the ingredient.
- akIngredient: the ingredient.
}

Float Function GetEffectiveScrollCost(Actor akSource, Scroll akScroll) global native
{
- Return value: the total effective cost of all the scroll's effects.
- akSource: the user of the scroll.
- akScroll: the scroll.
}

ActiveMagicEffect Function GetActiveMagicEffectFromActor(Actor akActor, MagicEffect akMagicEffect) global native
{
- akActor: the actor to check.
- akMagicEffect: the base magic to look for.
- Returns: the instance (ActiveMagicEffect) of akMagicEffect on the akActor.
}

Function SetRefAsNoAIAcquire(ObjectReference akObject, Bool SetNoAIAquire) global native
{
- akObject: the objectreference
- SetNoAIAquire: to disable or enable SetNoAIAquire field. 
}

Function CastSpellFromRef(Actor akSource, Spell akSpell, ObjectReference akTarget, ObjectReference akOriginRef) global native
{ 
- akSource: The caster of the spell.
- akSpell: Spell to cast.
- akTarget: An ObjectReference at which to aim the spell.
- akOriginRef: The ObjectReference where to cast the spell from.
}

Function CastSpellFromPointToPoint(Actor akSource, Spell akSpell, Float StartPoint_X, Float StartPoint_Y, Float StartPoint_Z, Float EndPoint_X, Float EndPoint_Y, Float EndPoint_Z) native global
{
- akSource: The caster.
- akSpell: The spell to cast.
- StartPoint_X: The X position of the starting point.
- StartPoint_Y: The Y position of the starting point.
- StartPoint_Z: The Z position of the starting point.
- EndPoint_X: The X position of the ending point.
- EndPoint_Y: The Y position of the ending point.
- EndPoint_Z: The Z position of the ending point.
}


Function LaunchAmmo(Actor akCaster, Ammo akAmmo, Weapon akWeapon, String NodeSource = "", Int iSlot = -1, ObjectReference akTarget = None, Potion akPoison = None, Projectile akProjectile) Global Native
{
; based off of fenix31415's and po3's launcharrow function
- akCaster: the actor "casting" the ammo.
- akWeapon: the weapon that's being used.
- NodeSource: the name of the skeleton bone node of the akCaster, the ammo is launch from.
- iSlot:
	-1 	Uses NodeSource or default weapon node if empty
	0 	Left hand node
	1 	Right hand node
	2 	Head node
	; iSlot might be buggy though, best practice is to use an actual node source
- akTarget: the target of the ammo.	(not sure if this is needed.)
- akPoison: the poison being applied. (might be buggy)
- akProjectile: the base projectile.	
}

; ============================= NON-NATIVE FUNCTIONS =============================

Function CastSpellFromHand(Actor akSource, Spell akSpell, Bool IsLeftHand, Float DistanceVar = 2000.0, Float HeightVar = 100.0, Float Offset_NoSneak_Left_X = 30.0, Float Offset_NoSneak_Left_Y = 30.0, Float Offset_NoSneak_Left_Z = 110.0, Float Offset_NoSneak_Right_X = 30.0, Float Offset_NoSneak_Right_Y = -30.0, Float Offset_NoSneak_Right_Z = 110.0, Float Offset_Sneak_Left_X = 30.0, Float Offset_Sneak_Left_Y = 30.0, Float Offset_Sneak_Left_Z = 70.0, Float Offset_Sneak_Right_X = 30.0, Float Offset_Sneak_Right_Y = -30.0, Float Offset_Sneak_Right_Z = 70.0)	global
	{
	- akSource: The caster.
	- akSpell: The spell to cast.
	- IsLeftHand: True if cast from the left hand, false if cast from the right hand.
	- DistanceVar: Optional, the distance from the caster, where the destination marker is spawned. Default is 2000 units.
	- HeightVar: Optional, the height difference at which destination marker is spawned. Default is 100 units.
	- Offset_NoSneak_Left_X: Optional, X Offset for left hand when the actor is not sneaking. Default value is 30.0.
	- Offset_NoSneak_Left_Y: Optional, Y Offset for left hand when the actor is not sneaking. Default value is 30.0.
	- Offset_NoSneak_Left_Z: Optional, Z Offset for left hand when the actor is not sneaking. Default value is 110.0.
	- Offset_NoSneak_Right_X: Optional, X Offset for right hand when the actor is not sneaking. Default value is 30.0.
	- Offset_NoSneak_Right_Y: Optional, Y Offset for right hand when the actor is not sneaking. Default value is -30.0.
	- Offset_NoSneak_Right_Z: Optional, Z Offset for right hand when the actor is not sneaking. Default value is 110.0.
	- Offset_Sneak_Left_X: Optional, X Offset for left hand when the actor is sneaking. Default value is 30.0.
	- Offset_Sneak_Left_Y: Optional, Y Offset for left hand when the actor is sneaking. Default value is 30.0.
	- Offset_Sneak_Left_Z: Optional, Z Offset for left hand when the actor is sneaking. Default value is 70.0.
	- Offset_Sneak_Right_X: Optional, X Offset for right hand when the actor is sneaking. Default value is 30.0.
	- Offset_Sneak_Right_Y: Optional, Y Offset for right hand when the actor is sneaking. Default value is -30.0.
	- Offset_Sneak_Right_Z: Optional, Z Offset for right hand when the actor is sneaking. Default value is 70.0.
	}
		; ======= Example of use =======
		; CastSpellFromHand(YsoldaRef, IceSpike, true)
		; 
		; Have Ysolda cast an Ice Spike spell from her left hand.
		; ==============================
	
		Float GameX = akSource.GetAngleX()
		Float GameZ = akSource.GetAngleZ()
		Float AngleX = 90 + GameX  
		Float AngleZ
	
		Float SourceMarkerXOffset_Standard
		Float SourceMarkerYOffset_Standard
		Float SourceMarkerZOffset_Standard
	
		If GameZ < 90 
			AngleZ = 90 - GameZ
		Else
			AngleZ = 450 - GameZ
		EndIf
		
		If !akSource.IsSneaking()
			If 	IsLeftHand
				SourceMarkerXOffset_Standard = Offset_NoSneak_Left_X
				SourceMarkerYOffset_Standard = Offset_NoSneak_Left_Y
				SourceMarkerZOffset_Standard = Offset_NoSneak_Left_Z
			Else
				SourceMarkerXOffset_Standard = Offset_NoSneak_Right_X
				SourceMarkerYOffset_Standard = Offset_NoSneak_Right_Y
				SourceMarkerZOffset_Standard = Offset_NoSneak_Right_Z
			EndIf
		Else
			If 	IsLeftHand
				SourceMarkerXOffset_Standard = Offset_Sneak_Left_X
				SourceMarkerYOffset_Standard = Offset_Sneak_Left_Y
				SourceMarkerZOffset_Standard = Offset_Sneak_Left_Z
			Else
				SourceMarkerXOffset_Standard = Offset_Sneak_Right_X
				SourceMarkerYOffset_Standard = Offset_Sneak_Right_Y
				SourceMarkerZOffset_Standard = Offset_Sneak_Right_Z
			EndIf
		EndIf
			
		Float SourcePosX = akSource.GetPositionX()
		Float SourcePosY = akSource.GetPositionY()
		Float SourcePosZ = akSource.GetPositionZ()

		CastSpellFromPointToPoint(akSource, akSpell, (SourcePosX + (cos(AngleZ)*SourceMarkerXOffset_Standard - sin(AngleZ)*SourceMarkerYOffset_Standard)), (SourcePosY + (cos(AngleZ)*SourceMarkerYOffset_Standard + sin(AngleZ)*SourceMarkerXOffset_Standard)), (SourcePosZ + SourceMarkerZOffset_Standard), (SourcePosX + (DistanceVar * Math.Sin(AngleX) * Math.Cos(AngleZ))), (SourcePosY + (DistanceVar * Math.Sin(AngleX) * Math.Sin(AngleZ))), (SourcePosZ + (DistanceVar * Math.Cos(AngleX) + HeightVar)))

EndFunction

Function CastSpellFromRefAimed(Actor akSource, Spell akSpell, ObjectReference akOriginRef) global
	{ 
	- akSource: The caster of the spell.
	- akSpell: Spell to cast.
	- akOriginRef: The ObjectReference where to cast the spell from.
	}
	; This can be used to have an actor cast a spell from a ref to whatever the ref is pointing.

	Float OriginPosX = akOriginRef.GetPositionX()
	Float OriginPosY = akOriginRef.GetPositionY()
	Float OriginPosZ = akOriginRef.GetPositionZ()

	Float DistanceVar = 2000.0
	Float HeightVar = 100.0

	Float GameX = akOriginRef.GetAngleX()
	Float GameZ = akOriginRef.GetAngleZ()
	Float AngleX = 90 + GameX  
	Float AngleZ

	If GameZ < 90 
		AngleZ = 90 - GameZ
	Else
		AngleZ = 450 - GameZ
	EndIf
	
	CastSpellFromPointToPoint(akSource, akSpell, (OriginPosX + (cos(AngleZ) - sin(AngleZ))), (OriginPosY + (cos(AngleZ) + sin(AngleZ))), OriginPosZ, (OriginPosX + (DistanceVar * Math.Sin(AngleX) * Math.Cos(AngleZ))), (OriginPosY + (DistanceVar * Math.Sin(AngleX) * Math.Sin(AngleZ))), (OriginPosZ + (DistanceVar * Math.Cos(AngleX) + HeightVar)))
;	CastSpellFromPointToPoint(akSource, akSpell, OriginPosX, OriginPosY, OriginPosZ, (OriginPosX + (DistanceVar * Math.Sin(AngleX) * Math.Cos(AngleZ))), (OriginPosY + (DistanceVar * Math.Sin(AngleX) * Math.Sin(AngleZ))), (OriginPosZ + (DistanceVar * Math.Cos(AngleX) + HeightVar)))
;	CastSpellFromPointToPoint(akSource, akSpell, (OriginPosX + (cos(AngleZ)*30.0)), (OriginPosY + (sin(AngleZ)*30.0)), (OriginPosZ + 110.0), (OriginPosX + (DistanceVar * Math.Sin(AngleX) * Math.Cos(AngleZ))), (OriginPosY + (DistanceVar * Math.Sin(AngleX) * Math.Sin(AngleZ))), (OriginPosZ + (DistanceVar * Math.Cos(AngleX) + HeightVar)))

EndFunction

; ============================= WIP Functions =============================

;/
Function SetObjectRefFlag(ObjectReference akObject, Int FlagInt, Bool TurnOn)
{
- akObject: the objectreference
- FlagInt: the Int representing the flag.
	0	kIsGroundPiece 
	1	kCollisionsDisabled -> unknown?
	2	kDeleted 
	3	kHiddenFromLocalMap -> only for statics!
	4	kTurnOffFire 
	5	kInaccessible -> only for doors!
	6	kLODRespectsEnableState  -> only for statics!
	7	kStartsDead  -> only for actors!
	8	kDoesntLightWater 
	9	kMotionBlur  -> only for statics!
	10	kPersistent 
	11	kInitiallyDisabled 
	12	kIgnored 
	13	kStartUnconscious  -> only for actors!
	14	kSkyMarker 
	15	kHarvested   -> only for trees!
	16	kIsFullLOD   -> only for actors!
	17	kNeverFades   -> only for lights!
	18	kDoesntLightLandscape 
	19	kIgnoreFriendlyHits   -> only for actors!
	20	kNoAIAcquire 
	21	kCollisionGeometry_Filter 
	22	kCollisionGeometry_BoundingBox 
	23	kReflectedByAutoWater 
	24	kDontHavokSettle 
	25	kGround 
	26	kRespawns 
	27	kMultibound 
 - TurnOn: To turn the flag on or off.
}
/;


;/
Function RegisterForCollision(ObjectReference akObject) global native

Used for Registering whenever an object physically colides with an actor.
akObject: the object we are monitoring
/;

;/
Function OnObjectCollision(ObjectReference akObject, ObjectReference akTarget) global native

This is based of OnTrapHitStart(), but that function is objectreference function and needs to have one of its collision layers in the mesh to be set as L_TRAP.
akObject: The object that's colliding with something or someone.
akTarget: The object or actor being hit.
/;

;/
Function SetActorLevel(actor akActor, Int iLevel)
{Requires ConsoleUtil. Sets level of an actor.}
	ConsoleUtil.SetSelectedReference(akActor)
	ConsoleUtil.ExecuteCommand("setlevel " + iLevel + "")
EndFuntion
/;


; wish list...

;/
InfoTopic Function GetCurrentDialogueTopic()  global native
{Returns the InfoTopic that is currently being said by the NPC that's in the dialogue menu with the player.}
/;

;/
Bool Function IsPlayerControlEnabled(Int iControlType)
{Returns whether or not a speific player control is enabled
iControlType can be:
0: abMovement: player's movement controls. 
1: abFighting: player's combat controls. 
2: abCamSwitch: ability to switch point of view. 
3: abLooking: player's look controls. 
4: abSneaking: player's sneak controls. 
5: abMenu: menu controls (Journal, Inventory, Pause, etc.). 
6: abActivate: ability for player to activate objects. 
7 abJournalTabs: all Journal tabs except System. 
}
/;

;/
Function SetPlayerControl(Bool bEnable, Int iControlType)
{Disables the specified type of player controls.
- bEnable: true to enable a control, false to disable it.
- iControlType can be:
0: abMovement: player's movement controls. 
1: abFighting: player's combat controls. 
2: abCamSwitch: ability to switch point of view. 
3: abLooking: player's look controls. 
4: abSneaking: player's sneak controls. 
5: abMenu: menu controls (Journal, Inventory, Pause, etc.). 
6: abActivate: ability for player to activate objects. 
7 abJournalTabs: all Journal tabs except System. 
}
/;

;/ 
Function ReplaceBookText(Book akBook, String sOriginalText, String sNewText)
{Replaces text in a book.
- akBook: the book to edit.
- sOriginalText: the original string to replace.
- sNewtext: the new string to replace the original string with.
}
/;

;/
Function SetObjectAsFavorite(Form akObject, Int iHotkey = 0)	
{Sets an akObject as favorite, with akHotkey as possible hotkey. 
- iHotKey: 0 means no hotkey (default value) and 1-9 will be the requested hotkey. -1 will mean no longer favorited.
;Example: SetObjectAsFavorite(IronSword, 3) -> Set an IronSword as favorite and assign to hotkey 3.
}
/;

;/
Int Function GetHotKeyFavoriteObject(Form akForm)	;Gets the hotkey of item as integer. -1 means not favorited, 0 means favorited but no hotkey and 1-9 will be the hotkey.
	;Example:
	;If GetHotKeyFavoriteObject(IronSword) == 3
	;debug.notification("An IronSword is currently bound to 3.")
	;EndIf
/;

;/
Form Function GetObjectFavoriteHotkey(Int iHotkey)	;Gets the object currently bound to the hotkey. Returns a none if nothing is bound to the key.
	;If GetObjectFavoriteHotkey(3) == IronSword
	;debug.notification("Hotkey 3 is assigned to an IronSword.")
	;EndIf
/;	

;/
Function MarkInventoryAsOwned(ObjectReference akInventory, Actor akOwnerActor = None, Faction akOwnerfaction = None )	;Marks all items in the akInventory (container or actor ref) as owned by akOwner (either an actor or faction).
	If akInventory is an actor this should include outfit, death item and inventory.
	If both akOwnerActor and akOwnerfaction are none -> remove ownership
/;

;/
Function SetControlledActor(Actor akActor)	;akActor will be fully controllable, with them being able to move through load-doors, fully able to use all animations, and able to see their inventory, spells and perks.
	;Less than 0.5 second script load should be ideal, to make swapping control in combat viable.
	;note: It's unlikely this one can be done easily. I've attempted it, but couldn't get it to work.
/;	

;/
Function SetPlayerCombatAIDriven()	
; same as SetPlayerAIDriven, but for combat AI https://ck.uesp.net/wiki/SetPlayerAIDriven_-_Game
/;	

;/
Actor Function GetCommandingActor(Actor akActor)
; gets the commanding actor of this actor.
/;

;/
Actor[] Function GetCommandedActors(Actor akActor)
; gets all commanded actors of this actor.
/;

;/
Function SetCommandingActor(Actor CommandedActor, Actor CommandingActor = None)
; sets makes actors commanded and commanding of each other.
; if CommandingActor is None, clears the commanded actor from being commanded.
/;