#include "g_local.h"
#include "g_account.h"
#include "string.h"
#include <stdlib.h>
#include <algorithm>
#include "sqlite3/sqlite3.h"
#include "sqlite3/libsqlitewrapped.h"
#include "q_shared.h"
#include "g_adminshared.h"


//====Account Functions====//

void Cmd_GetNPC_F( gentity_t *ent ) {

	if(!isLoggedIn(ent))
	{
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"^1Error: You are not logged in.\n\"");
		return;
	}

	if( M_isNPCAccess(ent) ) {
				ent->client->pers.hasCheatAccess = qfalse;
				trap_SendServerCommand( ent->client->ps.clientNum, va ("print \"^5NPC Spawn Access ^1Removed.\n\"" ));
				G_LogPrintf( "deniedNPCaccess: %s\n", ent->client->pers.netname );
			}
			else{
				ent->client->pers.hasCheatAccess = qtrue;
				trap_SendServerCommand( ent->client->ps.clientNum, va ("print \"^5NPC Spawn Access ^2Granted.\n\"" ));
				G_LogPrintf( "NPCaccess: %s\n", ent->client->pers.netname );
			}
			return;
}

/*
=================

Cmd_AccountLogin_f

Ingame command : login <user> <password>
Account Login

=================
*/
void Cmd_AccountLogin_F( gentity_t * targetplayer )
{
	Database db(DATABASE_PATH);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	char userName[MAX_STRING_CHARS];
	char userPassword[MAX_STRING_CHARS];

	//Make sure they entered both a username and a password
	if( trap_Argc() < 3 ){
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^5Command Usage: qwlogin <username> <password>\n\"");
		return;
	}

	//Check if we're already logged in 
	if(isLoggedIn(targetplayer))
	{
		trap_SendServerCommand ( targetplayer->client->ps.clientNum, "print \"^5Already logged in!\n\"");
		return;
	}
	//Get the username and password
	trap_Argv( 1, userName, MAX_STRING_CHARS );
	std::string userNameSTR = userName;
	trap_Argv( 2, userPassword, MAX_STRING_CHARS );

	//Check if this username exists
	Query q(db);
	std::transform(userNameSTR.begin(), userNameSTR.end(),userNameSTR.begin(),::tolower);
	std::string DBname = q.get_string(va("SELECT name FROM users WHERE name='%s'",userNameSTR.c_str()));
	if(DBname.empty())
	{
		trap_SendServerCommand ( targetplayer->client->ps.clientNum, va( "print \"^1Error: Username %s does not exist.\n\"", userName ) );
		return;
	}

	//Check password
	std::string DBpassword = q.get_string(va("SELECT password FROM users WHERE name='%s'",userNameSTR.c_str()));
	if(DBpassword.empty() || strcmp(DBpassword.c_str(),userPassword) != 0 )
	{
		trap_SendServerCommand ( targetplayer->client->ps.clientNum, va("print \"^1Error: Incorrect password. \n\"",DBpassword.c_str()));
		return;
	}

	//Log the user in
	int userID = q.get_num(va("SELECT ID FROM users WHERE name='%s'",userNameSTR.c_str()));
	targetplayer->client->sess.userID = userID;
	targetplayer->client->sess.loggedinAccount = qtrue;

	LoadUser(targetplayer);

	trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "print \"^2Success: You are now logged in as %s!\n\"", userName ) );

	//Update the ui
	trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "lui_login" ) );
	return;
}

/*
=================

Cmd_AccountLogout_f

Ingame command : logout
Account Logout

=================
*/

void Cmd_AccountLogout_F(gentity_t * targetplayer)
{
	if(!isLoggedIn(targetplayer))
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^1Error: You are not logged in, so you can't logout.\n\"");
		return;
	}

	//Save the character
	if(targetplayer->client->sess.characterChosen == qtrue)
	{

	//Logout of Account
	targetplayer->client->sess.loggedinAccount = qfalse;
	targetplayer->client->sess.userID = NULL;

	//Deseoect Character
	targetplayer->client->sess.characterChosen = qfalse;
	targetplayer->client->sess.characterID = NULL;

	trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^2Success: You have been logged out.\n\"");

	//Remove all feats
	for(int k = 0; k < NUM_FEATS-1; k++)
	{
		targetplayer->client->featLevel[k] = FORCE_LEVEL_0;
	}

	//Remove all character skills
	for(int i = 0; i < NUM_SKILLS-1; i++)
	{
		targetplayer->client->skillLevel[i] = FORCE_LEVEL_0;
	}

	//Remove all force powers
	targetplayer->client->ps.fd.forcePowersKnown = 0;
	for(int j = 0; j < NUM_FORCE_POWERS-1; j++)
	{
		targetplayer->client->ps.fd.forcePowerLevel[j] = FORCE_LEVEL_0;
	}

	//Respawn client
	targetplayer->flags &= ~FL_GODMODE;
	targetplayer->client->ps.stats[STAT_HEALTH] = targetplayer->health = -999;
	SetTeam(targetplayer,"s");

	//Update the ui

	trap_SendServerCommand( targetplayer->client->ps.clientNum, va("lui_logout"));
	}
	return;
}

