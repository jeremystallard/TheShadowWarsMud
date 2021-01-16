#include "masterforms.h"

struct formlookup masterformslookup_table[MASTERFORMS]; 

void initmasterformslookup()
{

	masterformslookup_table[0].weapontype = WEAPON_SWORD;
	masterformslookup_table[0].formindex = 0;
	masterformslookup_table[0].gsn = gsn_blademaster;	
	masterformslookup_table[0].weapon_vnum  = OBJ_VNUM_HERON_SWORD;

	masterformslookup_table[1].weapontype = WEAPON_DAGGER;
	masterformslookup_table[1].formindex = 1;
	masterformslookup_table[1].gsn = gsn_duelling;	
	masterformslookup_table[1].weapon_vnum  = OBJ_VNUM_DRAGON_DAGGER;

	masterformslookup_table[2].weapontype = WEAPON_SPEAR;
	masterformslookup_table[2].formindex = 2;
	masterformslookup_table[2].gsn = gsn_speardancer;	
	masterformslookup_table[2].weapon_vnum  = OBJ_VNUM_RAVEN_SPEAR;

	masterformslookup_table[3].weapontype = WEAPON_AXE;
	masterformslookup_table[3].formindex = 3;
	masterformslookup_table[3].gsn = gsn_axemaster;	
	masterformslookup_table[3].weapon_vnum  = OBJ_VNUM_MF_AXE;

	masterformslookup_table[4].weapontype = WEAPON_FLAIL;
	masterformslookup_table[4].formindex = 4;
	masterformslookup_table[4].gsn = gsn_flailmaster;	
	masterformslookup_table[4].weapon_vnum  = OBJ_VNUM_MF_FLAIL;

	masterformslookup_table[5].weapontype = WEAPON_WHIP;
	masterformslookup_table[5].formindex = 5;
	masterformslookup_table[5].gsn = gsn_whipmaster;	
	masterformslookup_table[5].weapon_vnum  = OBJ_VNUM_MF_WHIP;

	masterformslookup_table[6].weapontype = WEAPON_STAFF;
	masterformslookup_table[6].formindex = 6;
	masterformslookup_table[6].gsn = gsn_staffmaster;	
	masterformslookup_table[6].weapon_vnum  = OBJ_VNUM_MF_STAFF;

	masterformslookup_table[7].weapontype = WEAPON_NONE;
	masterformslookup_table[7].formindex = 7;
	masterformslookup_table[7].gsn = gsn_martialarts;
	masterformslookup_table[7].weapon_vnum  = OBJ_VNUM_MF_KNUCKLES;
	
		
}

