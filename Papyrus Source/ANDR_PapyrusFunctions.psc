scriptName ANDR_PapyrusFunctions hidden

Import Math

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

Function CastSpellFromRef(Actor akSource, Spell akSpell, ObjectReference akTarget, ObjectReference akOriginRef) global native
{ 
- akSource: The caster of the spell.
- akSpell: Spell to cast.
- akTarget: An ObjectReference at which to aim the spell.
- akOriginRef: The ObjectReference where to cast the spell from.
}

Function CastSpellFromHand(Actor akSource, Spell akSpell, Bool IsLeftHand, Float DistanceVar = 2000.0, Float HeightVar = 100.0, Float Offset_NoSneak_Left_X = 30.0, Float Offset_NoSneak_Left_Y = 30.0, Float Offset_NoSneak_Left_Z = 110.0, Float Offset_NoSneak_Right_X = 30.0, Float Offset_NoSneak_Right_Y = -30.0, Float Offset_NoSneak_Right_Z = 110.0, Float Offset_Sneak_Left_X = 30.0, Float Offset_Sneak_Left_Y = 30.0, Float Offset_Sneak_Left_Z = 70.0, Float Offset_Sneak_Right_X = 30.0, Float Offset_Sneak_Right_Y = -30.0, Float Offset_Sneak_Right_Z = 70.0) native global
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
	