/*
=================

Cmd_AccountCreate_f

Ingame command : register <user> <password>
Account Creation

=================
*/
void Cmd_AccountCreate_F(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);
	
	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	char userName[MAX_STRING_CHARS];
	char userPassword[MAX_STRING_CHARS];

	//Make sure they entered both a user and a password
	if( trap_Argc() < 3 ){
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^5Command Usage: qwregister <user> <password>\n\"");
		return;
	}
	
	//Get the user and pass
	trap_Argv( 1, userName, MAX_STRING_CHARS );
	std::string userNameSTR = userName;
	trap_Argv( 2, userPassword, MAX_STRING_CHARS );
	

	//Check if that user exists already
	Query q(db);
	std::transform(userNameSTR.begin(), userNameSTR.end(),userNameSTR.begin(),::tolower);
	std::string DBname = q.get_string(va("SELECT name FROM users WHERE name='%s'",userNameSTR.c_str()));
	if(!DBname.empty())
	{
		trap_SendServerCommand ( targetplayer->client->ps.clientNum, va("print \"^1Error: Username %s is already in use.\n\"",DBname.c_str()));
		return;
	}
	//Create the account
	q.execute(va("INSERT INTO users(name,password) VALUES('%s','%s')",userNameSTR.c_str(),userPassword));

	//Log them in automatically
	int userID = q.get_num(va("SELECT ID FROM users WHERE name='%s'",userNameSTR.c_str()));

	targetplayer->client->sess.userID = userID;
	targetplayer->client->sess.loggedinAccount = qtrue;

	LoadUser(targetplayer);

	trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "print \"^5Account was successfully created! You are now logged in as %s.\n\"", userNameSTR.c_str()));

	//Update the ui
	trap_SendServerCommand( targetplayer->client->ps.clientNum, va("lui_login"));
	
	return;
}

//====Character Functions====//

/*
=================

Cmd_ListCharacters_F

Ingame Command : myCharacters
List all of the characters of an account

=================
*/
void Cmd_ListCharacters_F(gentity_t * targetplayer)
{
	StderrLog log;
	Database db(DATABASE_PATH, &log);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	//Make sure they're logged in
	if(!isLoggedIn(targetplayer))
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^1Error: You must be logged in to list characters.\n\"");
		return;
	}

	Query q(db);
	q.get_result(va("SELECT ID, name FROM characters WHERE userID='%i'",targetplayer->client->sess.userID));
	trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^5Characters:\n\"");
	while (q.fetch_row())
	{
		int ID = q.getval();
		std::string name = q.getstr();
		trap_SendServerCommand( targetplayer->client->ps.clientNum, va("print \"^3ID: ^7%i ^3Name: ^7%s\n\"",ID,name.c_str()));
	}
	q.free_result();

	return;
}

