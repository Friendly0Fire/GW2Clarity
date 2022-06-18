const auto g_Buffs = std::to_array<Buff>({
	// https://github.com/baaron4/GW2-Elite-Insights-Parser/blob/8e8ebd252eda839f3fa407d347e67a6065c8da74/GW2EIEvtcParser/ParserHelpers/SkillIDs.cs
	// Common Buffs
	{ "Buffs" },
	{ 743, "Aegis", 1 },
	{ 30328, "Alacrity", 1 },
	{ 725, "Fury", 1 },
	{ 740, "Might", 25 },
	{ 717, "Protection", 1 },
	{ 1187, "Quickness", 1 },
	{ 718, "Regeneration", 1 },
	{ 26980, "Resistance", 1 },
	{ 873, "Resolution", 1 },
	{ 1122, "Stability", 25 },
	{ 719, "Swiftness", 1 },
	{ 726, "Vigor", 1 },

	// Conditions
	{ "Conditions" },
	{ 736, "Bleeding", 1500 },
	{ 720, "Blinded", 1 },
	{ 737, "Burning", 1500 },
	{ 722, "Chilled", 1 },
	{ 861, "Confusion", 1500 },
	{ 721, "Crippled", 1 },
	{ 791, "Fear", 1 },
	{ 727, "Immobile", 1 },
	{ 723, "Poisoned", 1500 },
	{ 26766, "Slow", 1 },
	{ 27705, "Taunt", 1 },
	{ 19426, "Torment", 1500 },
	{ 738, "Vulnerability", 25 },
	{ 742, "Weakness", 1 },
	
	//// Auras
	{ "Auras" },
	{ 10332, "Chaos Aura", 1 }, // https://wiki.guildwars2.com/images/e/ec/Chaos_Aura.png
	{ 5677, "Fire Aura", 1 }, // https://wiki.guildwars2.com/images/c/ce/Fire_Aura.png
	{ 5579, "Frost Aura", 1 }, // https://wiki.guildwars2.com/images/8/87/Frost_Aura_%28effect%29.png
	{ 25518, "Light Aura", 1 }, // https://wiki.guildwars2.com/images/5/5a/Light_Aura.png
	{ 5684, "Magnetic Aura", 1 }, // https://wiki.guildwars2.com/images/0/0b/Magnetic_Aura_%28effect%29.png
	{ 5577, "Shocking Aura", 1 }, // https://wiki.guildwars2.com/images/5/5d/Shocking_Aura_%28effect%29.png
	{ 39978, "Dark Aura", 1 }, // https://wiki.guildwars2.com/images/e/ef/Dark_Aura.png
	//// Special
	{ "Special Status" },
	//{ 46668, "Diminished" }, // https://wiki.guildwars2.com/images/7/71/Diminished.png
	//{ 46587, "Malnourished" }, // https://wiki.guildwars2.com/images/6/67/Malnourished.png
	//{ 770, "Downed" }, // https://wiki.guildwars2.com/images/d/dd/Downed.png
	//{ 46842, "Exhaustion" }, // https://wiki.guildwars2.com/images/8/88/Exhaustion.png
	{ 13017, "Stealth", 1 }, // https://wiki.guildwars2.com/images/1/19/Stealth.png
	//{ 10269, "Hide in Shadows" }, // https://wiki.guildwars2.com/images/1/19/Stealth.png
	{ 64936, "Exposed" },
	{ 890, "Revealed", 1 }, // https://wiki.guildwars2.com/images/d/db/Revealed.png
	{ 5974, "Superspeed", 1 }, // https://wiki.guildwars2.com/images/c/c8/Superspeed.png
	//{ 762, "Determined (762)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 788, "Determined (788)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 848, "Resurrection" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 895, "Determined (895)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 3892, "Determined (3892)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 31450, "Determined (31450)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 52271, "Determined (52271)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 757, "Invulnerability (757)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 56227, "Invulnerability (56227)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 801, "Invulnerability (801)" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//{ 34113, "Spawn Protection?" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 872, "Stun", 1 }, // https://wiki.guildwars2.com/images/9/97/Stun.png
	{ 833, "Daze", 1 }, // https://wiki.guildwars2.com/images/7/79/Daze.png
	//{ 48209, "Exposed (48209)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	//{ 31589, "Exposed (31589)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	//{ 64936, "Exposed (64936)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	//{ 36781, "Unblockable" }, // https://wiki.guildwars2.com/images/f/f0/Unblockable_%28effect%29.png
	//{ 1159, "Encumbered" }, // https://wiki.guildwars2.com/images/d/d7/Encumbered.png
	{ 27048, "Celeritas Spores", 1 }, // https://wiki.guildwars2.com/images/7/7b/Blazing_Speed_Mushrooms.png
	{ 52182, "Branded Accumulation" }, // https://wiki.guildwars2.com/images/5/55/Achilles_Bane.png
	//// Racial
	//{ 12459, "Take Root" }, // https://wiki.guildwars2.com/images/b/b2/Take_Root.png
	//{ 12426, "Become the Bear" }, // https://wiki.guildwars2.com/images/7/7e/Become_the_Bear.png
	//{ 12405, "Become the Raven" }, // https://wiki.guildwars2.com/images/2/2c/Become_the_Raven.png
	//{ 12400, "Become the Snow Leopard" }, // https://wiki.guildwars2.com/images/7/78/Become_the_Snow_Leopard.png
	//{ 12393, "Become the Wolf" }, // https://wiki.guildwars2.com/images/f/f1/Become_the_Wolf.png
	//{ 12368, "Avatar of Melandru" }, // https://wiki.guildwars2.com/images/3/30/Avatar_of_Melandru.png
	//{ 12326, "Power Suit" }, // https://wiki.guildwars2.com/images/8/89/Summon_Power_Suit.png
	//{ 12366, "Reaper of Grenth" }, // https://wiki.guildwars2.com/images/0/07/Reaper_of_Grenth.png
	//{ 43503, "Charrzooka" }, // https://wiki.guildwars2.com/images/1/17/Charrzooka.png
	////
	//{ 33833, "Guild Item Research" }, // https://wiki.guildwars2.com/images/c/c6/Guild_Magic_Find_Banner_Boost.png
	////
	{ 37657, "Crystalline Heart" }, // https://wiki.guildwars2.com/images/5/56/Crystalline_Heart.png
	//// WvW
	//{ 14772, "Minor Borderlands Bloodlust" }, // https://wiki.guildwars2.com/images/f/f7/Major_Borderlands_Bloodlust.png
	//{ 14773, "Major Borderlands Bloodlust" }, // https://wiki.guildwars2.com/images/f/f7/Major_Borderlands_Bloodlust.png
	//{ 14774, "Superior Borderlands Bloodlust" }, // https://wiki.guildwars2.com/images/f/f7/Major_Borderlands_Bloodlust.png
	//{ 33120, "Blessing of Elements" }, // https://wiki.guildwars2.com/images/3/3c/Blessing_of_Air.png
	//{ 34031, "Flame's Embrace" }, // https://wiki.guildwars2.com/images/5/53/Flame%27s_Embrace.png
	//
	//{ 33719, "Sigil of Concentration" }, // https://wiki.guildwars2.com/images/b/b3/Superior_Sigil_of_Concentration.png
	//{ 53285, "Superior Rune of the Monk" }, // https://wiki.guildwars2.com/images/1/18/Superior_Rune_of_the_Monk.png
	//{ 9374, "Sigil of Corruption" }, // https://wiki.guildwars2.com/images/1/18/Superior_Sigil_of_Corruption.png
	//{ 9386, "Sigil of Life" }, // https://wiki.guildwars2.com/images/a/a7/Superior_Sigil_of_Life.png
	//{ 9385, "Sigil of Perception" }, // https://wiki.guildwars2.com/images/c/cc/Superior_Sigil_of_Perception.png
	//{ 9286, "Sigil of Bloodlust" }, // https://wiki.guildwars2.com/images/f/fb/Superior_Sigil_of_Bloodlust.png
	//{ 38588, "Sigil of Bounty" }, // https://wiki.guildwars2.com/images/f/f8/Superior_Sigil_of_Bounty.png
	//{ 9398, "Sigil of Benevolence" }, // https://wiki.guildwars2.com/images/5/59/Superior_Sigil_of_Benevolence.png
	//{ 22144, "Sigil of Momentum" }, // https://wiki.guildwars2.com/images/3/30/Superior_Sigil_of_Momentum.png
	//{ 46953, "Sigil of the Stars" }, // https://wiki.guildwars2.com/images/d/dc/Superior_Sigil_of_the_Stars.png
	//{ 43930, "Sigil of Severance" }, // https://wiki.guildwars2.com/images/c/c2/Superior_Sigil_of_Severance.png
	//{ 9441, "Sigil of Doom" }, // https://wiki.guildwars2.com/images/6/67/Superior_Sigil_of_Doom.png
	//
	//{ 36341, "Mistlock Instability: Adrenaline Rush" }, // https://wiki.guildwars2.com/images/7/72/Mistlock_Instability_Adrenaline_Rush.png
	//{ 22228, "Mistlock Instability: Afflicted" }, // https://wiki.guildwars2.com/images/3/3f/Mistlock_Instability_Afflicted.png
	//{ 53673, "Mistlock Instability: Boon Overload" }, // https://wiki.guildwars2.com/images/d/d7/Mistlock_Instability_Boon_Overload.png
	//{ 36386, "Mistlock Instability: Flux Bomb" }, // https://wiki.guildwars2.com/images/3/3f/Mistlock_Instability_Flux_Bomb.png
	//{ 48296, "Mistlock Instability: Fractal Vindicators" }, // https://wiki.guildwars2.com/images/4/48/Mistlock_Instability_Fractal_Vindicators.png
	//{ 54477, "Mistlock Instability: Frailty" }, // https://wiki.guildwars2.com/images/d/d6/Mistlock_Instability_Frailty.png
	//{ 47323, "Mistlock Instability: Hamstrung" }, // https://wiki.guildwars2.com/images/9/99/Mistlock_Instability_Hamstrung.png
	//{ 22293, "Mistlock Instability: Last Laugh" }, // https://wiki.guildwars2.com/images/5/58/Mistlock_Instability_Last_Laugh.png
	//{ 36224, "Mistlock Instability: Mists Convergence" }, // https://wiki.guildwars2.com/images/9/95/Mistlock_Instability_Mists_Convergence.png
	//{ 22277, "Mistlock Instability: No Pain, No Gain" }, // https://wiki.guildwars2.com/images/c/c3/Mistlock_Instability_No_Pain%2C_No_Gain.png
	//{ 54084, "Mistlock Instability: Outflanked" }, // https://wiki.guildwars2.com/images/0/0c/Mistlock_Instability_Outflanked.png
	//{ 32942, "Mistlock Instability: Social Awkwardness" }, // https://wiki.guildwars2.com/images/d/d2/Mistlock_Instability_Social_Awkwardness.png
	//{ 53932, "Mistlock Instability: Stick Together" }, // https://wiki.guildwars2.com/images/5/59/Mistlock_Instability_Stick_Together.png
	//{ 54237, "Mistlock Instability: Sugar Rush" }, // https://wiki.guildwars2.com/images/4/4c/Mistlock_Instability_Sugar_Rush.png
	//{ 36204, "Mistlock Instability: Toxic Trail" }, // https://wiki.guildwars2.com/images/d/df/Mistlock_Instability_Toxic_Trail.png
	//{ 46865, "Mistlock Instability: Vengeance" }, // https://wiki.guildwars2.com/images/c/c6/Mistlock_Instability_Vengeance.png
	//{ 54719, "Mistlock Instability: We Bleed Fire" }, // https://wiki.guildwars2.com/images/2/24/Mistlock_Instability_We_Bleed_Fire.png
	//{ 47288, "Mistlock Instability: Toxic Sickness" }, // https://wiki.guildwars2.com/images/6/6f/Mistlock_Instability_Toxic_Sickness.png
	//
	//// Generic
	{ "Generic Mechanics" },
	{ 38077, "Spectral Agony" }, // https://wiki.guildwars2.com/images/7/70/Spectral_Agony.png
	{ 15773, "Agony" }, // https://wiki.guildwars2.com/images/b/be/Agony.png
	{ 47856, "Hamstrung" }, // https://wiki.guildwars2.com/images/b/b9/Unseen_Burden.png
	//{ 18711, "Enraged (?)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 23235, "Enraged (??)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 50070, "Enraged 1 (100%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 61138, "Enraged 2 (100%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 31648, "Enraged 1 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 36078, "Enraged 2 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 53498, "Enraged 3 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 60674, "Enraged 4 (200%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 61006, "Enraged (300%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 31534, "Enraged (500%)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 34164, "Call of the Mists" }, // https://wiki.guildwars2.com/images/7/79/Call_of_the_Mists_%28raid_effect%29.png
	//{ 38793, "Untargetable" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	//// Strike Essences
	//{ 58786, "Essence of Vigilance Tier 1" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	//{ 58721, "Essence of Vigilance Tier 2" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	//{ 58182, "Essence of Vigilance Tier 3" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	//{ 60173, "Essence of Vigilance Tier 4" }, // https://wiki.guildwars2.com/images/a/a4/Essence_of_Vigilance.png
	//{ 58254, "Power of Vigilance Tier 2" }, // https://wiki.guildwars2.com/images/8/86/Power_of_Vigilance.png
	//{ 58712, "Power of Vigilance Tier 3" }, // https://wiki.guildwars2.com/images/8/86/Power_of_Vigilance.png
	//{ 58623, "Essence of Resilience Tier 1" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	//{ 58841, "Essence of Resilience Tier 2" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	//{ 58839, "Essence of Resilience Tier 3" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	//{ 60057, "Essence of Resilience Tier 4" }, // https://wiki.guildwars2.com/images/b/b6/Essence_of_Resilience.png
	//{ 58278, "Power of Resilience Tier 2" }, // https://wiki.guildwars2.com/images/d/d3/Power_of_Resilience.png
	//{ 60312, "Power of Resilience Tier 4" }, // https://wiki.guildwars2.com/images/d/d3/Power_of_Resilience.png
	//{ 58480, "Essence of Valor Tier 1" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	//{ 58585, "Essence of Valor Tier 2" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	//{ 58792, "Essence of Valor Tier 3" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	//{ 60062, "Essence of Valor Tier 4" }, // https://wiki.guildwars2.com/images/6/6f/Essence_of_Valor.png
	//{ 58385, "Power of Valor Tier 1" }, // https://wiki.guildwars2.com/images/6/64/Power_of_Valor.png
	//{ 58414, "Power of Valor Tier 2" }, // https://wiki.guildwars2.com/images/6/64/Power_of_Valor.png
	//// Unknown Fixation
	{ { 48533, 58136, 34508, 47434, 36998, 61503, 61306, 61304, 61566, 66140 }, "Fixated" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	////////////////////////////////////////////////
	//// Mordremoth
	//{ 32208, "Parietal Mastery" }, // https://wiki.guildwars2.com/images/7/76/Parietal_Mastery.png
	//{ 33305, "Parietal Origin" }, // https://wiki.guildwars2.com/images/7/76/Parietal_Mastery.png
	//{ 33705, "Temporal Mastery" }, // https://wiki.guildwars2.com/images/8/80/Temporal_Mastery.png
	//{ 33009, "Temporal Origin" }, // https://wiki.guildwars2.com/images/8/80/Temporal_Mastery.png
	//{ 33834, "Occipital Mastery" }, // https://wiki.guildwars2.com/images/9/9a/Occipital_Mastery.png
	//{ 32218, "Occipital Origin" }, // https://wiki.guildwars2.com/images/9/9a/Occipital_Mastery.png
	//{ 33194, "Frontal Mastery" }, // https://wiki.guildwars2.com/images/4/44/Frontal_Mastery.png
	//{ 33095, "Frontal Origin" }, // https://wiki.guildwars2.com/images/4/44/Frontal_Mastery.png
	//{ 33470, "Exposed (Mordremoth)" }, // https://wiki.guildwars2.com/images/6/6b/Exposed.png
	//{ 33378, "Weakened (Effect 1)" }, // https://wiki.guildwars2.com/images/8/8a/Weakened.png
	//{ 33904, "Weakened (Effect 2)" }, // https://wiki.guildwars2.com/images/8/8a/Weakened.png
	//{ 33763, "Empowered (Hearts and Minds)" }, // https://wiki.guildwars2.com/images/5/5e/Empowered_%28Hearts_and_Minds%29.png
	//{ 32762, "Power (Hearts and Minds)" }, // https://wiki.guildwars2.com/images/e/ec/Power_%28Hearts_and_Minds%29.png
	//{ 33131, "Shifty Aura" }, // https://wiki.guildwars2.com/images/7/78/Branded_Aura.png
	//{ 18981, "Fiery Block" }, // https://wiki.guildwars2.com/images/d/de/Shield_Stance.png
	////////////////////////////////////////////////
	//// VG
	{ "Vale Guardian" },
	//{ 31413, "Blue Pylon Power" }, // https://wiki.guildwars2.com/images/6/6e/Blue_Pylon_Power.png
	{ 31695, "Pylon Attunement: Red", "red_pylon" }, // https://wiki.guildwars2.com/images/9/94/Pylon_Attunement-_Red.png
	{ 31317, "Pylon Attunement: Blue", "blue_pylon" }, // https://wiki.guildwars2.com/images/6/6e/Blue_Pylon_Power.png
	{ 31852, "Pylon Attunement: Green", "green_pylon" }, // https://wiki.guildwars2.com/images/a/aa/Pylon_Attunement-_Green.png
	//{ 31539, "Unstable Pylon: Red" }, // https://wiki.guildwars2.com/images/3/36/Unstable_Pylon_%28Red%29.png
	//{ 31884, "Unstable Pylon: Blue" }, // https://wiki.guildwars2.com/images/c/c3/Unstable_Pylon_%28Blue%29.png
	//{ 31828, "Unstable Pylon: Green" }, // https://wiki.guildwars2.com/images/9/9d/Unstable_Pylon_%28Green%29.png
	//{ 34979, "Unbreakable" }, // https://wiki.guildwars2.com/images/5/56/Xera%27s_Embrace.png
	{ 51371, "Unstable Magic Spike", "range" }, //
	//// Gorseval
	{ "Gorseval" },
	//{ 31722, "Spirited Fusion" }, // https://wiki.guildwars2.com/images/e/eb/Spirited_Fusion.png
	//{ 31877, "Protective Shadow" }, // https://wiki.guildwars2.com/images/8/87/Protective_Shadow.png
	{ 31623, "Ghastly Prison" }, // https://wiki.guildwars2.com/images/6/62/Ghastly_Prison.png
	//{ 31548, "Vivid Echo" }, // https://wiki.guildwars2.com/images/4/4f/Vivid_Echo.png
	{ 31498, "Spectral Darkness" }, // https://wiki.guildwars2.com/images/a/a8/Spectral_Darkness.png
	//// Sabetha
	{ "Sabetha" },
	{ 34108, "Shell-Shocked" }, // https://wiki.guildwars2.com/images/3/39/Shell-Shocked.png
	{ 31473, "Sapper Bomb" }, // https://wiki.guildwars2.com/images/8/88/Sapper_Bomb_%28effect%29.png
	{ 31485, "Time Bomb" }, // https://wiki.guildwars2.com/images/9/91/Time_Bomb.png
	////////////////////////////////////////////////
	//// Slothasor
	{ "Slothasor" },
	//{ 34467, "Narcolepsy" }, // https://wiki.guildwars2.com/images/e/eb/Determined.png
	{ 34496, "Nauseated" }, // https://wiki.guildwars2.com/images/3/30/Nauseated.png
	//{ 34362, "Magic Transformation" }, // https://wiki.guildwars2.com/images/4/45/Magic_Transformation.png
	{ 34387, "Volatile Poison" }, // https://wiki.guildwars2.com/images/1/12/Volatile_Poison.png
	//// Trio
	{ "Bandit Trio" },
	//{ 34434, "Not the Bees!" }, // https://wiki.guildwars2.com/images/0/08/Throw_Jar.png
	//{ 34464, "Slow Burn" }, // https://wiki.guildwars2.com/images/6/6e/Heat_Wave_%28Matthias_Gabrel_effect%29.png
	//{ 34392, "Targeted" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	{ 34393, "Target!", "target" }, // https://wiki.guildwars2.com/images/0/09/Target.png
	//{ 34439, "Locust Trail" }, // https://wiki.guildwars2.com/images/0/09/Target.png
	//// Matthias
	{ "Matthias" },
	//{ 34376, "Blood Shield Abo" }, // https://wiki.guildwars2.com/images/a/a6/Blood_Shield.png
	//{ 34518, "Blood Shield" }, // https://wiki.guildwars2.com/images/a/a6/Blood_Shield.png
	//{ 34422, "Blood Fueled" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	//{ 34428, "Blood Fueled Abo" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	{ 34450, "Unstable Blood Magic" }, // https://wiki.guildwars2.com/images/0/09/Unstable_Blood_Magic.png
	{ 34416, "Corruption" }, // https://wiki.guildwars2.com/images/3/34/Locust_Trail.png
	{ 34367, "Unbalanced" }, // https://wiki.guildwars2.com/images/8/80/Unbalanced.png
	{ 34511, "Zealous Benediction" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	{ 34369, "Snowstorm" }, // https://wiki.guildwars2.com/images/2/26/Snowstorm_%28Matthias_Gabrel_effect%29.png
	{ 34526, "Heat Wave" }, // https://wiki.guildwars2.com/images/6/6e/Heat_Wave_%28Matthias_Gabrel_effect%29.png
	{ 34472, "Downpour" }, // https://wiki.guildwars2.com/images/4/4a/Downpour.png
	//{ 34433, "Snowstorm (Matthias)" }, // https://wiki.guildwars2.com/images/2/26/Snowstorm_%28Matthias_Gabrel_effect%29.png
	//{ 34458, "Heat Wave (Matthias)" }, // https://wiki.guildwars2.com/images/6/6e/Heat_Wave_%28Matthias_Gabrel_effect%29.png
	//{ 34568, "Downpour (Matthias)" }, // https://wiki.guildwars2.com/images/4/4a/Downpour.png
	//{ 34548, "Unstable" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	////////////////////////////////////////////////
	//// KC
	{ "Keep Construct" },
	//{ 35096, "Compromised" }, // https://wiki.guildwars2.com/images/4/48/Compromised.png
	//{ 35025, "Xera's Boon" }, // https://wiki.guildwars2.com/images/0/04/Xera%27s_Boon.png
	{ 35103, "Xera's Fury" }, // https://wiki.guildwars2.com/images/d/dd/Xera%27s_Fury.png
	{ { 34912, 34925 }, "Statue Fixated", "fixated" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 35140, "Incoming!" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ { 35091, 35014 }, "Crimson Attunement" }, // https://wiki.guildwars2.com/images/3/3e/Crimson_Attunement.png
	{ { 35109, 34992 }, "Radiant Attunement" }, // https://wiki.guildwars2.com/images/6/68/Radiant_Attunement.png
	//{ 35119, "Magic Blast" }, // https://wiki.guildwars2.com/images/a/a9/Magic_Blast_Intensity.png
	//{ 35075, "Gaining Power" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//// Twisted Castle
	{ "Twisted Castle" },
	//{ 34918, "Spatial Distortion" }, // https://wiki.guildwars2.com/images/7/7a/Bloodstone_Blessed.png
	{ 35006, "Madness" }, // https://wiki.guildwars2.com/images/e/ee/Madness.png
	//{ 35106, "Still Waters" }, // https://wiki.guildwars2.com/images/5/5c/Still_Waters_%28effect%29.png
	//{ 34955, "Soothing Waters" }, // https://wiki.guildwars2.com/images/8/8f/Soothing_Waters.png
	//{ 34963, "Chaotic Haze" }, // https://wiki.guildwars2.com/images/4/48/Lava_Font.png
	//{ 34927, "Creeping Pursuit" }, // https://wiki.guildwars2.com/images/f/f8/Creeping_Pursuit.png
	//// Xera
	{ "Xera" },
	{ 34965, "Derangement" }, // https://wiki.guildwars2.com/images/c/ca/Derangement.png
	//{ 35084, "Bending Chaos" }, // https://wiki.guildwars2.com/images/3/39/Target%21.png
	//{ 35162, "Shifting Chaos" }, // https://wiki.guildwars2.com/images/0/04/Shifting_Chaos.png
	//{ 35032, "Twisting Chaos" }, // https://wiki.guildwars2.com/images/6/60/Twisting_Chaos.png
	{ 35000, "Intervention" }, // https://wiki.guildwars2.com/images/a/a2/Intervention_%28effect%29.png
	//{ 35168, "Bloodstone Protection" }, // https://wiki.guildwars2.com/images/4/4e/Bloodstone_Protection.png
	//{ 34917, "Bloodstone Blessed" }, // https://wiki.guildwars2.com/images/7/7a/Bloodstone_Blessed.png
	//{ 34883, "Void Zone" }, // https://wiki.guildwars2.com/images/5/56/Void_Zone.png
	////////////////////////////////////////////////
	//// Cairn
	{ "Cairn" },
	//{ 38049, "Shared Agony" }, // https://wiki.guildwars2.com/images/5/53/Shared_Agony.png
	//{ 37675, "Enraged (Cairn)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 38153, "Unseen Burden", "hamstrung" }, // https://wiki.guildwars2.com/images/b/b9/Unseen_Burden.png
	{ 38098, "Countdown" }, // https://wiki.guildwars2.com/images/0/05/Countdown.png
	{ 37714, "Gaze Avoidance" }, // https://wiki.guildwars2.com/images/1/10/Gaze_Avoidance.png
	//// MO
	{ "Mursaat Overseer" },
	//{ 37626, "Empowered" }, // https://wiki.guildwars2.com/images/9/9c/Empowered_%28Mursaat_Overseer%29.png
	//{ 38155, "Mursaat Overseer's Shield" }, // https://wiki.guildwars2.com/images/8/84/Mursaat_Overseer%27s_Shield.png
	//{ 37813, "Protect (SAK)" }, // https://wiki.guildwars2.com/images/f/f6/Protect.png
	//{ 37697, "Dispel (SAK)" }, // https://wiki.guildwars2.com/images/8/84/Mursaat_Overseer%27s_Shield.png
	//{ 37779, "Claim (SAK)" }, // https://wiki.guildwars2.com/images/e/ef/Claim.png
	//// Samarog
	{ "Samarog" },
	{ 37868, "Fixated (Samarog)", "fixated" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 38223, "Fixated (Guldhem)", "fixated" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 37693, "Fixated (Rigom)", "fixated" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 37966, "Inevitable Betrayal (Big)" }, // https://wiki.guildwars2.com/images/b/b8/Feeding_Frenzy_%28GW1%29.png
	{ 38247, "Inevitable Betrayal (Small)" }, // https://wiki.guildwars2.com/images/b/b8/Feeding_Frenzy_%28GW1%29.png
	//{ 37892, "Soul Swarm" }, // https://wiki.guildwars2.com/images/0/0e/Soul_Swarm_%28effect%29.png
	//// Deimos
	{ "Deimos" },
	//{ 38224, "Unnatural Signet" }, // https://wiki.guildwars2.com/images/2/20/Unnatural_Signet.png
	{ 38187, "Weak Minded", "unseen_burden" }, // https://wiki.guildwars2.com/images/3/38/Unseen_Burden_%28Deimos%29.png
	{ 37733, "Tear Instability" }, // https://wiki.guildwars2.com/images/1/11/Tear_Instability.png
	{ 37871, "Form Up and Advance!" }, // https://wiki.guildwars2.com/images/5/56/Form_Up_and_Advance%21.png
	//{ 37718, "Devour" }, // https://wiki.guildwars2.com/images/3/3d/Devour.png
	{ 38266, "Unseen Burden [??]", "unseen_burden" }, // https://wiki.guildwars2.com/images/3/38/Unseen_Burden_%28Deimos%29.png
	////////////////////////////////////////////////
	//// Soulless Horror
	{ "Soulless Horror" },
	{ 48349, "Exile's Embrace" }, // https://wiki.guildwars2.com/images/b/b4/Exile%27s_Embrace.png
	{ 47414, "Necrosis" }, // https://wiki.guildwars2.com/images/4/47/Ichor.png
	//// River
	{ "River of Souls" },
	{ 47470, "Soul Siphon" }, // https://wiki.guildwars2.com/images/f/f7/Soul_Siphon.png
	{ 47219, "Desmina's Protection" }, // https://wiki.guildwars2.com/images/b/b3/Desmina%27s_Protection.png
	//{ 47122, "Follower's Asylum" }, // https://wiki.guildwars2.com/images/b/b3/Desmina%27s_Protection.png
	//{ 48331, "Spirit Form" }, // https://wiki.guildwars2.com/images/2/2e/Spirit_Form_%28Hall_of_Chains%29.png
	{ { 48154, 48583, 48281 }, "Mortal Coil" }, // https://wiki.guildwars2.com/images/4/41/Mortal_Coil.png
	{ { 47758, 48848 }, "Energy Threshold", "spirit_form" }, // https://wiki.guildwars2.com/images/2/2e/Spirit_Form_%28Hall_of_Chains%29.png
	//// Broken King
	{ "Broken King" },
	{ 47776, "Frozen Wind" }, // https://wiki.guildwars2.com/images/3/3a/Frozen_Wind.png
	//{ 47595, "Shield of Ice" }, // https://wiki.guildwars2.com/images/3/38/Shield_of_Ice.png
	{ 47022, "Glaciate" }, // https://wiki.guildwars2.com/images/b/ba/Glaciate.png
	//// Eater of Soul
	{ "Eater of Souls" },
	//{ 48541, "Soul Digestion" }, // https://wiki.guildwars2.com/images/0/08/Soul_Digestion.png
	{ 47090, "Reclaimed Energy" }, // https://wiki.guildwars2.com/images/2/21/Reclaimed_Energy.png
	{ 47473, "Empowered" }, // https://wiki.guildwars2.com/images/d/de/Empowered_%28Statue_of_Death%29.png
	////  Eyes
	{ "Eyes" },
	//{ 47635, "Last Grasp (Judgment)" }, // https://wiki.guildwars2.com/images/2/26/Last_Grasp.png
	//{ 47278, "Last Grasp (Fate)" }, // https://wiki.guildwars2.com/images/2/26/Last_Grasp.png
	//{ 47251, "Exposed (Statue of Darkness)" }, // https://wiki.guildwars2.com/images/4/42/Exposed_%28Statue_of_Darkness%29.png
	//{ 48779, "Light Carrier" }, // https://wiki.guildwars2.com/images/f/f1/Torch_Fielder.png
	//{ 48477, "Empowered (Light Thief)" }, // https://wiki.guildwars2.com/images/0/08/Soul_Digestion.png
	//// Dhuum
	{ "Dhuum" },
	{ 46950, "Fractured Spirit" }, // https://wiki.guildwars2.com/images/c/c3/Fractured_Spirit.png
	{ 47476, "Residual Affliction" }, // https://wiki.guildwars2.com/images/1/12/Residual_Affliction.png
	{ 47646, "Arcing Affliction" }, // https://wiki.guildwars2.com/images/f/f0/Arcing_Affliction.png
	//{ 47929, "One-Track Mind" }, // https://wiki.guildwars2.com/images/6/68/Tracked.png
	//{ 47181, "Imminent Demise" }, // https://wiki.guildwars2.com/images/5/58/Superheated_Metal.png
	//{ 47202, "Lethal Report" }, // https://wiki.guildwars2.com/images/0/02/Mantra_of_Signets.png
	{ 48773, "Hastened Demise" }, // https://wiki.guildwars2.com/images/5/5b/Hastened_Demise.png
	//{ 49125, "Echo's Pick up" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	////////////////////////////////////////////////
	//// CA
	{ "Conjured Amalgamate" },
	{ 52667, "Greatsword Power" }, // https://wiki.guildwars2.com/images/3/3b/Greatsword_Power_%28effect%29.png
	//{ 53030, "Fractured - Enemy" }, // https://wiki.guildwars2.com/images/7/78/Branded_Aura.png
	//{ 52213, "Fractured - Allied" }, // https://wiki.guildwars2.com/images/7/78/Branded_Aura.png
	{ 52754, "Conjured Shield" }, // https://wiki.guildwars2.com/images/8/83/Conjured_Shield_%28effect%29.png
	//{ 52973, "Conjured Protection" }, // https://wiki.guildwars2.com/images/8/83/Bloodstone-Infused_shield.png
	//{ 53003, "Shielded" }, // https://wiki.guildwars2.com/images/4/47/Golem-Powered_Shielding.png
	//{ 52074, "Augmented Power" }, // https://wiki.guildwars2.com/images/4/47/Golem-Powered_Shielding.png
	{ 53075, "Locked On" }, // https://wiki.guildwars2.com/images/3/39/Target%21.png
	//{ 52255, "CA Invul" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	//{ 52430, "Arm Up" }, // https://wiki.guildwars2.com/images/d/d3/Blood_Fueled.png
	//{ 52943, "Fixation" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//// Twin Largos
	{ "Twin Largos" },
	//{ 52211, "Aquatic Aura (Kenut)" }, // https://wiki.guildwars2.com/images/4/44/Expose_Weakness.png
	//{ 52929, "Aquatic Aura (Nikare)" }, // https://wiki.guildwars2.com/images/f/fd/Fractured_%28effect%29.png
	{ 51935, "Waterlogged" }, // https://wiki.guildwars2.com/images/8/89/Waterlogged.png
	//{ 52626, "Enraged (Twin Largos)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//// Qadim
	{ "Qadim" },
	//{ 52568, "Flame Armor" }, // https://wiki.guildwars2.com/images/e/e7/Magma_Orb.png
	//{ 52588, "Fiery Surge" }, // https://wiki.guildwars2.com/images/f/f9/Fiery_Surge.png
	{ 52035, "Power of the Lamp" }, // https://wiki.guildwars2.com/images/e/e5/Break_Out%21.png
	//{ 52408, "Unbearable Flames" }, // https://wiki.guildwars2.com/images/2/21/Expel_Excess_Magic_Poison.png
	//{ 52464, "Parry" }, // https://wiki.guildwars2.com/images/2/28/Parry_%28effect%29.png
	//{ 52863, "Mythwright Surge" }, // https://wiki.guildwars2.com/images/7/7a/Swiftness_%28effect%29.png
	{ 51726, "Lamp Bond" }, // https://wiki.guildwars2.com/images/d/db/Lamp_Bond.png
	//{ 52040, "Enraged (Wywern)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 51933, "Enraged (Qadim)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 52075, "Resistance (Lava Elemental)" }, // https://wiki.guildwars2.com/images/1/18/Fire_Shield.png
	//{ 53048, "Shielded (Lava Elemental)" }, // https://wiki.guildwars2.com/images/1/18/Fire_Shield.png
	////////////////////////////////////////////////
	//// Adina
	{ "Cardinal Adina" },
	//{ 56204, "Pillar Pandemonium" }, // https://wiki.guildwars2.com/images/d/d9/Captain%27s_Inspiration.png
	{ 56593, "Radiant Blindness" }, // https://wiki.guildwars2.com/images/6/6c/Radiant_Blindness.png
	//{ 56099, "Diamond Palisade (Damage)" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	//{ 56636, "Diamond Palisade" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	{ 56440, "Eroding Curse" }, // https://wiki.guildwars2.com/images/d/de/Toxic_Gas.png
	//// Sabir
	{ "Cardinal Sabir" },
	//{ 56100, "Ion Shield" }, // https://wiki.guildwars2.com/images/9/94/Ion_Shield.png
	{ 56123, "Violent Currents" }, // https://wiki.guildwars2.com/images/0/06/Violent_Currents.png
	//{ 56172, "Repulsion Field" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	//{ 56391, "Electrical Repulsion" }, // https://wiki.guildwars2.com/images/d/dd/Xera%27s_Fury.png
	//{ 56474, "Electro-Repulsion" }, // https://wiki.guildwars2.com/images/4/42/Exposed_%28Statue_of_Darkness%29.png
	//{ 56216, "Eye of the Storm" }, // https://wiki.guildwars2.com/images/5/52/Mending_Waters_%28effect%29.png
	//{ 56394, "Bolt Break" }, // https://wiki.guildwars2.com/images/7/74/Mesmer_icon_white.png
	//// Peerless Qadim
	{ "Qadim the Peerless" },
	//{ 56582, "Erratic Energy" }, // https://wiki.guildwars2.com/images/4/45/Unstable.png
	//{ 56104, "Power Share" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	{ 56118, "Sapping Surge" }, // https://wiki.guildwars2.com/images/6/6f/Guilt_Exploitation.png
	//{ 56182, "Chaos Corrosion" }, // https://wiki.guildwars2.com/images/f/fd/Fractured_%28effect%29.png
	//{ 56510, "Fixated (Qadim the Peerless)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 56475, "Magma Drop" }, // https://wiki.guildwars2.com/images/2/24/Targeted.png
	//{ 56424, "Critical Mass" }, // https://wiki.guildwars2.com/images/b/bf/Orb_of_Ascension_%28effect%29.png
	{ 56609, "Kinetic Abundance" }, // https://wiki.guildwars2.com/images/6/64/Kinetic_Abundance.png
	{ 56540, "Enfeebled Force" }, // https://wiki.guildwars2.com/images/b/b6/Enfeebled_Force.png
	//{ 56360, "Backlashing Beam" }, // https://wiki.guildwars2.com/images/0/04/Xera%27s_Boon.png
	//{ 56257, "Clutched by Chaos" }, // https://wiki.guildwars2.com/images/8/87/Protective_Shadow.png
	//{ 56237, "Cleansed Conductor" }, // https://wiki.guildwars2.com/images/a/a9/Magic_Blast_Intensity.png
	//{ 56655, "Poisoned Power" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 56250, "Incorporeal" }, // https://wiki.guildwars2.com/images/8/8b/Magic_Aura.png
	//{ 56264, "Flare-Up" }, // https://wiki.guildwars2.com/images/8/8b/Magic_Aura.png
	//{ 56467, "Unbridled Chaos" }, // https://wiki.guildwars2.com/images/4/42/Exposed_%28Statue_of_Darkness%29.png
	////////////////////////////////////////////////
	//// Fractals
	{ 33652, "Rigorous Certainty" }, // https://wiki.guildwars2.com/images/6/60/Desert_Carapace.png
	//{ 47248, "Fractal Savant" }, // https://wiki.guildwars2.com/images/c/cb/Malign_9_Agony_Infusion.png
	//{ 48191, "Fractal Prodigy" }, // https://wiki.guildwars2.com/images/1/11/Mighty_9_Agony_Infusion.png
	//{ 48414, "Fractal Champion" }, // https://wiki.guildwars2.com/images/3/3d/Precise_9_Agony_Infusion.png
	//{ 47222, "Fractal God" }, // https://wiki.guildwars2.com/images/2/22/Healing_9_Agony_Infusion.png
	//// Siax
	//{ 36998, "Fixated (Nightmare)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//// Ensolyss
	//{ 37498, "Determination (Ensolyss)" }, // https://wiki.guildwars2.com/images/4/41/Gambit_Exhausted.png
	//// Artsariiv
	{ "Artsariiv" },
	//{ 36498, "Enraged (Fractal)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	{ 38880, "Corporeal Reassignment" }, // https://wiki.guildwars2.com/images/9/94/Redirect_Anomaly.png
	//{ 39442, "Blinding Radiance" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	//{ 38841, "Determination (Viirastra)" }, // https://wiki.guildwars2.com/images/4/41/Gambit_Exhausted.png
	//// Arkk
	{ "Arkk" },
	//{ 39558, "Fixated (Bloom 3)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 39928, "Fixated (Bloom 2)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 39131, "Fixated (Bloom 1)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 38985, "Fixated (Bloom 4)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 39268, "Cosmic Meteor", "monster" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	//{ 39111, "Diaphanous Shielding" }, // https://wiki.guildwars2.com/images/5/57/Diaphanous_Shielding.png
	//{ 33750, "Electrocuted" }, // https://wiki.guildwars2.com/images/9/91/Air_Attunement.png
	//// Ai, Keeper of the Peak
	{ "Ai, Keeper of the Peak" },
	//{ 61402, "Tidal Barrier" }, // https://wiki.guildwars2.com/images/b/b1/Primed_Bottle.png
	//{ 61224, "Whirlwind Shield" }, // https://wiki.guildwars2.com/images/b/b1/Primed_Bottle.png
	//{ 61220, "Resilient Form" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	//{ 61435, "Cacophonous Mind" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	//{ 61208, "Crushing Guilt" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	//{ 61304, "Fixated (Fear 3)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 61306, "Fixated (Fear 2)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 61503, "Fixated (Fear 1)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	//{ 61566, "Fixated (Fear 4)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	{ 61444, "Charged Leap" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	{ 61512, "Tidal Bargain", "waterlogged" }, // https://wiki.guildwars2.com/images/1/13/Crowd_Favor.png
	////////////////////////////////////////////////
	//// Icebrood
	//{ 57969, "Hypothermia" }, // https://wiki.guildwars2.com/images/d/d5/Hypothermia_%28story_effect%29.png
	//// Fraenir of Jormag
	//{ 58376, "Frozen" }, // https://wiki.guildwars2.com/images/6/6a/Frostbite_%28Bitterfrost_Frontier%29.png
	//{ 58276, "Snowblind" }, // https://wiki.guildwars2.com/images/6/6a/Frostbite_%28Bitterfrost_Frontier%29.png
	//// Voice and Claw
	//{ 58619, "Enraged (V&C)" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//// Boneskinner
	{ "Boneskinner" },
	{ 58860, "Tormenting Aura" }, // https://wiki.guildwars2.com/images/c/c0/Darkness.png
	//// Whisper of Jormag
	{ "Whisper of Jormag" },
	//{ 59223, "Whisper Teleport Out" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 59054, "Whisper Teleport Back" }, // https://wiki.guildwars2.com/images/7/78/Vengeance_%28Mordrem%29.png
	//{ 59105, "Frigid Vortex" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ { 59100, 59120 }, "Chains of Frost" }, // https://wiki.guildwars2.com/images/6/63/Use_Soul_Binder.png
	//{ 59120, "Chains of Frost Application" }, // https://wiki.guildwars2.com/images/5/5f/Monster_Skill.png
	//{ 59073, "Brain Freeze" }, // https://wiki.guildwars2.com/images/6/6a/Frostbite_%28Bitterfrost_Frontier%29.png
	//// Frezie
	//{ 53510, "Icy Barrier" }, // https://wiki.guildwars2.com/images/3/38/Shield_of_Ice.png
	//// Mai Trin
	{ "Mai Trin" },
	{ { 65900, 67831 }, "Shared Destruction", "monster" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 67872, "Photon Saturation" },
	//{ 64843, "Immune to damage and conditions." }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 65238, "Mai Trin ?????" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 66409, "Chaos and Destruction" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//// Ankka
	//{ 67447, "Necrotic Ritual" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 66962, "Ankka ???" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 64179, "Hallucinations" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 63621, "Energy Transfer" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//// Minister Li
	{ 65869, "Target Order 1" },
	{ 65088, "Target Order 2" },
	{ 64377, "Target Order 3" },
	{ 64999, "Target Order 4" },
	{ 65717, "Target Order 5" },
	{ 67965, "Infirmity" },
	{ 67972, "Debilitated" },
	//{ 64465, "Stronger Together" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 63840, "Vitality Equalizer 1" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 66894, "Vitality Equalizer 2" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 65250, "Destructive Aura" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 67047, "Equalization Matrix" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 64834, "Lethal Inspiration" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	{ 65662, "Extreme Vulnerability" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 66140, "Fixated (Kaineng Overlook)" }, // https://wiki.guildwars2.com/images/6/66/Fixated.png
	////
	//{ 64524, "Influence of the Void" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 60517, "Targeted (Dragon Void)" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 65729, "Void Repulsion 1" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 65577, "Void Repulsion 2" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 63605, "Aerial Defense" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 66800, "Void Immunity" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
	//{ 65563, "Void Shell" }, // https://wiki.guildwars2.com/images/6/65/Windfall.png
});