;Function CastSpellFromHand(Actor akSource, Spell akSpell, Bool IsLeftHand, Float DistanceVar = 2000.0, Float HeightVar = 100.0, Bool UseCustomObject = False, Form akObjectBase = None, Float Offset_NoSneak_Left_X = 30.0, Float Offset_NoSneak_Left_Y = 30.0, Float Offset_NoSneak_Left_Z = 110.0, Float Offset_NoSneak_Right_X = 30.0, Float Offset_NoSneak_Right_Y = -30.0, Float Offset_NoSneak_Right_Z = 110.0, Float Offset_Sneak_Left_X = 30.0, Float Offset_Sneak_Left_Y = 30.0, Float Offset_Sneak_Left_Z = 70.0, Float Offset_Sneak_Right_X = 30.0, Float Offset_Sneak_Right_Y = -30.0, Float Offset_Sneak_Right_Z = 70.0)	global
;{
;- akSource: The caster.
;- akSpell: The spell to cast.
;- IsLeftHand: True if cast from the left hand, false if cast from the right hand.
;- DistanceVar: Optional, the distance from the caster, where the destination marker is spawned. Default is 2000 units.
;- HeightVar: Optional, the height difference at which destination marker is spawned. Default is 100 units.
;- UseCustomObject: Optional, set to true if you want to assign akObjectBase to a custom object. Default is false.
;- akObjectBase: Optional, base object of the markers to spawn. Default is set to an xmarker.
;- Offset_NoSneak_Left_X: Optional, X Offset for left hand when the actor is not sneaking. Default value is 30.0.
;- Offset_NoSneak_Left_Y: Optional, Y Offset for left hand when the actor is not sneaking. Default value is 30.0.
;- Offset_NoSneak_Left_Z: Optional, Z Offset for left hand when the actor is not sneaking. Default value is 110.0.
;- Offset_NoSneak_Right_X: Optional, X Offset for right hand when the actor is not sneaking. Default value is 30.0.
;- Offset_NoSneak_Right_Y: Optional, Y Offset for right hand when the actor is not sneaking. Default value is -30.0.
;- Offset_NoSneak_Right_Z: Optional, Z Offset for right hand when the actor is not sneaking. Default value is 110.0.
;- Offset_Sneak_Left_X: Optional, X Offset for left hand when the actor is sneaking. Default value is 30.0.
;- Offset_Sneak_Left_Y: Optional, Y Offset for left hand when the actor is sneaking. Default value is 30.0.
;- Offset_Sneak_Left_Z: Optional, Z Offset for left hand when the actor is sneaking. Default value is 70.0.
;- Offset_Sneak_Right_X: Optional, X Offset for right hand when the actor is sneaking. Default value is 30.0.
;- Offset_Sneak_Right_Y: Optional, Y Offset for right hand when the actor is sneaking. Default value is -30.0.
;- Offset_Sneak_Right_Z: Optional, Z Offset for right hand when the actor is sneaking. Default value is 70.0.
;}
;	; ======= Example of use =======
;	; CastSpellFromHand(YsoldaRef, IceSpike, true)
;	; 
;	; Have Ysolda cast an Ice Spike spell from her left hand.
;	; ==============================
;
;	If UseCustomObject == False
;		akObjectBase = (Game.GetFormFromFile(0x3B, "Skyrim.esm"))
;	EndIf
;
;    Float GameX = akSource.GetAngleX()
;    Float GameZ = akSource.GetAngleZ()
;    Float AngleX = 90 + GameX  
;    Float AngleZ
;
;	Float SourceMarkerXOffset_Standard
;	Float SourceMarkerYOffset_Standard
;	Float SourceMarkerZOffset_Standard
;
;    If GameZ < 90 
;        AngleZ = 90 - GameZ
;    Else
;        AngleZ = 450 - GameZ
;    EndIf
;	
;	ObjectReference SourceMarkerRef = akSource.PlaceAtMe(akObjectBase)
;    ObjectReference DestinationMarkerRef = akSource.PlaceAtMe(akObjectBase)
;	
;	If !akSource.IsSneaking()
;		If 	IsLeftHand
;			SourceMarkerXOffset_Standard = Offset_NoSneak_Left_X
;			SourceMarkerYOffset_Standard = Offset_NoSneak_Left_Y
;			SourceMarkerZOffset_Standard = Offset_NoSneak_Left_Z
;		Else
;			SourceMarkerXOffset_Standard = Offset_NoSneak_Right_X
;			SourceMarkerYOffset_Standard = Offset_NoSneak_Right_Y
;			SourceMarkerZOffset_Standard = Offset_NoSneak_Right_Z
;		EndIf
;	Else
;		If 	IsLeftHand
;			SourceMarkerXOffset_Standard = Offset_Sneak_Left_X
;			SourceMarkerYOffset_Standard = Offset_Sneak_Left_Y
;			SourceMarkerZOffset_Standard = Offset_Sneak_Left_Z
;		Else
;			SourceMarkerXOffset_Standard = Offset_Sneak_Right_X
;			SourceMarkerYOffset_Standard = Offset_Sneak_Right_Y
;			SourceMarkerZOffset_Standard = Offset_Sneak_Right_Z
;		EndIf
;	EndIf
;	SourceMarkerRef.MoveTo(akSource, (cos(AngleZ)*SourceMarkerXOffset_Standard - sin(AngleZ)*SourceMarkerYOffset_Standard), (cos(AngleZ)*SourceMarkerYOffset_Standard + sin(AngleZ)*SourceMarkerXOffset_Standard), SourceMarkerZOffset_Standard) 
;	
;    DestinationMarkerRef.MoveTo(akSource, DistanceVar * Math.Sin(AngleX) * Math.Cos(AngleZ), DistanceVar * Math.Sin(AngleX) * Math.Sin(AngleZ), DistanceVar * Math.Cos(AngleX) + HeightVar)
;    CastSpellFromRef(akSource, akSpell, DestinationMarkerRef, SourceMarkerRef)
;
;    DestinationMarkerRef.Disable()
;	SourceMarkerRef.Disable()
;    Utility.Wait(0.5)
;    DestinationMarkerRef.Delete()    
;	SourceMarkerRef.Delete()	
;EndFunction

;Function RegisterForCollision(ObjectReference akObject) global native

; Used for Registering whenever an object physically colides with an actor.
; akObject: the object we are monitoring

;Function OnObjectCollision(ObjectReference akObject, ObjectReference akTarget) global native

; This is based of OnTrapHitStart(), but that function is objectreference function and needs to have one of its collision layers in the mesh to be set as L_TRAP.
; akObject: The object that's colliding with something or someone.
; akTarget: The object or actor being hit.