/*
=================

Cmd_CreateCharacter_F

Ingame Command : createCharacter <name>
Creates a new character and binds it to a useraccount

=================
*/
void Cmd_CreateCharacter_F(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	//Make sure they're logged in
	if(!isLoggedIn(targetplayer))
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^1Error: You must be logged in to create a character.\n\"");
		return;
	}

	//Make sure they entered a character
	if( trap_Argc() < 2 ){
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^5Command Usage: /qwcreateCharacter <name> <model> <modelscale>\n\"");
		return;
	}

	char userinfo[MAX_INFO_STRING];
	trap_GetUserinfo( targetplayer->client->ps.clientNum, userinfo, MAX_INFO_STRING );

	//Get the character name
	char charName[MAX_STRING_CHARS];
	trap_Argv( 1, charName, MAX_STRING_CHARS );
	std::string charNameSTR = charName;

	char charModel[MAX_STRING_CHARS];
	trap_Argv( 2, charModel, MAX_STRING_CHARS );
	std::string charModelSTR = charModel;

	char temp[MAX_STRING_CHARS];
	trap_Argv( 3, temp, MAX_STRING_CHARS );
	targetplayer->client->sess.modelScale = atoi(temp);

	//Check if the character exists
	Query q(db);
	std::transform(charNameSTR.begin(), charNameSTR.end(),charNameSTR.begin(),::tolower);
	std::string DBname = q.get_string(va("SELECT name FROM characters WHERE userID='%i' AND name='%s'",targetplayer->client->sess.userID,charNameSTR.c_str()));

	//Create character
	q.execute( va( "INSERT INTO characters(userID,name) VALUES('%i','%s')", targetplayer->client->sess.userID, charNameSTR.c_str() ) );
	q.execute( va( "UPDATE characters set model='%s' WHERE ID='%i'", charModelSTR.c_str(), targetplayer->client->sess.characterID ) );
	q.execute( va( "UPDATE characters set modelscale='%i' WHERE ID='%i'", targetplayer->client->sess.modelScale, targetplayer->client->sess.characterID ) );
	q.execute( va( "UPDATE characters set level='1' WHERE ID='%i'", targetplayer->client->sess.characterID ) );
	q.execute( va( "UPDATE characters set xp='0' WHERE ID='%i'", targetplayer->client->sess.characterID ) );
	q.execute( va( "UPDATE characters set playerclass='0' WHERE ID='%i'", targetplayer->client->sess.characterID ) );

	trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "print \"^2Sucess: Character created.\n\"" ) );

	return;
}

/*
=================

Cmd_SelectCharacter_F

Ingame Command : character <name>
Loads the character data and executes the keys effects

=================
*/
void Cmd_SelectCharacter_F(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	//Make sure they're logged in
	if(!isLoggedIn(targetplayer))
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^1Error: You must be logged in to select a character.\n\"");
		return;
	}

	//Make sure they entered a character
	if( trap_Argc() < 2 ){
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^5Command Usage: /qwcharacter <name>\n\"");
		return;
	}

	//Get the character name
	char charName[MAX_STRING_CHARS];
	trap_Argv( 1, charName, MAX_STRING_CHARS );
	std::string charNameSTR = charName;

	//Check if the character exists
	Query q(db);
	std::transform(charNameSTR.begin(), charNameSTR.end(),charNameSTR.begin(),::tolower);
	int charID = q.get_num(va("SELECT ID FROM characters WHERE userID='%i' AND name='%s'",targetplayer->client->sess.userID,charNameSTR.c_str()));
	if(charID == 0)
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "print \"^1Error: Character %s does not exist.\n\"", charNameSTR.c_str() ) );
		return;
	}
	
	//Update that we have a character selected
	targetplayer->client->sess.characterChosen = qtrue;
	targetplayer->client->sess.characterID = charID;

	SetTeam(targetplayer,"f");

	LoadCharacter(targetplayer);

	trap_SendServerCommand( targetplayer->client->ps.clientNum, va("print \"^5Your character is selected as: %s.\n\"",charName));

	targetplayer->flags &= ~FL_GODMODE;
	targetplayer->client->ps.stats[STAT_HEALTH] = targetplayer->health = -999;
	player_die (targetplayer, targetplayer, targetplayer, 100000, MOD_SUICIDE);

	return;
}

