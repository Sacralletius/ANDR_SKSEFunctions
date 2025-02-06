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

	Float GameX = akSource.GetAngleX()
	Float GameZ = akSource.GetAngleZ()
	Float AngleX = 90 + GameX  
	Float AngleZ

	If GameZ < 90 
		AngleZ = 90 - GameZ
	Else
		AngleZ = 450 - GameZ
	EndIf
	
	CastSpellFromPointToPoint(akSource, akSpell, (OriginPosX + (cos(AngleZ) - sin(AngleZ))), (OriginPosY + (cos(AngleZ) + sin(AngleZ))), OriginPosZ, (OriginPosX + (DistanceVar * Math.Sin(AngleX) * Math.Cos(AngleZ))), (OriginPosY + (DistanceVar * Math.Sin(AngleX) * Math.Sin(AngleZ))), (OriginPosZ + (DistanceVar * Math.Cos(AngleX) + HeightVar)))
EndFunction

; ============================= WIP Functions =============================

;Function SetObjectRefFlag(ObjectReference akObject, Int FlagInt, Bool TurnOn)
;{
;- akObject: the objectreference
;- FlagInt: the Int representing the flag.
	;0	kIsGroundPiece 
	;1	kCollisionsDisabled -> unknown?
	;2	kDeleted 
	;3	kHiddenFromLocalMap -> only for statics!
	;4	kTurnOffFire 
	;5	kInaccessible -> only for doors!
	;6	kLODRespectsEnableState  -> only for statics!
	;7	kStartsDead  -> only for actors!
	;8	kDoesntLightWater 
	;9	kMotionBlur  -> only for statics!
	;10	kPersistent 
	;11	kInitiallyDisabled 
	;12	kIgnored 
	;13	kStartUnconscious  -> only for actors!
	;14	kSkyMarker 
	;15	kHarvested   -> only for trees!
	;16	kIsFullLOD   -> only for actors!
	;17	kNeverFades   -> only for lights!
	;18	kDoesntLightLandscape 
	;19	kIgnoreFriendlyHits   -> only for actors!
	;20	kNoAIAcquire 
	;21	kCollisionGeometry_Filter 
	;22	kCollisionGeometry_BoundingBox 
	;23	kReflectedByAutoWater 
	;24	kDontHavokSettle 
	;25	kGround 
	;26	kRespawns 
	;27	kMultibound 
; - TurnOn: To turn the flag on or off.
;}



;Function RegisterForCollision(ObjectReference akObject) global native

; Used for Registering whenever an object physically colides with an actor.
; akObject: the object we are monitoring

;Function OnObjectCollision(ObjectReference akObject, ObjectReference akTarget) global native

; This is based of OnTrapHitStart(), but that function is objectreference function and needs to have one of its collision layers in the mesh to be set as L_TRAP.
; akObject: The object that's colliding with something or someone.
; akTarget: The object or actor being hit.