const struct masterform_type masterforms_table[] =
{
	/* 0 - 11 Blademaster */
	{"{BThe Grapevine Twines{x","The Grapevine Twines, trying to send $p flying from{x", "{BHeron Wading in the Rushes{x"}, /*Blademaster 1 */	
	{"{BRibbon in the Air{x","blademaster attack 2{x", "{BStones Falling From the Cliff{x"}, /*Blademaster 2 */	
	{"{BSwallow Takes Flight{x","blademaster attack 3{x", "{BParting the Silk{x"}, /*Blademaster 3 */	
	{"{BMoon on the Water{x","blademaster attack 4{x", "{BThe Wood Grouse Dances{x"}, /*Blademaster 4 */	
	{"{BLightning of the Three Prongs{x","blademaster attack 5{x", "{BLeaf on the Breeze{x"}, /*Blademaster 5 */	
	{"{BThe Boar Rushes Down the Mountain{x","blademaster attack 6{x", "{BThe River Undercuts the Bank{x"}, /*Blademaster 6 */	
	{"{BStrikes the Spark{x","blademaster attack 7{x", "{BRain Falling From Above{x"}, /*Blademaster 7 */	
	{"{BApple Blossoms in the Wind{x","blademaster attack 8{x", "{BThe Kingfisher takes a Silverback{x"}, /*Blademaster 8 */	
	{"{BArc of the Moon{x","blademaster attack 9{x", "{BBundling Straw{x"}, /*Blademaster 9 */	
	{"{BThe Cat Dances on the Wall{x","blademaster attack 10{x", "{BThe Boar Rushes Downhill{x"}, /*Blademaster 10 */	
	{"{BThe Falling Leaf{x","blademaster attack 11{x", "{BWatered Silk{x"}, /*Blademaster 11 */	
	{"{BParting the Silk{x","blademaster attack 12{x", "{BThe Swallow Takes Flight{x"}, /*Blademaster 12 */	
	
	/* 12 - 23 - Duelling */
	{"{CThe Wolf's Grace{x","duelling attack 1{x", "{CSurgeons Nightmare{x"}, /*duelling 1 */	
	{"{CThe Thieves Embrace{x","duelling attack 2{x", "{CThe Razor Shield{x"}, /*duelling 2 */	
	{"{CThe Snakes Bite{x","duelling attack 3{x", "{CSevering Roots{x"}, /*duelling 3 */	
	{"{CThe Porcupine Spines{x","duelling attack 4{x", "{CThe Tangle of the Vine{x"}, /*duelling 4 */	
	{"{CThe Raking Talons{x","duelling attack 5{x", "{CThe Adders Sting{x"}, /*duelling 5 */	
	{"{CThe Earth's Bite{x","duelling attack 6{x", "{CThe Tangle of the Vine{x"}, /*duelling 6 */	
	{"{CThe Blinding Spark{x","duelling attack 7{x", "{CThe Earth's Bite{x"}, /*duelling 7 */	
	{"{CThe Adders Sting{x","duelling attack 8{x", "{CThe Raking Talons{x"}, /*duelling 8 */	
	{"{CThe Tangle of the Vine{x","duelling attack 9{x", "{CThe Porcupine Spines{x"}, /*duelling 9 */	
	{"{CSevering Roots{x","duelling attack 10{x", "{CThe Snakes Bite{x"}, /*duelling 10 */	
	{"{CThe Razor Shield{x","duelling attack 11{x", "{CThe Thieves Embrace{x"}, /*duelling 11 */	
	{"{CSurgeons Nightmare{x","duelling attack 12{x", "{CThe Wolf's Grace{x"}, /*duelling 12 */	

	/* 24 - 47 - speardancer */
	{"{GClutch of the Seven Murderers{x","Clutch of the Seven Murderers{x binding $p{x", "{GCrossing the River{x"}, /*speardancer 1 */	
	{"{GBiting Grains of Sand{x","speardancer attack 2{x", "{GStorm Over Mountains{x"}, /*speardancer 2 */	
	{"{GFangs of the Two-Step{x","speardancer attack 3{x", "{GCha al'Dazar{x"}, /*speardancer 3 */	
	{"{GSavage Whirlwind of Shields{x","speardancer attack 4{x", "{GDragon's Roll{x"}, /*speardancer 4 */	
	{"{GStab of the Searing Sun{x","speardancer attack 5{x", "{GBitter Submission{x"}, /*speardancer 5 */	
	{"{GIcy Stare of the Gara{x","speardancer attack 6{x", "{GBlack As Night{x"}, /*speardancer 6 */	
	{"{GLioness on the Hunt{x","speardancer attack 7{x", "{GFloating Viper-Snap{x"}, /*speardancer 7 */	
	{"{GThunder Ripping the Dunes{x","speardancer attack 8{x", "{GLioness on the Hunt{x"}, /*speardancer 8 */	
	{"{GBitter Submission{x","speardancer attack 9{x", "{GSavage Whirlwind of Shields{x"}, /*speardancer 9 */	
	{"{GStorm Over Mountains{x","speardancer attack 10{x", "{GClutch of Seven Murders{x"}, /*speardancer 10 */	
	{"{GCrossing The River{x","speardancer attack 11{x", "{GBiting Grains of Sand{x"}, /*speardancer 11 */	
	{"{GFloating Viper-Snap{x","speardancer attack 12{x", "{GIcy Stare of the Gara{x"}, /*speardancer 12 */	

	/* 48 - 59 - axemaster */
	{"{RFelling the Oak{x","AxeMaster attack 1{x", "{RHeadsmans Curse{x"}, /*AxeMaster 1 */	
	{"{RArc of the Moon{x","AxeMaster attack 2{x", "{REye of the Wolf{x"}, /*AxeMaster 2 */	
	{"{RWrath of the Heavens{x","AxeMaster attack 3{x", "{RThe Razor Shield{x"}, /*AxeMaster 3 */	
	{"{RHammer of Doom{x","AxeMaster attack 4{x", "{RThe Reaper Strikes{x"}, /*AxeMaster 4 */	
	{"{RSevering Roots{x","AxeMaster attack 5{x", "{RCharging Aurochs{x"}, /*AxeMaster 5 */	
	{"{RSplitting Stones{x","AxeMaster attack 6{x", "{RHammer of Doom{x"}, /*AxeMaster 6 */	
	{"{RSparks from the Anvil{x","AxeMaster attack 7{x", "{RThe Reaper Strikes{x"}, /*AxeMaster 7 */	
	{"{RCharging Aurochs{x","AxeMaster attack 8{x", "{RSplitting Stones{x"}, /*AxeMaster 8 */	
	{"{RThe Reaper Strikes{x","AxeMaster attack 9{x", "{RSevering Roots{x"}, /*AxeMaster 9 */	
	{"{RThe Razor Shield{x","AxeMaster attack 10{x", "{RWrath of the Heavens{x"}, /*AxeMaster 10 */	
	{"{REye of the Wolf{x","AxeMaster attack 11{x", "{RArc of the Moon{x"}, /*AxeMaster 11 */	
	{"{RHeadsmans Curse{x","AxeMaster attack 12{x", "{RFelling the Oak{x"}, /*AxeMaster 12 */	

	/* 60 - 71 - flailmaster */
	{"{ySinking Stones{x","Flailmaster attack 1{x", "{ySpinning Swings{x"}, /*Flailmaster 1 */	
	{"{yCrushing Blow{x","Flailmaster attack 2{x", "{yBoulders Falling from a Cliff{x"}, /*Flailmaster 2 */	
	{"{yCracking Walnuts{x","Flailmaster attack 3{x", "{yHammering the Forge{x"}, /*Flailmaster 3 */	
	{"{yCrushing Strikes{x","Flailmaster attack 4{x", "{yCrushing Swing{x"}, /*Flailmaster 4 */	
	{"{yBloody Pudding{x","Flailmaster attack 5{x", "{yDeath on a Stick{x"}, /*Flailmaster 5 */	
	{"{yAnvil Falling{x","Flailmaster attack 6{x", "{yFalling Thunder{x"}, /*Flailmaster 6 */	
	{"{yCausing Stars{x","Flailmaster attack 7{x", "{yBreaking Heads{x"}, /*Flailmaster 7 */	
	{"{yFalling Thunder{x","Flailmaster attack 8{x", "{yAnvil Falling{x"}, /*Flailmaster 8 */	
	{"{yDeath on a stick{x","Flailmaster attack 9{x", "{yBloody Pudding{x"}, /*Flailmaster 9 */	
	{"{ySpinning Swing{x","Flailmaster attack 10{x", "{yCrushing Strikes{x"}, /*Flailmaster 10 */	
	{"{yCrushing Swing{x","Flailmaster attack 11{x", "{yCracking Walnuts{x"}, /*Flailmaster 11 */	
	{"{yBoulders Falling from a Cliff{x","Flailmaster attack 12{x", "{ySinking Stones{x"}, /*Flailmaster 12 */	

	/* 72 - 83 - whipmaster */
	{"{MCat of Nine Tails{x","Whipmaster attack 1{x", "{MTeamsters Twist{x"}, /*Whipmaster 1 */	
	{"{MViper Strike{x","Whipmaster attack 2{x", "{MHangmans Friend{x"}, /*Whipmaster 2 */	
	{"{MFlaying the Flesh{x","Whipmaster attack 3{x", "{MRattlesnakes Strike{x"}, /*Whipmaster 3 */	
	{"{MFlick of the Lash{x","Whipmaster attack 4{x", "{MSting of the Cobra{x"}, /*Whipmaster 4 */	
	{"{MWhirring Dervish{x","Whipmaster attack 5{x", "{MDevils Tail{x"}, /*Whipmaster 5 */	
	{"{MThe Forked Tongue{x","Whipmaster attack 6{x", "{MScorpions Sting{x"}, /*Whipmaster 6 */	
	{"{MScorpion Sting{x","Whipmaster attack 7{x", "{MThe Forked Tonge{x"}, /*Whipmaster 7 */	
	{"{MDevils Tail{x","Whipmaster attack 8{x", "{MWhirring Dervish{x"}, /*Whipmaster 8 */	
	{"{MSting of the Cobra{x","Whipmaster attack 9{x", "{MFlick of the Lash{x"}, /*Whipmaster 9 */	
	{"{MRattlesnakes Strike{x","Whipmaster attack 10{x", "{MFlaying the Flesh{x"}, /*Whipmaster 10 */	
	{"{MHangmans Friend{x","Whipmaster attack 11{x", "{MViper Strike{x"}, /*Whipmaster 11 */	
	{"{MTeamsters Twist{x","Whipmaster attack 12{x", "{MCat of Nine Tails{x"}, /*Whipmaster 12 */	

	/* 84 - 95 - staffmaster */
	{"{YFalling Leaves{x","Staffmaster attack 1{x", "{YSnapping Branches{x"}, /*Staffmaster 1 */	
	{"{YLunge of the Timberwolf{x","Staffmaster attack 2{x", "{YCreeper Vine Entwines{x"}, /*Staffmaster 2 */	
	{"{YFalling Pine{x","Staffmaster attack 3{x", "{YRage of an Ogier{x"}, /*Staffmaster 3 */	
	{"{YThe Widowmaker{x","Staffmaster attack 4{x", "{YMovement in the Leaves{x"}, /*Staffmaster 4 */	
	{"{YStrike of the Badger{x","Staffmaster attack 5{x", "{YThe Old Oak{x"}, /*Staffmaster 5 */	
	{"{YThunder in the Woods{x","Staffmaster attack 6{x", "{YLeaves in the Wind{x"}, /*Staffmaster 6 */	
	{"{YLeaves in the Wind{x","Staffmaster attack 7{x", "{YThunder in the Woods{x"}, /*Staffmaster 7 */	
	{"{YThe Old Oak{x","Staffmaster attack 8{x", "{YStrike of the Badger{x"}, /*Staffmaster 8 */	
	{"{YMovement in the Leaves{x","Staffmaster attack 9{x", "{YWillows in the Wind{x"}, /*Staffmaster 9 */	
	{"{YRage of an Ogier{x","Staffmaster attack 10{x", "{YFalling Pine{x"}, /*Staffmaster 10 */	
	{"{YCreeper Vine Entwines{x","Staffmaster attack 11{x", "{YLunge of the Timberwolf{x"}, /*Staffmaster 11 */	
	{"{YSnapping Branches{x","Staffmaster attack 12{x", "{YFalling Leaves{x"}, /*Staffmaster 12 */	

	/* 96 - 107 - martialarts */
	{"{WMonkey in the Trees{x","MartialArts attack 1{x", "{WBadger in the Dark{x"}, /*MartialArts 1 */	
	{"{WTiger in the Grass{x","MartialArts attack 2{x", "{WFox in the Hen House{x"}, /*MartialArts 2 */	
	{"{WPanther in the Night{x","MartialArts attack 3{x", "{WGoose on the Wind{x"}, /*MartialArts 3 */	
	{"{WLeopard in the Snow{x","MartialArts attack 4{x", "{WTail of a Gator{x"}, /*MartialArts 4 */	
	{"{WBear in the Cave{x","MartialArts attack 5{x", "{WHeron in the Marshes{x"}, /*MartialArts 5 */	
	{"{WSwan on the Lake{x","MartialArts attack 6{x", "{WSnake in the Grass{x"}, /*MartialArts 6 */	
	{"{WCat on a Hot Tin Roof{x","MartialArts attack 7{x", "{WBear in the Cave{x"}, /*MartialArts 7 */	
	{"{WHeron in the Marshes{x","MartialArts attack 8{x", "{WLeopard in the Snow{x"}, /*MartialArts 8 */	
	{"{WTail of a Gator{x","MartialArts attack 9{x", "{WPanther in the Night{x"}, /*MartialArts 9 */	
	{"{WElephant in the Village{x","MartialArts attack 10{x", "{WTiger in the Grass{x"}, /*MartialArts 10 */	
	{"{WFox in the Hen House{x","MartialArts attack 11{x", "{WMonkey in the Trees{x"}, /*MartialArts 11 */	
	{"{WBadger in the Dark{x","MartialArts attack 12{x", "{WSalmon Climbing the Falls{x"}, /*MartialArts 12 */	
	{NULL, 	NULL, NULL}	


} ;