/*
=================

LoadCharacter

Loads the character data

=================
*/
void DetermineDodgeMax(gentity_t *ent);
void LoadCharacter(gentity_t * targetplayer)
{
	LoadSkills(targetplayer);
	LoadForcePowers(targetplayer);
	LoadFeats(targetplayer);
	LoadAttributes(targetplayer);

	
	//Create new power string
	std::string newForceString;
	newForceString.append(va("%i-%i-",FORCE_MASTERY_JEDI_KNIGHT,FORCE_LIGHTSIDE));
	int i;
	for(i = 0; i < NUM_FORCE_POWERS; i++)
	{
	char tempForce[2];
    itoa(targetplayer->client->ps.fd.forcePowerLevel[i],tempForce,10);
	newForceString.append(tempForce);
	}
	for(i = 0; i < NUM_SKILLS; i++)
	{
	char tempSkill[2];
    itoa(targetplayer->client->skillLevel[i],tempSkill,10);
	newForceString.append(tempSkill);
	}
	for(i = 0; i < NUM_FORCE_POWERS; i++)
	{
	char tempFeat[2];
    itoa(targetplayer->client->featLevel[i],tempFeat,10);
	newForceString.append(tempFeat);
	}
	trap_SendServerCommand( targetplayer->client->ps.clientNum, va("forcechanged x %s\n", newForceString.c_str()));
	
	DetermineDodgeMax(targetplayer);
	return;
}
/*
=================

LoadFeats

Loads the character feats

=================
*/
void LoadFeats(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}
	Query q(db);
	std::string feats = q.get_string(va("SELECT feats FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));
	int size = (feats.size() < NUM_FEATS) ? feats.size() : NUM_FEATS;
	for(int i = 0; i < size; i++)
	{
		char temp = feats[i];
		int level = temp - '0';
		targetplayer->client->featLevel[i] = level;
	}
	return;
}


/*
=================

LoadSkills

Loads the character skills

=================
*/
void LoadSkills(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}
	Query q(db);
	std::string skills = q.get_string(va("SELECT skills FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));
	int size = (skills.size() < NUM_SKILLS) ? skills.size() : NUM_SKILLS;
	for(int i = 0; i < size; i++)
	{
		char temp = skills[i];
		int level = temp - '0';
		targetplayer->client->skillLevel[i] = level;
	}
	return;
}


/*
=================

LoadForcePowers

Loads the character force powers

=================
*/
void LoadForcePowers(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}
	Query q(db);
	std::string powers = q.get_string(va("SELECT force FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));
	int size = (powers.size() < NUM_FORCE_POWERS) ? powers.size() : NUM_FORCE_POWERS;
	for(int i = 0; i < size; i++)
	{
		char temp = powers[i];
		int level = temp - '0';
		targetplayer->client->ps.fd.forcePowerLevel[i] = level;
		if(level > 0)
			targetplayer->client->ps.fd.forcePowersKnown |= (1 << i);
	}
	return;
}

/*
=================

GetForceLevel

Takes the level stored in the database and returns force power value

=================
*/
int GetForceLevel(int level)
{
	switch(level)
	{
		case 1:
			return FORCE_LEVEL_1;
		case 2:
			return FORCE_LEVEL_2;
		case 3:
			return FORCE_LEVEL_3;
		default:
			return FORCE_LEVEL_0;
	}
}

/*
=================

GrantSkill

Grants the appropriate skill with default level to the player
=====
*/
void GrantSkill(int charID, int skill)
{
	Database db(DATABASE_PATH);
	Query q(db);

	q.execute(va("INSERT INTO skills(charID,skill,level) VALUES('%i','%i','%i')",charID,skill,1));
	return;
}

/*
=================

UpdateSkill

Updates the appropriate skill with new level
=====
*/
void UpdateSkill(int charid, int skill, int level)
{
	Database db(DATABASE_PATH);
	Query q(db);
	
	int skillID = q.get_num(va("SELECT ID FROM skills WHERE charID='%i' AND skill='%i'",charid,skill));
	q.execute(va("UPDATE skills set level='%i' WHERE ID='%i'",level,skillID));
	return;
}

/*
=================

GrantFP

Grants the appropriate forcepower with default level to the player
=====
*/
void GrantFP(int charID, int forcepower)
{
	Database db(DATABASE_PATH);
	Query q(db);

	q.execute(va("INSERT INTO forcepowers(charID,forcepower,level) VALUES('%i','%i','%i')",charID,forcepower,1));
	return;
}

/*
=================

UpdateFP

Updates the appropriate forcepower with new level
=====
*/
void UpdateFP(int charid, int forcepower, int level)
{
	Database db(DATABASE_PATH);
	Query q(db);
	
	int fpID = q.get_num(va("SELECT ID FROM forcepowers WHERE charID='%i' AND forcepower='%i'",charid,forcepower));
	q.execute(va("UPDATE forcepowers set level='%i' WHERE ID='%i'",level,fpID));
	return;
}

/*
=================

HasForcePower

Checks if the character has this forcepower
=====
*/
qboolean HasForcePower(int charid, int power)
{
	Database db(DATABASE_PATH);
	Query q(db);

	q.get_result(va("SELECT * FROM forcepower WHERE charID='%i' AND forcepower='%i'",charid,power));
	int forcepower = q.num_rows();
	if(forcepower)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================

HasSkill

Checks if the character has this skill
=====
*/
qboolean HasSkill(int charid, int skill)
{
	Database db(DATABASE_PATH);
	Query q(db);

	q.get_result(va("SELECT * FROM skills WHERE charID='%i' AND skill='%i'",charid,skill));
	int valid = q.num_rows();
	if(valid)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================

HasFeat

Checks if the character has this feat
=====
*/
qboolean HasFeat(int charid, int featID)
{
	Database db(DATABASE_PATH);
	Query q(db);
	/*if(featID == FT_NONE)
	{
		return qtrue;
	}*/
	q.get_result(va("SELECT * FROM feats WHERE charID='%i' AND featID='%i'",charid,featID));
	int feat = q.num_rows();
	if(feat)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}
/*
=================

InsertFeat

Inserts feat into the database
=====
*/
void InsertFeat(int charID,int featID)
{
	Database db(DATABASE_PATH);
	Query q(db);
	q.execute(va("INSERT INTO feats(charID,featID) VALUES('%i','%i')",charID,featID));
	return;
}

/*
=================

Cmd_CharacterInfo_F

Spits out the character information

Ingame command: qwcharacterInfo
=====
*/
void Cmd_CharacterInfo_F(gentity_t * targetplayer)
{
	if((targetplayer->client->sess.loggedinAccount) && (targetplayer->client->sess.characterChosen))
	{
		Database db(DATABASE_PATH);
		Query q(db);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

		//Get their character info from the database
		//Name
		std::string charNameSTR = q.get_string( va( "SELECT name FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID ) );
		//Model
		std::string charModelSTR = q.get_string( va( "SELECT model FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID ) );
		//ModelScale
		int charModelScale = q.get_num( va( "SELECT modelscale FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID ) );
		//Level
		int charLevel = q.get_num( va( "SELECT xp FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID ) );
		//XP
		int charXP = q.get_num( va( "SELECT xp FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID ) );
		//PlayerClass
		int charPlayerClass = q.get_num( va( "SELECT playerclass FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID ) );

		//Print the info to their console
		trap_SendServerCommand ( targetplayer->client->ps.clientNum, va( "print \"^5Character Info:\nName: %s\nModel: %s\nModel Scale: %d\nLevel: %d\nXP: %d\nPlayer Class: %d\n\"", charNameSTR.c_str(), charModelSTR.c_str(), charModelScale, charLevel, charXP, charPlayerClass ) );
		return;
	}
	else
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^1Error: You must be logged in and have a character selected in order to view your character's info.\n\"" );
		return;
	}
}

/*
=================

isLoggedIn


=====
*/
//Returns whether we're logged in or not
qboolean isLoggedIn(gentity_t* targetplayer){
	if(targetplayer->client->sess.loggedinAccount)
		return qtrue;
	else
		return qfalse;
}

/*
=================

isInCharacter


=====
*/
//Returns whether we're in character or not
qboolean isInCharacter(gentity_t* targetplayer){
	if(targetplayer->client->sess.characterChosen)
		return qtrue;
	else
		return qfalse;
}

/*
=================

LoadUser

Loads the user properties

=====
*/
void LoadUser(gentity_t * targetplayer){
	Database db(DATABASE_PATH);
	Query q(db);
	
	int userID = targetplayer->client->sess.userID;

	//Checks if the user is admin
	int isAdmin = q.get_num(va("SELECT admin FROM users WHERE ID='%i'",userID));
	if(isAdmin)
	{//We're giving them admin if they are
		targetplayer->client->sess.openrpIsAdmin = qtrue;
	}
	return;
}


/*
=================

SaveCharacter

Saves the character information to the database

=====
*/
void SaveCharacter(gentity_t * targetplayer) 
{
        Database db(DATABASE_PATH);
        Query q(db);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

		std::string featString;
		std::string skillString;
		std::string forceString;

        //Create feat string
        for(int i = 0; i < NUM_FEATS; i++)
        {
		 char tempFeat[2];
		 itoa(targetplayer->client->featLevel[i],tempFeat,10);
		 featString.append(tempFeat);
        }
        //Create skill string
        for(int j = 0; j < NUM_SKILLS; j++)
        {
		 char tempSkill[2];
         itoa(targetplayer->client->skillLevel[j],tempSkill,10);
		 skillString.append(tempSkill);
        }
        //Create force string
        for(int k = 0; k < NUM_FORCE_POWERS-1; k++)
        {
		 char tempForce[2];
         itoa(targetplayer->client->ps.fd.forcePowerLevel[k],tempForce,10);
		 forceString.append(tempForce);
        }
      
      //Update feats in database
	  q.execute(va("UPDATE characters set feats='%s' WHERE ID='%i'",featString.c_str(),targetplayer->client->sess.characterID));
      //Update skills in database
      q.execute(va("UPDATE characters set skills='%s' WHERE ID='%i'",skillString.c_str(),targetplayer->client->sess.characterID));
      //Update force in database
      q.execute(va("UPDATE characters set force='%s' WHERE ID='%i'",forceString.c_str(),targetplayer->client->sess.characterID));

        return;
}

/*
=================

Grant Admin

=====
*/
void Cmd_GrantAdmin_F( gentity_t * ent )
{
	Database db(DATABASE_PATH);
	Query q(db);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	char accountName[MAX_TOKEN_CHARS];
	short int adminLevel;
	char temp[33];

	if(ent->client->sess.openrpIsAdmin == qfalse)
	{
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"^1You are not allowed to use this command.\n\"");
		return;
	}

	if( trap_Argc() < 2 ){
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"^5Command Usage: GrantAdmin <accountname>\n\"");
		return;
	}

	trap_Argv( 1, accountName, MAX_STRING_CHARS );
	trap_Argv( 2, temp, MAX_STRING_CHARS );

	adminLevel = atoi( temp );
	
	if ( !adminLevel == 1 || 2 || 3 || 4 || 5 || 6 || 7 || 8 || 9 || 10 )
	{
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"The admin level must be a number from 1-10.\n\"" );
		return;
	}

	int valid = q.get_num( va( "SELECT * FROM users WHERE name='%s'", accountName ) );
	if( !valid )
	{
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"This user does not exist\n\"" );
		return;
	}

	q.execute( va( "UPDATE users set admin='1' WHERE name='%s'", accountName ) );

	q.execute( va( "UPDATE users set adminlevel='%d' WHERE name='%s'", adminLevel, accountName ) );

	trap_SendServerCommand( ent->client->ps.clientNum, va( "print \"^5Admin level %d granted to %s.\n\"", adminLevel, accountName ) );
	return;
}

/*
=================

SV Grant Admin

=====
*/
void Cmd_SVGrantAdmin_F()
{
	Database db(DATABASE_PATH);
	Query q(db);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	char accountName[MAX_TOKEN_CHARS];
	short int adminLevel;
	char temp[33];

	if( trap_Argc() < 2 ){
		G_Printf("Usage: GrantAdmin <accountname> <adminlevel1-10>\n");
		return;
	}

	trap_Argv( 1, accountName, MAX_STRING_CHARS );
	trap_Argv( 2, temp, MAX_STRING_CHARS );

	adminLevel = atoi( temp );
	
	if (!adminLevel == 1 || 2 || 3 || 4 || 5 || 6 || 7 || 8 || 9 || 10)
	{
		G_Printf( "The admin level must be a number from 1-10.\n" );
		return;
	}
	
	int valid = q.get_num(va("SELECT * FROM users WHERE name='%s'",accountName));
	if(!valid)
	{
		G_Printf("This user does not exist.\n");
		return;
	}

	q.execute( va( "UPDATE users set admin='1' WHERE name='%s'", accountName ) );

	q.execute( va( "UPDATE users set adminlevel='%d' WHERE name='%s'", adminLevel, accountName ) );

	G_Printf( "Admin level %d granted to %s.\n", adminLevel, accountName );
	return;
}

/*
=================

Remove Admin

=====
*/
void Cmd_RemoveAdmin_F( gentity_t * ent )
{
	Database db(DATABASE_PATH);
	Query q(db);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	char accountName[MAX_TOKEN_CHARS];

	if(ent->client->sess.openrpIsAdmin == qfalse)
	{
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"^1You are not allowed to use this command.\n\"");
		return;
	}

	if( trap_Argc() < 2 ){
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"^5Command Usage: RemoveAdmin <accountname>\n\"");
		return;
	}
	trap_Argv( 1, accountName, MAX_STRING_CHARS );
	
	int valid = q.get_num(va("SELECT * FROM users WHERE name='%s'",accountName));
	if(!valid)
	{
		trap_SendServerCommand( ent->client->ps.clientNum, "print \"This user does not exist\n\"");
		return;
	}

	q.execute(va("UPDATE users set admin='0' WHERE name='%s'",accountName));

	trap_SendServerCommand( ent->client->ps.clientNum, "print \"Admin removed\n\"");
	return;
}

/*
=================

SV Remove Admin

=====
*/
void Cmd_SVRemoveAdmin_F()
{
	Database db(DATABASE_PATH);
	Query q(db);

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	char accountName[MAX_TOKEN_CHARS];

	if( trap_Argc() < 2 ){
		G_Printf("Usage: RemoveAdmin <accountname>\n");
		return;
	}
	trap_Argv( 1, accountName, MAX_STRING_CHARS );
	
	int valid = q.get_num(va("SELECT * FROM users WHERE name='%s'",accountName));
	if(!valid)
	{
		G_Printf("This user does not exist\n");
		return;
	}

	q.execute(va("UPDATE users set admin='0' WHERE name='%s'",accountName));

	G_Printf("Admin removed\n");
	return;
}

/*
=================

Load Attributes

=====
*/
void LoadAttributes(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);
	Query q(db);
	
	char userinfo[MAX_INFO_STRING];
	trap_GetUserinfo( targetplayer->client->ps.clientNum, userinfo, MAX_INFO_STRING );


	//Name
	std::string name = q.get_string(va("SELECT name FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));

    //Model
	std::string model = q.get_string(va("SELECT model FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));
	Info_SetValueForKey( userinfo, "model", model.c_str() );
	trap_SetUserinfo( targetplayer->client->ps.clientNum, userinfo );
	ClientUserinfoChanged( targetplayer->client->ps.clientNum );

	//Model scale
	int modelScale = q.get_num(va("SELECT modelscale FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));
	targetplayer->client->ps.iModelScale= modelScale;
	targetplayer->client->sess.modelScale= modelScale;

	//XP
	int XP = q.get_num(va("SELECT xp FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID));

	//Level
	int level = q.get_num(va("SELECT level FROM characters WHERE ID='%i'", targetplayer->client->sess.characterID));

	//Player Class
	int playerClass = q.get_num(va("SELECT playerclass FROM characters WHERE ID='%s'", targetplayer->client->sess.characterID));

	return;
}

/*
=================

Give XP

=====
*/
void Cmd_GiveXP_F(gentity_t * targetplayer)
{
	Database db(DATABASE_PATH);
	Query q(db);

	char charName[MAX_STRING_CHARS], temp[MAX_STRING_CHARS];
	int changedXP;

	
	if(targetplayer->client->sess.openrpIsAdmin == qfalse)
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, "print \"^1Error: You are not allowed to use this command.\n\"");
		return;
	}

	if (!db.Connected())
	{
		G_Printf("Database not connected, %s\n",DATABASE_PATH);
		return;
	}

	if( trap_Argc() < 2 ){
		G_Printf("^5Command Usage: qwGrantXP <accountname> <addedOrSubtractedXP>\n");
		return;
	}

	//Character name
	trap_Argv( 1, charName, MAX_STRING_CHARS );
	std::string charNameSTR = charName;

	//XP Added or removed.
	trap_Argv( 2, temp, MAX_STRING_CHARS );
	changedXP = atoi(temp);

	//Check if the character exists
	std::transform(charNameSTR.begin(), charNameSTR.end(),charNameSTR.begin(),::tolower);
	int charID = q.get_num( va( "SELECT ID FROM characters WHERE userID='%i' AND name='%s'",targetplayer->client->sess.userID,charNameSTR.c_str() ) );

	if(charID == 0)
	{
		trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "print \"^1Error: Character %s does not exist.\n\"", charNameSTR.c_str() ) );
		return;
	}

	int currentXP = q.get_num( va( "SELECT xp FROM characters WHERE ID='%i'",targetplayer->client->sess.characterID ) );

	int newXPTotal = currentXP + changedXP;

	q.execute( va( "UPDATE characters set xp='%i' WHERE ID='%i'", newXPTotal, targetplayer->client->sess.characterID ) );

	trap_SendServerCommand( targetplayer->client->ps.clientNum, va( "print \"^2Success: XP has been given to that character.\n\"" ) );

	return;
}