sh_int find_relevant_masterform(CHAR_DATA *ch)
{
	OBJ_DATA * obj;
	int weapon_type = WEAPON_NONE;
	sh_int masterform = 0;
	int i;
	obj = get_eq_char( ch, WEAR_WIELD );
			
	if (obj != NULL)
		weapon_type = obj->value[0];
		
	for (i = 0; i < MASTERFORMS; i++)
	{
		if (masterformslookup_table[i].weapontype == weapon_type)
	   {
			masterform = masterformslookup_table[i].gsn;   	
	   		break;
	   }
	}
	return masterform;
	
}

char * get_master_extended_attack(CHAR_DATA *ch, int index)
{
	sh_int masterform = find_relevant_masterform(ch);
	int formmultiplier = 0;
	int i = 0;
	for (i = 0; i < MASTERFORMS; i++)
	{
		if (masterformslookup_table[i].gsn == masterform)
		{
			formmultiplier = masterformslookup_table[i].formindex;
			break;	
		}	
	}
	return masterforms_table[(formmultiplier * MOVES_PER_FORM) + index].extended_attackString;
		
}

char * get_master_attack(sh_int masterform, int index)
{
	int formmultiplier = 0;
	int i = 0;
        char buf[256];
	if (index < 0 || index > 11)
	{
		sprintf(buf, "GetMasterAttack: index invalid: %d{x",index);
	        wiznet(buf,NULL,NULL,WIZ_SECURE,0,0);
		index = number_range(0,11);
	}
	for (i = 0; i < MASTERFORMS; i++)
	{
		if (masterformslookup_table[i].gsn == masterform)
		{
			formmultiplier = masterformslookup_table[i].formindex;
			break;	
		}	
	}
	
	return masterforms_table[(formmultiplier * MOVES_PER_FORM) + index].attackstring;
		
}

char * get_master_defend(sh_int masterform, int index)
{
	int formmultiplier = 0;
	int i = 0;
        char buf[256];
        if (index < 0 || index > 11)
        {
                sprintf(buf, "GetMasterAttack: index invalid: %d{x",index);
                wiznet(buf,NULL,NULL,WIZ_SECURE,0,0);
                index = number_range(0,11);
        }
	for (i = 0; i < MASTERFORMS; i++)
	{
		if (masterformslookup_table[i].gsn == masterform)
		{
			formmultiplier = masterformslookup_table[i].formindex;
			break;	
		}	
	}
	
	return masterforms_table[(formmultiplier * MOVES_PER_FORM) + index].defensestring;
		
}

int getmfskill(CHAR_DATA * ch)
{
	int skill_gsn = find_relevant_masterform(ch);
	int skill = 0;
	if (skill_gsn != 0)
	{
		skill = get_skill(ch,skill_gsn);
	}
	return skill;
	 	
}


// Check if CH can do mf with current weapons
bool check_mf(CHAR_DATA *ch, sh_int gsn)
{
  OBJ_DATA * wield1;
  OBJ_DATA * wield2;

  wield1 = get_eq_char(ch,WEAR_WIELD);
  wield2 = get_eq_char(ch,WEAR_SECOND_WIELD);
 
  if (!wield1)
  { 
      if (gsn != gsn_martialarts)
      {
  	  return FALSE;
      }
      else
      {
	return TRUE;
      }
  }

  if (wield1->item_type != ITEM_WEAPON) {
	return FALSE;
  }

  if (wield1->value[0] != get_mf_weapontype(gsn))
  {
  	return FALSE;
  }
  
  if ((gsn == gsn_duelling) && wield1 != NULL && wield2 != NULL && (wield1->value[0]== WEAPON_DAGGER && wield2->value[0] == WEAPON_DAGGER)) //duelling
  {
  	 	return TRUE;
  } 
  
  if ((gsn == gsn_speardancer) && wield1 != NULL && wield2 != NULL && (wield1->value[0]== WEAPON_SPEAR && wield2->value[0] == WEAPON_SPEAR)) //Speardancer
  {
  	 	return TRUE;
  } 

  //Shields allowed with speardancer
  if ((gsn == gsn_speardancer) && wield1 != NULL && wield1->value[0]== WEAPON_SPEAR && get_eq_char(ch, WEAR_SHIELD != NULL))
  {
    return TRUE;
  }
  
  // If wielding a shield also
  if (get_eq_char(ch, WEAR_SHIELD) != NULL && !IS_SET(ch->merits, MERIT_AMBIDEXTROUS)) {
    return FALSE;
  }

  // Only with 1 weapon unless ambidex merit
  if (get_eq_char(ch, WEAR_SECOND_WIELD) != NULL && !IS_SET(ch->merits, MERIT_AMBIDEXTROUS))
  {
    return FALSE;
  }
  
  return TRUE;
}	


bool is_masterform(sh_int gsn)
{ 
  bool retval = FALSE;
  int i = 0;
  for (i = 0; i < MASTERFORMS; i++)
  {
	if (masterformslookup_table[i].gsn == gsn)
	{
		retval = TRUE;
		break;
	}
  }
  return retval;
}

bool char_knows_masterform(CHAR_DATA *ch)
{
  bool retval = FALSE;
  int i = 0;
  for (i = 0; i < MASTERFORMS; i++)
  {
    if (get_skill(ch,masterformslookup_table[i].gsn) > 0)
    {
	retval = TRUE;
	break;
    }
  }
  return retval;
}

unsigned int get_mf_weapontype(sh_int gsn_mf)
{
   int i = 0;
   unsigned int retval = 0;
   for (i = 0 ; i < MASTERFORMS; i++)
   {
	if (masterformslookup_table[i].gsn == gsn_mf)
        {
           retval = masterformslookup_table[i].weapontype;
           break;	
        }
   }
   return retval;
}

unsigned int get_mf_weapon(sh_int gsn_mf)
{
   int i = 0;
   unsigned int retval = 0;
   for (i = 0 ; i < MASTERFORMS; i++)
   {
	if (masterformslookup_table[i].gsn == gsn_mf)
        {
           retval = masterformslookup_table[i].weapon_vnum;
           break;	
        }
   }
   return retval;
}
